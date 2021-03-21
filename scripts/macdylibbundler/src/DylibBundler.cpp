/*
The MIT License (MIT)

Copyright (c) 2014 Marianne Gagnon
Copyright (c) 2020 SCG82

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */

#include "DylibBundler.h"

#include "Dependency.h"
#include "Settings.h"
#include "Utils.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <utility>

#ifdef __linux
#include <linux/limits.h>
#endif
#ifndef __clang__
#include <sys/types.h>
#endif

using namespace std;

vector<Dependency> deps;
map<string, vector<Dependency>> deps_per_file;
map<string, bool> deps_collected;
set<string> frameworks;
bool bundleQtPlugins_called = false;

void addDependency(const string& path, const string& dependent_file)
{
    Dependency dependency(path, dependent_file);

    // check if this library was already added to |deps| to avoid duplicates
    bool in_deps = false;
    for (auto& dep : deps)
    {
        if (dependency.MergeIfIdentical(dep))
            in_deps = true;
    }

    // check if this library was already added to |deps_per_file[dependent_file]| to avoid duplicates
    bool in_deps_per_file = false;
    for (auto& dep : deps_per_file[dependent_file])
    {
        if (dependency.MergeIfIdentical(dep))
            in_deps_per_file = true;
    }

    // check if this library is in /usr/lib, /System/Library, or in ignored list
    if (!Settings::isPrefixBundled(dependency.Prefix()))
        return;

    if (!in_deps && dependency.IsFramework())
        frameworks.insert(dependency.OriginalPath());

    if (!in_deps)
        deps.push_back(dependency);
    if (!in_deps_per_file)
        deps_per_file[dependent_file].push_back(dependency);
}

void collectDependencies(const string& dependent_file)
{
    if (deps_collected.find(dependent_file) != deps_collected.end())
        return;

    map<string, string> cmds_values;
    map<string, vector<string>> cmds_results;

    cmds_values["LC_LOAD_DYLIB"] = "name";
    cmds_values["LC_RPATH"] = "path";

    parseLoadCommands(dependent_file, cmds_values, cmds_results);

    for (const auto& rpath : cmds_results["LC_RPATH"])
    {
        Settings::addRpathForFile(dependent_file, rpath);
        if (Settings::verboseOutput())
            cout << "  rpath: " << rpath << endl;
    }

    for (const auto& dep_path : cmds_results["LC_LOAD_DYLIB"])
    {
        // skip system/ignored prefixes
        if (Settings::isPrefixBundled(dep_path))
            addDependency(dep_path, dependent_file);
    }

    deps_collected[dependent_file] = true;
}

void collectSubDependencies()
{
    if (Settings::verboseOutput())
    {
        cout << "(pre sub) # OF FILES: " << Settings::filesToFixCount() << endl;
        cout << "(pre sub) # OF DEPS: " << deps.size() << endl;
    }

    size_t deps_size = deps.size();
    size_t dep_counter = deps_size;
    while (true)
    {
        deps_size = deps.size();
        for (size_t n=0; n<deps_size; ++n)
        {
            string original_path = deps[n].OriginalPath();
            if (Settings::verboseOutput())
                cout << "  (collect sub deps) original path: " << original_path << endl;

            if (isRpath(original_path))
                original_path = searchFilenameInRpaths(original_path);

            collectDependencies(original_path);
        }

        // if no more dependencies were added on this iteration, stop searching
        if (deps.size() == deps_size)
            break;
    }

    if (Settings::verboseOutput())
    {
        cout << "(post sub) # OF FILES: " << Settings::filesToFixCount() << endl;
        cout << "(post sub) # OF DEPS: " << deps.size() << endl;
    }

    if (Settings::bundleLibs() && Settings::bundleFrameworks())
    {
        if (!bundleQtPlugins_called || (deps.size() != dep_counter))
            bundleQtPlugins();
    }
}

void changeLibPathsOnFile(const string& file_to_fix)
{
    if (deps_collected.find(file_to_fix) == deps_collected.end())
        collectDependencies(file_to_fix);

    cout << "* Fixing dependencies on " << file_to_fix << "\n";

    for (auto& dep : deps_per_file[file_to_fix])
        dep.FixDependentFile(file_to_fix);
}

void fixRpathsOnFile(const string& original_file, const string& file_to_fix)
{
    if (!Settings::fileHasRpath(original_file))
        return;

    vector<string> rpaths_to_fix = Settings::getRpathsForFile(original_file);
    for (const auto& rpath_to_fix : rpaths_to_fix)
    {
        string command = "install_name_tool -rpath " + rpath_to_fix + " "
                       + Settings::insideLibPath() + " " + file_to_fix;
        if (systemp(command) != 0)
        {
            cerr << "\n\n/!\\ ERROR: An error occured while trying to fix rpath "
                 << rpath_to_fix << " of " << file_to_fix << endl;
            exit(1);
        }
    }
}

