#!/bin/bash

# Example usage: ./numa-bind.sh --cpu_bind_range 0-35,36-71,72-107,108-143 --mem_bind 0,0,1,1 $COLL $COLL_ARGS

# Parse command line arguments for CPU and memory binding.
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --cpu_bind_range)
            CPU_BIND_RANGE="$2"
            shift
            shift
            ;;
        --mem_bind)
            MEM_BIND="$2"
            shift
            shift
            ;;
        *)
            break
            ;;
    esac
done


if [ -n "$OMPI_COMM_WORLD_LOCAL_RANK" ]; then
    LOCAL_RANK=$OMPI_COMM_WORLD_LOCAL_RANK
elif [ -n "$SLURM_LOCALID" ]; then
    LOCAL_RANK=$SLURM_LOCALID
else
    echo "Could not determine local rank. Exiting."
    exit 1
fi

mapfile -t CPU_BIND_RANGE_LIST <<< "$(echo $CPU_BIND_RANGE | tr ',' '\n')"
CPU_RANGE_FOR_RANK=${CPU_BIND_RANGE_LIST[$LOCAL_RANK]}

mapfile -t MEM_BIND_LIST <<< "$(echo $MEM_BIND | tr ',' '\n')"
MEM_BIND_FOR_RANK=${MEM_BIND_LIST[$LOCAL_RANK]}

numactl --physcpubind=$CPU_RANGE_FOR_RANK --membind=$MEM_BIND_FOR_RANK $@