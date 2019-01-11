#!/bin/bash

# https://github.com/$BINTRAY_OWNER/greenbox/blob/apps_modular/upload_app.sh

cd "$(dirname "$0")"

BINTRAY_API_KEY=${BINTRAY_API_KEY:-`cat bintray_api_key`}
BINTRAY_OWNER=hernad
BINTRAY_REPOS=harbour
BINTRAY_PACKAGE=harbour-windows-${BINTRAY_ARCH}
BINTRAY_PACKAGE_VER=$BUILD_BUILDNUMBER

#pacman --noconfirm -S curl zip unzip
#pacman --noconfirm -S --needed mingw-w64-$MINGW_ARCH-postgresql mingw-w64-$MINGW_ARCH-icu mingw-w64-$MINGW_ARCH-curl mingw-w64-$MINGW_ARCH-openssl
#mkdir artifacts
#echo "test" > artifacts/test.txt

FILE=${BINTRAY_PACKAGE}_${BINTRAY_PACKAGE_VER}.zip

cd artifacts && zip -r -v $FILE .

ls -lh $FILE

set
echo uploading $FILE to bintray ...

if [ "$HB_COMPILER" == "mingw64" ] ; then
   MINGW_BASE='mingw64'
else
   MINGW_BASE='mingw32' 
fi
CURL=/${MINGW_BASE}/bin/curl

$CURL -s -T $FILE \
      -u $BINTRAY_OWNER:$BINTRAY_API_KEY \
      --header "X-Bintray-Override: 1" \
     https://api.bintray.com/content/$BINTRAY_OWNER/$BINTRAY_REPOS/$BINTRAY_PACKAGE/$BINTRAY_PACKAGE_VER/$FILE

$CURL -s -u $BINTRAY_OWNER:$BINTRAY_API_KEY \
   -X POST https://api.bintray.com/content/$BINTRAY_OWNER/$BINTRAY_REPOS/$BINTRAY_PACKAGE/$BINTRAY_PACKAGE_VER/publish
