#!/bin/bash

# This test runs a sanity check with the LD_PRELOAD way of using the shim library.

print_usage() {
    echo "Usage: $0 <mrc so parent dir> [mrc so name (default: libmrc.so)]"
}

arch=$(uname -m)
MRC_LIB_DIR=${1:?$(print_usage)}
MRC_LIB_SO=${2:-"libmrc.so"}

# Export for the shim layer.
export VMRC_LIBMRC_SO=${VMRC_LIBMRC_SO:-"$MRC_LIB_DIR/$MRC_LIB_SO"}
export VMRC_LIBIBVERBS_SO=${VMRC_LIBIBVERBS_SO:-"/lib/${arch}-linux-gnu/libibverbs.so.1"}
export LD_LIBRARY_PATH=$MRC_LIB_DIR:$LD_LIBRARY_PATH # Needed to resolve locations during dlopen.

LD_PRELOAD=$PWD/libibverbs.so ./tests/check_sanity
