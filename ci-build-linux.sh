#!/bin/bash

set

sudo apt-get update -y
sudo apt-get install -y g++ gcc libc6 \
     libx11-dev libpcre3-dev libssl-dev \
     libncurses5 libstdc++6  libpq-dev lib32z1


echo "install to: $HB_INSTALL_PREFIX"

set

HB_INSTALL_PREFIX=$(pwd)/artifacts

make
make install

