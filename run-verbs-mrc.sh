#!/bin/bash

set -x

arch=$(uname -m)

export MRC_LIB_PATH=$PWD/mrc-header-lib
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH # Needed to resolve locations of doca libraries that libnv_mrc.so depends on.

# Please put absolute paths here.
export VMRC_LIBMRC_SO=$PWD/mrc-header-lib/libnv_mrc.so
export VMRC_LIBIBVERBS_SO=/lib/${arch}-linux-gnu/libibverbs.so.1

#LD_PRELOAD=$PWD/libibverbs.so ./tests/check_ibv_overwrites_dlopen
LD_PRELOAD=$PWD/libibverbs.so ./tests/check_ibv_overwrites
