CC=gcc
LDFLAGS=-libverbs

SRCS=src/vmrc_symbols.c

HEADERS_IN=src/include/vmrc_symbols.h.in

HEADERS=src/include/vmrc_symbols.h

OBJECTS=$(SRCS:.c=.o)

TARGETS=libverbs_mrc.so

all: $(TARGETS)

libverbs_mrc.so: $(OBJECTS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(OBJECTS): %.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

src/include/vmrc_symbols.h: src/include/vmrc_symbols.h.in
	$(CC) -E $< > $@ 

TESTS=tests/check_vmrc_symbols.c

TESTS_OBJ=$(TESTS:.c=)

tests: $(TESTS_OBJ)

$(TESTS_OBJ): %: %.c libverbs_mrc.so
	$(CC) $(CFLAGS) $< -o $@ -L$(PWD) -lverbs_mrc $(LDFLAGS)
