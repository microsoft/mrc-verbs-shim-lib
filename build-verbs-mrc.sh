#!/bin/bash

export MRC_H_PATH=${MRC_H_PATH:-"/opt/mellanox/doca/include/"}

make clean
make
make tests tests_internal
