// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/* Overwrite ibverbs calls. */

#include <arpa/inet.h>
#include <errno.h>
#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/vmrc_ht.h"
#include "include/vmrc_log.h"
#include "include/vmrc_symbols.h"
#include "mrc.h"

#define VMRC_DEF_VIS __attribute__((visibility("default")))

/* Versioned symbol alias and defines a wrapper that calls <name>_internal. */
#define VMRC_WRAP_SYMVER(name, ver, rettype, params, args)      \
  __asm__(".symver ovwrt_" #name ", " #name "@@" ver);          \
  VMRC_DEF_VIS rettype ovwrt_##name params {                    \
    struct vmrc_symbols_t* symbols;                             \
    VMRC_DEBUG_PRINT("In " #name);                              \
    symbols = vmrc_symbols_get();                               \
    VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols"); \
    return symbols->name##_internal args;                       \
  }

/* Pass through IBverbs symbols. */
VMRC_WRAP_SYMVER(ibv_fork_init, "IBVERBS_1.1", int, (void), ())
VMRC_WRAP_SYMVER(ibv_get_device_list, "IBVERBS_1.1", struct ibv_device**, (int* num_devices), (num_devices))
VMRC_WRAP_SYMVER(ibv_free_device_list, "IBVERBS_1.1", void, (struct ibv_device * *list), (list))
VMRC_WRAP_SYMVER(ibv_get_device_name, "IBVERBS_1.1", const char*, (struct ibv_device * device), (device))
VMRC_WRAP_SYMVER(ibv_get_async_event, "IBVERBS_1.1", int, (struct ibv_context * context, struct ibv_async_event* event),
                 (context, event))
VMRC_WRAP_SYMVER(ibv_ack_async_event, "IBVERBS_1.1", void, (struct ibv_async_event * event), (event))
VMRC_WRAP_SYMVER(ibv_query_device, "IBVERBS_1.1", int,
                 (struct ibv_context * context, struct ibv_device_attr* device_attr), (context, device_attr))
#undef ibv_query_port  // Undefine the macro and redefine after instantiating pass through for ibv_query_port.
VMRC_WRAP_SYMVER(ibv_query_port, "IBVERBS_1.1", int,
                 (struct ibv_context * context, uint8_t port_num, struct ibv_port_attr* port_attr),
                 (context, port_num, port_attr))
#define ibv_query_port(context, port_num, port_attr) ___ibv_query_port(context, port_num, port_attr)
VMRC_WRAP_SYMVER(ibv_query_gid, "IBVERBS_1.1", int,
                 (struct ibv_context * context, uint8_t port_num, int index, union ibv_gid* gid),
                 (context, port_num, index, gid))
VMRC_WRAP_SYMVER(ibv_alloc_pd, "IBVERBS_1.1", struct ibv_pd*, (struct ibv_context * context), (context))
VMRC_WRAP_SYMVER(ibv_dealloc_pd, "IBVERBS_1.1", int, (struct ibv_pd * pd), (pd))
#undef ibv_reg_mr  // Undefine the macro and redefine after instantiating pass through for ibv_reg_mr.
VMRC_WRAP_SYMVER(ibv_reg_mr, "IBVERBS_1.1", struct ibv_mr*, (struct ibv_pd * pd, void* addr, size_t length, int access),
                 (pd, addr, length, access))
#define ibv_reg_mr(pd, addr, length, access) \
  __ibv_reg_mr(pd, addr, length, access, __builtin_constant_p(((int)(access) & IBV_ACCESS_OPTIONAL_RANGE) == 0))
VMRC_WRAP_SYMVER(ibv_reg_mr_iova2, "IBVERBS_1.8", struct ibv_mr*,
                 (struct ibv_pd * pd, void* addr, size_t length, uint64_t iova, unsigned int access),
                 (pd, addr, length, iova, access))
VMRC_WRAP_SYMVER(ibv_reg_dmabuf_mr, "IBVERBS_1.12", struct ibv_mr*,
                 (struct ibv_pd * pd, uint64_t offset, size_t length, uint64_t iova, int fd, int access),
                 (pd, offset, length, iova, fd, access))
VMRC_WRAP_SYMVER(ibv_dereg_mr, "IBVERBS_1.1", int, (struct ibv_mr * mr), (mr))
VMRC_WRAP_SYMVER(ibv_event_type_str, "IBVERBS_1.1", const char*, (enum ibv_event_type event), (event))

/* Print info to not use the verb and exit. */
#define VMRC_WRAP_SYMVER_ERR(name, ver, rettype, params, args)                                           \
  __asm__(".symver ovwrt_" #name ", " #name "@@" ver);                                                   \
  VMRC_DEF_VIS rettype ovwrt_##name params {                                                             \
    VMRC_CHECK_PRINT_EXIT(NULL, 1, #name " cannot be used with verbs-mrc");                              \
    return 1; /* To satisfy the compiler. This line will never be reached since the above line exits. */ \
  }

VMRC_WRAP_SYMVER_ERR(ibv_query_ece, "IBVERBS_1.10", int, (struct ibv_qp * qp, struct ibv_ece* ece), (qp, ece))
VMRC_WRAP_SYMVER_ERR(ibv_set_ece, "IBVERBS_1.10", int, (struct ibv_qp * qp, struct ibv_ece* ece), (qp, ece))

/* We cannot overwrite qp_ex with shim since ibv_create_qp_ex is a function pointer. */
struct ibv_qp* vmrc_ibv_overwrite_create_qp_ex(struct ibv_context* context,
                                               struct ibv_qp_init_attr_ex* qp_init_attr_ex) {
  VMRC_CHECK_PRINT_EXIT(NULL, 1, "create_qp_ex cannot be used with verbs-mrc");
  return NULL;
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
  struct mrc_context_attr vmrc_context_attr;
  struct vmrc_ht* hashtable;
  int mrc_errno;
  struct verbs_context* vctx;
  int mrc_supported;

  VMRC_DEBUG_PRINT("In ibv_open_device");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  verbs_context = symbols->ibv_open_device_internal(device);
  VMRC_CHECK_PRINT_EXIT(verbs_context, 1, "ibv_open_device failed");

  /* Query the device if it has sufficient MRC capability. */
  mrc_errno = symbols->mrc_query_device_internal(verbs_context, &attr, &mrc_supported);
  if (mrc_errno != 0) {
    VMRC_DEBUG_PRINT_VA_ARGS(
        "Dev %s does not appear to support MRC. mrc_errno = %d. Simply returning the verbs context",
        symbols->ibv_get_device_name_internal(device), mrc_errno);
    return verbs_context;
  }
  if (!mrc_supported) {
    VMRC_DEBUG_PRINT_VA_ARGS("MRC not supported for dev = %s. Returning verbs context.",
                             symbols->ibv_get_device_name_internal(device));
    return verbs_context;
  }

  /* Create the MRC context. */
  memset(&vmrc_context_attr, 0, sizeof(vmrc_context_attr));
  vmrc_context = symbols->mrc_create_context_internal(verbs_context, &vmrc_context_attr);
  VMRC_CHECK_PRINT_EXIT(vmrc_context, 1, "Could not create MRC context");

  /* Get the hashtable. */
  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get hashtable");

  /* Insert key, value. */
  vmrc_ht_insert(hashtable, verbs_context, vmrc_context);

  /* Overwrite create_qp_ex. */
  vctx = verbs_get_ctx_op(verbs_context, create_qp_ex);
  if (!vctx) {
    mrc_errno = symbols->mrc_destroy_context_internal(vmrc_context);
    VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "Error in mrc_destroy_context");
    errno = EOPNOTSUPP;
    return NULL;
  }
  vctx->create_qp_ex = &vmrc_ibv_overwrite_create_qp_ex;

  return verbs_context;
}

/* Close the device. Here, before calling close with the verbs context, destroy the MRC context. */
__asm__(".symver ovwrt_ibv_close_device, ibv_close_device@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_close_device(struct ibv_context* verbs_context) {
  struct vmrc_symbols_t* symbols;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  int mrc_errno;
  void* addr_of_value;

  VMRC_DEBUG_PRINT("In ibv_close_device");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");

  /* Retrieving MRC context and destroying it. */
  vmrc_context = (struct mrc_context*)vmrc_ht_search_plus_addr(hashtable, verbs_context, &addr_of_value);
  if (vmrc_context == NULL) {
    VMRC_DEBUG_PRINT("No matching MRC context found. Simply closing the device");
    return symbols->ibv_close_device_internal(verbs_context);
  }

  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);

  /* Destroy all the QP hints. */
  struct vmrc_ht_linked_list *attr = vmrc_ht_attr_get(addr_of_value, VMRC_HT_ATTR_QP_HINT_IDX);
  while (attr != NULL) {
    struct mrc_qp_hint *qp_hint = (struct mrc_qp_hint*)attr->ptr_and_next[VMRC_HT_LL_PTR];
    mrc_errno = symbols->mrc_destroy_qp_hint_internal(qp_hint);
    VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "Error in mrc_destroy_qp_hint");
    attr = (struct vmrc_ht_linked_list*)attr->ptr_and_next[VMRC_HT_LL_NEXT];
  }

  mrc_errno = symbols->mrc_destroy_context_internal(vmrc_context);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "Error in mrc_destroy_context");

  /* Delete the entry from the hashtable. */
  vmrc_ht_delete(hashtable, verbs_context);

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

  vmrc_cq = (void*)cq->channel;
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
  verbs_cq->channel = (void*)vmrc_cq;

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

