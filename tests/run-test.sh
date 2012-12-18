#!/bin/bash

if test -z "$TOP_DIR"; then
    export BASE_DIR="$(dirname $0)"
else
    export BASE_DIR="$TOP_DIR/test"
fi
top_dir="$BASE_DIR/.."

if test -z "$NO_MAKE"; then
    make -C $top_dir >/dev/null || exit 1
fi

if test -z "$CUTTER"; then
    CUTTER="$(make -s -C $top_dir echo-cutter)"
fi

case `uname` in
    CYGWIN*)
        PATH="$top_dir/src/.libs:$PATH"
        ;;
    Darwin)
        DYLD_LIBRARY_PATH="$top_dir/src/.libs:$DYLD_LIBRARY_PATH"
        export DYLD_LIBRARY_PATH
        ;;
    *BSD)
        LD_LIBRARY_PATH="$top_dir/src.libs:$LD_LIBRARY_PATH"
        export LD_LIBRARY_PATH
        ;;
    *)
        :
        ;;
esac

$CUTTER -s $BASE_DIR "$@" $BASE_DIR
