#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int num_devices = 0;
  struct ibv_device** dev_list = NULL;
  struct ibv_context* context = NULL;
  struct ibv_pd* pd = NULL;
  struct ibv_qp* qp = NULL;

  dev_list = ibv_get_device_list(&num_devices);
  const char** dev_names = (const char**)calloc(num_devices, sizeof(const char*));
  for (int i = 0; i < num_devices; ++i) {
    dev_names[i] = ibv_get_device_name(dev_list[i]);
    fprintf(stderr, "dev_name[%2d] = %s\n", i, dev_names[i]);
  }

  context = ibv_open_device(dev_list[0]);

  pd = ibv_alloc_pd(context);
  qp = ibv_create_qp(pd, NULL);


  return 0;
}
