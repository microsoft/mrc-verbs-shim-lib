// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/* Load all verbs symbols to a structure. */

#define _GNU_SOURCE
#include "include/vmrc_symbols.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/vmrc_log.h"
#include "include/vmrc_version.h"

#define IBVERBS_VERSION_1_1 "IBVERBS_1.1"    // For most ibverbs symbols.
#define IBVERBS_VERSION_1_8 "IBVERBS_1.8"    // For ibv_reg_mr_iova2.
#define IBVERBS_VERSION_1_12 "IBVERBS_1.12"  // For ibv_reg_dmabuf_mr.
#define IBVERBS_VERSION_1_10 "IBVERBS_1.10"  // For ibv_query_ece, ibv_set_ece.

/* Loads with version IBVERBS_VERSION_1_1. */
#define LOAD_IBVERBS_SYM(handle, symbol, funcptr)                                                       \
  do {                                                                                                  \
    void** cast = (void**)&funcptr;                                                                     \
    void* tmp = dlvsym(handle, symbol, IBVERBS_VERSION_1_1);                                            \
    if (tmp == NULL) {                                                                                  \
      fprintf(stderr, "dlvsym failed on %s - %s version %s\n", symbol, dlerror(), IBVERBS_VERSION_1_1); \
      goto teardown;                                                                                    \
    }                                                                                                   \
    *cast = tmp;                                                                                        \
  } while (0)

#define LOAD_IBVERBS_SYM_VER(handle, symbol, funcptr, version)                              \
  do {                                                                                      \
    void** cast = (void**)&funcptr;                                                         \
    void* tmp = dlvsym(handle, symbol, version);                                            \
    if (tmp == NULL) {                                                                      \
      fprintf(stderr, "dlvsym failed on %s - %s version %s\n", symbol, dlerror(), version); \
      goto teardown;                                                                        \
    }                                                                                       \
    *cast = tmp;                                                                            \
  } while (0)

#define LOAD_MRC_SYM(handle, symbol, funcptr)                          \
  do {                                                                 \
    void** cast = (void**)&funcptr;                                    \
    void* tmp = dlsym(handle, symbol);                                 \
    if (tmp == NULL) {                                                 \
      fprintf(stderr, "dlsym failed on %s - %s\n", symbol, dlerror()); \
      goto teardown;                                                   \
    }                                                                  \
    *cast = tmp;                                                       \
  } while (0)

struct vmrc_symbols_t* vmrc_symbols_get() {
  static struct vmrc_symbols_t* cache = NULL;
  if (cache != NULL) return cache;

  static void* ibv_handle = NULL;
  static void* mrc_handle = NULL;

  cache = (struct vmrc_symbols_t*)calloc(1, sizeof(struct vmrc_symbols_t));
  if (cache == NULL) {
    fprintf(stderr, "Allocating (struct vmrc_symbols_t) failed\n");
    goto teardown;
  }

  VMRC_DEBUG_PRINT_VA_ARGS("version %d.%d.%d", VMRC_VERSION_MAJOR, VMRC_VERSION_MINOR, VMRC_VERSION_PATCH);

  const char* verbs_lib_path = getenv("VMRC_LIBIBVERBS_SO");
  VMRC_CHECK_PRINT_EXIT(verbs_lib_path, 1, "VMRC_LIBIBVERBS_SO env var is not set.");

  VMRC_DEBUG_PRINT_VA_ARGS("Loading verbs symbols from %s", verbs_lib_path);

  ibv_handle = dlopen(verbs_lib_path, RTLD_NOW);
  if (!ibv_handle) {
    fprintf(stderr, "Failed to open libibverbs.so\n");
    goto teardown;
  }

  /* Load IBverbs symbols. */

  LOAD_IBVERBS_SYM(ibv_handle, "ibv_get_device_list", cache->ibv_get_device_list_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_free_device_list", cache->ibv_free_device_list_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_get_device_name", cache->ibv_get_device_name_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_open_device", cache->ibv_open_device_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_close_device", cache->ibv_close_device_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_get_async_event", cache->ibv_get_async_event_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_ack_async_event", cache->ibv_ack_async_event_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_query_device", cache->ibv_query_device_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_query_port", cache->ibv_query_port_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_query_gid", cache->ibv_query_gid_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_alloc_pd", cache->ibv_alloc_pd_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_dealloc_pd", cache->ibv_dealloc_pd_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_reg_mr", cache->ibv_reg_mr_internal);
  LOAD_IBVERBS_SYM_VER(ibv_handle, "ibv_reg_mr_iova2", cache->ibv_reg_mr_iova2_internal, "IBVERBS_1.8");
  LOAD_IBVERBS_SYM_VER(ibv_handle, "ibv_reg_dmabuf_mr", cache->ibv_reg_dmabuf_mr_internal, "IBVERBS_1.12");
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_dereg_mr", cache->ibv_dereg_mr_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_fork_init", cache->ibv_fork_init_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_event_type_str", cache->ibv_event_type_str_internal);
  LOAD_IBVERBS_SYM_VER(ibv_handle, "ibv_query_ece", cache->ibv_query_ece_internal, "IBVERBS_1.10");
  LOAD_IBVERBS_SYM_VER(ibv_handle, "ibv_set_ece", cache->ibv_set_ece_internal, "IBVERBS_1.10");

  /* Load MRC symbols. */

  const char* mrc_lib_path = getenv("VMRC_LIBMRC_SO");
  VMRC_CHECK_PRINT_EXIT(mrc_lib_path, 1, "VMRC_LIBMRC_SO env var is not set.");

  VMRC_DEBUG_PRINT_VA_ARGS("Loading mrc symbols from %s", mrc_lib_path);

  mrc_handle = dlopen(mrc_lib_path, RTLD_NOW);
  if (!mrc_handle) {
    fprintf(stderr, "Failed to open %s\n", mrc_lib_path);
    goto teardown;
  }

  LOAD_MRC_SYM(mrc_handle, "mrc_query_device", cache->mrc_query_device_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_create_context", cache->mrc_create_context_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_destroy_context", cache->mrc_destroy_context_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_create_cq", cache->mrc_create_cq_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_poll_cq", cache->mrc_poll_cq_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_destroy_cq", cache->mrc_destroy_cq_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_create_qp", cache->mrc_create_qp_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_destroy_qp", cache->mrc_destroy_qp_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_create_qp_hint", cache->mrc_create_qp_hint_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_destroy_qp_hint", cache->mrc_destroy_qp_hint_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_query_qp", cache->mrc_query_qp_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_modify_qp", cache->mrc_modify_qp_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_get_qpn", cache->mrc_get_qpn_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_post_recv", cache->mrc_post_recv_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_post_send", cache->mrc_post_send_internal);

  return cache;

teardown:
  if (cache) free(cache);
  cache = NULL;
  if (ibv_handle) dlclose(ibv_handle);
  if (mrc_handle) dlclose(mrc_handle);
  return NULL;
}
