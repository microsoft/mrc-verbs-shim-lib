#!/bin/bash

set -eu

print_usage() {
    echo "Usage: $0 <IP_CLIENT> <IP_SERVER>"
    echo "Example: $0 10.0.0.4 10.0.0.5"
    echo "Environment variables:"
    echo "  MRC_LIB_DIR: Directory where the MRC library is located. (required)"
    echo "  MRC_LIB_SO: Name of the MRC library file (default: libmrc.so)."
    echo "  ADDNL_ARGS: Additional arguments to pass to the ib_write_bw command. (default: empty)"
    echo "  NQPS: Number of QPs to use (default: 4)."
    echo "  DUR: Duration of the test in seconds (default: 5)."
    echo "  BINDING_FILE: Path to the NIC-GPU-CPU-NUMA binding file (default: ../default-nic-gpu-cpu-numa-binding.txt (.. is relative to script))."
    echo "  SERVER_INIC: Index of the server NIC to use from the binding file (default: 0)."
    echo "  CLIENT_INIC: Index of the client NIC to use from the binding file (default: 0)."
    echo "  USE_CUDA: Whether to use CUDA for the test (0 or 1, default: 0)."
    echo "  GID_INDEX: GID index to use for the test (default: 3)."
}

if [ "$#" -ne 2 ]; then
    print_usage
    exit 1
fi

if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
    print_usage
    exit 0
fi

MRC_LIB_DIR=${MRC_LIB_DIR:?"Error: MRC_LIB_DIR is not set"}
MRC_LIB_SO=${MRC_LIB_SO:-"libmrc.so"}
echo "Using MRC library directory: $MRC_LIB_DIR"
echo "Using MRC library file: $MRC_LIB_SO"

SCRIPT_DIR=$(dirname "$(realpath "$0")")
cd $SCRIPT_DIR

IP_CLIENT=$1
IP_SERVER=$2
ADDNL_ARGS=${ADDNL_ARGS:-""}
NQPS=${NQPS:-4}
DUR=${DUR:-5}
BINDING_FILE=${BINDING_FILE:-"$(realpath "$SCRIPT_DIR/../default-nic-gpu-cpu-numa-binding.txt")"}
SERVER_INIC=${SERVER_INIC:-0}
CLIENT_INIC=${CLIENT_INIC:-0}
USE_CUDA=${USE_CUDA:-0}
GID_INDEX=${GID_INDEX:-3}

mapfile -t NIC_ARR < <(grep -v '^#' $BINDING_FILE | awk -F, '{print $1}')
mapfile -t GPU_ARR < <(grep -v '^#' $BINDING_FILE | awk -F, '{print $2}')
mapfile -t CPU_ARR < <(grep -v '^#' $BINDING_FILE | awk -F, '{print $3}')
mapfile -t NUMA_ARR < <(grep -v '^#' $BINDING_FILE | awk -F, '{print $4}')

COMMON_CMD="cd $PWD; MRC_LIB_DIR=$MRC_LIB_DIR MRC_LIB_SO=$MRC_LIB_SO ./run-script.sh"
SERVER_CMD="$COMMON_CMD numactl --physcpubind=${CPU_ARR[$SERVER_INIC]} --membind=${NUMA_ARR[$SERVER_INIC]}"
SERVER_CMD+=" perftest/ib_write_bw -q $NQPS -d ${NIC_ARR[$SERVER_INIC]} "
if [ "$USE_CUDA" -eq 1 ]; then
    SERVER_CMD+="--use_cuda ${GPU_ARR[$SERVER_INIC]} "
fi
SERVER_CMD+=" -x ${GID_INDEX} --ipv6 --report_gbits --duration $DUR $ADDNL_ARGS"

CLIENT_CMD="$COMMON_CMD numactl --physcpubind=${CPU_ARR[$CLIENT_INIC]} --membind=${NUMA_ARR[$CLIENT_INIC]}"
CLIENT_CMD+=" perftest/ib_write_bw -q $NQPS -d ${NIC_ARR[$CLIENT_INIC]} "
if [ "$USE_CUDA" -eq 1 ]; then
    CLIENT_CMD+="--use_cuda ${GPU_ARR[$CLIENT_INIC]} "
fi
CLIENT_CMD+=" -x ${GID_INDEX} --ipv6 --report_gbits --duration $DUR $ADDNL_ARGS $IP_SERVER"

echo "Server IP: $IP_SERVER, Server command: $SERVER_CMD"
echo "Client IP: $IP_CLIENT, Client command: $CLIENT_CMD"

ssh $IP_SERVER "$SERVER_CMD" &
sleep 2
ssh $IP_CLIENT "$CLIENT_CMD"