#include "include/vmrc_ht.h"
#include "include/vmrc_log.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Hash table size should be 2^bits. */
#define VMRC_HT_BITS 7
#define VMRC_HT_SIZE 128

/* Hashtable entry. */
struct vmrc_ht_entry {
  void *key;   /* ibv_context ptr. */
  void *value; /* mrc_context ptr. */
  struct vmrc_ht_entry *next;
};

/* Define the structure for the hashtable. */
struct vmrc_ht {
  struct vmrc_ht_entry *table[VMRC_HT_SIZE];
};

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

/* Free the memory allocated for the hashtable. */
void vmrc_ht_free(struct vmrc_ht *hashtable) {
  for (int i = 0; i < VMRC_HT_SIZE; i++) {
    struct vmrc_ht_entry *entry = hashtable->table[i];
    while (entry != NULL) {
      struct vmrc_ht_entry *temp = entry;
      entry = entry->next;
      free(temp);
    }
  }
  free(hashtable);
}
