/* Overwrites to ibv calls. */

#include <stdio.h>

#include "include/vmrc_symbols.h"

struct ibv_device** ibv_get_device_list(int* num_devices) {
  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  return symbols->ibv_get_device_list_internal(num_devices);
}

const char* ibv_get_device_name(struct ibv_device* device) {
  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  return symbols->ibv_get_device_name_internal(device);
}
