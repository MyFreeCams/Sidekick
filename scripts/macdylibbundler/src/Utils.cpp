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

#include "Utils.h"

#include "Settings.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

#include <sys/param.h>
#ifndef __clang__
#include <sys/types.h>
#endif
#include <unistd.h>

using namespace std;

string filePrefix(const string& in)
{
    return in.substr(0, in.rfind('/') + 1);
}

string stripPrefix(const string& in)
{
    return in.substr(in.rfind('/') + 1);
}

string getFrameworkRoot(const string& in)
{
    return in.substr(0, in.find(".framework") + 10);
}

string getFrameworkPath(const string& in)
{
    return in.substr(in.rfind(".framework/") + 11);
}

string stripLSlash(const string& in)
{
    if (in[0] == '.' && in[1] == '/')
        return in.substr(2, in.size());
    return in;
}

string& rtrim(string& s)
{
    s.erase(find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());
    return s;
}

string systemOutput(const string& cmd)
{
    FILE* command_output = nullptr;
    char output[128];
    int amount_read = 1;
    string full_output;

    try
    {
        command_output = popen(cmd.c_str(), "r");
        if (!command_output)
            throw;

        while (amount_read > 0)
        {
            amount_read = fread(output, 1, 127, command_output);
            if (amount_read <= 0)
                break;
            output[amount_read] = '\0';
            full_output += output;
        }

        int return_value = pclose(command_output);
        if (return_value != 0)
            return "";

        return full_output;
    }
    catch (...)
    {
        cerr << "An error occured while executing command " << cmd << endl;
        pclose(command_output);
        return "";
    }
}

int systemp(const string& cmd)
{
    if (!Settings::quietOutput())
        cout << "    " << cmd << "\n";
    return system(cmd.c_str());
}

void tokenize(const string& str, const char* delim, vector<string>* vectorarg)
{
    vector<string>& tokens = *vectorarg;
    string delimiters(delim);

    // skip delimiters at beginning
    auto lastPos = str.find_first_not_of(delimiters, 0);
    // find first non-delimiter
    auto pos = str.find_first_of(delimiters, lastPos);

    while (pos != string::npos || lastPos != string::npos)
    {
        // found a token, add it to the vector
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // skip delimiters
        lastPos = str.find_first_not_of(delimiters, pos);
        // find next non-delimiter
        pos = str.find_first_of(delimiters, lastPos);
    }
}

vector<string> lsDir(const string& path)
{
    string cmd = "ls " + path;
    string output = systemOutput(cmd);
    vector<string> files;
    tokenize(output, "\n", &files);
    return files;
}

bool fileExists(const string& filename)
{
    if (access(filename.c_str(), F_OK) != -1)
        return true;
    string delims = " \f\n\r\t\v";
    string rtrimmed = filename.substr(0, filename.find_last_not_of(delims) + 1);
    string ftrimmed = rtrimmed.substr(rtrimmed.find_first_not_of(delims));
    if (access(ftrimmed.c_str(), F_OK) != -1)
        return true;
    return false;
}

bool isRpath(const string& path)
{
    return path.find("@rpath") != string::npos || path.find("@loader_path") != string::npos;
}

string bundleExecutableName(const string& app_bundle_path)
{
    string cmd = "/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' " + app_bundle_path + "Contents/Info.plist";
    string output = systemOutput(cmd);
    return rtrim(output);
}

void changeId(const string& binary_file, const string& new_id)
{
    string command = "install_name_tool -id \"" + new_id + "\" \"" + binary_file + "\"";
    if (systemp(command) != 0)
    {
        cerr << "\n\nError: An error occured while trying to change identity of library " << binary_file << endl;
        exit(1);
    }
}

void changeInstallName(const string& binary_file, const string& old_name, const string& new_name)
{
    string command = "install_name_tool -change \"" + old_name + "\" \"" + new_name + "\" \"" + binary_file + "\"";
    if (systemp(command) != 0)
    {
        cerr << "\n\nError: An error occured while trying to fix dependencies of " << binary_file << endl;
        exit(1);
    }
}

