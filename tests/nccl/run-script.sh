#!/bin/bash
# Place this script from inside nccl-tests.

check_if_file_exists() {
    if [ ! -f "$1" ]; then
        echo "Error: Required file '$1' not found. Please ensure it exists and try again."
        exit 1
    fi
}

print_usage() {
    echo "Usage: $0 <NUM_NODES> <BENCH>"
    echo "Example: $0 2 sendrecv"
    echo "Environment variables:"
    echo "  MRC_LIB_DIR: Directory where the MRC library is located. (required)"
    echo "  MRC_LIB_SO: Name of the MRC library file (default: libmrc.so)."
    echo "  PPN: Processes per node (default: 4)."
    echo "  INIC_LIST: Comma-separated list of NIC indices in the order to use from the binding file (default: 1,0,3,2)."
    echo "  BINDING_FILE: Path to the NIC-GPU-CPU-NUMA binding file (default: ../default-nic-gpu-cpu-numa-binding.txt (.. is relative to script))."
    echo "  MPI_NETDEV: Network device to use for MPI (default: enP22p1s0f1)."
    echo "  GID_INDEX: GID index to use for the test (default: 3)."
    echo "  OUTER_ITER: Number of outer iterations for the test (default: 1)."
    echo "  INNER_ITER: Number of inner iterations for the test (default: 50)."
    echo "  WARMUP_ITER: Number of warmup iterations for the test (default: 50)."
    echo "  LONG_RUN: Whether to run in long-running mode (0 or 1, default: 0). In long-running mode, the test will run indefinitely at END_MSG_SIZE."
    echo "  BEGIN_MSG_SIZE: Starting message size for the test (default: 4K)."
    echo "  END_MSG_SIZE: Ending message size for the test (default: 4G). In long-running mode, this will be the message size that runs indefinitely."
    echo "  HOSTFILE: Path to the MPI hostfile (default: ./hostfile)."
    echo "  ADDNL_MPI_ARGS: Additional arguments to pass to mpirun. (default: empty)"
    echo "  ADDNL_NCCL_ENV: Additional environment variables to pass to NCCL. Should be in the format '-x VAR=value'. (default: empty)"
}

set -eu

SCRIPT_DIR=$(dirname "$(realpath "$0")")
cd $SCRIPT_DIR

if [ "$#" -ne 2 ]; then
    print_usage
    exit 1
fi

if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    print_usage
    exit 0
fi

MRC_LIB_DIR=${MRC_LIB_DIR:?"Error: MRC_LIB_DIR is not set"}
MRC_LIB_DIR=$(realpath $MRC_LIB_DIR)
MRC_LIB_SO=${MRC_LIB_SO:-"libmrc.so"}
NUM_NODES=${1:?"Error: NUM_NODES argument is required"}
BENCH=${2:?"Error: BENCH argument is required"}
PPN=${PPN:-4}
INIC_LIST=${INIC_LIST:-"1,0,3,2"}
BINDING_FILE=${BINDING_FILE:-"$(realpath "$SCRIPT_DIR/../default-nic-gpu-cpu-numa-binding.txt")"}
MPI_NETDEV=${MPI_NETDEV:-enP22p1s0f1}
GID_INDEX=${GID_INDEX:-3}
OUTER_ITER=${OUTER_ITER:-1}
INNER_ITER=${INNER_ITER:-50}
WARMUP_ITER=${WARMUP_ITER:-50}
LONG_RUN=${LONG_RUN:-0}
DEBUG=${DEBUG:-0}
LONG_RUN=${LONG_RUN:-0}
BEGIN_MSG_SIZE=${BEGIN_MSG_SIZE:-4K}
END_MSG_SIZE=${END_MSG_SIZE:-4G}
HOSTFILE=${HOSTFILE:-"./hostfile"}
ADDNL_MPI_ARGS=${ADDNL_MPI_ARGS:-""}
ADDNL_NCCL_ENV=${ADDNL_NCCL_ENV:-""}

echo "Running NCCL test on $NUM_NODES nodes with $PPN GPUs per node with benchmark $BENCH. Using MRC library from $MRC_LIB_DIR/$MRC_LIB_SO"

mapfile -t NIC_ARR < <(grep -v '^#' $BINDING_FILE | awk -F, '{print $1}')
mapfile -t GPU_ARR < <(grep -v '^#' $BINDING_FILE | awk -F, '{print $2}')
mapfile -t CPU_ARR < <(grep -v '^#' $BINDING_FILE | awk -F, '{print $3}')
mapfile -t NUMA_ARR < <(grep -v '^#' $BINDING_FILE | awk -F, '{print $4}')
mapfile -t INIC_ARR < <(echo $INIC_LIST | tr ',' '\n')

