# Introduction

The verbs-mrc-shim library is a lightweight library that enables existing libibverbs applications and AI communication libraries (such as NCCL/RCCL) to use the new Multipath Reliable Connection (MRC) transport with no code changes and no performance penalty. RDMA OPs supported by the shim are same as the ones supported by MRC, i.e.,  `RDMA_WRITE` and/or `RDMA_WRITE_WITH_IMM`. 

Specifically, the shim  provides overwrites for common libibverbs symbols used by communication libraries. The overwritten functions internally create and manage MRC-related resources and take care of translating libibverbs API calls to MRC API calls. The user experience will be as though they were calling the common libibverbs APIs.

The shim layer can be used: 
- by preloading the shim library via `LD_PRELOAD` or
- by dynamically loading the shim library symbols at runtime via `dlopen`.

We show an example for each.

# Building

To build the mrc-verbs-shim library, simply execute:

```bash
MRC_H_PATH=<Path to folder containing mrc.h> ./build-verbs-mrc.sh
```

This will create the the shim layer's `libibverbs.so` library in `build/lib`. This library consists of overwrites for several of libibverbs symbols. For a list of symbols it overwrites, please run `objdump -T build/lib/libibverbs.so`.

To quickly check if the shim library works, run:
```bash
MRC_LIB_DIR=<folder containing libmrc.so and dependencies> MRC_LIB_SO=<vendor libmrc.so> ./check_sanity.sh
```
You should see the list of all RDMA devices on the node. The `ibv_open_device_list` call is intercepted by the shim library. It will also list the version of the installed shim layer library at the top.

# Tests

We have packaged two tests with the shim library: (i) verbs perftest and (ii) NCCL.

## Verbs perftest

The [rdma-perftest](https://github.com/linux-rdma/perftest) repo does not yet support MRC. To run it over MRC, clone and build verbs perftest as follows:
```
cd tests/perftest
./build-script.sh
```
This will build perftest with `--disable-ibv_wr_api --disable-cq_ex` flags.

To run perftest over MRC via our shim, execute:
```
cd tests/perftest
MRC_LIB_DIR=<Directory containing MRC shared lib> MRC_LIB_SO=<libmrc.so> ./trigger-script.sh <ip-client> <ip-server> # Starts both server and client.
```

## NCCL

To clone and build NCCL and NCCL-tests, run:
```
cd tests/nccl
./build-script.sh
```
This will build [v2.30](https://github.com/NVIDIA/nccl/tree/v2.30) branch of NCCL. This version of NCCL supports the user to provide an absolute path of `libibverbs.so` to use in `dlopen` via `NCCL_IBVERBS_LIB`. The absolute path of the shim library should be provided with this env var. Please take a look at `run-script.sh` in `tests/nccl`.

To run NCCL with shim using just the MRC backend,
```
cd tests/nccl/nccl-tests
# Prepare a file called `hostfile` with the ips of the nodes
./run-script.sh <# of nodes> <collective> # For e.g., ./run-script.sh 4 sendrecv
```

# How does it work?

## Requirements

Your CCL/app should:

- Not use `_ex` APIs. For e.g., `ibv_crate_cq_ex` and `ibv_create_qp_ex`.
- Not use WR APIs and instead use `ibv_post_send` and `ibv_post_recv`.
- Only use `RDMA_WRITE` or `RDMA_WRITE_WITH_IMM` RDMA ops. `RDMA_READ` and `RDMA_SEND` is not supported.

## Environment variables

The variables needed to be exported are:
```bash
export VMRC_LIBMRC_SO=<absolute path to vendor libmrc.so>
export VMRC_LIBIBVERBS_SO=<absolute path to vendor libibverbs.so.1>
MRC_LIB_PATH=<path to folders containing .so files to resolve vendor libmrc.so symbols (colon seprated)>
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH
```

## Execution


To use MRC over shim library with your application,
- Export the above variables

- If your CCL/app compiles against rdma-core (for e.g., perftest) and satisfies the above requirements, then `LD_PRELOAD` the shim library and run your application as:
```
LD_PRELOAD=<Absolute path of shim library's libibverbs.so> <your app/ccl>
```
- If your CCL/app loads `libibverbs.so` at runtime (for e.g., NCCL), then `dlopen` the shim library's `libibverbs.so` instead of the usual `libibverbs.so` library.
