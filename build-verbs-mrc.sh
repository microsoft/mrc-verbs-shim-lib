#!/bin/bash

#export MRC_H_PATH=$PWD/mrc-header-lib # This is the default in Makefile.
export DEBUG=1

#export MRC_LIB_PATH=$PWD/mrc-header-lib
#export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH

make clean
make 
make tests tests_internal
