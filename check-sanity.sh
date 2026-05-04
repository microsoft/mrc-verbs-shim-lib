#!/bin/bash

# This test runs a sanity check with the LD_PRELOAD way of using the shim library.

arch=$(uname -m)
MRC_LIB_DIR=${MRC_LIB_DIR:?"Error: MRC_LIB_DIR is not set"}
MRC_LIB_SO=${MRC_LIB_SO:-"libmrc.so"}

# Export for the shim layer.
export VMRC_LIBMRC_SO=${VMRC_LIBMRC_SO:-"$MRC_LIB_DIR/$MRC_LIB_SO"}
export VMRC_LIBIBVERBS_SO=${VMRC_LIBIBVERBS_SO:-"/lib/${arch}-linux-gnu/libibverbs.so.1"}
export LD_LIBRARY_PATH=$MRC_LIB_DIR:$LD_LIBRARY_PATH # Needed to resolve locations during dlopen.

LD_PRELOAD=$PWD/build/lib/debug/libibverbs_debug.so ./tests/check_sanity
