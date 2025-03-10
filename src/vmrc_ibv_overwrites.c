/* Overwrite ibverbs calls. */

#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/vmrc_ht.h"
#include "include/vmrc_log.h"
#include "include/vmrc_symbols.h"

struct ibv_device** ibv_get_device_list(int* num_devices) {
  VMRC_DEBUG_PRINT("In ibv_get_device_list");

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  return symbols->ibv_get_device_list_internal(num_devices);
}

const char* ibv_get_device_name(struct ibv_device* device) {
  VMRC_DEBUG_PRINT("In ibv_get_device_name");

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  return symbols->ibv_get_device_name_internal(device);
}

struct ibv_context* ibv_open_device(struct ibv_device* device) {
  VMRC_DEBUG_PRINT("In ibv_open_device");

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  struct ibv_context* verbs_context = symbols->ibv_open_device_internal(device);

  /* Create the MRC context (creating another verbs context just for testing). */
  struct ibv_context* context = symbols->ibv_open_device_internal(device);

  /* Get the hashtable. */
  struct vmrc_ht* hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get hashtable");

  /* Insert key, value. */
  vmrc_ht_insert(hashtable, verbs_context, context);

  return verbs_context;
}

struct ibv_cq* ibv_create_cq(struct ibv_context* verbs_context, int cqe, void* cq_context,
                             struct ibv_comp_channel* channel, int comp_vector) {
  VMRC_DEBUG_PRINT("In ibv_create_cq");

  struct vmrc_ht* hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");

  struct ibv_context* context = vmrc_ht_search(hashtable, verbs_context); /* This will be an MRC context later. */
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  return symbols->ibv_create_cq_internal(verbs_context, cqe, cq_context, channel,
                                         comp_vector); /* This will be an MRC create cq call. */
}

struct ibv_qp* ibv_create_qp(struct ibv_pd* pd, struct ibv_qp_init_attr* qp_init_attr) {
  VMRC_DEBUG_PRINT("In ibv_create_qp");

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  /* Get corresponding MRC context. */
  struct ibv_context* verbs_context = pd->context;
  VMRC_CHECK_PRINT_EXIT(verbs_context, 1, "pd->context turned out to be NULL");
  struct vmrc_ht* hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get hashtable");
  struct ibv_context* context = vmrc_ht_search(hashtable, verbs_context); /* This will be an MRC context later. */
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);

  /* We will use MRC context to create MRC QP (creating verbs qp just for testing). */
  return symbols->ibv_create_qp_internal(pd, qp_init_attr); /* This will be an MRC create qp call. */
}
