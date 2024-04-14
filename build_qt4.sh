#!/bin/bash

echo "Trying to build PE-bear..."

#QT check

QT_VER=$(qmake -v)
QTV="version"
if echo "$QT_VER" | grep -q "$QTV"; then
    QT4_FOUND=$(whereis qt4)
    if echo "$QT4_FOUND" | grep -q "lib"; then
        echo "[+] Qt4 found!"
    else
        echo "Install Qt4 SDK first"
        exit -2
    fi
else
    echo "Install Qt4 SDK first"
    exit -1
fi

CMAKE_VER=$(cmake --version)
CMAKEV="cmake version"
if echo "$CMAKE_VER" | grep -q "$CMAKEV"; then
    echo "[+] CMake found!"
else
    echo "[-] CMake NOT found!"
    echo "Install cmake first"
    exit -1
fi

echo $CMAKE_VER
mkdir build_qt4
echo "[+] build directory created"
cd build_qt4
cmake -DUSE_QT4=ON -DCMAKE_INSTALL_PREFIX:PATH=$(pwd) ..
cmake --build . --target install
make
cd ..
