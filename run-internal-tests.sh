#!/bin/bash

export MRC_LIB_PATH=$PWD/mrc-header-lib
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH
export VMRC_LIBMRC_SO=$PWD/mrc-header-lib/libnv_mrc.so
export VMRC_LIBIBVERBS_SO=/lib/x86_64-linux-gnu/libibverbs.so.1
export VMRC_SYSTEM_JSON=$PWD/mrc-header-lib/system.json

$@
