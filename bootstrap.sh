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

    build)
        cd ${PREFIX}
        autoreconf --install && ./configure --enable-tracing && make -C yadro
        ;;

    run)
        if [ ! -e "${TARGET}" ]; then
            echo "Run \"$0 build\" before this action"
            exit 1
        fi

        if [ ${EUID} != 0 ]; then
            echo "This script required root privilegies"
            sudo $0 $@
            exit 0
        fi

        snmpd -f -Le -C -Ddlmod,yadro \
            --rocommunity=public --dlmod="yadro ${TARGET}" \
            --trapcommunity=public --trap2sink=localhost
        ;;

    *)
        echo "Usage: $0 <build | run | clean>"
        exit 1
        ;;
esac
