#!/bin/bash

MINGW=i686

pacman -Sy base-devel mingw-w64-$MINGW-toolchain git

export HB_ARCHITECTURE=win
export HB_COMPILER=mingw

export PATH=/c/msys64/mingw32/bin/$PATH

export HB_INSTALL_PREFIX=$(pwd)/harbour
