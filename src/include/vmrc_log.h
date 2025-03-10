#ifndef _VMRC_LOG_H_
#define _VMRC_LOG_H_

/* Check if val is 0. If it is not 0, print error message and exit with an error code. */

#ifndef VMRC_NOCHECK

#define VMRC_CHECK_PRINT_EXIT(val, errcode, msg)      \
  do {                                                \
    if (!val) {                                       \
      fprintf(stderr, "verbs-mrc: error: " msg "\n"); \
      exit(errcode);                                  \
    }                                                 \
  } while (0);

#define VMRC_CHECK_PRINT_EXIT_VA_ARGS(val, errcode, msg, ...)      \
  do {                                                             \
    if (!val) {                                                    \
      fprintf(stderr, "verbs-mrc: error: " msg "\n", __VA_ARGS__); \
      exit(errcode);                                               \
    }                                                              \
  } while (0);

#else /* #ifndef VMRC_NOCHECK */

#define VMRC_CHECK_PRINT_EXIT(val, errcode, msg)
#define VMRC_CHECK_PRINT_EXIT_VA_ARGS(val, errcode, msg, ...)

#endif

/* Debug prints. */
#ifdef VMRC_DEBUG

#define VMRC_DEBUG_PRINT(msg) fprintf(stderr, "verbs-mrc: print: " msg "\n");
#define VMRC_DEBUG_PRINT_VA_ARGS(msg, ...) fprintf(stderr, "verbs-mrc: print: " msg "\n", __VA_ARGS__);

#else /* #ifdef VMRC_DEBUG */

#define VMRC_DEBUG_PRINT(msg)
#define VMRC_DEBUG_PRINT_VA_ARGS(msg, ...)

#endif /* #ifdef VMRC_DEBUG */

#endif /* #ifndef _VMRC_LOG_H_ */
