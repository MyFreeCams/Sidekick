mac dylib bundler v2
================


About
-----

Mac OS X (now macOS) introduced an innovative and very useful way to package applications: app bundles.
While their design has all that is needed to ease distribution of resources and frameworks, it
seems like dynamic libraries (.dylib) are very complicated to distribute. Sure, applications developed
specifically for macOS (OS X) won't make use of them, however applications ported from Linux or other Unices may have
dependencies that will only compile as dylibs. By default, there exists no mechanism to bundle them but some command-line utilities provided by Apple - however it turns out that for a single program it is often necessary to issue dozens of commands! This often leads each developer to create their own "home solution" which are often hacky, non-portable and/or suboptimal.

**dylibbundler** is a small command-line programs that aims to make bundling .dylibs as easy as possible.
It automatically determines which dylibs are needed by your program, copies these libraries inside the app bundle, and fixes both them and the executable to be ready for distribution... all this with a single command! It will also work if your program uses plug-ins that have dependencies too.

It usually involves 2 actions :
* Creating a directory (by default called *Frameworks*) that can be placed inside the *Contents* folder of the app bundle.
* Fixing the executable file so that it is aware of the new location of its dependencies.


Installation
------------
In Terminal, cd to the main directory of dylibbundler and type "make". You can install with "sudo make install".


Using dylibbundler
----------------------------------
Here is a list of flags you can pass to dylibbundler:

`-h`, `--help`
<blockquote>
Displays a summary of options
</blockquote>

`-a`, `--app` (path to app bundle)
<blockquote>
Application bundle to make self-contained. Fixes the main executable of the app bundle. Add additional binary files to fix up with the `-x` flag.
</blockquote>

`-x`, `--fix-file` (executable or plug-in filepath)
<blockquote>
Fixes given executable or plug-in file (ex: .dylib, .so). Anything on which `otool -L` works is accepted by `-x`. dylibbundler will walk through the dependencies of the specified file to build a dependency list. It will also fix the said files' dependencies so that it expects to find the libraries relative to itself (e.g. in the app bundle) instead of at an absolute path (e.g. /usr/local/lib). To pass multiple files to fix, simply specify multiple `-x` flags.
</blockquote>

<!--
`-b`, `--bundle-deps`
<blockquote>
Copies libaries to a local directory, fixes their internal name so that they are aware of their new location,
fixes dependencies where bundled libraries depend on each other. If this option is not passed, no libraries will be prepared for distribution.
</blockquote>
 -->

`-f`, `--frameworks`
<blockquote>
Copy framework dependencies to app bundle and fix internal names and rpaths. If this option is not passed, dependencies contained in frameworks will be ignored. dylibbundler will also deploy Qt frameworks & plugins, eliminating the need to use `macdeployqt`.
</blockquote>

`-d`, `--dest-dir` (directory)
> Sets the name of the directory in wich distribution-ready dylibs will be placed, relative to `./MyApp.app/Contents/`. (Default is `Frameworks`).

`-p`, `--install-path` (libraries install path)
> Sets the "inner" installation path of libraries, usually inside the bundle and relative to executable. (Default is `@executable_path/../Frameworks/`, which points to a directory named `Frameworks` inside the `Contents` directory of the bundle.)

*The difference between `-d` and `-p` is that `-d` is the location dylibbundler will put files in, while `-p` is the location where the libraries will be expected to be found when you launch the app (often using @executable_path, @loader_path, or @rpath).*

`-s`, `--search-path` (search path)
> Check for libraries in the specified path.

`-i`, `--ignore` (path)
> Dylibs in (path) will be ignored. By default, dylibbundler will ignore libraries installed in `/usr/lib` & `/System/Library` since they are assumed to be present by default on all macOS installations. *(It is usually recommend not to install additional stuff in `/usr/`, always use ` /usr/local/` or another prefix to avoid confusion between system libs and libs you added yourself)*

`-of`, `--overwrite-files`
> When copying libraries to the output directory, allow overwriting files when one with the same name already exists.

`-cd`, `--create-dir`
> If the output directory does not exist, create it.

`-od`, `--overwrite-dir`
> If the output directory already exists, completely erase its current content before adding anything to it. (This option implies --create-dir)

`-n`, `--just-print`
> Print the dependencies found (without copying into app bundle).

`-q`, `--quiet`
> Less verbose output.

`-v`, `--verbose`
> More verbose output (only recommended for debugging).

`-V`, `--version`
> Print dylibbundler version number and exit.

A command may look like
`% dylibbundler -cd -of -f -q -a ./HelloWorld.app -x ./HelloWorld.app/Contents/PlugIns/printsupport`
