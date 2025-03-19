/* Overwrite ibverbs calls. */

#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/vmrc_ht.h"
#include "include/vmrc_log.h"
#include "include/vmrc_symbols.h"
#include "mrc.h"

/*
 * Test overwrite of ibv_get_device_list.
 */
struct ibv_device** ibv_get_device_list(int* num_devices) {
  struct vmrc_symbols_t* symbols;

  VMRC_DEBUG_PRINT("In ibv_get_device_list");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  return symbols->ibv_get_device_list_internal(num_devices);
}

/*
 * Test overwrite of ibv_get_device_name.
 */
const char* ibv_get_device_name(struct ibv_device* device) {
  struct vmrc_symbols_t* symbols;

  VMRC_DEBUG_PRINT("In ibv_get_device_name");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  return symbols->ibv_get_device_name_internal(device);
}

/*
 * Overwrites ibv_open_device. Queries if the device supports MRC. Errors out if the required capability is not present.
 * Creates an ibv_context. Also, creates an mrc_context. Keeps the (ibv_context, mrc_context) key-value pair in the hash
 * table. Returns the created ibv_context. The returned verbs context can be used to alloc pd and register memory.
 */
struct ibv_context* ibv_open_device(struct ibv_device* device) {
  struct mrc_attr attr;
  struct vmrc_symbols_t* symbols;
  struct ibv_context* verbs_context;
  struct mrc_context* vmrc_context;
  struct vmrc_ht* hashtable;
  int errno;

  VMRC_DEBUG_PRINT("In ibv_open_device");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  verbs_context = symbols->ibv_open_device_internal(device);
  VMRC_CHECK_PRINT_EXIT(verbs_context, 1, "ibv_open_device failed");

  /* Query the device if it has sufficient MRC capability. */
  errno = symbols->mrc_query_device_internal(verbs_context, &attr);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(errno == 0, 1, "Error while calling mrc_query_device. Returned %d", errno);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(attr.mrc_version != (uint32_t)MRC_VERSION_0, 1,
                                "MRC not supported. attr.mrc_version = %d", attr.mrc_version);

  /* Create the MRC context. */
  vmrc_context = symbols->mrc_create_context_internal(verbs_context, attr.mrc_version);
  VMRC_CHECK_PRINT_EXIT(vmrc_context, 1, "Could not create MRC context");

  /* Get the hashtable. */
  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get hashtable");

  /* Insert key, value. */
  vmrc_ht_insert(hashtable, verbs_context, vmrc_context);

  return verbs_context;
}

/* Close the device. Here, before calling close with the verbs context, destroy the MRC context. */
int ibv_close_device(struct ibv_context* verbs_context) {
  struct vmrc_symbols_t* symbols;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  int errno;

  VMRC_DEBUG_PRINT("In ibv_close_device");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");

  /* Retrieving MRC context and destroying it. */
  vmrc_context = (struct mrc_context*)vmrc_ht_search(hashtable, verbs_context);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);
  errno = symbols->mrc_destroy_context_internal(vmrc_context);
  VMRC_CHECK_PRINT_EXIT(errno == 0, 1, "Error in mrc_destroy_context");

  /* Destroy the verbs context. */
  return symbols->ibv_close_device_internal(verbs_context);
}

/* Create MRC CQ from the input parameters and return the pointer to MRC CQ. */
struct ibv_cq* ibv_create_cq(struct ibv_context* verbs_context, int cqe, void* cq_context,
                             struct ibv_comp_channel* channel, int comp_vector) {
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  struct vmrc_symbols_t* symbols;
  struct mrc_cq* cq;

  VMRC_DEBUG_PRINT("In ibv_create_cq");

  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");

  vmrc_context = (struct mrc_context*)vmrc_ht_search(hashtable, verbs_context);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  cq = symbols->mrc_create_cq_internal(vmrc_context, cqe, cq_context, (struct mrc_comp_channel*)channel, comp_vector);
  VMRC_CHECK_PRINT_EXIT(cq, 1, "Error in mrc_create_cq");

  return (struct ibv_cq*)cq;
}

/******** IN PROGRESS *********/
/* Create a dummy (struct ibv_qp) with qp_num field equal to the MRC qp num. Return pointer to this. Store the mrc qp in
 * send_cq. The ev_array will be later stored in recv_cq. */
struct ibv_qp* ibv_create_qp(struct ibv_pd* pd, struct ibv_qp_init_attr* qp_init_attr) {
  struct vmrc_symbols_t* symbols;
  struct ibv_context* verbs_context;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  struct mrc_qp_init_attr mrc_qp_attr;

  VMRC_DEBUG_PRINT("In ibv_create_qp");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  /* Get verbs context and get the corresponding MRC context through hashtable. */
  verbs_context = pd->context;
  VMRC_CHECK_PRINT_EXIT(verbs_context, 1, "pd->context turned out to be NULL");
  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get hashtable");
  vmrc_context = vmrc_ht_search(hashtable, verbs_context);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);

  /* Fill MRC QP attributes. */
  memset(&mrc_qp_attr, 0, sizeof(struct mrc_qp_init_attr));

  mrc_qp_attr.qp_context = qp_init_attr->qp_context;
  mrc_qp_attr.send_cq = (struct mrc_cq*)qp_init_attr->send_cq;
  mrc_qp_attr.recv_cq = (struct mrc_cq*)qp_init_attr->recv_cq;
  mrc_qp_attr.pd = pd;

  //    mrc_qp_attr.qp_context = NULL;
  //    mrc_qp_attr.pd = ctx->pd;
  //    mrc_qp_attr.send_cq = ctx->mrc_send_cq;
  //    mrc_qp_attr.recv_cq = (user_param->verb == WRITE_IMM) ? ctx->mrc_recv_cq : ctx->mrc_send_cq;
  //    mrc_qp_attr.cap.max_send_wr  = user_param->tx_depth;
  //    mrc_qp_attr.cap.max_send_sge = 1;
  //    mrc_qp_attr.cap.max_recv_wr  = user_param->rx_depth;
  //    mrc_qp_attr.cap.max_recv_sge = MAX_RECV_SGE

  /* We will use MRC context to create MRC QP (creating verbs qp just for testing). */
  return symbols->ibv_create_qp_internal(pd, qp_init_attr); /* This will be an MRC create qp call. */
}
