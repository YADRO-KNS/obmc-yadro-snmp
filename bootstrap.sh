#!/bin/sh

PREFIX=$(readlink -f "${0%/*}")
AUTOCONF_FILES="aclocal.m4 autom4te.cache compile \
        config.guess config.sub configure depcomp \
        install-sh ltmain.sh Makefile.in missing  \
        ar-lib config.h.in"

TARGET=${PREFIX}/yadro/.libs/yadro.so

case $1 in
    clean)
        cd ${PREFIX}
        test -f Makefile && make maintainer-clean
        for file in ${AUTOCONF_FILES}; do
            find -name "$file" | xargs -r rm -rf
        done
        exit 0
        ;;
esac

autoreconf --install ${PREFIX}
echo 'Run: "./configure ${CONFIGURE_FLAGS} && make"'
