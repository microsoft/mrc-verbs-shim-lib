CC = gcc
CFLAGS += -fPIC
DEBUG ?= 0
MKFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
MRC_H_PATH ?= $(MKFILE_DIR)/mrc-header-lib # Pass MRC_H_PATH

$(info MRC_H_PATH set to $(MRC_H_PATH))

CFLAGS := -I$(MRC_H_PATH) $(CFLAGS)

ifneq ($(DEBUG),0)
CFLAGS += -g -DVMRC_DEBUG # No indentation is crucial here.
endif

SRCS=src/vmrc_symbols.c \
	src/vmrc_ibv_overwrites.c \
	src/vmrc_ht.c \
	src/cJSON.c \
	src/vmrc_json.c

HEADERS=src/include/vmrc_symbols.h \
	src/include/vmrc_ht.h \
	src/include/vmrc_log.h \
	src/include/cJSON.h \
	src/include/vmrc_json.h

OBJECTS=$(SRCS:.c=.o)

OBJECTS_INTERNAL=$(SRCS:.c=_internal.o)

TARGETS=libibverbs.so libibverbs_internal.so

all: $(TARGETS)

libibverbs.so: $(OBJECTS)
	$(CC) -fPIC -shared -o $@ $^ $(LDFLAGS) -Wl,--version-script=version_script.map

libibverbs_internal.so: $(OBJECTS_INTERNAL)
	$(CC) -fPIC -shared -o $@ $^ $(LDFLAGS) -Wl,--version-script=version_script.map

$(OBJECTS): %.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -fvisibility=hidden $< -o $@

$(OBJECTS_INTERNAL): %_internal.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

# TESTS_INTERNAL are to check verbs_mrc library and other auxiliary tests.

TESTS_INTERNAL=tests/check_vmrc_symbols.c \
	       tests/check_vmrc_ht.c \
	       tests/check_cjson.c \
	       tests/check_parse_system_json.c

TESTS_INTERNAL_OBJ=$(TESTS_INTERNAL:.c=)

tests_internal: $(TESTS_INTERNAL_OBJ)

$(TESTS_INTERNAL_OBJ): %: %.c libibverbs_internal.so
	$(CC) $(CFLAGS) $< -o $@ -L$(MKFILE_DIR) -libverbs_internal $(LDFLAGS)

# TESTS are to check libverbs_mrc.so with verbs calls.

TESTS=tests/check_ibv_overwrites.c \
      tests/check_pd_context.c \
      tests/check_ibv_overwrites_dlopen.c \
      tests/check_extract_ipv6_from_gid.c

TESTS_OBJ=$(TESTS:.c=)

tests: $(TESTS_OBJ)

$(TESTS_OBJ): %: %.c libibverbs.so
	$(CC) $(CFLAGS) $< -o $@ -libverbs $(LDFLAGS)

# Formatting (taken from Yang's addition to MRPScrub).

FORMAT_SOURCES=$(SRCS) $(HEADERS) $(TESTS_INTERNAL) $(TESTS)

.PHONY: format
format:
	clang-format --verbose -i $(FORMAT_SOURCES)


.PHONY: lint
lint:
	clang-format --verbose --dry-run $(FORMAT_SOURCES)

.PHONY: clean
clean:
	rm -f $(TARGETS)
	rm -f $(OBJECTS) $(OBJECTS_INTERNAL)
	rm -f $(TESTS_INTERNAL_OBJ)
	rm -f $(TESTS_OBJ)

