/* Overwrite ibverbs calls. */

#include <errno.h>
#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/vmrc_ht.h"
#include "include/vmrc_log.h"
#include "include/vmrc_symbols.h"
#include "mrc.h"

#define VMRC_DEF_VIS __attribute__ ((visibility ("default")))

/*
 * Test overwrite of ibv_get_device_list.
 */
__asm__ (".symver ovwrt_ibv_get_device_list, ibv_get_device_list@@IBVERBS_1.1");
VMRC_DEF_VIS struct ibv_device** ovwrt_ibv_get_device_list(int* num_devices) {
  struct vmrc_symbols_t* symbols;

  VMRC_DEBUG_PRINT("In ibv_get_device_list");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  return symbols->ibv_get_device_list_internal(num_devices);
}

/*
 * Test overwrite of ibv_get_device_name.
 */

__asm__(".symver ovwrt_ibv_get_device_name, ibv_get_device_name@@IBVERBS_1.1");
VMRC_DEF_VIS const char* ovwrt_ibv_get_device_name(struct ibv_device* device) {
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
__asm__(".symver ovwrt_ibv_open_device, ibv_open_device@@IBVERBS_1.1");
VMRC_DEF_VIS struct ibv_context* ovwrt_ibv_open_device(struct ibv_device* device) {
  struct mrc_attr attr;
  struct vmrc_symbols_t* symbols;
  struct ibv_context* verbs_context;
  struct mrc_context* vmrc_context;
  struct vmrc_ht* hashtable;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ibv_open_device");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  verbs_context = symbols->ibv_open_device_internal(device);
  VMRC_CHECK_PRINT_EXIT(verbs_context, 1, "ibv_open_device failed");

  /* Query the device if it has sufficient MRC capability. */
  mrc_errno = symbols->mrc_query_device_internal(verbs_context, &attr);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(mrc_errno == 0, 1, "Error while calling mrc_query_device. Returned %d. %s", mrc_errno,
                                strerror(mrc_errno));
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
__asm__(".symver ovwrt_ibv_close_device, ibv_close_device@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_close_device(struct ibv_context* verbs_context) {
  struct vmrc_symbols_t* symbols;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ibv_close_device");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");

  /* Retrieving MRC context and destroying it. */
  vmrc_context = (struct mrc_context*)vmrc_ht_search(hashtable, verbs_context);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);
  mrc_errno = symbols->mrc_destroy_context_internal(vmrc_context);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "Error in mrc_destroy_context");

  /* Destroy the verbs context. */
  return symbols->ibv_close_device_internal(verbs_context);
}

/* Overwrite for poll_cq. This is passed as a function pointer. */
int vmrc_ibv_overwrite_poll_cq(struct ibv_cq* cq, int num_entries, struct ibv_wc* wc) {
  struct vmrc_symbols_t* symbols;
  struct mrc_cq* vmrc_cq;
  int ret;

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  vmrc_cq = (struct mrc_cq*)cq->channel;
  ret = symbols->mrc_poll_cq_internal(vmrc_cq, num_entries, wc);

  return ret;
}

/* Overwrite for ibv_create_cq. */
__asm__(".symver ovwrt_ibv_create_cq, ibv_create_cq@@IBVERBS_1.1");
VMRC_DEF_VIS struct ibv_cq* ovwrt_ibv_create_cq(struct ibv_context* verbs_context, int cqe, void* cq_context,
                                  struct ibv_comp_channel* channel, int comp_vector) {
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  struct vmrc_symbols_t* symbols;
  struct ibv_cq* verbs_cq;
  struct mrc_cq* vmrc_cq;
  struct ibv_context* dummy_verbs_context;

  VMRC_DEBUG_PRINT("In ibv_create_cq");

  VMRC_CHECK_PRINT_EXIT(channel == NULL, 1, "Non-NULL completion channel not yet supported");

  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");

  vmrc_context = (struct mrc_context*)vmrc_ht_search(hashtable, verbs_context);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  vmrc_cq = symbols->mrc_create_cq_internal(vmrc_context, cqe, cq_context, NULL, comp_vector);
  VMRC_CHECK_PRINT_EXIT(vmrc_cq, 1, "Error in mrc_create_cq");

  /* Allocate dummy verbs cq. */
  verbs_cq = calloc(1, sizeof(struct ibv_cq));
  VMRC_CHECK_PRINT_EXIT(verbs_cq, 1, "Unable to allocate the dummy verbs CQ");

  /* Store vmrc_cq in verbs_cq->channel. */
  verbs_cq->channel = (struct ibv_comp_channel*)vmrc_cq;

  /* Put the input cq_context in verbs_cq->cq_context. */
  verbs_cq->cq_context = cq_context;

  /* Allocate dummy verbs context. */
  dummy_verbs_context = calloc(1, sizeof(struct ibv_context));
  VMRC_CHECK_PRINT_EXIT(dummy_verbs_context, 1, "Unable to allocate the dummy verbs context");

  /* Replace poll_cq in the dummy verbs context. */
  dummy_verbs_context->ops.poll_cq = &vmrc_ibv_overwrite_poll_cq;

  /* Put the dummy verbs context in verbs_cq's context. */
  verbs_cq->context = dummy_verbs_context;

  return verbs_cq;
}

