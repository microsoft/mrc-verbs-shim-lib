// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/include/vmrc_json.h"

int main() {
  int num_evs;
  uint32_t *ev_list;

  ev_list = vmrc_json_get_ev_list("1002:01::1", "1002:04::1", &num_evs);

  fprintf(stderr, "num_evs: %d\n", num_evs);
  for (int i = 0; i < num_evs; ++i) {
    fprintf(stderr, "ev_list[%4d] = %" PRIu32 "\n", i, ev_list[i]);
  }

  return 0;
}
