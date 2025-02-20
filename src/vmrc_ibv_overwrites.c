/* Overwrite ibv calls. */

#include <stdio.h>

#include "include/vmrc_symbols.h"

struct ibv_device** ibv_get_device_list(int* num_devices) {
#ifdef VERBS_MRC_DEBUG
  fprintf(stderr, "verbs-mrc: In ibv_get_device_list overwrite\n");
#endif

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  if (!symbols) {
    fprintf(stderr, "Error: Could not get symbols in verbs-mrc shim layer\n");
  }

  return symbols->ibv_get_device_list_internal(num_devices);
}

const char* ibv_get_device_name(struct ibv_device* device) {
#ifdef VERBS_MRC_DEBUG
  fprintf(stderr, "verbs-mrc: In ibv_get_device_name overwrite\n");
#endif

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  if (!symbols) {
    fprintf(stderr, "Error: Could not get symbols in verbs-mrc shim layer\n");
  }

  return symbols->ibv_get_device_name_internal(device);
}
