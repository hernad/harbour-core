#!/bin/bash

set

if [ "$BUILD_ARCH" == "ia32" ] ; then

   dpkg -L libpq5:i386
   # /usr/lib/libpq.so.5

   export HB_USER_CFLAGS=-m32
   export HB_USER_DFLAGS='-m32 -L/usr/lib32'
   export HB_USER_LDFLAGS='-m32 -L/usr/lib32'
   
else
   sudo apt-get update -y
   sudo apt-get install -y g++ gcc libc6 \
      libx11-dev libpcre3-dev libssl-dev \
      libncurses5 libstdc++6  libpq-dev lib32z1
fi

echo "install to: $HB_INSTALL_PREFIX"

set

export HB_INSTALL_PREFIX=$(pwd)/artifacts
set | grep HB_

make
make install



