// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef _VMRC_SYMBOLS_H_
#define _VMRC_SYMBOLS_H_

#include <infiniband/verbs.h>

#include "mrc.h"

/* This will have the needed symbols from both libmrc.so and libibverbs.so. */
struct vmrc_symbols_t {
  /*
   * IBverbs symbols.
   */
  int (*ibv_fork_init_internal)(void);
  struct ibv_device **(*ibv_get_device_list_internal)(int *num_devices);
  void (*ibv_free_device_list_internal)(struct ibv_device **list);
  const char *(*ibv_get_device_name_internal)(struct ibv_device *device);
  struct ibv_context *(*ibv_open_device_internal)(struct ibv_device *device);
  int (*ibv_close_device_internal)(struct ibv_context *context);
  int (*ibv_get_async_event_internal)(struct ibv_context *context, struct ibv_async_event *event);
  void (*ibv_ack_async_event_internal)(struct ibv_async_event *event);
  int (*ibv_query_device_internal)(struct ibv_context *context, struct ibv_device_attr *device_attr);
  int (*ibv_query_port_internal)(struct ibv_context *context, uint8_t port_num, struct ibv_port_attr *port_attr);
  int (*ibv_query_gid_internal)(struct ibv_context *context, uint8_t port_num, int index, union ibv_gid *gid);
  struct ibv_pd *(*ibv_alloc_pd_internal)(struct ibv_context *context);
  int (*ibv_dealloc_pd_internal)(struct ibv_pd *pd);
  struct ibv_mr *(*ibv_reg_mr_internal)(struct ibv_pd *pd, void *addr, size_t length, int access);
  struct ibv_mr *(*ibv_reg_mr_iova2_internal)(struct ibv_pd *pd, void *addr, size_t length, uint64_t iova,
                                              unsigned int access); /* IB_VERBS1.8 */
  struct ibv_mr *(*ibv_reg_dmabuf_mr_internal)(struct ibv_pd *pd, uint64_t offset, size_t length, uint64_t iova, int fd,
                                               int access); /* IB_VERBS1.12 */
  int (*ibv_dereg_mr_internal)(struct ibv_mr *mr);
  int (*ibv_query_ece_internal)(struct ibv_qp *qp, struct ibv_ece *ece); /* IB_VERBS1.10 */
  int (*ibv_set_ece_internal)(struct ibv_qp *qp, struct ibv_ece *ece);   /* IB_VERBS1.10 */
  const char *(*ibv_event_type_str_internal)(enum ibv_event_type event);
  /*
   * MRC symbols.
   */

  int (*mrc_query_device_internal)(struct ibv_context *context, struct mrc_attr *attr, int *supported);
  struct mrc_context *(*mrc_create_context_internal)(struct ibv_context *vcontext,
                                                     struct mrc_context_attr *context_attr);
  int (*mrc_destroy_context_internal)(struct mrc_context *mrc_ctx);
  struct mrc_cq *(*mrc_create_cq_internal)(struct mrc_context *mrc_ctx, int cqe, void *cq_context,
                                           struct mrc_comp_channel *channel, int comp_vector);
  int (*mrc_poll_cq_internal)(struct mrc_cq *cq, int num_entries, struct ibv_wc *wc);
  int (*mrc_destroy_cq_internal)(struct mrc_cq *cq);
  struct mrc_qp *(*mrc_create_qp_internal)(struct mrc_context *mrc_ctx, struct mrc_qp_init_attr *mrc_qp_attr);
  int (*mrc_destroy_qp_internal)(struct mrc_qp *qp);
  struct mrc_qp_hint *(*mrc_create_qp_hint_internal)(struct mrc_context *mrc_ctx,
                                                     struct mrc_qp_hint_init_attr *init_attr);
  int (*mrc_destroy_qp_hint_internal)(struct mrc_qp_hint *qp_hint);

  int (*mrc_query_qp_internal)(struct mrc_qp *qp, struct ibv_qp_attr *vattr, int vattr_mask,
                               struct mrc_qp_attr *mrc_attr, int mrc_attr_mask, struct mrc_qp_init_attr *init_attr);
  int (*mrc_modify_qp_internal)(struct mrc_qp *qp, struct ibv_qp_attr *vattr, int vattr_mask,
                                struct mrc_qp_attr *mrc_attr, int mrc_attr_mask);
  int (*mrc_get_qpn_internal)(struct mrc_qp *qp, uint32_t *qpn);
  int (*mrc_post_recv_internal)(struct mrc_qp *qp, struct ibv_recv_wr *wr, struct ibv_recv_wr **bad_wr);
  int (*mrc_post_send_internal)(struct mrc_qp *qp, struct ibv_send_wr *wr, struct ibv_send_wr **bad_wr);
};

/* Returns NULL if error. Otherwise returns a ptr to (struct vmrc_symbols_t*) with symbols loaded. */
struct vmrc_symbols_t *vmrc_symbols_get();

#endif /* _VMRC_SYMBOLS_H_ */
