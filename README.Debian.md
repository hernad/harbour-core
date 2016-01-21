

change log
-----------

    dch -iU -m
    apt-get purge harbour
    dpkg-buildpackage



<pre>
Distribution: unstable
Urgency: medium
Maintainer: Ernad Husremovic <hernad@bring.out.ba>
Changed-By: Ernad Husremovic <hernad@bring.out.ba>
Description:
 harbour    - harbour programming language
Changes:
 harbour (3.4.0-4) unstable; urgency=medium
 .
   * debian info
Checksums-Sha1:
 c06735efceaa30bcb8f9343e7d20a534eef93dc3 568 harbour_3.4.0-4.dsc
 6677a0ead0dbbcc803483007f4af8f8ce01843b6 9401816 harbour_3.4.0-4_amd64.deb
Files:
 5bf4f392f277b4da3e19acdfca80da37 568 devel optional harbour_3.4.0-4.dsc
 84b97be3b23f08464089cc91bd122b9e 9401816 devel optional harbour_3.4.0-4_amd64.deb

</pre>


     dput bintray harbour_3.4.0-4_amd64.changes 


     curl -u $BINTRAY_API_USER:$BINTRAY_API_KEY -X POST https://api.bintray.com/content/hernad/deb/harbour/3.4.0/publish

        => {files:3}


