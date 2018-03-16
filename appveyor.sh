#!/bin/bash

echo "hello world"

#sudo apt install -y zip

echo "artifakt: $BUILD_ARTIFACT tag: $APPVEYOR_REPO_TAG_NAME pwd: $(pwd)"

gcc --version

# https://redmine.bring.out.ba/issues/35387

sudo dpkg --add-architecture i386
#curl -LO https://dl.bintray.com/hernad/harbour/hb-linux-i386.tar.gz
#tar xf hb-linux-i386.tar.gz

export HB_PLATFORM=linux
export HB_ROOT=$(pwd)/harbour
export HB_USER_CFLAGS=-m32
export HB_USER_DFLAGS='-m32 -L/usr/lib32'
export HB_USER_LDFLAGS='-m32 -L/usr/lib32'


export MINGW_INCLUDE=/usr/include
export HB_WITH_CURL=$MINGW_INCLUDE HB_WITH_SSL=$MINGW_INCLUDE HB_WITH_PGSQL=$MINGW_INCLUDE

sudo apt-get update -y
sudo apt-get build-essential flex bison

sudo apt install -y g++-multilib gcc-multilib libc6:i386 \
     libx11-dev:i386 libpcre3-dev:i386 libssl-dev:i386 libcurl-dev:i386 \
     libncurses5:i386 libstdc++6:i386 lib32stdc++6  libpq-dev:i386 lib32z1

PATH=$HB_ROOT/bin:$PATH

echo $PATH

export F18_VER=${APPVEYOR_REPO_TAG_NAME:=0.0.0}
scripts/update_f18_ver_ch.sh $F18_VER

export LX_UBUNTU=1
#source scripts/set_envars.sh

export F18_POS=1
export F18_RNAL=1
hbmk2 -workdir=.h F18.hbp

cp -av /usr/lib/i386-linux-gnu/libpq.so* .

HB_INSTALL_PREFIX=$(pwd)/harbour

set MINGW_INCLUDE=c:\msys64\%MYS2_ARCH%\include 
set HB_WITH_CURL=%MINGW_INCLUDE% 
set HB_WITH_OPENSSL=%MINGW_INCLUDE% 
set HB_WITH_PGSQL=%MINGW_INCLUDE%

make
make install

zip harbour_${BUILD_ARTIFACT}_${APPVEYOR_REPO_TAG_NAME}.zip harbour