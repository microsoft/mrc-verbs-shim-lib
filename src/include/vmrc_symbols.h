#ifndef _VMRC_SYMBOLS_H_
#define _VMRC_SYMBOLS_H_

#include <infiniband/verbs.h>

/* This will be filled with libmrc.so symbols. Filling it with libibverbs.so just for checking. */
struct vmrc_symbols_t {
  struct ibv_device** (*ibv_internal_get_device_list)(int* num_devices);
};

struct vmrc_symbols_t* vmrc_symbols_get();

#endif
