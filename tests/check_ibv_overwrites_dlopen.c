// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#define _GNU_SOURCE

#include <dlfcn.h>
#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/include/vmrc_symbols.h"

#define IBVERBS_VERSION "IBVERBS_1.1"
#define LOAD_IBVERBS_SYM(handle, symbol, funcptr)                                                   \
  do {                                                                                              \
    void** cast = (void**)&funcptr;                                                                 \
    void* tmp = dlvsym(handle, symbol, IBVERBS_VERSION);                                            \
    if (tmp == NULL) {                                                                              \
      fprintf(stderr, "dlvsym failed on %s - %s version %s\n", symbol, dlerror(), IBVERBS_VERSION); \
      exit(1);                                                                                      \
    }                                                                                               \
    *cast = tmp;                                                                                    \
  } while (0)

int main() {
  int num_devices = 0;
  struct ibv_device** dev_list = NULL;
  struct ibv_context* context = NULL;
  struct ibv_pd* pd = NULL;
  struct ibv_qp* qp = NULL;
  struct ibv_qp_init_attr qp_init_attr;
  struct ibv_cq* cq = NULL;
  static void* ibv_handle = NULL;

  struct vmrc_symbols_t symbols;

  fprintf(stderr, " --- \n In check_ibv_overwrites_dlopen \n ---\n");

  /* Load symbols. */
  ibv_handle = dlopen("libibverbs.so", RTLD_NOW);
  if (!ibv_handle) {
    fprintf(stderr, "Failed to open libibverbs.so\n");
    exit(1);
  }

  LOAD_IBVERBS_SYM(ibv_handle, "ibv_get_device_list", symbols.ibv_get_device_list_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_get_device_name", symbols.ibv_get_device_name_internal);
  LOAD_IBVERBS_SYM(ibv_handle, "ibv_open_device", symbols.ibv_open_device_internal);

  dev_list = symbols.ibv_get_device_list_internal(&num_devices);
  const char** dev_names = (const char**)calloc(num_devices, sizeof(const char*));
  for (int i = 0; i < num_devices; ++i) {
    dev_names[i] = symbols.ibv_get_device_name_internal(dev_list[i]);
    fprintf(stderr, "dev_name[%2d] = %s\n", i, dev_names[i]);
  }

  // context = ibv_open_device(dev_list[0]);

  // pd = ibv_alloc_pd(context);

  // cq = ibv_create_cq(context, 32 /*int cqe*/, NULL /*void* cq_context*/, NULL /*struct ibv_comp_channel* channel*/,
  //                    0 /*int comp_vector*/);

  // memset(&qp_init_attr, 0, sizeof(struct ibv_qp_init_attr));
  // qp_init_attr.send_cq = cq;
  // qp_init_attr.recv_cq = cq;
  // qp_init_attr.qp_type = IBV_QPT_RC;
  // qp_init_attr.cap.max_send_wr = 4;
  // qp_init_attr.cap.max_recv_wr = 4;
  // qp_init_attr.cap.max_send_sge = 1;
  // qp_init_attr.cap.max_recv_sge = 1;

  // qp = ibv_create_qp(pd, &qp_init_attr);

  return 0;
}
