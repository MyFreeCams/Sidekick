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

#include "Settings.h"

#include "Utils.h"

#include <cstdlib>
#include <map>
#include <utility>

#include <sys/param.h>

using std::map;
using std::string;
using std::vector;

namespace Settings {

bool overwrite_files = false;
bool overwrite_dir = false;
bool create_dir = false;
bool quiet_output = false;
bool verbose_output = false;
bool bundle_libs = true;
bool bundle_frameworks = false;

string dest_folder_str = "./libs/";
string dest_folder_str_app = "./Frameworks/";
string dest_folder = dest_folder_str;
string dest_path = dest_folder;

string inside_path_str = "@executable_path/../libs/";
string inside_path_str_app = "@executable_path/../Frameworks/";
string inside_path = inside_path_str;

string app_bundle;
string appBundle() { return app_bundle; }
void appBundle(string path)
{
    app_bundle = std::move(path);
    char buffer[PATH_MAX];
    if (realpath(app_bundle.c_str(), buffer))
        app_bundle = buffer;

    if (app_bundle[app_bundle.size()-1] != '/')
        app_bundle += "/"; // fix path if needed so it ends with '/'

    string bundle_executable_path = app_bundle + "Contents/MacOS/" + bundleExecutableName(app_bundle);
    if (realpath(bundle_executable_path.c_str(), buffer))
        bundle_executable_path = buffer;
    addFileToFix(bundle_executable_path);

    if (inside_path == inside_path_str)
        inside_path = inside_path_str_app;
    if (dest_folder == dest_folder_str)
        dest_folder = dest_folder_str_app;

    dest_path = app_bundle + "Contents/" + stripLSlash(dest_folder);
    if (realpath(dest_path.c_str(), buffer))
        dest_path = buffer;
    if (dest_path[dest_path.size()-1] != '/')
        dest_path += "/";
}
bool appBundleProvided() { return !app_bundle.empty(); }

string destFolder() { return dest_path; }
void destFolder(string path)
{
    dest_path = std::move(path);
    if (appBundleProvided())
        dest_path = app_bundle + "Contents/" + stripLSlash(dest_folder);
    char buffer[PATH_MAX];
    if (realpath(dest_path.c_str(), buffer))
        dest_path = buffer;
    if (dest_path[dest_path.size()-1] != '/')
        dest_path += "/";
}

string insideLibPath() { return inside_path; }
void insideLibPath(string p)
{
    inside_path = std::move(p);
    if (inside_path[inside_path.size()-1] != '/')
        inside_path += "/";
}

string executableFolder() { return app_bundle + "Contents/MacOS/"; }
string frameworksFolder() { return app_bundle + "Contents/Frameworks/"; }
string pluginsFolder() { return app_bundle + "Contents/PlugIns/"; }
string resourcesFolder() { return app_bundle + "Contents/Resources/"; }

vector<string> files;
void addFileToFix(string path)
{
    char buffer[PATH_MAX];
    if (realpath(path.c_str(), buffer))
        path = buffer;
    files.push_back(path);
}

vector<string> filesToFix() { return files; }
size_t filesToFixCount() { return files.size(); }

vector<string> prefixes_to_ignore;
void ignorePrefix(string prefix)
{
    if (prefix[prefix.size()-1] != '/')
        prefix += "/";
    prefixes_to_ignore.push_back(prefix);
}
bool isPrefixIgnored(const string& prefix)
{
    for (const auto& prefix_to_ignore : prefixes_to_ignore)
    {
        if (prefix == prefix_to_ignore)
            return true;
    }
    return false;
}

bool isPrefixBundled(const string& prefix)
{
    if (!bundle_frameworks && prefix.find(".framework") != string::npos)
        return false;
    if (prefix.find("@executable_path") != string::npos)
        return false;
    if (prefix.find("/usr/lib/") == 0)
        return false;
    if (prefix.find("/System/Library/") == 0)
        return false;
    if (isPrefixIgnored(prefix))
        return false;
    return true;
}

vector<string> search_paths;
vector<string> searchPaths() { return search_paths; }
void addSearchPath(const string& path) { search_paths.push_back(path); }

vector<string> user_search_paths;
vector<string> userSearchPaths() { return user_search_paths; }
void addUserSearchPath(const string& path) { user_search_paths.push_back(path); }

bool canCreateDir() { return create_dir; }
void canCreateDir(bool permission) { create_dir = permission; }

bool canOverwriteDir() { return overwrite_dir; }
void canOverwriteDir(bool permission) { overwrite_dir = permission; }

bool canOverwriteFiles() { return overwrite_files; }
void canOverwriteFiles(bool permission) { overwrite_files = permission; }

bool bundleLibs() { return bundle_libs; }
void bundleLibs(bool status) { bundle_libs = status; }

bool bundleFrameworks() { return bundle_frameworks; }
void bundleFrameworks(bool status) { bundle_frameworks = status; }

bool quietOutput() { return quiet_output; }
void quietOutput(bool status) { quiet_output = status; }

bool verboseOutput() { return verbose_output; }
void verboseOutput(bool status) { verbose_output = status; }

// if some libs are missing prefixes, then more stuff will be necessary to do
bool missing_prefixes = false;
bool missingPrefixes() { return missing_prefixes; }
void missingPrefixes(bool status) { missing_prefixes = status; }

map<string, string> rpath_to_fullpath;
string getFullPath(const string& rpath) { return rpath_to_fullpath[rpath]; }
void rpathToFullPath(const string& rpath, const string& fullpath) { rpath_to_fullpath[rpath] = fullpath; }
bool rpathFound(const string& rpath) { return rpath_to_fullpath.find(rpath) != rpath_to_fullpath.end(); }

map<string, vector<string>> rpaths_per_file;
vector<string> getRpathsForFile(const string& file) { return rpaths_per_file[file]; }
void addRpathForFile(const string& file, const string& rpath) { rpaths_per_file[file].push_back(rpath); }
bool fileHasRpath(const string& file) { return rpaths_per_file.find(file) != rpaths_per_file.end(); }

} // namespace Settings
