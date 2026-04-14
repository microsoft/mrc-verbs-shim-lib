#!/bin/bash

export MRC_LIB_DIR=${MRC_LIB_DIR:?"Error: MRC_LIB_DIR is not set"}
export MRC_LIB_SO=${MRC_LIB_SO:-"libmrc.so"}

IP_CLIENT=$1
IP_SERVER=$2

ssh $IP_SERVER "cd $PWD; MRC_LIB_DIR=$MRC_LIB_DIR MRC_LIB_SO=$MRC_LIB_SO ./run-script.sh \
numactl -N 0 perftest/ib_write_bw -q 4 -d mlx5_0 --use_cuda 1 \
-x 3 --ipv6 --report_gbits --use_data_direct --use_cuda_dmabuf --wait_destroy=10 --duration 5" &
sleep 2
ssh $IP_CLIENT "cd $PWD; MRC_LIB_DIR=$MRC_LIB_DIR MRC_LIB_SO=$MRC_LIB_SO ./run-script.sh \
numactl -N 0 perftest/ib_write_bw -q 4 -d mlx5_0 --use_cuda 1 \
-x 3 --ipv6 --report_gbits --use_data_direct --use_cuda_dmabuf --wait_destroy=10 --duration 5 $IP_SERVER"