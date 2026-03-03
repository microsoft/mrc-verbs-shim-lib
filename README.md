# Introduction

Verbs-MRC shim library is a light-weight ilbrary that enables existing verbs applications and AI communication library, such as NCCL, to use the new MRC transport with no code changes or any noticeable performance penalty. Specifically, it enables any verbs application that uses `RDMA_WRITE` and/or `RDMA_WRITE_WITH_IMM` to immediately take advantage of libmrc with almost no code changes. The usual ibverbs function symbols are overwritten by the function symbols in the shim layer library either by preloading the library or by dynamically loading the symbol at runtime. The shim layer functions translate the necessary verbs function calls to their equivalent MRC function calls.

This can be used to quickly test your existing verbs application with MRC without any involved CCL-level changes.

# Requirements

Verbs application should:

- Not use `_ex` APIs. For e.g., `ibv_crate_cq_ex` and `ibv_create_qp_ex`.
- Not use WR APIs.
- Only use `RDMA_WRITE` or `RDMA_WRITE_WITH_IMM` ops.

Note that NCCL satisfies these constraints and hence can be used with the shim layer.

# Building verbs-mrc

To build mrc-verbs shim library, simply execute:

```bash
MRC_H_PATH=<Path to folder containing mrc.h> ./build-verbs-mrc.sh
```

This will have created the `libibverbs.so` library in the parent directory. This library consists of overwrites for several of verbs symbols.

# Running with verbs-mrc

To run your verbs application or CCL with the shim layer, export the below variables.

```bash
export MRC_LIB_PATH=<path to folder containing .so files to resolve vendor libmrc.so symbols>
export LD_LIBRARY_PATH=$MRC_LIB_PATH:$LD_LIBRARY_PATH # Needed to resolve locations of doca libraries that libnv_mrc.so depends on.

# Please put absolute paths here.
export VMRC_LIBMRC_SO=<full path to vendor libmrc.so>
export VMRC_LIBIBVERBS_SO=<full path to vendor libibverbs.so.1>
```

Then, you can 
- `LD_PRELOAD` the shim library's `libibverbs.so` if your verbs application only calls RDMA
- 

To quickly check if the shim library works, run:

```bash
```

# Using with CCLs and perftest

## NCCL

Clone the repo:
```bash
git clone https://github.com/NVIDIA/nccl.git
git checkout <version>
make -j build
cp <mrc-verbs-shim-lib>/libibverbs.so .
TODO: give a NCCL sendrecv run script with the shim layer.
```

## IBverbs perftest

Perftest, by default, 
- uses `ibv_create_cq_ex` API to create completion queues, and
- enables Work Request (WR) APIs.

WR APIs can be disabled via `--disable-ibv_wr_api`. However, to disable using `ibv_create_cq_ex` and instead use `ibv_create_cq`, This has to be changed. There is another bug that does not allow perftest to compile with `--disable-ibv_wr_api`. Both these issues are addresed in the open PR (TODO). So, please clone this fork until the PR is merged.


## ibverbs perftest

To use verbs-mrc with ibverbs perftest, you have to build the perftest with `--disable-ibv_wr_api` flag. This will disable the use of work request API. But currently, there is a bug in perftest that will lead to a segmentation fault if you build with this flag. The seg fault is because of a missing macro guard. The array `ctx->dci_stream_id` is allocated only when both `HAVE_IBV_WR_API` and `HAVE_DCS` macro definitions are present. However, while creating qp, only `HAVE_DCS` macro is checked before assigning `ctx->dci_stream_id[i] = 0;`.

```sh
         if (user_param->work_rdma_cm == OFF) {
             modify_qp_to_init(ctx, user_param, i);
         }
+#ifdef HAVE_IBV_WR_API
 #ifdef HAVE_DCS
         ctx->dci_stream_id[i] = 0;
+#endif
 #endif
         qp_index++;
```

Once the above fix is applied, build perftest as:

```bash
CUDA_HOME_PATH=$(dirname $(dirname $(which nvcc)))
./autogen.sh && ./configure --disable-ibv_wr_api CUDA_H_PATH=$CUDA_HOME_PATH/include/cuda.h && make -j
```

## NCCL

To currently use verbs-mrc with NCCL, you have to build NCCL with the (exported) environment variable `RDMA_CORE` set to 1. This because, only then, the shared library `libnccl.so` will have the undefined verbs symbols and any executable when linked with `libnccl.so` has to resolve these symbols at compile time. Otherwise, NCCL loads the verbs symbols at run time while `dlopen` and this requires us to overwrite each and every verbs call that NCCL uses in verbs-mrc. This will be part of future efforts. Building with `RDMA_CORE` set to 1 will allow us to only overwrite the minimal set of functions.

However, NCCL currently has a bug which throws a compilation error when compiled with `RDMA_CORE` set to 1. This is because of a missing macro guard in `src/misc/ibwrap.cc`. This can be fixed via the below patch.

```sh
diff --git a/src/misc/ibvwrap.cc b/src/misc/ibvwrap.cc
index 698465c..2729931 100644
--- a/src/misc/ibvwrap.cc
+++ b/src/misc/ibvwrap.cc
@@ -8,7 +8,12 @@
 #include <sys/types.h>
 #include <unistd.h>

+#ifdef NCCL_BUILD_RDMA_CORE
+#include <infiniband/verbs.h>
+#else
 #include "ibvcore.h"
+#endif
+
 #include "ibvsymbols.h"

 static pthread_once_t initOnceControl = PTHREAD_ONCE_INIT;
```

Once this is fixed, build NCCL as

```bash
export RDMA_CORE=1
make -j
```

# Running with verbs-mrc

While running, `LD_PRELOAD` the verbs-mrc's `libibverbs.so`.

```bash
LD_PRELOAD=<libibverbs.so from verbs-mrc> ...
```

See `run-verbs-mrc.sh` for an example.
