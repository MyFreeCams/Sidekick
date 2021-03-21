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
#include "Settings.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#ifndef __clang__
#include <sys/types.h>
#endif

using namespace std;

const string VERSION = "2.2.0 (2021-03-18)";

void showHelp()
{
    cout << "Usage: dylibbundler -a file.app [options]\n";
    cout << "Options:\n";
    cout << "  -a,  --app                   Application bundle to make self-contained\n";
    cout << "  -x,  --fix-file              Copy file's dependencies to app bundle and fix internal names and rpaths\n";
    cout << "  -f,  --frameworks            Copy dependencies that are frameworks (experimental)\n";
    cout << "  -d,  --dest-dir              Directory to copy dependencies, relative to <app>/Contents (default: ./Frameworks)\n";
    cout << "  -p,  --install-path          Inner path (@rpath) of bundled dependencies (default: @executable_path/../Frameworks/)\n";
    cout << "  -s,  --search-path           Add directory to search path\n";
    cout << "  -i,  --ignore                Ignore dependencies in this directory (default: /usr/lib & /System/Library)\n";
    cout << "  -of, --overwrite-files       Allow overwriting files in output directory\n";
    cout << "  -cd, --create-dir            Create output directory if needed\n";
    cout << "  -od, --overwrite-dir         Overwrite (delete) output directory if it exists (implies --create-dir)\n";
    cout << "  -n,  --just-print            Print the dependencies found (without copying into app bundle)\n";
    cout << "  -q,  --quiet                 Less verbose output\n";
    cout << "  -v,  --verbose               More verbose output\n";
    cout << "  -V,  --version               Print dylibbundler version number and exit\n";
    cout << "  -h,  --help                  Print this message and exit\n";
}

int main(int argc, const char* argv[])
{
    // parse arguments
    for (int i=0; i<argc; i++)
    {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--app") == 0)
        {
            i++;
            Settings::appBundle(argv[i]);
            continue;
        }
        else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--fix-file") == 0)
        {
            i++;
            Settings::addFileToFix(argv[i]);
            continue;
        }
        else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--bundle-frameworks") == 0)
        {
            Settings::bundleFrameworks(true);
            continue;
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dest-dir") == 0)
        {
            i++;
            Settings::destFolder(argv[i]);
            continue;
        }
        else if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--install-path") == 0)
        {
            i++;
            Settings::insideLibPath(argv[i]);
            continue;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--search-path") == 0)
        {
            i++;
            Settings::addUserSearchPath(argv[i]);
            continue;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--ignore") == 0)
        {
            i++;
            Settings::ignorePrefix(argv[i]);
            continue;
        }
        else if (strcmp(argv[i], "-of") == 0 || strcmp(argv[i], "--overwrite-files") == 0)
        {
            Settings::canOverwriteFiles(true);
            continue;
        }
        else if (strcmp(argv[i], "-cd") == 0 || strcmp(argv[i], "--create-dir") == 0)
        {
            Settings::canCreateDir(true);
            continue;
        }
        else if (strcmp(argv[i], "-od") == 0 || strcmp(argv[i], "--overwrite-dir") == 0)
        {
            Settings::canOverwriteDir(true);
            Settings::canCreateDir(true);
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--just-print") == 0)
        {
            Settings::bundleLibs(false);
            continue;
        }
        else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0)
        {
            Settings::quietOutput(true);
            continue;
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            Settings::verboseOutput(true);
            continue;
        }
        else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--bundle-libs") == 0)
        {
            // old flag, on by default now. ignore.
            continue;
        }
        else if (strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0)
        {
            cout << "dylibbundler " << VERSION << endl;
            exit(0);
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            showHelp();
            exit(0);
        }
        else if (i > 0)
        {
            // unknown flag, abort
            cerr << "Unknown flag " << argv[i] << endl << endl;
            showHelp();
            exit(1);
        }
    }

    if (Settings::filesToFixCount() < 1)
    {
        showHelp();
        exit(0);
    }

    cout << "Collecting dependencies...\n";

    for (const auto& file_to_fix : Settings::filesToFix())
        collectDependencies(file_to_fix);

    collectSubDependencies();
    bundleDependencies();

    return 0;
}
