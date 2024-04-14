#!/bin/bash

echo "Trying to build PE-bear..."

#QT check

QT_VER=$(qmake -v)
str="$QT_VER"
substr="Qt version 5"

echo $QT_VER
if [[ $str == *"$substr"* ]]; then
    echo "[+] Qt5 found!"
else
    str2=$(whereis qt5)
    substr2="/qt5"
    if [[ $str2 == *"$substr2"* ]]; then
        echo "[+] Qt5 found!"
    else
        echo "Install Qt5 SDK first"
        exit -1
    fi
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
mkdir build_qt5
echo "[+] build directory created"
cd build_qt5
cmake cmake -DUSE_QT5=ON -DCMAKE_INSTALL_PREFIX:PATH=$(pwd) ..
cmake --build . --target install
make
cd ..

