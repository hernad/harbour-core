#!/bin/bash

export HB_ROOT=~/harbour
export HB_INSTALL_PREFIX=$HB_ROOT


export PATH=$PATH:$HB_ROOT/bin:$PATH

export F18_GT_CONSOLE=1
export F18_DEBUG=1
export F18_POS=1

echo 'dnf install xorg-x11-fonts-misc terminus-fonts terminus-fonts-console'
echo 'dnf install postgresql-devel libX11-devel libstdc++-static'
echo 'dnf groupinstall "Development Tools" "Development Libraries"''
echo 'dnf install gcc-c++ # node native'
