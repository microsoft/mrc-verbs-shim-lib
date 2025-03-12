/* Load all verbs symbols to a structure. */

#define _GNU_SOURCE
#include "include/vmrc_symbols.h"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

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

#ifdef VERBS_MRC_DEBUG
  fprintf(stderr, "verbs-mrc: Loading ibv symbols from libibverbs.so\n");
#endif

  static void* ibv_handle = NULL;
  static void* mrc_handle = NULL;

  cache = (struct vmrc_symbols_t*)calloc(1, sizeof(struct vmrc_symbols_t));
  if (cache == NULL) {
    fprintf(stderr, "verbs-mrc: Allocating (struct vmrc_symbols_t) failed\n");
    goto teardown;
  }

  ibv_handle = dlopen("libibverbs.so", RTLD_NOW);
  if (!ibv_handle) {
    fprintf(stderr, "Failed to open libibverbs.so\n");
    goto teardown;
  }

  LOAD_IBVERBS_SYM(ibv_handle, "ibv_get_device_list", cache->ibv_get_device_list_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_get_device_name", cache->ibv_get_device_name_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_open_device", cache->ibv_open_device_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_create_qp", cache->ibv_create_qp_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_create_cq", cache->ibv_create_cq_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_close_device", cache->ibv_close_device_internal);

  /*

  mrc_handle = dlopen("libmrc.so", RTLD_NOW);
  if (!mrc_handle) {
    fprintf(stderr, "Failed to open libmrc.so \n");
    goto teardown;
  }

  LOAD_MRC_SYM(mrc_handle, "mrc_query_device", cache->mrc_query_device_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_create_context", cache->mrc_create_context_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_destroy_context", cache->mrc_destroy_context_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_create_qp", cache->mrc_create_qp_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_modify_qp", cache->mrc_modify_qp_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_destroy_qp", cache->mrc_destroy_qp_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_create_cq", cache->mrc_create_cq_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_destroy_cq", cache->mrc_destroy_cq_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_poll_cq", cache->mrc_poll_cq_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_create_ev_array_explicit", cache->mrc_create_ev_array_explicit_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_get_qpn", cache->mrc_get_qpn_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_post_recv", cache->mrc_post_recv_internal);
  LOAD_MRC_SYM(mrc_handle, "mrc_post_send", cache->mrc_post_send_internal);

  */

  return cache;

teardown:
  if (cache) free(cache);
  if (!ibv_handle) dlclose(ibv_handle);
  return NULL;
}