void copyFile(const string& from, const string& to)
{
    bool overwrite = Settings::canOverwriteFiles();
    if (fileExists(to) && !overwrite)
    {
        cerr << "\n\nError: File " << to << " already exists. Remove it or enable overwriting (-of)\n";
        exit(1);
    }

    // copy file/directory
    string overwrite_permission = overwrite ? "-f " : "-n ";
    string command = "cp -R " + overwrite_permission + from + " \"" + to + "\"";
    if (from != to && systemp(command) != 0)
    {
        cerr << "\n\nError: An error occured while trying to copy file " << from << " to " << to << endl;
        exit(1);
    }

    // give file/directory write permission
    string command2 = "chmod -R +w \"" + to + "\"";
    if (systemp(command2) != 0)
    {
        cerr << "\n\nError: An error occured while trying to set write permissions on file " << to << endl;
        exit(1);
    }
}

bool deleteFile(const string& path, bool overwrite)
{
    string overwrite_permission = overwrite ? "-f \"" : " \"";
    string command = "rm -r " + overwrite_permission + path +"\"";
    if (systemp(command) != 0)
    {
        cerr << "\n\nError: An error occured while trying to delete " << path << endl;
        return false;
    }
    return true;
}

bool deleteFile(const string& path)
{
    bool overwrite = Settings::canOverwriteFiles();
    return deleteFile(path, overwrite);
}

bool mkdir(const string& path)
{
    if (Settings::verboseOutput())
        cout << "Creating directory " << path << endl;
    string command = "mkdir -p \"" + path + "\"";
    if (systemp(command) != 0)
    {
        cerr << "\n/!\\ ERROR: An error occured while creating " << path << endl;
        return false;
    }
    return true;
}

void createDestDir()
{
    string dest_folder = Settings::destFolder();
    if (Settings::verboseOutput())
        cout << "Checking output directory " << dest_folder << "\n";

    bool dest_exists = fileExists(dest_folder);

    if (dest_exists && Settings::canOverwriteDir())
    {
        cout << "Erasing old output directory " << dest_folder << "\n";
        if (!deleteFile(dest_folder))
        {
            cerr << "\n\n/!\\ ERROR: An error occured while attempting to overwrite destination folder\n";
            exit(1);
        }
        dest_exists = false;
    }

    if (!dest_exists)
    {
        if (Settings::canCreateDir())
        {
            cout << "Creating output directory " << dest_folder << "\n\n";
            if (!mkdir(dest_folder))
            {
                cerr << "\n/!\\ ERROR: An error occured while creating " << dest_folder << endl;
                exit(1);
            }
        }
        else
        {
            cerr << "\n\n/!\\ ERROR: Destination folder does not exist. Create it or pass the '-cd' or '-od' flag\n";
            exit(1);
        }
    }
}

string getUserInputDirForFile(const string& filename, const string& dependent_file)
{
    for (auto& search_path : Settings::userSearchPaths())
    {
        if (!search_path.empty() && search_path[search_path.size() - 1] != '/')
            search_path += "/";
        if (fileExists(search_path + filename))
        {
            if (!Settings::quietOutput())
            {
                cerr << (search_path + filename) << " was found\n"
                     << "/!\\ WARNING: dylibbundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: Check the executable with 'otool -L'\n";
            }
            return search_path;
        }
    }

    while (true)
    {
        if (Settings::quietOutput())
            cerr << "\n/!\\ WARNING: Dependency " << filename << " of " << dependent_file << " not found\n";

        cout << "\nPlease specify the directory where this file is located (or enter 'quit' to abort): ";
        fflush(stdout);

        string prefix;
        cin >> prefix;
        cout << endl;

        if (prefix == "quit" || prefix == "exit" || prefix == "abort")
            exit(1);

        if (!prefix.empty() && prefix[prefix.size()-1] != '/')
            prefix += "/";

        string fullpath = prefix + filename;
        if (fileExists(fullpath))
        {
            cerr << fullpath << " was found\n"
                 << "/!\\ WARNING: dylibbundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: Check the executable with 'otool -L'\n";
            Settings::addUserSearchPath(prefix);
            return prefix;
        }
        else
        {
            cerr << fullpath << " does not exist. Try again...\n";
            continue;
        }
    }
}

