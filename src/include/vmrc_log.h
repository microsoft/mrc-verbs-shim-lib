// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef _VMRC_LOG_H_
#define _VMRC_LOG_H_

#ifndef VMRC_NOCHECK

/* Check if val is 0. If it is not 0, print error message and exit with an error code. */
#define VMRC_CHECK_PRINT_EXIT(val, errcode, msg) \
  do {                                           \
    if (!(val)) {                                \
      fprintf(stderr,                            \
              "mrc-verbs-shim-lib: error: " msg  \
              " (%s:%d)"                         \
              "\n",                              \
              __FILE__, __LINE__);               \
      exit(errcode);                             \
    }                                            \
  } while (0);

#define VMRC_CHECK_PRINT_EXIT_VA_ARGS(val, errcode, msg, ...) \
  do {                                                        \
    if (!(val)) {                                             \
      fprintf(stderr,                                         \
              "mrc-verbs-shim-lib: error: " msg               \
              " (%s:%d)"                                      \
              "\n",                                           \
              __VA_ARGS__, __FILE__, __LINE__);               \
      exit(errcode);                                          \
    }                                                         \
  } while (0);

#else /* #ifndef VMRC_NOCHECK */

#define VMRC_CHECK_PRINT_EXIT(val, errcode, msg)
#define VMRC_CHECK_PRINT_EXIT_VA_ARGS(val, errcode, msg, ...)

#endif /* #ifndef VMRC_NOCHECK */

/* Debug prints. */
#ifdef VMRC_DEBUG

#define VMRC_DEBUG_PRINT(msg) fprintf(stderr, "mrc-verbs-shim-lib: debug: " msg "\n");
#define VMRC_DEBUG_PRINT_VA_ARGS(msg, ...) fprintf(stderr, "mrc-verbs-shim-lib: debug: " msg "\n", __VA_ARGS__);

#else /* #ifdef VMRC_DEBUG */

#define VMRC_DEBUG_PRINT(msg)
#define VMRC_DEBUG_PRINT_VA_ARGS(msg, ...)

#endif /* #ifdef VMRC_DEBUG */

#define VMRC_INFO_PRINT(msg) fprintf(stderr, "mrc-verbs-shim-lib: info: " msg "\n");
#define VMRC_INFO_PRINT_VA_ARGS(msg, ...) fprintf(stderr, "mrc-verbs-shim-lib: info: " msg "\n", __VA_ARGS__);

#endif /* #ifndef _VMRC_LOG_H_ */
