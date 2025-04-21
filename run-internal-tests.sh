#!/bin/bash

arch=$(uname -m)

export MRC_LIB_PATH=/opt/mellanox/doca/lib/$(arch)-linux-gnu/
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH
export VMRC_LIBMRC_SO=/opt/mellanox/doca/lib/$(arch)-linux-gnu/libnv_mrc.so
export VMRC_LIBIBVERBS_SO=/lib/${arch}-linux-gnu/libibverbs.so.1
export VMRC_SYSTEM_JSON=$PWD/opt/mrc-config/system.json

$@
