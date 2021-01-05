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

#include "Dependency.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>

#include <sys/param.h>
#ifndef __clang__
#include <sys/types.h>
#endif

#include "Settings.h"
#include "Utils.h"

using namespace std;

Dependency::Dependency(string path, const string& dependent_file)
    : is_framework(false)
{
    char buffer[PATH_MAX];
    rtrim_in_place(path);
    string original_file;
    string warning_msg;

    if (isRpath(path))
    {
        original_file = searchFilenameInRpaths(path, dependent_file);
    }
    else if (realpath(path.c_str(), buffer))
    {
        original_file = buffer;
    }
    else
    {
        warning_msg = "\n/!\\ WARNING: Cannot resolve path '" + path + "'\n";
        original_file = path;
    }

    if (Settings::verboseOutput())
    {
        cout << "** Dependency ctor **" << endl;
        if (path != dependent_file)
            cout << "  dependent file:  " << dependent_file << endl;
        cout << "  dependency path: " << path << endl;
        cout << "  original_file:   " << original_file << endl;
    }

    // check if given path is a symlink
    if (original_file != path)
        AddSymlink(path);

    prefix = filePrefix(original_file);
    filename = stripPrefix(original_file);

    if (!prefix.empty() && prefix[prefix.size()-1] != '/')
        prefix += "/";

    // check if this dependency is in /usr/lib, /System/Library, or in ignored list
    if (!Settings::isPrefixBundled(prefix))
        return;

    if (original_file.find(".framework") != string::npos)
    {
        is_framework = true;
        string framework_root = getFrameworkRoot(original_file);
        string framework_path = getFrameworkPath(original_file);
        string framework_name = stripPrefix(framework_root);
        prefix = filePrefix(framework_root);
        filename = framework_name + "/" + framework_path;
        if (Settings::verboseOutput())
        {
            cout << "  framework root: " << framework_root << endl;
            cout << "  framework path: " << framework_path << endl;
            cout << "  framework name: " << framework_name << endl;
        }
    }

    // check if the lib is in a known location
    if (prefix.empty() || !fileExists(prefix+filename))
    {
        vector<string> search_paths = Settings::searchPaths();
        if (search_paths.empty())
            initSearchPaths();
        // check if file is contained in one of the paths
        for (const auto& search_path : search_paths)
        {
            if (fileExists(search_path+filename))
            {
                warning_msg += "FOUND " + filename + " in " + search_path + "\n";
                prefix = search_path;
                Settings::missingPrefixes(true);
                break;
            }
        }
    }

    if (!Settings::quietOutput())
        cout << warning_msg;

    // if the location is still unknown, ask the user for search path
    if (!Settings::isPrefixIgnored(prefix) && (prefix.empty() || !fileExists(prefix+filename)))
    {
        if (!Settings::quietOutput())
            cerr << "\n/!\\ WARNING: Dependency " << filename << " of " << dependent_file << " not found\n";
        if (Settings::verboseOutput())
            cout << "     path: " << (prefix+filename) << endl;
        Settings::missingPrefixes(true);
        Settings::addSearchPath(getUserInputDirForFile(filename, dependent_file));
    }
    new_name = filename;
}

string Dependency::InnerPath() const
{
    return Settings::insideLibPath() + new_name;
}

string Dependency::InstallPath() const
{
    return Settings::destFolder() + new_name;
}

void Dependency::AddSymlink(const string& path)
{
    if (find(symlinks.begin(), symlinks.end(), path) == symlinks.end())
        symlinks.push_back(path);
}

bool Dependency::MergeIfIdentical(Dependency& dependency)
{
    if (dependency.OriginalFilename() == filename)
    {
        for (const auto& symlink : symlinks)
        {
            dependency.AddSymlink(symlink);
        }
        return true;
    }
    return false;
}

void Dependency::CopyToBundle() const
{
    string original_path = OriginalPath();
    string dest_path = InstallPath();

    if (is_framework)
    {
        original_path = getFrameworkRoot(original_path);
        dest_path = Settings::destFolder() + stripPrefix(original_path);
    }

    if (Settings::verboseOutput())
    {
        string inner_path = InnerPath();
        cout << "  - original path: " << original_path << endl;
        cout << "  - inner path:    " << inner_path << endl;
        cout << "  - dest_path:     " << dest_path << endl;
        cout << "  - install path:  " << InstallPath() << endl;
    }

    copyFile(original_path, dest_path);

    if (is_framework)
    {
        string headers_symlink = dest_path + string("/Headers");
        string headers_path;
        char buffer[PATH_MAX];
        if (realpath(rtrim(headers_symlink).c_str(), buffer))
        {
            headers_path = buffer;
            deleteFile(headers_path, true);
        }
        deleteFile(headers_symlink, true);
        deleteFile(dest_path + "/*.prl");
    }

    changeId(InstallPath(), "@rpath/" + new_name);
}

void Dependency::FixDependentFile(const string& dependent_file) const
{
    changeInstallName(dependent_file, OriginalPath(), InnerPath());
    for (const auto& symlink : symlinks)
    {
        changeInstallName(dependent_file, symlink, InnerPath());
    }

    if (!Settings::missingPrefixes()) return;

    changeInstallName(dependent_file, filename, InnerPath());
    for (const auto& symlink : symlinks)
    {
        changeInstallName(dependent_file, symlink, InnerPath());
    }
}

void Dependency::Print() const
{
    cout << "\n* " << filename << " from " << prefix << endl;
    for (const auto& symlink : symlinks)
    {
        cout << "    symlink --> " << symlink << endl;
    }
}
