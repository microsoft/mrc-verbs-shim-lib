#!/bin/bash

set -x

arch=$(uname -m)
DOCA_LIB_DIR=${DOCA_LIB_DIR:-"/opt/mellanox/doca/lib/$(arch)-linux-gnu/"}

export MRC_LIB_PATH=$DOCA_LIB_DIR
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH # Needed to resolve locations of doca libraries that libnv_mrc.so depends on.

# Please put absolute paths here.
export VMRC_LIBMRC_SO=${VMRC_LIBMRC_SO:-"$DOCA_LIB_DIR/libnv_mrc.so"}
export VMRC_LIBIBVERBS_SO=${VMRC_LIBIBVERBS_SO:-"/lib/${arch}-linux-gnu/libibverbs.so.1"}

if [ -f "$PWD/mrc-header-lib/system.json" ]; then
    export VMRC_SYSTEM_JSON="$PWD/mrc-header-lib/system.json"
else
    export VMRC_SYSTEM_JSON=${VMRC_SYSTEM_JSON:-"/opt/mrc-config/system.json"}
fi

#LD_PRELOAD=$PWD/libibverbs.so ./tests/check_ibv_overwrites_dlopen
LD_PRELOAD=$PWD/libibverbs.so ./tests/check_ibv_overwrites
