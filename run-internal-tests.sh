#!/bin/bash

export LD_LIBRARY_PATH=$PWD/test-mrc-header-lib/:$LD_LIBRARY_PATH

export VMRC_LIBMRC_SO=$PWD/test-mrc-header-lib/libnv_mrc.so.2.10.0340

$@
