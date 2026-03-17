CC = gcc
CFLAGS += -fPIC
DEBUG ?= 0
MKFILE_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Check MRC_H_PATH only if target is not format, lint, or clean
ifndef MRC_H_PATH
ifneq ($(filter-out format lint clean,$(MAKECMDGOALS)),)
$(error MRC_H_PATH is not defined. Please set it to the path of the header path of the MRC library.)
endif
endif

$(info MRC_H_PATH set to $(MRC_H_PATH))

CFLAGS := -I$(MRC_H_PATH) $(CFLAGS)

ifneq ($(DEBUG),0)
CFLAGS += -g -DVMRC_DEBUG # No indentation is crucial here.
endif

SRCS=src/vmrc_symbols.c \
	src/vmrc_ibv_overwrites.c \
	src/vmrc_ht.c

HEADERS=src/include/vmrc_symbols.h \
	src/include/vmrc_ht.h \
	src/include/vmrc_log.h

OBJECTS=$(SRCS:.c=.o)

TARGETS=libibverbs.so

all: $(TARGETS)

libibverbs.so: $(OBJECTS)
	$(CC) -fPIC -shared -o $@ $^ $(LDFLAGS) -Wl,--version-script=version_script.map

$(OBJECTS): %.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) -fvisibility=hidden $< -o $@

TESTS=tests/check_sanity.c

TESTS_OBJ=$(TESTS:.c=)

tests: $(TESTS_OBJ)

$(TESTS_OBJ): %: %.c libibverbs.so
	$(CC) $(CFLAGS) $< -o $@ -libverbs $(LDFLAGS)

# Formatting.

FORMAT_SOURCES=$(SRCS) $(HEADERS) $(TESTS)

.PHONY: format
format:
	clang-format --verbose -i $(FORMAT_SOURCES)

.PHONY: lint
lint:
	clang-format --verbose --dry-run $(FORMAT_SOURCES)

.PHONY: clean
clean:
	rm -f $(TARGETS)
	rm -f $(OBJECTS)
	rm -f $(TESTS_OBJ)

