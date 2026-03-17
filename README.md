# Introduction

Verbs-MRC shim library is a lightweight library that enables existing verbs applications and AI communication library, such as NCCL/RCCL, to use the new MRC transport with no code changes or any noticeable performance penalty. Specifically, it enables any verbs application that uses `RDMA_WRITE` and/or `RDMA_WRITE_WITH_IMM` to immediately take advantage of the MRC transport protocol with almost no code changes. The usual ibverbs function symbols are overwritten by the function symbols in the shim layer library. This is done either (i) by preloading the shim library via `LD_PRELOAD` or (ii) by dynamically loading the shim library at runtime via `dlopen`. The shim translates the necessary verbs function calls to their equivalent MRC function calls.

# Building verbs-mrc

To build mrc-verbs shim library, simply execute:

```bash
MRC_H_PATH=<Path to folder containing mrc.h> ./build-verbs-mrc.sh
```

This will have created the `libibverbs.so` library in the parent directory. This library consists of overwrites for several of verbs symbols. For a list of symbols it overwrites, please run `objdump -T libibverbs.so`.

To quickly check if the shim library works, run:
```bash
./check_sanity.sh
```
You should see the list of all RDMA devices on the node.

# Tests

We have packaged two tests with the shim library: (i) verbs perftest and (ii) NCCL.

## Verbs perftest

To clone and build verbs perftest, run:
```
cd tests/perftest
./build-script.sh
```
This will build perftest with `--disable-ibv_wr_api --disable-cq_ex` flags.

To run perftest over shim, execute:
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
This clones a fork

# Incorporating mrc-verbs-shim-lib with your CCL/application

## Requirements

Your CCL/app should:

- Not use `_ex` APIs. For e.g., `ibv_crate_cq_ex` and `ibv_create_qp_ex`.
- Not use WR APIs and instead use `ibv_post_send` and `ibv_post_recv`.
- Only use `RDMA_WRITE` or `RDMA_WRITE_WITH_IMM` ops.

## Running with verbs-mrc

To use MRC over shim library with your application,
- Export a few variables:

```bash
export MRC_LIB_PATH=<path to folder containing .so files to resolve vendor libmrc.so symbols>
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH
# Please put absolute paths here.
export VMRC_LIBMRC_SO=<full path to vendor libmrc.so>
export VMRC_LIBIBVERBS_SO=<full path to vendor libibverbs.so.1>
```
- If your CCL/app, compiles against rdma-core (for e.g., perftest), then `LD_PRELOAD` the shim library and run your application as:
```
LD_PRELOAD=<Absolute path of shim library's libibverbs.so> <your app/ccl>
```
- If your CCL/app, loads `libibverbs.so.1` at runtime (for e.g., NCCL), then `dlopen` the shim library's `libibverbs.so` by providing its full path.