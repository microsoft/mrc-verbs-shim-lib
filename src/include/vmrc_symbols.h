#ifndef _VMRC_SYMBOLS_H_
#define _VMRC_SYMBOLS_H_

#include <infiniband/verbs.h>

/* This will be filled with libmrc.so and libibverbs.so symbols. */
struct vmrc_symbols_t {
  struct ibv_device** (*ibv_get_device_list_internal)(int* num_devices);
  const char* (*ibv_get_device_name_internal)(struct ibv_device* device);
};

/* Returns NULL if error. Otherwise returns a ptr to (struct vmrc_symbols_t*) with symbols loaded. */
struct vmrc_symbols_t* vmrc_symbols_get();

#endif /* _VMRC_SYMBOLS_H_ */
