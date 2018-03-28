#!/bin/bash

MINGW=i686

pacman -Sy base-devel mingw-w64-$MINGW-toolchain git

exprort HB_ARCHITECTURE=win
export HB_COMPILER=mingw

export PATH=/c/msys64/mingw32/bin/$PATH
