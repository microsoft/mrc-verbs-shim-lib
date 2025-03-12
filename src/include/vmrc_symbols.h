#ifndef _VMRC_SYMBOLS_H_
#define _VMRC_SYMBOLS_H_

#include <infiniband/verbs.h>

/* This will have the needed symbols from both libmrc.so and libibverbs.so. */
struct vmrc_symbols_t {
  struct ibv_device** (*ibv_get_device_list_internal)(int* num_devices);
  const char* (*ibv_get_device_name_internal)(struct ibv_device* device);
  struct ibv_context* (*ibv_open_device_internal)(struct ibv_device* device);
  struct ibv_qp* (*ibv_create_qp_internal)(struct ibv_pd* pd, struct ibv_qp_init_attr* qp_init_attr);
  struct ibv_cq* (*ibv_create_cq_internal)(struct ibv_context* context, int cqe, void* cq_context,
                                           struct ibv_comp_channel* channel, int comp_vector);
};

/* Returns NULL if error. Otherwise returns a ptr to (struct vmrc_symbols_t*) with symbols loaded. */
struct vmrc_symbols_t* vmrc_symbols_get();

#endif /* _VMRC_SYMBOLS_H_ */
