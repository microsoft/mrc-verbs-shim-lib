/* Overwrite ibv calls. */

#include <stdio.h>

#include "include/vmrc_ht.h"
#include "include/vmrc_symbols.h"

struct ibv_device** ibv_get_device_list(int* num_devices) {
#ifdef VERBS_MRC_DEBUG
  fprintf(stderr, "verbs-mrc: In ibv_get_device_list overwrite\n");
#endif

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  if (!symbols) {
    fprintf(stderr, "Error: Could not get symbols in verbs-mrc shim layer\n");
  }

  return symbols->ibv_get_device_list_internal(num_devices);
}

const char* ibv_get_device_name(struct ibv_device* device) {
#ifdef VERBS_MRC_DEBUG
  fprintf(stderr, "verbs-mrc: In ibv_get_device_name overwrite\n");
#endif

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  if (!symbols) {
    fprintf(stderr, "Error: Could not get symbols in verbs-mrc shim layer\n");
  }

  return symbols->ibv_get_device_name_internal(device);
}

struct ibv_context* ibv_open_device(struct ibv_device* device) {
#ifdef VERBS_MRC_DEBUG
  fprintf(stderr, "verbs-mrc: In ibv_open_device\n");
#endif

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  if (!symbols) {
    fprintf(stderr, "Error: Could not get symbols in verbs-mrc shim layer\n");
  }

  struct ibv_context* verbs_context = symbols->ibv_open_device_internal(device);

  /* Create the MRC context (creating another verbs context just for testing). */
  struct ibv_context* context = symbols->ibv_open_device_internal(device);

  /* Get the hashtable. */
  struct vmrc_ht* hashtable = vmrc_ht_get();
  fprintf(stderr, "in ibv_open_device, ibv_context = %p, mrc_context = %p\n", verbs_context, context);

  /* Insert key, value. */
  vmrc_ht_insert(hashtable, verbs_context, context);

  return verbs_context;
}

struct ibv_qp* ibv_create_qp(struct ibv_pd* pd, struct ibv_qp_init_attr* qp_init_attr) {
#ifdef VERBS_MRC_DEBUG
  fprintf(stderr, "verbs-mrc: In ibv_create_qp\n");
#endif

  struct vmrc_symbols_t* symbols = vmrc_symbols_get();
  if (!symbols) {
    fprintf(stderr, "Error: Could not get symbols in verbs-mrc shim layer\n");
  }

  /* Get corresponding MRC context. */
  struct ibv_context* verbs_context = pd->context;
  struct vmrc_ht* hashtable = vmrc_ht_get();
  struct ibv_context* context = vmrc_ht_search(hashtable, verbs_context);

  fprintf(stderr, "ibv_context = %p, mrc_context = %p\n", verbs_context, context);

  fprintf(stderr, "Exiting before creating qp.\n");
  return NULL;

  /* Use MRC context to create MRC QP (creating verbs qp just for testing). */
  return symbols->ibv_create_qp_internal(pd, qp_init_attr);
}
