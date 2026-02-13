// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <stdio.h>

void print_ipv6_address(union ibv_gid *gid) {
  char ipv6_str[INET6_ADDRSTRLEN];
  inet_ntop(AF_INET6, gid->raw, ipv6_str, INET6_ADDRSTRLEN);
  printf("IPv6 Address: %s\n", ipv6_str);
}

int main() {
  int num_devices = 0;
  struct ibv_device **dev_list = NULL;
  struct ibv_context *context;
  uint8_t port_num = 1;  // Port number to query
  int gid_index = 0;     // GID index to query
  union ibv_gid gid;

  dev_list = ibv_get_device_list(&num_devices);
  context = ibv_open_device(dev_list[0]);

  if (ibv_query_gid(context, port_num, gid_index, &gid)) {
    fprintf(stderr, "Failed to query GID\n");
    return 1;
  }

  print_ipv6_address(&gid);

  return 0;
}
