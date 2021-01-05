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

#ifndef DYLIBBUNDLER_SETTINGS_H_
#define DYLIBBUNDLER_SETTINGS_H_

#include <string>
#include <vector>

#ifndef __clang__
#include <sys/types.h>
#endif

using std::string;
using std::vector;

namespace Settings {

bool isPrefixBundled(const string& prefix);
bool isPrefixIgnored(const string& prefix);
void ignorePrefix(string prefix);

string appBundle();
void appBundle(string path);
bool appBundleProvided();

string destFolder();
void destFolder(string path);

string insideLibPath();
void insideLibPath(string p);

string executableFolder();
string frameworksFolder();
string pluginsFolder();
string resourcesFolder();

vector<string> filesToFix();
void addFileToFix(string path);
size_t filesToFixCount();

vector<string> searchPaths();
void addSearchPath(const string& path);

vector<string> userSearchPaths();
void addUserSearchPath(const string& path);

bool canCreateDir();
void canCreateDir(bool permission);

bool canOverwriteDir();
void canOverwriteDir(bool permission);

bool canOverwriteFiles();
void canOverwriteFiles(bool permission);

bool bundleLibs();
void bundleLibs(bool status);

bool bundleFrameworks();
void bundleFrameworks(bool status);

bool quietOutput();
void quietOutput(bool status);

bool verboseOutput();
void verboseOutput(bool status);

bool missingPrefixes();
void missingPrefixes(bool status);

string getFullPath(const string& rpath);
void rpathToFullPath(const string& rpath, const string& fullpath);
bool rpathFound(const string& rpath);

vector<string> getRpathsForFile(const string& file);
void addRpathForFile(const string& file, const string& rpath);
bool fileHasRpath(const string& file);

}  // namespace Settings

#endif  // DYLIBBUNDLER_SETTINGS_H_
