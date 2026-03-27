#!/bin/bash

# Copy this script inside nccl-tests.
# Note that we disable flush. This will be re-enabled once GDRCopy support is added for aarch64.

set -u
NUM_NODES=$1
PPN=4
BENCH=$2

DEBUG=${DEBUG:-0}
LONG_RUN=${LONG_RUN:-0}
HOSTFILE=${HOSTFILE:-"./hostfile"}

source /etc/profile.d/modules.sh 2>/dev/null || true
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"

echo "Running NCCL test on $NUM_NODES nodes with $PPN GPUs per node"

module load mpi/hpcx

LD_LIBRARY_PATH=$(realpath $PWD/../nccl/build/lib):$LD_LIBRARY_PATH

arch=$(uname -m)

export MRC_LIB_PATH=/opt/mellanox/doca/lib/${arch}-linux-gnu/
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH # Needed to resolve locations of doca libraries that libnv_mrc.so depends on.

# Please put absolute paths here.
export VMRC_LIBMRC_SO=/opt/mellanox/doca/lib/${arch}-linux-gnu/libnv_mrc.so
export VMRC_LIBIBVERBS_SO=/lib/${arch}-linux-gnu/libibverbs.so.1

# Recent verbs-mrc shim layer branch include-other-verbs-calls
export NCCL_LIBIBVERBS_SO=$(realpath $PWD/../../../libibverbs.so)

# Check if NCCL_LIBIBVERBS_SO exists (atleast on the trigger node)
if [ ! -f "$NCCL_LIBIBVERBS_SO" ]; then
    echo "Error: NCCL_LIBIBVERBS_SO not found at $NCCL_LIBIBVERBS_SO"
    exit 1
fi

FE_NETDEV=enP22p1s0f1 # This is the MANA NIC netdev name.

COLL=$(realpath $PWD/build/${BENCH}_perf)

# Set COLL_ARGS based on LONG_RUN mode
if [ "$LONG_RUN" -eq 1 ]; then
    echo "Long running mode enabled: running at 4G indefinitely"
    COLL_ARGS="-w 50 -n 100 -b 4G -e 4G -g1 -c 1 -R 1 -N 0"
else
    COLL_ARGS="-b 4K -f2 -e 4G -g1 -c 1 -R 1 -w 50 -n 50 -N 1"
fi

NCCL_ENV="
  --allow-run-as-root \
  -mca plm_rsh_no_tree_spawn 1 -mca plm_rsh_num_concurrent 8192 \
  --map-by ppr:$PPN:node --bind-to none \
  --hostfile $HOSTFILE \
   -x UCX_TLS=tcp \
  -x LD_LIBRARY_PATH \
  -x VMRC_LIBMRC_SO=/opt/mellanox/doca/lib/aarch64-linux-gnu/libnv_mrc.so \
  -x VMRC_LIBIBVERBS_SO=/lib/aarch64-linux-gnu/libibverbs.so.1 \
  -x NCCL_LIBIBVERBS_SO=$NCCL_LIBIBVERBS_SO \
  -mca coll_hcoll_enable 0 \
  --mca btl tcp,vader,self \
  --mca pml ob1 \
  --mca btl_tcp_if_include enP22p1s0f1 \
  -x UCX_NET_DEVICES=enP22p1s0f1 \
  -x NCCL_SOCKET_IFNAME=enP22p1s0f1 \
  -x NCCL_NET_PLUGIN=none \
  -x NCCL_IB_DISABLE=0 \
  -x NCCL_SHM_DISABLE=1 \
  -x NCCL_P2P_DISABLE=1 \
  -x NCCL_MNNVL_ENABLE=0 \
  -x CUDA_VISIBLE_DEVICES=0,1,2,3 \
  -x NCCL_IB_HCA=mlx5_1,mlx5_0,mlx5_3,mlx5_2 \
  -x NCCL_IB_ECE_ENABLE=0 \
  -x NCCL_IB_GID_INDEX=3 \
  -x NCCL_CROSS_NIC=0 \
  -x NCCL_NVLS_ENABLE=0 \
  -x NCCL_GDR_FLUSH_DISABLE=1 \
  -x NCCL_GDRCOPY_ENABLE=1 \
  -x NCCL_GDRCOPY_SYNC_ENABLE=1 \
  -x NCCL_IB_QPS_PER_CONNECTION=2 \
  -x NCCL_IB_SPLIT_DATA_ON_QPS=1 \
  -x NCCL_IB_TC=$((1 << 2)) -x NCCL_IB_FIFO_TC=$((3 << 2))

if [ "$PPN" -eq 4 ]; then
       NCCL_ENV+=" -x NCCL_TESTS_SPLIT_MASK=0x3"
elif [ "$PPN" -eq 2 ]; then
       NCCL_ENV+=" -x NCCL_TESTS_SPLIT_MASK=0x1"
elif [ "$PPN" -eq 1 ]; then
       NCCL_ENV+=" -x NCCL_TESTS_SPLIT_MASK=0x0"
else
       echo "NCCL_TESTS_SPLIT_MASK cannot be set for PPN = $PPN. Exiting."
       exit 1
fi

if [ "$DEBUG" -eq 0 ]; then
        NCCL_ENV+=" -x NCCL_DEBUG=WARN"
else
        NCCL_ENV+=" -x NCCL_DEBUG=TRACE -x NCCL_DEBUG_SUBSYS=ALL"
fi

set -x
date
echo "mpirun -np $((NUM_NODES*PPN)) $NCCL_ENV $COLL $COLL_ARGS"
mpirun -np $((NUM_NODES*PPN)) \
        $NCCL_ENV \
        $COLL $COLL_ARGS
set +x