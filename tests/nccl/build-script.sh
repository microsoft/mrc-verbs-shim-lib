#!/bin/bash

set -x

rm -rf nccl
rm -rf nccl-tests

git clone --branch v2.30.3-1 https://github.com/NVIDIA/nccl.git
git clone --branch v2.18.2 https://github.com/NVIDIA/nccl-tests.git

pushd nccl
make -j src.build NVCC_GENCODE="-gencode=arch=compute_100,code=sm_100 -gencode=arch=compute_120,code=sm_120 -gencode=arch=compute_120,code=compute_120" #GB200
popd


pushd nccl-tests
module load mpi/hpcx # or your favorite MPI module.
NCCL_HOME=$(realpath $PWD/../nccl/build/)
LD_LIBRARY_PATH=$NCCL_HOME/lib:$LD_LIBRARY_PATH
make -j VERBOSE=1 MPI=1 MPI_HOME=$MPI_HOME NCCL_HOME=$NCCL_HOME
popd