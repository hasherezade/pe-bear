# PE-bear

<img src="./logo/main_ico.png" alt="PE-bear logo" width=128>

[![Build status](https://ci.appveyor.com/api/projects/status/q2smuy32pqqo0oyn?svg=true)](https://ci.appveyor.com/project/0nsec/pe-bear)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/14648384b52b4d979bc1f2246edbd496)](https://app.codacy.com/gh/0nsec/pe-bear/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)
[![Last Commit](https://img.shields.io/github/last-commit/0nsec/pe-bear/main)](https://github.com/0nsec/pe-bear/commits)

[![GitHub release](https://img.shields.io/github/release/0nsec/pe-bear.svg)](https://github.com/0nsec/pe-bear/releases) 
[![Github All Releases](https://img.shields.io/github/downloads/0nsec/pe-bear/total.svg)](https://github.com/0nsec/pe-bear/releases)
[![Github Latest Release](https://img.shields.io/github/downloads/0nsec/pe-bear/latest/total.svg)](https://github.com/0nsec/pe-bear/releases) 

## About This Project

This project is **modified by [0nsec](https://github.com/0nsec)** with AI-powered reverse engineering capabilities.

**Original Project:** [PE-bear by hasherezade](https://github.com/hasherezade/pe-bear)  

**⭐ Credits:** This project is based on the excellent PE-bear tool originally created by **[hasherezade](https://github.com/hasherezade)**. All credit for the core PE analysis functionality goes to her and the original contributors.

---

## Enhanced Features

### AI-Powered Reverse Engineering
- **OpenAI GPT Integration** (GPT-4, GPT-4-turbo, GPT-3.5-turbo)
  - Intelligent code and assembly analysis
  - Automated malware behavior detection
  - Function explanation and deobfuscation suggestions
  
- **Google Gemini API Support** (Gemini Pro, Gemini 1.5)
  - Alternative AI provider option
  - Context-aware analysis
  
- **AI Assistant Features:**
  - Import/Export table analysis with threat assessment
  - Suspicious string detection and categorization
  - Malware family identification
  - Security implication analysis
  - Anti-analysis technique detection

### Enhanced Analysis Tools
- **VirusTotal Integration**
  - File hash scanning
  - Detection rate analysis
  - Malware report retrieval
  
- **Advanced Hashing**
  - MD5, SHA-1, SHA-256, SHA-512
  - Import hash (ImpHash)
  - Rich header hash
  
- **Improved Analysis**
  - Entropy visualization
  - Overlay data detection
  - Enhanced string extraction with Unicode support
  - Suspicious API detection

### Export Capabilities
- JSON structured data export
- XML format export
- Comprehensive analysis reports
- AI analysis results export

### Configuration
- Secure API key management
- Multi-provider AI support
- Customizable AI models
- Persistent settings storage

---

## 🐻 Original Description

PE-bear is a multiplatform reversing tool for PE files. Its objective is to deliver fast and flexible "first view" for malware analysts, stable and capable to handle malformed PE files.

Signatures for PE-bear:
+ [SIG.txt](SIG.txt) (updated: Oct 17, 2022) - *contains signatures from [PEid's UserDB](http://www.softpedia.com/get/Programming/Packers-Crypters-Protectors/PEiD-updated.shtml) - converted by a script provided by [crashish](http://crashish.blogspot.com/2013/09/peid-signature-conversion-for-pe-bear.html)*

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
git clone --recursive https://github.com/0nsec/pe-bear.git
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

> An archive of **old releases** is available here: https://github.com/hasherezade/
pe-bear-releases


## Using the AI Features

### Setup
1. Open PE-bear
2. Go to **AI Assistant → AI Settings**
3. Enter your API keys:
   - **OpenAI**: Get from https://platform.openai.com/api-keys
   - **Gemini**: Get from https://makersuite.google.com/app/apikey
   - **VirusTotal**: Get from https://www.virustotal.com/gui/my-apikey
4. Select your preferred AI model
5. Enable your desired AI provider
6. Save settings

### Features
- **Open AI Assistant** (Ctrl+Alt+A): Access the AI analysis panel
- **Analyze Imports**: AI analyzes imported functions for capabilities and threats
- **Analyze Exports**: AI examines exported functions
- **Analyze Strings**: AI identifies suspicious strings and patterns
- **Detect Malware**: Comprehensive AI-powered malware analysis
- **VirusTotal Scan**: Query file hash against VirusTotal database

---

