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

#pragma once

#ifndef DYLIBBUNDLER_UTILS_H_
#define DYLIBBUNDLER_UTILS_H_

#include <map>
#include <string>
#include <vector>

using std::map;
using std::string;
using std::vector;

string filePrefix(const string& in);
string stripPrefix(const string& in);

string getFrameworkRoot(const string& in);
string getFrameworkPath(const string& in);

string stripLSlash(const string& in);

// trim from end (in place)
void rtrim_in_place(string& s);
// trim from end (copying)
string rtrim(string s);

// execute a command in the native shell and return output in string
string systemOutput(const string& cmd);
// run a command in the system shell (like 'system') but also print the command to stdout
int systemp(const string& cmd);

void tokenize(const string& str, const char* delimiters, vector<string>*);

vector<string> lsDir(const string& path);
bool fileExists(const string& filename);
bool isRpath(const string& path);

string bundleExecutableName(const string& app_bundle_path);

void changeId(const string& binary_file, const string& new_id);
void changeInstallName(const string& binary_file, const string& old_name, const string& new_name);

void copyFile(const string& from, const string& to);
bool deleteFile(const string& path, bool overwrite);
bool deleteFile(const string& path);
bool mkdir(const string& path);

void createDestDir();

string getUserInputDirForFile(const string& filename, const string& dependent_file);

void otool(const string& flags, const string& file, vector<string>& lines);
void parseLoadCommands(const string& file, const map<string, string>& cmds_values, map<string, vector<string>>& cmds_results);

string searchFilenameInRpaths(const string& rpath_file, const string& dependent_file);
string searchFilenameInRpaths(const string& rpath_file);

// check the same paths the system would search for dylibs
void initSearchPaths();

void createQtConf(string directory);

#endif  // DYLIBBUNDLER_UTILS_H_