/* Overwrite for ibv_destroy_cq. */
__asm__(".symver ovwrt_ibv_destroy_cq, ibv_destroy_cq@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_destroy_cq(struct ibv_cq* verbs_cq) {
  struct vmrc_symbols_t* symbols;
  struct mrc_cq* vmrc_cq;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ovwrt_ibv_destroy_cq");

  /* Free the dummy verbs context. */
  free(verbs_cq->context);

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  /* Free MRC cq. */
  vmrc_cq = (void*)verbs_cq->channel;
  mrc_errno = symbols->mrc_destroy_cq_internal(vmrc_cq);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "mrc_destroy_cq failed");

  /* Free the dummy verbs cq. */
  free(verbs_cq);

  return 0;
}

/* Overwrite of ibv_post_send. */
int vmrc_ibv_overwrite_post_send(struct ibv_qp* qp, struct ibv_send_wr* wr, struct ibv_send_wr** bad_wr) {
  struct vmrc_symbols_t* symbols;
  struct mrc_qp* vmrc_qp;
  int mrc_errno;
  struct ibv_send_wr* tmp_wr = wr;

  VMRC_DEBUG_PRINT("In vmrc_ibv_overwrite_post_send");

  while (tmp_wr != NULL) {
    VMRC_CHECK_PRINT_EXIT_VA_ARGS(
        tmp_wr->opcode == IBV_WR_RDMA_WRITE || tmp_wr->opcode == IBV_WR_RDMA_WRITE_WITH_IMM, 1,
        "Expected opcode to be either RDMA_WRITE or RDMA_WRITE_WITH_IMM. Instead, it is %d", tmp_wr->opcode);
    tmp_wr = tmp_wr->next;
  }

  /* Get MRC QP from QP's send_cq. */
  vmrc_qp = (void*)qp->send_cq;
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
  vmrc_qp = (void*)qp->send_cq;
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
  void* ptr;
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
  mrc_qp_attr.send_cq = (void*)qp_init_attr->send_cq->channel;
  mrc_qp_attr.recv_cq = (void*)qp_init_attr->recv_cq->channel;
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
  verbs_qp->send_cq = (void*)vmrc_qp;

  /* Allocate dummy verbs context. */
  dummy_verbs_context = calloc(1, sizeof(struct ibv_context));
  VMRC_CHECK_PRINT_EXIT(dummy_verbs_context, 1, "Unable to allocate the dummy verbs context");

  /* Put overwrites of post_send, post_recv. */
  dummy_verbs_context->ops.post_send = &vmrc_ibv_overwrite_post_send;
  dummy_verbs_context->ops.post_recv = &vmrc_ibv_overwrite_post_recv;

  /* Put the dummy verbs context in the returned QP's qp->context. */
  verbs_qp->context = dummy_verbs_context;

  /* Put the actual verbs context in verbs_qp->pd. This will be used to get gid of this QP when the QP is transitioned
   * to INIT and to get the matching MRC context while creating EV array. Put the verbs context in verbs_qp->recv_cq. */
  verbs_qp->pd = pd;
  verbs_qp->recv_cq = (void*)verbs_context;

  /* Allocate 128 bits (16 uint8_t) + 2 void * entries and assign the pointer to srq. This will be used to store the gid
   * raw of this QP and for storing the input send_cq and recv_cq. */
  verbs_qp->srq = (void*)calloc(16 + 2 * (sizeof(void*) / sizeof(uint8_t)), sizeof(uint8_t));
  ptr = (void*)verbs_qp->srq;
  ptr = ptr + (16 / sizeof(void*)); /* 16 bytes aka 128 bits */
  ptr = (void*)qp_init_attr->send_cq;
  ptr = ptr + 1;
  ptr = (void*)qp_init_attr->recv_cq;

  return verbs_qp;
}

