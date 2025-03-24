# Introduction

verbs-mrc is a collection of verbs function overwrites that enable MRC capabilities with verbs application that uses `RDMA_WRITE` or `RDMA_WRITE_WITH_IMM` ops. Primary applications that will use this include the vanilla ibverbs perftests and NCCL. Other important applications include MPRScrub and the many-to-many perftest.

# Building verbs-mrc

To build verbs MRC, simply execute 

```bash
./build-verbs-mrc.sh
```

in the parent directory.

# Building ibverbs perftest and NCCL

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

Please see b2ccb69c87452978025ff2ce7aa7b86335e1cb26 commit in MPRScrub. Once this is fixed, build perftest as:

```bash
CUDA_HOME_PATH=$(dirname $(dirname $(which nvcc)))
./autogen.sh && ./configure --disable-ibv_wr_api CUDA_H_PATH=$CUDA_HOME_PATH/include/cuda.h && make -j
```

## NCCL

To currently use verbs-mrc with NCCL, you have to build NCCL with the (exported) environment variable `RDMA_CORE` set to 1. This because the shared library `libnccl.so` will have the undefined verbs symbols and any executable when linked with `libnccl.so` has to resolve these symbols at compile time. Otherwise, NCCL loads the verbs symbols at run time while `dlopen` and this requires us to overwrite each and every verbs call that NCCL uses in verbs-mrc. This will be part of future efforts. Building with `RDMA_CORE` set to 1 will allow us to only overwrite the minimal set of functions.

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
