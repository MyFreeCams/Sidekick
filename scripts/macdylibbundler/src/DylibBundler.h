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

#ifndef DYLIBBUNDLER_DYLIBBUNDLER_H_
#define DYLIBBUNDLER_DYLIBBUNDLER_H_

#include <string>
#include <vector>

void addDependency(const std::string& path, const std::string& dependent_file);
void collectDependencies(const std::string& dependent_file);
void collectSubDependencies();
void changeLibPathsOnFile(const std::string& file_to_fix);
void fixRpathsOnFile(const std::string& original_file, const std::string& file_to_fix);
void bundleDependencies();
void bundleQtPlugins();

#endif  // DYLIBBUNDLER_DYLIBBUNDLER_H_