void bundleDependencies()
{
    for (const auto& dep : deps)
        dep.Print();
    cout << "\n";

    // copy & fix up dependencies
    if (Settings::bundleLibs())
    {
        createDestDir();
        for (const auto& dep : deps)
        {
            dep.CopyToBundle();
            changeLibPathsOnFile(dep.InstallPath());
            fixRpathsOnFile(dep.OriginalPath(), dep.InstallPath());
        }
    }

    // fix up selected files
    for (const auto& file : Settings::filesToFix())
    {
        changeLibPathsOnFile(file);
        fixRpathsOnFile(file, file);
    }
}

void bundleQtPlugins()
{
    bool qtCoreFound = false;
    bool qtGuiFound = false;
    bool qtNetworkFound = false;
    bool qtSqlFound = false;
    bool qtSvgFound = false;
    bool qtMultimediaFound = false;
    bool qt3dRenderFound = false;
    bool qt3dQuickRenderFound = false;
    bool qtPositioningFound = false;
    bool qtLocationFound = false;
    bool qtTextToSpeechFound = false;
    bool qtWebViewFound = false;
    string original_file;

    for (const auto& framework : frameworks)
    {
        if (framework.find("QtCore") != string::npos)
        {
            qtCoreFound = true;
            original_file = framework;
        }
        if (framework.find("QtGui") != string::npos)
            qtGuiFound = true;
        if (framework.find("QtNetwork") != string::npos)
            qtNetworkFound = true;
        if (framework.find("QtSql") != string::npos)
            qtSqlFound = true;
        if (framework.find("QtSvg") != string::npos)
            qtSvgFound = true;
        if (framework.find("QtMultimedia") != string::npos)
            qtMultimediaFound = true;
        if (framework.find("Qt3DRender") != string::npos)
            qt3dRenderFound = true;
        if (framework.find("Qt3DQuickRender") != string::npos)
            qt3dQuickRenderFound = true;
        if (framework.find("QtPositioning") != string::npos)
            qtPositioningFound = true;
        if (framework.find("QtLocation") != string::npos)
            qtLocationFound = true;
        if (framework.find("TextToSpeech") != string::npos)
            qtTextToSpeechFound = true;
        if (framework.find("WebView") != string::npos)
            qtWebViewFound = true;
    }

    if (!qtCoreFound)
        return;
    if (!bundleQtPlugins_called)
        createQtConf(Settings::resourcesFolder());

    bundleQtPlugins_called = true;

    const auto fixupPlugin = [original_file](const string& plugin)
    {
        string dest = Settings::pluginsFolder();
        string framework_root = getFrameworkRoot(original_file);
        string prefix = filePrefix(framework_root);
        string qt_prefix = filePrefix(prefix.substr(0, prefix.size()-1));
        string qt_plugins_prefix = qt_prefix + "plugins/";
        if (fileExists(qt_plugins_prefix + plugin) && !fileExists(dest + plugin))
        {
            mkdir(dest + plugin);
            copyFile(qt_plugins_prefix + plugin, dest);
            vector<string> files = lsDir(dest + plugin + "/");
            for (const auto& file : files)
            {
                Settings::addFileToFix(dest + plugin+"/"+file);
                collectDependencies(dest + plugin+"/"+file);
                changeId(dest + plugin+"/"+file, "@rpath/" + plugin+"/"+file);
            }
        }
    };

    string framework_root = getFrameworkRoot(original_file);
    string prefix = filePrefix(framework_root);
    string qt_prefix = filePrefix(prefix.substr(0, prefix.size()-1));
    string qt_plugins_prefix = qt_prefix + "plugins/";
    string dest = Settings::pluginsFolder();

    if (!fileExists(dest + "platforms"))
    {
        mkdir(dest + "platforms");
        copyFile(qt_plugins_prefix + "platforms/libqcocoa.dylib", dest + "platforms");
        Settings::addFileToFix(dest + "platforms/libqcocoa.dylib");
        collectDependencies(dest + "platforms/libqcocoa.dylib");
        changeId(dest + "platforms/libqcocoa.dylib", "@rpath/platforms/libqcocoa.dylib");
    }

    fixupPlugin("printsupport");
    fixupPlugin("styles");
    fixupPlugin("imageformats");
    fixupPlugin("iconengines");
    if (!qtSvgFound)
        systemp("rm -f " + dest + "imageformats/libqsvg.dylib");
    if (qtGuiFound)
    {
        fixupPlugin("platforminputcontexts");
        fixupPlugin("virtualkeyboard");
    }
    if (qtNetworkFound)
        fixupPlugin("bearer");
    if (qtSqlFound)
        fixupPlugin("sqldrivers");
    if (qtMultimediaFound)
    {
        fixupPlugin("mediaservice");
        fixupPlugin("audio");
    }
    if (qt3dRenderFound)
    {
        fixupPlugin("sceneparsers");
        fixupPlugin("geometryloaders");
    }
    if (qt3dQuickRenderFound)
        fixupPlugin("renderplugins");
    if (qtPositioningFound)
        fixupPlugin("position");
    if (qtLocationFound)
        fixupPlugin("geoservices");
    if (qtTextToSpeechFound)
        fixupPlugin("texttospeech");
    if (qtWebViewFound)
        fixupPlugin("webview");

    collectSubDependencies();
}
