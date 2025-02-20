/* Load all verbs symbols to a structure. */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/vmrc_symbols.h"

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

struct vmrc_symbols_t* vmrc_symbols_get() {
  static struct vmrc_symbols_t* cache = NULL;
  if (cache != NULL) return cache;

#ifdef VERBS_MRC_DEBUG
  fprintf(stderr, "verbs-mrc: Loading ibv symbols from libibverbs.so\n");
#endif

  static void* ibv_handle = NULL;

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

  return cache;

teardown:
  if (cache) free(cache);
  if (!ibv_handle) dlclose(ibv_handle);
  return NULL;
}