/*
 * Overwrite of ibv_query_qp.
 */

__asm__(".symver ovwrt_ibv_query_qp, ibv_query_qp@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_query_qp(struct ibv_qp* verbs_qp, struct ibv_qp_attr* vattr, int vattr_mask,
                                            struct ibv_qp_init_attr* vinit_attr) {
  struct vmrc_symbols_t* symbols;
  int mrc_errno;
  struct mrc_qp_init_attr mrc_init_attr;
  struct mrc_qp* vmrc_qp;
  struct mrc_qp_attr mrc_attr;
  int mrc_attr_mask = 0;
  void* ptr;

  VMRC_DEBUG_PRINT("In ibv_query_qp");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  /* Get MRC qp. */
  vmrc_qp = (void*)verbs_qp->send_cq;

  /* Call mrc query qp internal using mrc_qp and mrc_init_attr. */
  mrc_errno = symbols->mrc_query_qp_internal(vmrc_qp, vattr, vattr_mask, &mrc_attr, mrc_attr_mask, &mrc_init_attr);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "mrc_query_qp failed");

  /* From the returned mrc_qp_init_attr, fill init_attr (verbs attr). */
  vinit_attr->qp_context = mrc_init_attr.qp_context;
  ptr = (void*)verbs_qp->srq;
  ptr = ptr + (16 / sizeof(void*)); /* First 128 bits are for the raw gid. */
  vinit_attr->send_cq = (void*)ptr;
  ptr = ptr + 1;
  vinit_attr->recv_cq = (void*)ptr;
  vinit_attr->qp_type = IBV_QPT_RC;
  vinit_attr->cap = mrc_init_attr.cap;
  vinit_attr->sq_sig_all = mrc_init_attr.sq_sig_all;

  return 0;
}

