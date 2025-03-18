#!/bin/bash

export MRC_H_PATH=$PWD/test-mrc-header-lib
export DEBUG=1

make clean
make 
make tests tests_internal