void otool(const string& flags, const string& file, vector<string>& lines)
{
    string command = "/usr/bin/otool " + flags + " \"" + file + "\"";
    string output = systemOutput(command);

    if (output.find("can't open file") != string::npos
        || output.find("No such file") != string::npos
        || output.find("at least one file must be specified") != string::npos
        || output.empty())
    {
        cerr << "\n\n/!\\ ERROR: Cannot find file " << file << " to read its load commands\n";
        exit(1);
    }

    tokenize(output, "\n", &lines);
}

void parseLoadCommands(const string& file,
                       const map<string, string>& cmds_values,
                       map<string, vector<string>>& cmds_results)
{
    vector<string> raw_lines;
    otool("-l", file, raw_lines);

    for (const auto& cmd_value : cmds_values)
    {
        vector<string> lines;
        string cmd = cmd_value.first;
        string value = cmd_value.second;
        string cmd_line = "cmd " + cmd;
        string value_line = value + " ";
        bool searching = false;

        for (const auto& raw_line : raw_lines)
        {
            if (raw_line.find(cmd_line) != string::npos)
            {
                if (searching)
                {
                    cerr << "\n\n/!\\ ERROR: Failed to find " << value << " before next cmd\n";
                    exit(1);
                }
                searching = true;
            }
            else if (searching)
            {
                size_t start_pos = raw_line.find(value_line);
                if (start_pos == string::npos)
                    continue;
                size_t start = start_pos + value.size() + 1; // exclude data label "|value| "
                size_t end = string::npos;
                if (value == "name" || value == "path")
                {
                    size_t end_pos = raw_line.find(" (");
                    if (end_pos == string::npos)
                        continue;
                    end = end_pos - start;
                }
                lines.push_back(raw_line.substr(start, end));
                searching = false;
            }
        }
        cmds_results[cmd] = lines;
    }
}

