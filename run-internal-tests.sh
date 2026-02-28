#!/bin/bash

arch=$(uname -m)
DOCA_LIB_DIR=${DOCA_LIB_DIR:-"/opt/mellanox/doca/lib/$(arch)-linux-gnu/"}

export MRC_LIB_PATH=$DOCA_LIB_DIR
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH
export VMRC_LIBMRC_SO=${VMRC_LIBMRC_SO:-"$DOCA_LIB_DIR/libnv_mrc.so"}
export VMRC_LIBIBVERBS_SO=${VMRC_LIBIBVERBS_SO:-"/lib/${arch}-linux-gnu/libibverbs.so.1"}

$@
