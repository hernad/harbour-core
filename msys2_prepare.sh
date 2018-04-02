#!/bin/bash

MINGW=i686

echo "run install packages:"
echo pacman -Sy base-devel mingw-w64-$MINGW-toolchain git

export HB_ARCHITECTURE=win
export HB_COMPILER=mingw

export PATH=/c/msys64/mingw32/bin:$PATH

export HB_INSTALL_PREFIX=$(pwd)/harbour
export PATH=$PATH:$(pwd)/harbour/bin:$PATH:$(pwd)/node-x86

export HB_USER_CFLAGS=-DHB_TR_LEVEL=5