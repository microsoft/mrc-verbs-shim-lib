/* Overwrites to ibv calls. */

#include <stdio.h>

#include "include/vmrc_symbols.h"

struct ibv_device** ibv_get_device_list(int* num_devices) {
  fprintf(stderr, "In verbs-mrc: File: %s Line: %d\n", __FILE__, __LINE__);
  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  fprintf(stderr, "symbols = %p\n", symbols);

  return symbols->ibv_get_device_list_internal(num_devices);
}

const char* ibv_get_device_name(struct ibv_device* device) {
  fprintf(stderr, "In verbs-mrc: File: %s Line: %d\n", __FILE__, __LINE__);
  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  fprintf(stderr, "symbols = %p\n", symbols);

  return symbols->ibv_get_device_name_internal(device);
}
