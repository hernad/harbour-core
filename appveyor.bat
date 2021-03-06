
REM https://superuser.com/questions/345964/start-bash-shell-cygwin-with-correct-path-without-changing-directory
set CHERE_INVOKING=yes

rem Matrix-driven Appveyor CI script for libmypaint
rem Currently only does MSYS2 builds.
rem https://www.appveyor.com/docs/installed-software#mingw-msys-cygwin
rem Needs the following vars:
rem    MSYS2_ARCH:  x86_64 or i686
rem    MSYSTEM:  MINGW64 or MINGW32

rem Set the paths appropriately
set PATH=C:\msys64\%MSYSTEM%\bin;C:\msys64\usr\bin;%PATH%

rem Upgrade the MSYS2 platform
REM bash -lc "pacman --noconfirm --sync --refresh --refresh pacman"
REM bash -lc "pacman --noconfirm --sync --refresh --refresh --sysupgrade --sysupgrade"
bash -xc "pacman --noconfirm -S curl zip unzip"

rem Install required tools
REM bash -xlc "pacman --noconfirm -S --needed base-devel"
REM bash -xlc "pacman --noconfirm -S --needed mingw-w64-i686-toolchain git"

rem Install the relevant native dependencies
REM bash -xlc "pacman --noconfirm -S --needed mingw-w64-%MSYS2_ARCH%-json-c"
REM bash -xlc "pacman --noconfirm -S --needed mingw-w64-%MSYS2_ARCH%-glib2"
REM bash -xlc "pacman --noconfirm -S --needed mingw-w64-%MSYS2_ARCH%-gobject-introspection"
bash -xc "pacman --noconfirm -S --needed mingw-w64-%MSYS2_ARCH%-postgresql mingw-w64-%MSYS2_ARCH%-icu mingw-w64-%MSYS2_ARCH%-curl mingw-w64-%MSYS2_ARCH%-openssl"


rem Invoke subsequent bash in the build tree
REM cd %APPVEYOR_BUILD_FOLDER%
REM bash -xlc "cd /c ; curl -LO https://dl.bintray.com/hernad/windows/hbwin.tar.gz ; tar xf hbwin.tar.gz"
REM set PATH=C:\hbwin\bin;%PATH%

rem Build/test scripting
bash -xc "set pwd"

REM 4x backslash to get '\' 
bash -xc "export HB_ARCHITECTURE=win HB_COMPILER=mingw MINGW_INCLUDE=C:\\\\msys64\\\\%MSYSTEM%\\\\include; export HB_WITH_CURL=${MINGW_INCLUDE} HB_WITH_OPENSSL=${MINGW_INCLUDE} HB_WITH_PGSQL=${MINGW_INCLUDE} HB_WITH_ICU=${MINGW_INCLUDE} HB_INSTALL_PREFIX=C:\\\\projects\\\\harbour-core\\\\harbour HB_VER=${APPVEYOR_REPO_TAG_NAME:=0.0.0}; ./win-make.exe ; ./win-make.exe install"

REM postgresql dlls libpq.dll i kompanija
REM bash -xlc "curl -LO https://dl.bintray.com/hernad/F18/postgresql_windows_x86_dlls.zip; unzip postgresql_windows_x86_dlls.zip"

bash -xc "zip harbour_${BUILD_ARTIFACT}_${APPVEYOR_REPO_TAG_NAME}.zip -r harbour"
