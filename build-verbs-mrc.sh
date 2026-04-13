#!/bin/bash

if [ -z "$MRC_H_PATH" ]; then
    echo "Error: MRC_H_PATH is not set"
    exit 1
fi

export MRC_H_PATH=${MRC_H_PATH}

make clean
make all