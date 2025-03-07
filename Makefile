CC=gcc
CFLAGS=-fPIC
LDFLAGS=

ifdef DEBUG
	CFLAGS += -DVERBS_MRC_DEBUG
endif

SRCS=src/vmrc_symbols.c \
	src/vmrc_ibv_overwrites.c \
	src/vmrc_ht.c

HEADERS=src/include/vmrc_symbols.h \
	src/include/vmrc_ht.h

OBJECTS=$(SRCS:.c=.o)

TARGETS=libverbs_mrc.so

all: $(TARGETS)

libverbs_mrc.so: $(OBJECTS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(OBJECTS): %.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

# TESTS_INTERNAL are to check verbs_mrc library and other auxiliary tests.

TESTS_INTERNAL=tests/check_vmrc_symbols.c \
	       tests/check_vmrc_ht.c

TESTS_INTERNAL_OBJ=$(TESTS_INTERNAL:.c=)

tests_internal: $(TESTS_INTERNAL_OBJ)

$(TESTS_INTERNAL_OBJ): %: %.c libverbs_mrc.so
	$(CC) $(CFLAGS) $< -o $@ -L$(PWD) -lverbs_mrc $(LDFLAGS)

# TESTS are to check libverbs_mrc.so with verbs calls.

TESTS=tests/check_ibv_device_overwrites.c \
      tests/check_pd_context.c

TESTS_OBJ=$(TESTS:.c=)

tests: $(TESTS_OBJ)

$(TESTS_OBJ): %: %.c libverbs_mrc.so
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
	rm -f libverbs_mrc.so
	rm -f $(OBJECTS)
	rm -f $(TESTS_INTERNAL_OBJ)
	rm -f $(TESTS_OBJ)

