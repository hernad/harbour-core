#!/bin/bash

# Configure
cd "$(dirname "$0")"
source 'ci-library.sh'
deploy_enabled && mkdir artifacts
git_config user.email 'hernad@bring.out.ba'
git_config user.name  'Ernad Husremovic'
git config core.autocrlf true
# git remote add upstream 'https://github.com/hernad/harbour-core'
# git fetch --quiet upstream
# reduce time required to install packages by disabling pacman's disk space checking
sed -i 's/^CheckSpace/#CheckSpace/g' /etc/pacman.conf

# Detect
list_commits  || failure 'Could not detect added commits'
# list_packages || failure 'Could not detect changed files'
message 'Processing changes' "${commits[@]}"
# test -z "${packages}" && success 'No changes in package recipes'
# define_build_order || failure 'Could not determine build order'

# Build
# message 'Building packages' "${packages[@]}"
# execute 'Updating system' update_system
# execute 'Approving recipe quality' check_recipe_quality
# for package in "${packages[@]}"; do
#     execute 'Building binary' makepkg-mingw --noconfirm --noprogressbar --skippgpcheck --nocheck --syncdeps --rmdeps --cleanbuild
#     execute 'Building source' makepkg --noconfirm --noprogressbar --skippgpcheck --allsource --config '/etc/makepkg_mingw64.conf'
#     execute 'Installing' yes:pacman --noprogressbar --upgrade *.pkg.tar.xz
#     deploy_enabled && mv "${package}"/*.pkg.tar.xz artifacts
#     deploy_enabled && mv "${package}"/*.src.tar.gz artifacts
#     unset package
# done

# Deploy
# deploy_enabled && cd artifacts || success 'All packages built successfully'
# execute 'Generating pacman repository' create_pacman_repository "${PACMAN_REPOSITORY_NAME:-ci-build}"
# execute 'Generating build references'  create_build_references  "${PACMAN_REPOSITORY_NAME:-ci-build}"
# execute 'SHA-256 checksums' sha256sum *
# success 'All artifacts built successfully'

set

export WIN_DRIVE=$1

pacman --noconfirm -S curl zip unzip
pacman --noconfirm -S --needed mingw-w64-$MINGW_ARCH-postgresql mingw-w64-$MINGW_ARCH-icu mingw-w64-$MINGW_ARCH-curl mingw-w64-$MINGW_ARCH-openssl

# export HB_ARCHITECTURE=win 

# D:\msys64\mingw32\bin\gcc.exe
# cygpath `which gcc` -d
# /mingw64/include/libpq-fe.h

# win64
# mingw64


if [ "$HB_COMPILER" == "mingw64" ] ; then
   # PATH=/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0/:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl
   MINGW_BASE='mingw64'
else
   # PATH=/mingw32/bin:/usr/local/bin:/usr/bin:/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0/:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl
   MINGW_BASE='mingw32' 
fi
export MINGW_INCLUDE=$WIN_DRIVE:\\\\msys64\\\\${MINGW_BASE}\\\\include

export HB_WITH_CURL=${MINGW_INCLUDE} HB_WITH_OPENSSL=${MINGW_INCLUDE} HB_WITH_PGSQL=${MINGW_INCLUDE} HB_WITH_ICU=${MINGW_INCLUDE} 
export HB_INSTALL_PREFIX=$(pwd)/artifacts

echo "install to: $HB_INSTALL_PREFIX"
# WIN_DRIVE:\\\\harbour 
# export HB_VER=${APPVEYOR_REPO_TAG_NAME:=0.0.0}

set

./win-make.exe 
./win-make.exe install

