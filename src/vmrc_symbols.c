/* Load all verbs symbols to a structure. */

#define _GNU_SOURCE
#include "include/vmrc_symbols.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/vmrc_log.h"

#define IBVERBS_VERSION "IBVERBS_1.1"
#define LOAD_IBVERBS_SYM(handle, symbol, funcptr)                                                   \
  do {                                                                                              \
    void** cast = (void**)&funcptr;                                                                 \
    void* tmp = dlvsym(handle, symbol, IBVERBS_VERSION);                                            \
    if (tmp == NULL) {                                                                              \
      fprintf(stderr, "dlvsym failed on %s - %s version %s\n", symbol, dlerror(), IBVERBS_VERSION); \
      goto teardown;                                                                                \
    }                                                                                               \
    *cast = tmp;                                                                                    \
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
    fprintf(stderr, "verbs-mrc: Allocating (struct vmrc_symbols_t) failed\n");
    goto teardown;
  }

  VMRC_DEBUG_PRINT("Loading ibv symbols from libibverbs.so");

  const char* verbs_lib_path = getenv("VMRC_LIBIBVERBS_SO");
  VMRC_CHECK_PRINT_EXIT(verbs_lib_path, 1, "VMRC_LIBIBVERBS_SO env var is not set.");

  VMRC_DEBUG_PRINT_VA_ARGS("Loading verbs symbols from %s", verbs_lib_path);

  ibv_handle = dlopen(verbs_lib_path, RTLD_NOW);
  if (!ibv_handle) {
    fprintf(stderr, "Failed to open libibverbs.so\n");
    goto teardown;
  }

  /* For this to work with NCCL, I have to put an interface for all the verbs calls that NCCL uses. */
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_get_device_list", cache->ibv_get_device_list_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_get_device_name", cache->ibv_get_device_name_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_open_device", cache->ibv_open_device_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_close_device", cache->ibv_close_device_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_create_qp", cache->ibv_create_qp_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_create_cq", cache->ibv_create_cq_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_query_gid", cache->ibv_query_gid_internal);

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
  LOAD_MRC_SYM(mrc_handle, "mrc_create_qp_group", cache->mrc_create_qp_group_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_destroy_qp_group", cache->mrc_destroy_qp_group_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_create_qp_hint", cache->mrc_create_qp_hint_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_destroy_qp_hint", cache->mrc_destroy_qp_hint_internal);
  //LOAD_MRC_SYM(mrc_handle, "mrc_create_ev_array", cache->mrc_create_ev_array_internal);
  //LOAD_MRC_SYM(mrc_handle, "mrc_destroy_ev_array", cache->mrc_destroy_ev_array_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_query_qp", cache->mrc_query_qp_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_modify_qp", cache->mrc_modify_qp_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_get_qpn", cache->mrc_get_qpn_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_post_recv", cache->mrc_post_recv_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_post_send", cache->mrc_post_send_internal);

  return cache;

teardown:
  if (cache) free(cache);
  if (!ibv_handle) dlclose(ibv_handle);
  if (!mrc_handle) dlclose(mrc_handle);
  return NULL;
}