GPU_ORDER=()
NIC_ORDER=()
CPU_RANGE_ORDER=()
NUMA_ORDER=()
for i in ${INIC_ARR[@]}; do
    GPU_ORDER+=(${GPU_ARR[i]})
    NIC_ORDER+=(${NIC_ARR[i]})
    CPU_RANGE_ORDER+=(${CPU_ARR[i]})
    NUMA_ORDER+=(${NUMA_ARR[i]})
done
CUDA_VISIBLE_DEVICES=$(IFS=,; echo "${GPU_ORDER[*]}")
NCCL_IB_HCA=$(IFS=,; echo "${NIC_ORDER[*]}")
PHYS_CPU_BIND_RANGE=$(IFS=,; echo "${CPU_RANGE_ORDER[*]}")
MEM_BIND=$(IFS=,; echo "${NUMA_ORDER[*]}")

LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
LD_LIBRARY_PATH=$MRC_LIB_DIR:$(realpath $SCRIPT_DIR/nccl/build/lib):$LD_LIBRARY_PATH

arch=$(uname -m)
VMRC_LIBMRC_SO=$MRC_LIB_DIR/$MRC_LIB_SO
VMRC_LIBIBVERBS_SO=/lib/${arch}-linux-gnu/libibverbs.so.1
check_if_file_exists $VMRC_LIBMRC_SO
check_if_file_exists $VMRC_LIBIBVERBS_SO
NCCL_IBVERBS_LIB=$(realpath $SCRIPT_DIR/../../build/lib/libibverbs.so)
check_if_file_exists $NCCL_IBVERBS_LIB

COLL=$(realpath $SCRIPT_DIR/nccl-tests/build/${BENCH}_perf)

# Set COLL_ARGS based on LONG_RUN mode
if [ "$LONG_RUN" -eq 1 ]; then
    echo "Long running mode enabled: running at $END_MSG_SIZE indefinitely"
    COLL_ARGS="-w $WARMUP_ITER -n $INNER_ITER -b $END_MSG_SIZE -e $END_MSG_SIZE -g1 -c 1 -R 1 -N 0"
else
    COLL_ARGS="-b $BEGIN_MSG_SIZE -f2 -e $END_MSG_SIZE -g1 -c 1 -R 1 -w $WARMUP_ITER -n $INNER_ITER -N $OUTER_ITER"
fi

MPI_ARGS="\
  --allow-run-as-root \
  --map-by ppr:$PPN:node --bind-to none \
  --hostfile $HOSTFILE \
  --mca plm_rsh_no_tree_spawn 1 --mca plm_rsh_num_concurrent 8192 \
  --mca pml ob1 \
  --mca btl vader,self,tcp \
  --mca btl_tcp_if_include $MPI_NETDEV \
  --mca coll ^hcoll,ucc,xhc \
  $ADDNL_MPI_ARGS"

NCCL_ENV=" \
  -x LD_LIBRARY_PATH=$LD_LIBRARY_PATH \
  -x VMRC_LIBMRC_SO=$VMRC_LIBMRC_SO \
  -x VMRC_LIBIBVERBS_SO=$VMRC_LIBIBVERBS_SO \
  -x NCCL_IBVERBS_LIB=$NCCL_IBVERBS_LIB \
  -x NCCL_SOCKET_IFNAME=$MPI_NETDEV \
  -x NCCL_NET_PLUGIN=none \
  -x NCCL_TUNER_PLUGIN=none \
  -x NCCL_IB_DISABLE=0 \
  -x NCCL_SHM_DISABLE=1 \
  -x NCCL_P2P_DISABLE=1 \
  -x NCCL_MNNVL_ENABLE=0 \
  -x CUDA_VISIBLE_DEVICES=$CUDA_VISIBLE_DEVICES \
  -x NCCL_IB_HCA=$NCCL_IB_HCA \
  -x NCCL_IB_ECE_ENABLE=0 \
  -x NCCL_IB_GID_INDEX=$GID_INDEX \
  -x NCCL_CROSS_NIC=0 \
  -x NCCL_NVLS_ENABLE=0 \
  -x NCCL_GDR_FLUSH_DISABLE=1 \
  -x NCCL_GDRCOPY_ENABLE=1 \
  -x NCCL_GDRCOPY_SYNC_ENABLE=1 \
  -x NCCL_IB_QPS_PER_CONNECTION=2 \
  -x NCCL_IB_SPLIT_DATA_ON_QPS=1 \
  $ADDNL_NCCL_ENV"

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
        NCCL_ENV+=" -x NCCL_DEBUG=INFO"
fi

CMD="mpirun -np $((NUM_NODES*PPN)) \
       $MPI_ARGS \
       $NCCL_ENV \
       ./numa-bind.sh --cpu_bind_range $PHYS_CPU_BIND_RANGE --mem_bind $MEM_BIND $COLL $COLL_ARGS"
date
echo "Running command: $CMD"
eval $CMD