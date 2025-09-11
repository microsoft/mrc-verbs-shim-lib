#ifndef _VMRC_SYMBOLS_H_
#define _VMRC_SYMBOLS_H_

#include <infiniband/verbs.h>

#include "mrc.h"

/* This will have the needed symbols from both libmrc.so and libibverbs.so. */
struct vmrc_symbols_t {
  /*
   * IBverbs symbols.
   */
  struct ibv_device **(*ibv_get_device_list_internal)(int *num_devices);
  const char *(*ibv_get_device_name_internal)(struct ibv_device *device);
  struct ibv_context *(*ibv_open_device_internal)(struct ibv_device *device);
  int (*ibv_close_device_internal)(struct ibv_context *context);
  struct ibv_qp *(*ibv_create_qp_internal)(struct ibv_pd *pd, struct ibv_qp_init_attr *qp_init_attr);
  struct ibv_cq *(*ibv_create_cq_internal)(struct ibv_context *context, int cqe, void *cq_context,
                                           struct ibv_comp_channel *channel, int comp_vector);
  int (*ibv_query_gid_internal)(struct ibv_context *context, uint8_t port_num, int index, union ibv_gid *gid);

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
  // struct mrc_ev_array *(*mrc_create_ev_array_internal)(struct mrc_context *mrc_ctx, int count,
  //                                                      enum mrc_ev_state *state_array, uint32_t *val_array);
  // int (*mrc_destroy_ev_array_internal)(struct mrc_ev_array *ev_array);
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
