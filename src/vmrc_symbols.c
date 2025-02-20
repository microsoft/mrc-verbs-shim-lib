/* Load all verbs symbols to a structure. */

#include <stdlib.h>

#include "include/vmrc_symbols.h"

#define IBVERBS_VERSION "IBVERBS_1.1"
#define LOAD_SYM(handle, symbol, funcptr)                                              \
  do {                                                                                 \
    cast = (void**)&funcptr;                                                           \
    tmp = dlvsym(handle, symbol, IBVERBS_VERSION);                                     \
    if (tmp == NULL) {                                                                 \
      WARN("dlvsym failed on %s - %s version %s", symbol, dlerror(), IBVERBS_VERSION); \
      goto teardown;                                                                   \
    }                                                                                  \
    *cast = tmp;                                                                       \
  } while (0)

struct vmrc_symbols_t* vmrc_symbols_get() {
  static struct vmrc_symbols_t* cache = NULL;
  if (cache != NULL) return cache;

  /* Allocate struct vmrc_symbols_t and load symbols. */
  cache = (struct vmrc_symbols_t*)calloc(1, sizeof(struct vmrc_symbols_t));

  /* dlopen. */

teardown:
  if (cache) free(cache);
  return NULL;
}
