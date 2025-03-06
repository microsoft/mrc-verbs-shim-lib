#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int num_devices = 0;
  struct ibv_device** dev_list = NULL;

  dev_list = ibv_get_device_list(&num_devices);
  const char** dev_names = (const char**)calloc(num_devices, sizeof(const char*));
  for (int i = 0; i < num_devices; ++i) {
    dev_names[i] = ibv_get_device_name(dev_list[i]);
    fprintf(stderr, "dev_name[%2d] = %s\n", i, dev_names[i]);
  }

  return 0;
}
