#ifndef _VMRC_JSON_H_
#define _VMRC_JSON_H_

#include <stdint.h>

char *vmrc_json_read(const char *system_json_fname);
uint32_t *vmrc_json_get_ev_list(char *my_ipv6_str, char *rem_ipv6_str, int *num_evs);

#endif /* _VMRC_JSON_H_ */
