/* We need a hashtable to map ibv_context ptrs to mrc_context ptrs. Both ibv_context and mrc_context need to be alive
 * throughout the duration of the program. */

#ifndef _VMRC_HT_H_
#define _VMRC_HT_H_

struct vmrc_ht;

struct vmrc_ht *vmrc_ht_get();
void vmrc_ht_insert(struct vmrc_ht *hashtable, void *key, void *value);
void *vmrc_ht_search(struct vmrc_ht *hashtable, void *key);
void vmrc_ht_free(struct vmrc_ht *hashtable);

#endif /* _VMRC_HT_H_ */
