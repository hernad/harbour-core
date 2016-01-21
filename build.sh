#/bin/bash

COMMENT=${1:-"updates from upstream"}

echo "changelog comment:" $COMMENT

dch -iU -m  "$COMMENT" --distribution "unstable"
apt-get purge harbour

mv .git ..

rm ../harbour_3.4.*_amd64.changes

dpkg-buildpackage

mv ../.git .

cd ..
dput bintray harbour_3.4.*_amd64.changes 
curl -u $BINTRAY_API_USER:$BINTRAY_API_KEY -X POST https://api.bintray.com/content/hernad/deb/harbour/3.4.0/publish

cd harbour-core
echo end
