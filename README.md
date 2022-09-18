PE-bear
-
<img src="./logo/main_ico.png" alt="PE-bear logo">

[![Build status](https://ci.appveyor.com/api/projects/status/q2smuy32pqqo0oyn?svg=true)](https://ci.appveyor.com/project/hasherezade/pe-bear)
[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

[![GitHub release](https://img.shields.io/github/release/hasherezade/pe-bear.svg)](https://github.com/hasherezade/pe-bear/releases) 
[![Github All Releases](https://img.shields.io/github/downloads/hasherezade/pe-bear-releases/total.svg)](https://github.com/hasherezade/pe-bear-releases/releases) 
[![Github Latest Release](https://img.shields.io/github/downloads/hasherezade/pe-bear/latest/total.svg)](https://github.com/hasherezade/pe-bear/releases) 

PE-bear is a freeware reversing tool for PE files. Its objective is to deliver fast and flexible “first view” for malware analysts, stable and capable to handle malformed PE files.

Signatures for PE-bear:
+ [SIG.txt](SIG.txt) (updated: 22.01.2014) - *contains signatures from [PEid's UserDB](http://www.softpedia.com/get/Programming/Packers-Crypters-Protectors/PEiD-updated.shtml) - converted by a script provided by [crashish](http://crashish.blogspot.com/2013/09/peid-signature-conversion-for-pe-bear.html)*

## Clone

Use **recursive clone** to get the repo together with the submodule:

```console
git clone --recursive https://github.com/hasherezade/pe-bear.git
```

## Builds

Download the latest [release](https://github.com/hasherezade/pe-bear/releases).

![](https://community.chocolatey.org/favicon.ico) Available also via [Chocolatey](https://community.chocolatey.org/packages/pebear)

🧪 Fresh **test builds** (ahead of the official release) can be downloaded from the [AppVeyor build server](https://ci.appveyor.com/project/hasherezade/pe-bear). They are created on each commit to the `main` branch. You can download them by clicking on the build version, then choosing the tab `Artifacts`. WARNING: those builds may be unstable.
