#!/bin/bash

set

if [ "$BUILD_ARCH" == "ia32" ] ; then

   sudo apt-get -y remove libpq-dev libpq5

   sudo apt-get install -y g++-multilib gcc-multilib libc6:i386 \
     libx11-dev:i386 libpcre3-dev:i386 libssl-dev:i386 \
     libncurses5:i386 libstdc++6:i386 lib32stdc++6  libpq-dev:i386 lib32z1

   dpkg -L libpq5:i386

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

make
make install

