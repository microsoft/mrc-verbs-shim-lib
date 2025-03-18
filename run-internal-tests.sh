#!/bin/bash

export MRC_LIB_PATH=$PWD/test-mrc-header-lib
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH
export VMRC_LIBMRC_SO=$PWD/test-mrc-header-lib/libnv_mrc.so

$@
