#include <stdio.h>

#include "../src/include/vmrc_ht.h"

int main() {
  /* Create a hashtable. */
  struct vmrc_ht *hashtable = vmrc_ht_get();

  /* Example pointers to use as keys and values. */
  int key1 = 1, value1 = 100;
  int key2 = 2, value2 = 200;

  /* Insert key-value pairs into the hashtable. */
  vmrc_ht_insert(hashtable, &key1, &value1);
  vmrc_ht_insert(hashtable, &key2, &value2);

  /* Search for values by keys. */
  int *result1 = (int *)vmrc_ht_search(hashtable, &key1);
  int *result2 = (int *)vmrc_ht_search(hashtable, &key2);

  /* Print the results. */
  if (result1) {
    printf("Value for key1: %d\n", *result1);
  } else {
    printf("Key1 not found\n");
  }

  if (result2) {
    printf("Value for key2: %d\n", *result2);
  } else {
    printf("Key2 not found\n");
  }

  /* Free the hashtable. */
  vmrc_ht_free(hashtable);

  return 0;
}
