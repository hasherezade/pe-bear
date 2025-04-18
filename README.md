PE-bear
-

<img src="./logo/main_ico.png" alt="PE-bear logo" width=128>

[![Build status](https://ci.appveyor.com/api/projects/status/q2smuy32pqqo0oyn?svg=true)](https://ci.appveyor.com/project/hasherezade/pe-bear)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/14648384b52b4d979bc1f2246edbd496)](https://app.codacy.com/gh/hasherezade/pe-bear/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Last Commit](https://img.shields.io/github/last-commit/hasherezade/pe-bear/main)](https://github.com/hasherezade/pe-bear/commits)

[![GitHub release](https://img.shields.io/github/release/hasherezade/pe-bear.svg)](https://github.com/hasherezade/pe-bear/releases) 
[![Github All Releases](https://img.shields.io/github/downloads/hasherezade/pe-bear/total.svg)](https://github.com/hasherezade/pe-bear/releases)
[![Github Latest Release](https://img.shields.io/github/downloads/hasherezade/pe-bear/latest/total.svg)](https://github.com/hasherezade/pe-bear/releases) 

PE-bear is a multiplatform reversing tool for PE files. Its objective is to deliver fast and flexible “first view” for malware analysts, stable and capable to handle malformed PE files.

Signatures for PE-bear:
+ [SIG.txt](SIG.txt) (updated: Oct 17, 2022) - *contains signatures from [PEid's UserDB](http://www.softpedia.com/get/Programming/Packers-Crypters-Protectors/PEiD-updated.shtml) - converted by a script provided by [crashish](http://crashish.blogspot.com/2013/09/peid-signature-conversion-for-pe-bear.html)*

## Builds

📦 ⚙️ Download the latest [release](https://github.com/hasherezade/pe-bear/releases).


### Windows Packaging

Available also via:
+  ![](https://community.chocolatey.org/favicon.ico) [Chocolatey](https://community.chocolatey.org/packages/pebear)
+  ![](https://avatars.githubusercontent.com/u/16618068?s=15) [Scoop](https://scoop.sh/#/apps?q=pe-bear)
+  [WinGet](https://learn.microsoft.com/en-us/windows/package-manager/winget/) (`winget install pe-bear`)
      
### Test Builds

🧪 Fresh **test builds** (ahead of the official release) can be downloaded from the [AppVeyor build server](https://ci.appveyor.com/project/hasherezade/pe-bear). They are created on each commit to the `main` branch. You can download them by clicking on the build version, then choosing the tab `Artifacts`. WARNING: those builds may be unstable.

> An archive of **old releases** is available here: https://github.com/hasherezade/pe-bear-releases

### Available releases

The **Linux** build requires appropriate version of **Qt to be installed**.

The **Windows** build with *vs13* suffix(built with Visual Studio 2013) has no external dependencies.

The **Windows** build with *vs19* suffix (built with Visual Studio 2019) requires the [redistributable package for Visual Studio 2015 - 2022](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).

The **Windows** build with *vs10* suffix is built with Qt4 (legacy) - in contrast to the other builds that are with Qt5 (recommended). It is prepared for the purpose of backward compatibility with old versions of Windows (i.e. XP), and may be lacking some of the features.

## How to build

### Requires:

+   [git](https://git-scm.com/downloads)
+   [cmake](http://www.cmake.org)
+   [Qt6](https://www.qt.io/download) (optional: Qt5, Qt4)
+   bearparser (submodule)
+   capstone (submodule)
+   sig_finder (submodule)

### Clone

Use **recursive clone** to get the repo together with submodules:

```console
git clone --recursive https://github.com/hasherezade/pe-bear.git
```

### Building on Windows

Use [CMake](http://www.cmake.org) to generate a Visual Studio project. Open in Visual Studio and build.


### Building on Linux and MacOS

To build it on Linux or MacOS you can use the given scripts:
+   [build.sh](./build.sh) - default, builds with the latest Qt
+   [build_qt6.sh](./build_qt6.sh) - builds with Qt6
+   [build_qt5.sh](./build_qt5.sh) - builds with Qt5
+   [build_qt4.sh](./build_qt4.sh) - builds with Qt4

To generate the `.app` bundle on MacOS you can use:
+   [macos_wrap.sh](./macos_wrap.sh)

More info on [📖 Wiki](https://github.com/hasherezade/pe-bear/wiki/Building-from-sources).

---

If you like PE-bear, you can support it by buying [the merch 🐻](https://teespring.com/pe-bear-ate-my-malwarez-v2?pid=377)

