#include <stdio.h>
#include <stdlib.h>

#include "../src/include/vmrc_symbols.h"

int main() {
  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  if (!symbols) return 1;

  int num_devices = 0;
  struct ibv_device** dev_list = symbols->ibv_get_device_list_internal(&num_devices);
  const char** dev_names = (const char**)calloc(num_devices, sizeof(const char*));

  if (!dev_list) {
    fprintf(stderr, "Error in retrieving device list\n");
    return 1;
  }

  for (int i = 0; i < num_devices; ++i) {
    dev_names[i] = symbols->ibv_get_device_name_internal(dev_list[i]);
    fprintf(stderr, "dev_name[%2d] = %s\n", i, dev_names[i]);
  }

  return 0;
}
