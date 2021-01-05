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

#ifndef DYLIBBUNDLER_DEPENDENCY_H_
#define DYLIBBUNDLER_DEPENDENCY_H_

#include <string>
#include <vector>

using std::string;
using std::vector;

class Dependency {
public:
    Dependency(string path, const string& dependent_file);

    [[nodiscard]] bool IsFramework() const { return is_framework; }

    [[nodiscard]] string Prefix() const { return prefix; }
    [[nodiscard]] string OriginalFilename() const { return filename; }
    [[nodiscard]] string OriginalPath() const { return prefix + filename; }

    [[nodiscard]] string InnerPath() const;
    [[nodiscard]] string InstallPath() const;

    void AddSymlink(const string& path);

    // Compare the given dependency with this one. If both refer to the same file,
    // merge both entries into one and return true.
    bool MergeIfIdentical(Dependency& dependency);

    void CopyToBundle() const;
    void FixDependentFile(const string& dependent_file) const;

    void Print() const;

private:
    bool is_framework;

    // origin
    string filename;
    string prefix;
    vector<string> symlinks;

    // installation
    string new_name;
};

#endif  // DYLIBBUNDLER_DEPENDENCY_H_
