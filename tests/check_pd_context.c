// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/* NCCL uses ibv_create_qp which takes a protection domain (pd) as an input. However, mrc_create_qp takes a mrc_context
 * as an input and hence is equivalent to ibv_create_qp_ex. One way to get around this is that `struct ibv_pd` has a
 * `struct ibv_context *` in it. This should ideally be pointer to the ibv_context used to created the pd. This program
 * verifies this. */

#include <errno.h>
#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  int num_devices = 0;
  struct ibv_device** dev_list = NULL;
  struct ibv_context* context = NULL;
  struct ibv_pd* pd = NULL;
  int test_return = 0;

  dev_list = ibv_get_device_list(&num_devices);
  const char** dev_names = (const char**)calloc(1, sizeof(const char*));
  dev_names[0] = ibv_get_device_name(dev_list[0]);
  printf("dev_name[0] = %s\n", dev_names[0]);

  /* Open the first device. */
  context = ibv_open_device(dev_list[0]);

  /* Create a protection domain. */
  pd = ibv_alloc_pd(context);

  if (pd->context != context) test_return = 1;

  if (ibv_dealloc_pd(pd)) {
    fprintf(stderr, "Failed to deallocate PD - %s\n", strerror(errno));
    return 1;
  }

  if (ibv_close_device(context)) {
    fprintf(stderr, "Failed to close device context\n");
    return 1;
  }

  ibv_free_device_list(dev_list);

  if (test_return) {
    fprintf(stderr, "Error: context=%p and pd->context=%p do not match.\n", pd->context, context);
    return 1;
  } else {
    printf("context and pd->context match.\n");
    return 0;
  }

  return 0;
}
