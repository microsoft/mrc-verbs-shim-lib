#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int num_devices = 0;
  struct ibv_device** dev_list = NULL;
  struct ibv_context* context = NULL;
  struct ibv_pd* pd = NULL;
  struct ibv_qp* qp = NULL;
  struct ibv_qp_init_attr qp_init_attr;
  struct ibv_cq* cq = NULL;

  dev_list = ibv_get_device_list(&num_devices);
  const char** dev_names = (const char**)calloc(num_devices, sizeof(const char*));
  for (int i = 0; i < num_devices; ++i) {
    dev_names[i] = ibv_get_device_name(dev_list[i]);
    fprintf(stderr, "dev_name[%2d] = %s\n", i, dev_names[i]);
  }

  context = ibv_open_device(dev_list[0]);

  pd = ibv_alloc_pd(context);

  cq = ibv_create_cq(context, 32 /*int cqe*/, NULL /*void* cq_context*/, NULL /*struct ibv_comp_channel* channel*/,
                     0 /*int comp_vector*/);

  memset(&qp_init_attr, 0, sizeof(struct ibv_qp_init_attr));
  qp_init_attr.send_cq = cq;
  qp_init_attr.recv_cq = cq;
  qp_init_attr.qp_type = IBV_QPT_RC;
  qp_init_attr.cap.max_send_wr = 4;
  qp_init_attr.cap.max_recv_wr = 4;
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;

  qp = ibv_create_qp(pd, &qp_init_attr);

  return 0;
}
