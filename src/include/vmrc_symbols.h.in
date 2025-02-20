#ifndef _VMRC_SYMBOLS_H_
#define _VMRC_SYMBOLS_H_

#include <infiniband/verbs.h>

#define DECLARE_FUNC(retdtype, funcname, ...) retdtype (*funcname)(__VA_ARGS__);

/* This will be filled with libmrc.so symbols. Filling it with libibverbs.so symbols just for checking. */
struct vmrc_symbols_t {
  DECLARE_FUNC(struct ibv_device**, ibv_get_device_list, int* num_devices);
};

struct vmrc_symbols_t* vmrc_symbols_get();

#endif
