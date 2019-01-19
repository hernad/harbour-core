#!/bin/bash

# https://github.com/$BINTRAY_OWNER/greenbox/blob/apps_modular/upload_app.sh
# https://github.com/hernad/azure_pipelines_test/blob/master/upload_bintray.sh

#cd "$(dirname "$0")"

BINTRAY_API_KEY=${BINTRAY_API_KEY:-`cat bintray_api_key`}
BINTRAY_OWNER=hernad
BINTRAY_REPOS=bringout
BINTRAY_PACKAGE=harbour-linux-${BINTRAY_ARCH}
BINTRAY_PACKAGE_VER=$BUILD_BUILDNUMBER

FILE=${BINTRAY_PACKAGE}_${BINTRAY_PACKAGE_VER}.zip

cd artifacts && zip -r -v $FILE .

ls -lh $FILE

set
echo uploading $FILE to bintray ...

CURL=curl

$CURL -s -T $FILE \
      -u $BINTRAY_OWNER:$BINTRAY_API_KEY \
      --header "X-Bintray-Override: 1" \
     https://api.bintray.com/content/$BINTRAY_OWNER/$BINTRAY_REPOS/$BINTRAY_PACKAGE/$BINTRAY_PACKAGE_VER/$FILE

$CURL -s -u $BINTRAY_OWNER:$BINTRAY_API_KEY \
   -X POST https://api.bintray.com/content/$BINTRAY_OWNER/$BINTRAY_REPOS/$BINTRAY_PACKAGE/$BINTRAY_PACKAGE_VER/publish

