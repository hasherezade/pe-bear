PE-bear
-
<img src="./logo/main_ico.png" alt="PE-bear logo">

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/38e58d4c65e243469dd7a7adfd1c85a3)](https://app.codacy.com/gh/hasherezade/pe-bear?utm_source=github.com&utm_medium=referral&utm_content=hasherezade/pe-bear&utm_campaign=Badge_Grade_Settings)
[![Build status](https://ci.appveyor.com/api/projects/status/q2smuy32pqqo0oyn?svg=true)](https://ci.appveyor.com/project/hasherezade/pe-bear)
[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

[![GitHub release](https://img.shields.io/github/release/hasherezade/pe-bear.svg)](https://github.com/hasherezade/pe-bear/releases) 
[![Github All Releases](https://img.shields.io/github/downloads/hasherezade/pe-bear-releases/total.svg)](https://github.com/hasherezade/pe-bear-releases/releases) 
[![Github Latest Release](https://img.shields.io/github/downloads/hasherezade/pe-bear/latest/total.svg)](https://github.com/hasherezade/pe-bear/releases) 

PE-bear is a multiplatform reversing tool for PE files. Its objective is to deliver fast and flexible “first view” for malware analysts, stable and capable to handle malformed PE files.

Signatures for PE-bear:
+ [SIG.txt](SIG.txt) (updated: 22.01.2014) - *contains signatures from [PEid's UserDB](http://www.softpedia.com/get/Programming/Packers-Crypters-Protectors/PEiD-updated.shtml) - converted by a script provided by [crashish](http://crashish.blogspot.com/2013/09/peid-signature-conversion-for-pe-bear.html)*

## Builds

📦 ⚙️ Download the latest [release](https://github.com/hasherezade/pe-bear/releases).

![](https://community.chocolatey.org/favicon.ico) Available also via [Chocolatey](https://community.chocolatey.org/packages/pebear)

🧪 Fresh **test builds** (ahead of the official release) can be downloaded from the [AppVeyor build server](https://ci.appveyor.com/project/hasherezade/pe-bear). They are created on each commit to the `main` branch. You can download them by clicking on the build version, then choosing the tab `Artifacts`. WARNING: those builds may be unstable.

> An archive of **old releases** is available here: https://github.com/hasherezade/pe-bear-releases

## How to build

### Requires:

+   [git](https://git-scm.com/downloads)
+   [cmake](http://www.cmake.org)
+   [Qt5](https://www.qt.io/download) (optionally Qt4)
+   bearparser (submodule of the current repository)
+   capstone (submodule of the current repository)

### Clone

Use **recursive clone** to get the repo together with submodules:

```console
git clone --recursive https://github.com/hasherezade/pe-bear.git
```

### Building on Windows

Use [CMake](http://www.cmake.org) to generate a Visual Studio project. Open in Visual Studio and build.


### Building on Linux and MacOS

To build it on Linux or MacOS you can use the given scripts:
+   [build.sh](./build.sh) - default, builds with Qt5
+   [build_qt5.sh](./build_qt5.sh) - builds with Qt5
+   [build_qt4.sh](./build_qt4.sh) - builds with Qt4

To generate the `.app` bundle on MacOS you can use:
+   [macos_wrap.sh](./macos_wrap.sh)
