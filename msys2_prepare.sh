#!/bin/bash

export MINGWARCH=i686

echo "run install packages (arch: $MINGWARCH):"
echo pacman -Sy base-devel mingw-w64-$MINGWARCH-toolchain git
echo pacman -Sy mingw-w64-$MINGWARCH-postgresql

export MINGW_INCLUDE="C:\\msys64\\${MINGWARCH}\\include"
echo $MINGW_INCLUDE
export HB_WITH_CURL=${MINGW_INCLUDE} HB_WITH_OPENSSL=${MINGW_INCLUDE} HB_WITH_PGSQL=${MINGW_INCLUDE} HB_WITH_ICU=${MINGW_INCLUDE} 

export HB_ARCHITECTURE=win
export HB_COMPILER=mingw

set | grep HB_

export PATH=/c/msys64/mingw32/bin:$PATH

cd $HOME

export HB_ROOT=$(pwd)/harbour-core/harbour

export HB_INSTALL_PREFIX=$HB_ROOT
export PATH=$PATH:$HB_ROOT/bin:$PATH:$(pwd)/node-x86

export HB_USER_CFLAGS=-DHB_TR_LEVEL=5