/* Overwrite for ibv_destroy_qp. */
__asm__(".symver ovwrt_ibv_destroy_qp, ibv_destroy_qp@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_destroy_qp(struct ibv_qp* verbs_qp) {
  struct vmrc_symbols_t* symbols;
  struct mrc_qp* vmrc_qp;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ovwrt_ibv_destroy_qp");

  /* Free the dummy verbs context. */
  free(verbs_qp->context);

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  /* Free MRC qp. */
  vmrc_qp = (void*)verbs_qp->send_cq;
  mrc_errno = symbols->mrc_destroy_qp_internal(vmrc_qp);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "mrc_destroy_qp failed");

  /* Free the dummy srq. */
  free(verbs_qp->srq);

  /* Free the dummy verbs qp. */
  free(verbs_qp);

  return 0;
}

/* Overwrite for ibv_modify_qp. */
__asm__(".symver ovwrt_ibv_modify_qp, ibv_modify_qp@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_modify_qp(struct ibv_qp* verbs_qp, struct ibv_qp_attr* vattr, int vattr_mask) {
  struct vmrc_symbols_t* symbols;
  struct mrc_qp* vmrc_qp;
  struct mrc_qp_attr mrc_attr;
  int mrc_attr_mask;
  struct ibv_context* verbs_context;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  uint8_t* gid_raw;
  struct mrc_qp_hint_init_attr vmrc_qp_hint_init_attr;
  struct mrc_qp_hint* vmrc_qp_hint;
  void* addr_of_value = NULL;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ibv_modify_qp");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  /* Get MRC QP from send_cq. */
  vmrc_qp = (void*)verbs_qp->send_cq;

  if (vattr->qp_state == IBV_QPS_INIT) {
    /* Get MRC context. */
    verbs_context = (void*)verbs_qp->recv_cq;
    hashtable = vmrc_ht_get();
    VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");
    vmrc_context = (struct mrc_context*)vmrc_ht_search_plus_addr(hashtable, verbs_context, &addr_of_value);
    VMRC_CHECK_PRINT_EXIT(vmrc_context, 1, "Could not find matching MRC context");

    /* Create MRC QP hint. */
    memset(&vmrc_qp_hint_init_attr, 0, sizeof(struct mrc_qp_hint_init_attr));
    vmrc_qp_hint_init_attr.attr.num_qps_per_peer = 1;
    vmrc_qp_hint_init_attr.attr.num_send_peers = 1;
    vmrc_qp_hint = symbols->mrc_create_qp_hint_internal(vmrc_context, &vmrc_qp_hint_init_attr);
    vmrc_ht_attr_insert(addr_of_value, (void*)vmrc_qp_hint, VMRC_HT_ATTR_QP_HINT_IDX);

    mrc_attr_mask = MRC_QP_HINT;
    memset(&mrc_attr, 0, sizeof(struct mrc_qp_attr));
    mrc_attr.qp_hint = vmrc_qp_hint;

  } else if (vattr->qp_state == IBV_QPS_RTR) {
    union ibv_gid my_gid;

    VMRC_CHECK_PRINT_EXIT(vattr->ah_attr.is_global == 1, 1,
                          "vattr->ah_attr.is_global is not 1. verbs_mrc only accepts global gids\n");

    /* Get the GID of this QP and store it in verbs_qp->srq. Assume correct port_num is passed during RTR transition. */
    verbs_context = (void*)verbs_qp->recv_cq;
    VMRC_CHECK_PRINT_EXIT(symbols->ibv_query_gid_internal(verbs_context, vattr->ah_attr.port_num,
                                                          vattr->ah_attr.grh.sgid_index, &my_gid) == 0,
                          1, "ibv_query_gid failed");

    /* Store my_gid.raw (128 bits) in (void *) verbs_qp->srq. */
    memcpy((void*)verbs_qp->srq, my_gid.raw, 16);

    /* Set the mrc_attr and mrc_attr_mask to pass in the mrc_ev_array. */
    mrc_attr_mask = 0U;
    memset(&mrc_attr, 0, sizeof(mrc_attr));

  } else if (vattr->qp_state == IBV_QPS_RTS) {
    mrc_attr_mask = 0U; /* No MRC related attr mask. */
  }
  mrc_errno = symbols->mrc_modify_qp_internal(vmrc_qp, vattr, vattr_mask, &mrc_attr, mrc_attr_mask);
  /* Reflect the new state in the dummy verbs qp. NCCL accesses ->state in ibvModifyQpLog */
  if (mrc_errno == 0) verbs_qp->state = vattr->qp_state;

  return mrc_errno;
}
