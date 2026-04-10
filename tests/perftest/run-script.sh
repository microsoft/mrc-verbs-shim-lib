#!/bin/bash

arch=$(uname -m)
MRC_LIB_DIR=${MRC_LIB_DIR:?"Error: MRC_LIB_DIR is not set"}
MRC_LIB_SO=${MRC_LIB_SO:-"libmrc.so"}

# Export for the shim layer.
export VMRC_LIBMRC_SO=${VMRC_LIBMRC_SO:-"$MRC_LIB_DIR/$MRC_LIB_SO"}
export VMRC_LIBIBVERBS_SO=${VMRC_LIBIBVERBS_SO:-"/lib/${arch}-linux-gnu/libibverbs.so.1"}
export LD_LIBRARY_PATH=$MRC_LIB_DIR:$LD_LIBRARY_PATH # Needed to resolve locations during dlopen.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHIM_LIB_DIR="$(cd "$SCRIPT_DIR/../../build/lib" && pwd)"

set -x

LD_PRELOAD=$SHIM_LIB_DIR/libibverbs.so $@