/* Overwrite of ibv_post_send. */
int vmrc_ibv_overwrite_post_send(struct ibv_qp* qp, struct ibv_send_wr* wr, struct ibv_send_wr** bad_wr) {
  struct vmrc_symbols_t* symbols;
  struct mrc_qp* vmrc_qp;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In vmrc_ibv_overwrite_post_send");

  /* Get MRC QP from QP's send_cq. */
  vmrc_qp = (struct mrc_qp*)qp->send_cq;
  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");
  mrc_errno = symbols->mrc_post_send_internal(vmrc_qp, wr, bad_wr);

  return mrc_errno;
}

/* Overwrite of ibv_post_recv. */
int vmrc_ibv_overwrite_post_recv(struct ibv_qp* qp, struct ibv_recv_wr* wr, struct ibv_recv_wr** bad_wr) {
  struct vmrc_symbols_t* symbols;
  struct mrc_qp* vmrc_qp;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In vmrc_ibv_overwrite_post_recv");

  /* Get MRC QP from QP's send_cq. */
  vmrc_qp = (struct mrc_qp*)qp->send_cq;
  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");
  mrc_errno = symbols->mrc_post_recv_internal(vmrc_qp, wr, bad_wr);

  return mrc_errno;
}

/* Create a dummy struct ibv_qp. Fill the required quantities in it and send it back. */
__asm__(".symver ovwrt_ibv_create_qp, ibv_create_qp@@IBVERBS_1.1");
VMRC_DEF_VIS struct ibv_qp* ovwrt_ibv_create_qp(struct ibv_pd* pd, struct ibv_qp_init_attr* qp_init_attr) {
  struct vmrc_symbols_t* symbols;
  struct ibv_context *verbs_context, *dummy_verbs_context;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  struct mrc_qp* vmrc_qp;
  struct ibv_qp* verbs_qp;
  struct mrc_qp_init_attr mrc_qp_attr;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ibv_create_qp");

  /* Check if the QP type is RC. Other QP types will cause an error. */
  VMRC_CHECK_PRINT_EXIT(qp_init_attr->qp_type == IBV_QPT_RC, 1, "Only RC QP types are supported");

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
  mrc_qp_attr.send_cq = (struct mrc_cq*)qp_init_attr->send_cq->channel;
  mrc_qp_attr.recv_cq = (struct mrc_cq*)qp_init_attr->recv_cq->channel;
  mrc_qp_attr.pd = pd;
  mrc_qp_attr.cap = qp_init_attr->cap; /* Copy the entire struct. */
  mrc_qp_attr.sq_sig_all = qp_init_attr->sq_sig_all;

  /* Create MRC QP. */
  vmrc_qp = symbols->mrc_create_qp_internal(vmrc_context, &mrc_qp_attr);
  VMRC_CHECK_PRINT_EXIT(vmrc_qp, 1, "Error while calling mrc_create_qp");

  /* Allocate a dummy ibv qp. */
  verbs_qp = calloc(1, sizeof(struct ibv_qp));
  VMRC_CHECK_PRINT_EXIT(verbs_qp, 1, "Unable to allocate the dummy verbs QP");

  /* Put MRC qp_num in verbs_qp->qp_num. */
  mrc_errno = symbols->mrc_get_qpn_internal(vmrc_qp, &verbs_qp->qp_num);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "Unable to call mrc_get_qpn");

  /* Put qp_context. */
  verbs_qp->qp_context = qp_init_attr->qp_context;

  /* Put MRC QP in send_cq. */
  verbs_qp->send_cq = (struct ibv_cq*)vmrc_qp;

  /* Allocate dummy verbs context. */
  dummy_verbs_context = calloc(1, sizeof(struct ibv_context));
  VMRC_CHECK_PRINT_EXIT(dummy_verbs_context, 1, "Unable to allocate the dummy verbs context");

  /* Put overwrites of post_send, post_recv. */
  dummy_verbs_context->ops.post_send = &vmrc_ibv_overwrite_post_send;
  dummy_verbs_context->ops.post_recv = &vmrc_ibv_overwrite_post_recv;

  /* Put the dummy verbs context in the returned QP's qp->context. */
  verbs_qp->context = dummy_verbs_context;

  return verbs_qp;
}
