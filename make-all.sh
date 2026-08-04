#!/bin/bash

sec2min() { printf "%d:%02d" "$((10#$1 / 60))" "$((10#$1 % 60))"; }

makei18n() {
    cd po
    make update-po
    cd ..
}

makeDocs() {
    cd docs
    make Docs
    cd ..
}

SECONDS=0

# delete generated files (maybe remove from git)
#rm -v include/icons.cpp include/c64_gfx.cpp include/c64_font.cpp include/title.cpp include/gamebackground.cpp \
#      include/gdash_icon_32.cpp include/gdash_icon_48.cpp include/levels.cpp  include/ataripal.cpp include/dtvpal.cpp include/for_html.cpp
rm -v po/gdash.pot po/de.gmo po/hu.gmo
rm -v ./configure ./aclocal.m4 ./config.h.in
find . \( -name "Makefile" -o -name "Makefile.in" \) -exec rm -v {} \;
# NOTE: we keep po/Makefile.in.in, po/POTFILES.in

#autoupdate # updates configure.ac
autoreconf # one command instead of the following four commands
#aclocal
#automake
#autoconf
#autoheader

case "$(uname -s)" in
    Linux*)     platform=Linux;;
    Darwin*)    platform=Mac
                export LDFLAGS="-framework OpenGL";;
    CYGWIN*)    platform=cygwin
                # makeDocs below requires X server
                export DISPLAY=:0.0
                ps | grep /usr/bin/xinit > /dev/null || \
                { startxwin > /dev/null 2>&1 & \
                  echo Wait 5s for X server...; \
                  sleep 5; } 
                ;;
    MINGW*)     platform=MinGW;;
    MSYS_NT*)   platform=Git;;
    *)          platform="UNKNOWN:$(uname -s)"
esac

./configure CPPFLAGS=-Wno-deprecated-declarations && make clean && makei18n && make && makeDocs

echo build time: $(sec2min $SECONDS)
echo -e "\a"
