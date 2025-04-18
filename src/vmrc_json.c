#include "include/vmrc_json.h"

#include <stdio.h>
#include <stdlib.h>

#include "include/cJSON.h"
#include "include/vmrc_log.h"

char *vmrc_json_read(const char *system_json_fname) {
  int ret;
  FILE *file;
  long length;
  char *buffer;
  size_t fread_ret;

  file = fopen(system_json_fname, "r");
  VMRC_CHECK_PRINT_EXIT(file, 1, "Failed to open system json file");

  ret = fseek(file, 0, SEEK_END);
  VMRC_CHECK_PRINT_EXIT(ret == 0, 1, "fseek 0 SEEK_END failed");
  length = ftell(file);
  VMRC_CHECK_PRINT_EXIT(length != -1, 1, "ftell failed");
  ret = fseek(file, 0, SEEK_SET);
  VMRC_CHECK_PRINT_EXIT(ret == 0, 1, "fseek 0 SEEK_SET failed");

  buffer = (char *)malloc(length + 1);
  VMRC_CHECK_PRINT_EXIT(buffer, 1, "Unable to allocate buffer to read system json file");
  fread_ret = fread(buffer, 1, length, file);
  VMRC_CHECK_PRINT_EXIT(fread_ret == (size_t)length, 1, "fread did not read the full system json file");
  buffer[length] = '\0';

  fclose(file);
  return buffer;
}

uint32_t *vmrc_json_get_ev_list(char *my_ipv6_str, char *rem_ipv6_str, int *num_evs) {
  static cJSON *json = NULL;
  cJSON *my_ipv6_obj = NULL;
  cJSON *rem_ipv6_obj = NULL;
  cJSON *rem_ipv6_arr = NULL;
  cJSON *ps_list = NULL;
  uint32_t *ev_list = NULL;

  if (json == NULL) {
    /* Read JSON file. */
    const char *mrc_system_json = getenv("VMRC_SYSTEM_JSON");
    char *json_data = NULL;

    VMRC_CHECK_PRINT_EXIT(mrc_system_json, 1, "VMRC_SYSTEM_JSON env var is not set.");
    VMRC_DEBUG_PRINT_VA_ARGS("Loading mrc system json from %s", mrc_system_json);

    json_data = vmrc_json_read(mrc_system_json);
    json = cJSON_Parse(json_data);
    VMRC_CHECK_PRINT_EXIT(json != NULL, 1, "cJSON_Parse failed");
  }

  /* Find the ev_list for my_ipv6_str and rem_ipv6_str. */

  my_ipv6_obj = cJSON_GetObjectItem(json, my_ipv6_str);
  VMRC_CHECK_PRINT_EXIT(my_ipv6_obj != NULL, 1, "my_ipv6_obj is NULL");

  rem_ipv6_obj = cJSON_GetObjectItem(my_ipv6_obj, rem_ipv6_str);
  VMRC_CHECK_PRINT_EXIT(rem_ipv6_obj != NULL, 1, "rem_ipv6_obj is NULL");

  /* If null, then it could be loopback. So, add a simple ev list. */
  if (rem_ipv6_obj == NULL) {
    *num_evs = 8;
    ev_list = (uint32_t *)calloc(*num_evs, sizeof(uint32_t));
    VMRC_CHECK_PRINT_EXIT(ev_list != NULL, 1, "ev_list allocation failed");
    for (int i = 0; i < *num_evs; ++i) {
      ev_list[i] = i + 1;
    }

    return ev_list;
  }

  rem_ipv6_arr = cJSON_GetArrayItem(rem_ipv6_obj, 0);
  VMRC_CHECK_PRINT_EXIT(rem_ipv6_arr != NULL, 1, "rem_ipv6_arr is NULL");

  ps_list = cJSON_GetObjectItem(rem_ipv6_arr, "ps_list");
  VMRC_CHECK_PRINT_EXIT(ps_list != NULL, 1, "ps_list is NULL");

  *num_evs = cJSON_GetArraySize(ps_list);
  ev_list = (uint32_t *)calloc(*num_evs, sizeof(uint32_t));
  VMRC_CHECK_PRINT_EXIT(ev_list != NULL, 1, "ev_list allocation failed");
  for (int i = 0; i < *num_evs; ++i) {
    ev_list[i] = cJSON_GetArrayItem(ps_list, i)->valueuint32_t;
    if (ev_list[i] == 0) ev_list[i] = 8; /* Zero EV value not allowed in DOCA. So, use 8 to use the same plane. */
  }

  // fprintf(stderr, "Printing EV values from json. *num_evs = %d\n", *num_evs);
  // for (int i=0; i<*num_evs; ++i) {
  //         fprintf(stderr, "ev_list[%4d]=%u\n", i, ev_list[i]);
  // }
  //  if(!ev_list ) free(ev_list);
  //  ev_list = NULL;

  // *num_evs = 8;
  // ev_list = (uint32_t *)calloc(*num_evs, sizeof(uint32_t));
  // VMRC_CHECK_PRINT_EXIT(ev_list != NULL, 1, "ev_list allocation failed");
  // for (int i = 0; i < *num_evs; ++i) {
  //   ev_list[i] = i;
  //   if (ev_list[i] == 0) ev_list[i] = 8;
  // }
  // return ev_list;

  return ev_list;
}
