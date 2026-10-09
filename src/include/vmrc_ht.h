// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/* We need a hashtable to map ibv_context ptrs to mrc_context ptrs. Both ibv_context and mrc_context need to be alive
 * throughout the duration of the program. */

#ifndef _VMRC_HT_H_
#define _VMRC_HT_H_

#define VMRC_HT_ATTR_QP_HINT_IDX 0
#define VMRC_HT_ATTR_ORIG_CREATE_QP_EX_IDX 1

#define VMRC_HT_LL_PTR 0
#define VMRC_HT_LL_NEXT 1

/* Hash table size should be 2^bits. */
#define VMRC_HT_BITS 7
#define VMRC_HT_SIZE 128
#define VMRC_HT_ATTR_SIZE 2

struct vmrc_ht_linked_list {
  void *ptr_and_next[2];
};

/* Hashtable entry. */
struct vmrc_ht_entry {
  void *value;                                         /* mrc_context ptr. */
  void *key;                                           /* ibv_context ptr. */
  struct vmrc_ht_linked_list *attr[VMRC_HT_ATTR_SIZE]; /* To store the QP hints allocated for this MRC context. */
  struct vmrc_ht_entry *next;
};

/* Define the structure for the hashtable. */
struct vmrc_ht {
  struct vmrc_ht_entry *table[VMRC_HT_SIZE];
};

struct vmrc_ht *vmrc_ht_get();
void vmrc_ht_insert(struct vmrc_ht *hashtable, void *key, void *value);
void vmrc_ht_delete(struct vmrc_ht *hashtable, void *key);
void vmrc_ht_attr_insert(void *addr_of_value, void *ptr, int idx);
void *vmrc_ht_attr_get(void *addr_of_value, int idx);
void *vmrc_ht_search(struct vmrc_ht *hashtable, void *key);
void *vmrc_ht_search_plus_addr(struct vmrc_ht *hashtable, void *key, void **addr_of_value);
void vmrc_ht_free(struct vmrc_ht *hashtable);

#endif /* _VMRC_HT_H_ */