string searchFilenameInRpaths(const string& rpath_file, const string& dependent_file)
{
    string fullpath;
    string suffix = rpath_file.substr(rpath_file.rfind('/') + 1);

    if (Settings::verboseOutput())
    {
        if (dependent_file != rpath_file)
            cout << "  dependent file: " << dependent_file << endl;
        cout << "    dependency: " << rpath_file << endl;
    }

    const auto check_path = [&](string path)
    {
        char buffer[PATH_MAX];
        string file_prefix = filePrefix(dependent_file);
        if (path.find("@executable_path") != string::npos || path.find("@loader_path") != string::npos)
        {
            if (path.find("@executable_path") != string::npos)
            {
                if (Settings::appBundleProvided())
                    path = regex_replace(path, regex("@executable_path/"), Settings::executableFolder());
            }
            else if (path.find("@loader_path") != string::npos)
            {
                if (dependent_file != rpath_file)
                    path = regex_replace(path, regex("@loader_path/"), file_prefix);
            }
            if (Settings::verboseOutput())
                cout << "    path to search: " << path << endl;
            if (realpath(path.c_str(), buffer))
            {
                fullpath = buffer;
                Settings::rpathToFullPath(rpath_file, fullpath);
                return true;
            }
        }
        else if (path.find("@rpath") != string::npos)
        {
            if (Settings::appBundleProvided())
            {
                path = regex_replace(path, regex("@rpath/"), Settings::executableFolder());
                if (Settings::verboseOutput())
                    cout << "    path to search: " << path << endl;
                if (realpath(path.c_str(), buffer))
                {
                    fullpath = buffer;
                    Settings::rpathToFullPath(rpath_file, fullpath);
                    return true;
                }
            }
            if (dependent_file != rpath_file)
            {
                string pathL = regex_replace(path, regex("@rpath/"), file_prefix);
                if (Settings::verboseOutput())
                    cout << "    path to search: " << pathL << endl;
                if (realpath(pathL.c_str(), buffer))
                {
                    fullpath = buffer;
                    Settings::rpathToFullPath(rpath_file, fullpath);
                    return true;
                }
            }
        }
        return false;
    };

    // fullpath previously stored
    if (Settings::rpathFound(rpath_file))
    {
        fullpath = Settings::getFullPath(rpath_file);
    }
    else if (!check_path(rpath_file))
    {
        for (auto rpath : Settings::getRpathsForFile(dependent_file))
        {
            if (rpath[rpath.size()-1] != '/')
                rpath += "/";
            string path = rpath + suffix;
            if (Settings::verboseOutput())
                cout << "    trying rpath: " << path << endl;
            if (check_path(path))
                break;
        }
    }

    if (fullpath.empty())
    {
        for (const auto& search_path : Settings::searchPaths())
        {
            if (fileExists(search_path + suffix))
            {
                if (Settings::verboseOutput())
                    cout << "FOUND " << suffix << " in " << search_path << endl;
                fullpath = search_path + suffix;
                break;
            }
        }
        if (fullpath.empty())
        {
            if (Settings::verboseOutput())
                cout << "  ** rpath fullpath: not found" << endl;
            if (!Settings::quietOutput())
                cerr << "\n/!\\ WARNING: Can't get path for '" << rpath_file << "'\n";

            fullpath = getUserInputDirForFile(suffix, dependent_file) + suffix;

            if (Settings::quietOutput() && fullpath.empty())
                cerr << "\n/!\\ WARNING: Can't get path for '" << rpath_file << "'\n";

            char buffer[PATH_MAX];
            if (realpath(fullpath.c_str(), buffer))
                fullpath = buffer;
        }
        else if (Settings::verboseOutput())
        {
            cout << "  ** rpath fullpath: " << fullpath << endl;
        }
    }
    else if (Settings::verboseOutput())
    {
        cout << "  ** rpath fullpath: " << fullpath << endl;
    }
    return fullpath;
}

string searchFilenameInRpaths(const string& rpath_file)
{
    return searchFilenameInRpaths(rpath_file, rpath_file);
}

void initSearchPaths()
{
    string searchPaths;
    char* dyldLibPath = getenv("DYLD_LIBRARY_PATH");
    if (dyldLibPath != nullptr)
        searchPaths = dyldLibPath;

    dyldLibPath = getenv("DYLD_FALLBACK_FRAMEWORK_PATH");
    if (dyldLibPath != nullptr)
    {
        if (!searchPaths.empty() && searchPaths[searchPaths.size()-1] != ':')
            searchPaths += ":";
        searchPaths += dyldLibPath;
    }

    dyldLibPath = getenv("DYLD_FALLBACK_LIBRARY_PATH");
    if (dyldLibPath != nullptr)
    {
        if (!searchPaths.empty() && searchPaths[searchPaths.size()-1] != ':')
            searchPaths += ":";
        searchPaths += dyldLibPath;
    }

    if (!searchPaths.empty())
    {
        stringstream ss(searchPaths);
        string item;
        while (getline(ss, item, ':'))
        {
            if (item[item.size()-1] != '/')
                item += "/";
            Settings::addSearchPath(item);
        }
    }
}

void createQtConf(string directory)
{
    string contents = "[Paths]\n"
                      "Plugins = PlugIns\n"
                      "Imports = Resources/qml\n"
                      "Qml2Imports = Resources/qml\n";
    if (directory[directory.size()-1] != '/')
        directory += "/";
    ofstream out(directory + "qt.conf");
    out << contents;
    out.close();
}
