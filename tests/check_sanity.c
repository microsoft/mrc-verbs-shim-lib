// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int num_devices = 0;
  struct ibv_device** dev_list = NULL;
  struct ibv_context* context = NULL;
  struct ibv_pd* pd = NULL;
  struct ibv_qp* qp = NULL;
  struct ibv_qp_init_attr qp_init_attr;
  struct ibv_cq* cq = NULL;

  dev_list = ibv_get_device_list(&num_devices);
  if (num_devices == 0) {
    fprintf(stderr, "No devices found.\n");
    exit(1);
  }

  const char** dev_names = (const char**)calloc(num_devices, sizeof(const char*));
  for (int i = 0; i < num_devices; ++i) {
    dev_names[i] = ibv_get_device_name(dev_list[i]);
    fprintf(stderr, "dev_name[%2d] = %s\n", i, dev_names[i]);
  }

  free(dev_names);
  ibv_free_device_list(dev_list);

  return 0;
}