#!/bin/bash

rm -rf perftest

git clone -b anantharamus/null-sge-wimm git@github.com:SreevatsaAnantharamu/perftest.git

cd perftest

CUDA_HOME_PATH=$(dirname $(dirname $(which nvcc)))
./autogen.sh && ./configure --disable-ibv_wr_api --disable-cq_ex CUDA_H_PATH=$CUDA_HOME_PATH/include/cuda.h && make -j
