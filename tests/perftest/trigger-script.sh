#!/bin/bash

export MRC_LIB_DIR=${MRC_LIB_DIR:?"Error: MRC_LIB_DIR is not set"}
export MRC_LIB_SO=${MRC_LIB_SO:-"libmrc.so"}

./run-script.sh numactl -N 0 perftest/ib_write_bw -q 4 -d mlx5_1 --use_cuda 0 \
-x 3 --ipv6 --report_gbits --use_data_direct --use_cuda_dmabuf --wait_destroy=10 --duration 5 &
sleep 2
./run-script.sh numactl -N 1 perftest/ib_write_bw -q 4 -d mlx5_3 --use_cuda 2 \
-x 3 --ipv6 --report_gbits --use_data_direct --use_cuda_dmabuf --wait_destroy=10 --duration 5 localhost