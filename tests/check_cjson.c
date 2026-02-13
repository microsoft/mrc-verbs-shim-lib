// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <stdio.h>

#include "../src/include/cJSON.h"

const char *json_data =
    "{\"blore\": {\"hassan\": [{\"plane\": 0, \"route_list\": [4, 9, 14, 19]}], \"mysore\": [{\"plane\": 0, "
    "\"ps_list\": [5, 10, 15, 20]}]}}";

void parse_json(const char *json_data) {
  cJSON *json = cJSON_Parse(json_data);
  if (json == NULL) {
    printf("Error parsing JSON\n");
    return;
  }

  cJSON *blore = cJSON_GetObjectItem(json, "blore");
  if (blore == NULL) {
    printf("Error finding 'blore' object\n");
    cJSON_Delete(json);
    return;
  }

  // Extract and print ps_list between blore and mysore
  cJSON *mysore = cJSON_GetObjectItem(blore, "mysore");
  if (mysore == NULL) {
    printf("Error finding 'mysore' object\n");
    cJSON_Delete(json);
    return;
  }

  cJSON *mysore_item = cJSON_GetArrayItem(mysore, 0);
  if (mysore_item == NULL) {
    printf("Error finding 'mysore' array item\n");
    cJSON_Delete(json);
    return;
  }

  cJSON *ps_list = cJSON_GetObjectItem(mysore_item, "ps_list");
  if (ps_list == NULL) {
    printf("Error finding 'ps_list'\n");
    cJSON_Delete(json);
    return;
  }

  int ps_list_size = cJSON_GetArraySize(ps_list);
  printf("ps_list between blore and mysore: ");
  for (int i = 0; i < ps_list_size; i++) {
    printf("%d ", cJSON_GetArrayItem(ps_list, i)->valueint);
  }
  printf("\n");

  // Extract and print route_list between blore and hassan
  cJSON *hassan = cJSON_GetObjectItem(blore, "hassan");
  if (hassan == NULL) {
    printf("Error finding 'hassan' object\n");
    cJSON_Delete(json);
    return;
  }

  cJSON *hassan_item = cJSON_GetArrayItem(hassan, 0);
  if (hassan_item == NULL) {
    printf("Error finding 'hassan' array item\n");
    cJSON_Delete(json);
    return;
  }

  cJSON *route_list = cJSON_GetObjectItem(hassan_item, "route_list");
  if (route_list == NULL) {
    printf("Error finding 'route_list'\n");
    cJSON_Delete(json);
    return;
  }

  int route_list_size = cJSON_GetArraySize(route_list);
  printf("route_list between blore and hassan: ");
  for (int i = 0; i < route_list_size; i++) {
    printf("%d ", cJSON_GetArrayItem(route_list, i)->valueint);
  }
  printf("\n");

  cJSON_Delete(json);
}

int main() {
  parse_json(json_data);
  return 0;
}
