// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "include/vmrc_ht.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/vmrc_log.h"

/* Knuth's multiplicative hash. Given ptr, get the fractional part of (ptr * 2^64 * (golden_ratio-1)), where
 * (golden_ratio-1) is (sqrt(5)-1)/2. Then, get the highest VMRC_HT_BITS. */
static unsigned int knuth_hash_64(void *ptr) {
  uint64_t address = (uint64_t)ptr;
  uint64_t constant = 11400714819323198485ULL; /* floor(2^64 * (golden_ratio-1)). */
  return (address * constant) >> (64 - VMRC_HT_BITS);
}

/* Create a new hashtable. */
struct vmrc_ht *vmrc_ht_get() {
  static struct vmrc_ht *cache_ht = NULL;
  if (cache_ht != NULL) return cache_ht;

  cache_ht = (struct vmrc_ht *)calloc(1, sizeof(struct vmrc_ht));
  VMRC_CHECK_PRINT_EXIT(cache_ht, 1, "Could not allocate hashtable");
  return cache_ht;
}

/* Insert a key-value pair into the hashtable. */
void vmrc_ht_insert(struct vmrc_ht *hashtable, void *key, void *value) {
  unsigned int index = knuth_hash_64(key);
  struct vmrc_ht_entry *new_entry = calloc(1, sizeof(struct vmrc_ht_entry));
  VMRC_CHECK_PRINT_EXIT(new_entry, 1, "Could not allocate new entry for the hashtable");
  new_entry->key = key;
  new_entry->value = value;
  new_entry->next = hashtable->table[index];
  hashtable->table[index] = new_entry;
}

/* Delete a key-value pair from the hashtable. */
void vmrc_ht_delete(struct vmrc_ht *hashtable, void *key) {
  unsigned int index = knuth_hash_64(key);
  struct vmrc_ht_entry *entry = hashtable->table[index];
  struct vmrc_ht_entry *prev = NULL;
  while (entry != NULL) {
    if (entry->key == key) {
      if (prev == NULL) {
        hashtable->table[index] = entry->next;
      } else {
        prev->next = entry->next;
      }
      for (int iattr = 0; iattr < VMRC_HT_ATTR_SIZE; iattr++) {
        struct vmrc_ht_linked_list *attr = entry->attr[iattr];
        while (attr != NULL) {
          struct vmrc_ht_linked_list *temp_attr = attr;
          attr = (struct vmrc_ht_linked_list *)attr->ptr_and_next[VMRC_HT_LL_NEXT];
          free(temp_attr);
        }
      }
      free(entry);
      return;
    }
    prev = entry;
    entry = entry->next;
  }
}

/* Insert attr corresponding to a value. */
void vmrc_ht_attr_insert(void *addr_of_value /*&value*/, void *ptr /*qp_group, qp_hint*/,
                         int idx /* VMRC_HT_ATTR_QP_GROUP_IDX, VMRC_HT_ATTR_QP_HINT_IDX*/) {
  VMRC_CHECK_PRINT_EXIT((idx >= 0) && (idx < VMRC_HT_ATTR_SIZE), 1,
                        "idx should be an integer in the range [0,VMRC_HT_ATTR_SIZE)");

  struct vmrc_ht_linked_list *new_attr = calloc(1, sizeof(struct vmrc_ht_linked_list));
  VMRC_CHECK_PRINT_EXIT(new_attr, 1, "Could not allocate new attr");

  struct vmrc_ht_entry *entry = (struct vmrc_ht_entry *)addr_of_value; /* value is the first entry of entry */
  struct vmrc_ht_linked_list *attr = entry->attr[idx];

  new_attr->ptr_and_next[VMRC_HT_LL_PTR] = ptr;
  new_attr->ptr_and_next[VMRC_HT_LL_NEXT] = attr;
  entry->attr[idx] = new_attr;
}

/* Get attr. */
void *vmrc_ht_attr_get(void *addr_of_value, int idx) {
  VMRC_CHECK_PRINT_EXIT((idx >= 0) && (idx < VMRC_HT_ATTR_SIZE), 1,
                        "idx should be an integer in the range [0,VMRC_HT_ATTR_SIZE)");

  struct vmrc_ht_entry *entry = (struct vmrc_ht_entry *)addr_of_value; /* value is the first entry of entry */
  struct vmrc_ht_linked_list *attr = (struct vmrc_ht_linked_list *)entry->attr[idx];
  return (void *)attr;
}

/* Search for a value by key in the hashtable. */
void *vmrc_ht_search(struct vmrc_ht *hashtable, void *key) {
  unsigned int index = knuth_hash_64(key);
  struct vmrc_ht_entry *entry = hashtable->table[index];
  while (entry != NULL) {
    if (entry->key == key) {
      return entry->value;
    }
    entry = entry->next;
  }
  return NULL;
}

/* Search for a value by key in the hashtable. Also, return the address where the value is stored. */
void *vmrc_ht_search_plus_addr(struct vmrc_ht *hashtable, void *key, void **addr_of_value) {
  unsigned int index = knuth_hash_64(key);
  struct vmrc_ht_entry *entry = hashtable->table[index];
  while (entry != NULL) {
    if (entry->key == key) {
      *addr_of_value = &entry->value; /* Same as entry since value is the first entry. */
      return entry->value;
    }
    entry = entry->next;
  }
  return NULL;
}

/* Free the memory allocated for the hashtable. */
void vmrc_ht_free(struct vmrc_ht *hashtable) {
  for (int i = 0; i < VMRC_HT_SIZE; i++) {
    struct vmrc_ht_entry *entry = hashtable->table[i];
    /* Release the memory allocated for the linked list as well. Otherwise, there will be a memory leak. */
    while (entry != NULL) {
      for (int iattr = 0; iattr < VMRC_HT_ATTR_SIZE; iattr++) {
        struct vmrc_ht_linked_list *attr = entry->attr[iattr];
        while (attr != NULL) {
          struct vmrc_ht_linked_list *temp_attr = attr;
          attr = (struct vmrc_ht_linked_list *)attr->ptr_and_next[VMRC_HT_LL_NEXT];
          free(temp_attr);
        }
      }
      struct vmrc_ht_entry *temp = entry;
      entry = entry->next;
      free(temp);
    }
  }
  free(hashtable);
}
