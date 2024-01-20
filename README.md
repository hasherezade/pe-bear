PE-bear
-

<img src="./logo/main_ico.png" alt="PE-bear logo">

[![Build status](https://ci.appveyor.com/api/projects/status/q2smuy32pqqo0oyn?svg=true)](https://ci.appveyor.com/project/hasherezade/pe-bear)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/14648384b52b4d979bc1f2246edbd496)](https://www.codacy.com/gh/hasherezade/pe-bear/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=hasherezade/pe-bear&amp;utm_campaign=Badge_Grade)
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

![](https://community.chocolatey.org/favicon.ico) Available also via [Chocolatey](https://community.chocolatey.org/packages/pebear)

🧪 Fresh **test builds** (ahead of the official release) can be downloaded from the [AppVeyor build server](https://ci.appveyor.com/project/hasherezade/pe-bear). They are created on each commit to the `main` branch. You can download them by clicking on the build version, then choosing the tab `Artifacts`. WARNING: those builds may be unstable.

> An archive of **old releases** is available here: https://github.com/hasherezade/pe-bear-releases

### Available releases

The **Linux** build requires appropriately **Qt_5.14 or Qt_5.15 to be installed**.

The **Windows** build with *vs13* suffix(built with Visual Studio 2013) has no external dependencies.

The **Windows** build with *vs19* suffix (built with Visual Studio 2019) requires [Redistributable packages for Visual Studio 2015, 2017, 2019, and 2022](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170).

The **Windows** build with *vs10* suffix is built with Qt4 (legacy) - in contrast to the other builds that are with Qt5 (recommended). It is prepared for the purpose of backward compatibility with old versions of Windows (i.e. XP).

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


---

If you like PE-bear, you can support it:

[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/donate/?hosted_button_id=FQX9B9VHCRBF4)

