/* Load all verbs symbols to the structure. */

#include "include/vmrc_symbols.h"


struct vmrc_symvols_t* vmrc_symbols_get() {
  static struct vmrc_symbols_t *cache=NULL;
  if (cache != NULL) return cache;


  /* Allocate struct vmrc_symbols_t and load symbols. */
}
