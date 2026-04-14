CC = gcc
CFLAGS += -fPIC
BUILDDIR ?= $(PWD)/build

# Check MRC_H_PATH only if target is not format, lint, or clean
ifneq ($(filter-out format lint clean install,$(MAKECMDGOALS)),)
ifndef MRC_H_PATH
$(error MRC_H_PATH is not defined. Please set it to the path of the header path of the MRC library.)
else
$(info MRC_H_PATH set to $(MRC_H_PATH))
endif
endif



INCDIR := $(BUILDDIR)/include
OBJDIR := $(BUILDDIR)/obj
BINDIR := $(BUILDDIR)/bin
LIBDIR := $(BUILDDIR)/lib


CFLAGS += -I$(MRC_H_PATH) -g

SRCS := src/vmrc_symbols.c \
	src/vmrc_ibv_overwrites.c \
	src/vmrc_ht.c

HEADERS := src/include/vmrc_symbols.h \
	src/include/vmrc_ht.h \
	src/include/vmrc_log.h \
	src/include/vmrc_version.h

OBJECTS := $(patsubst src/%.c, $(OBJDIR)/%.o, $(SRCS))
DEBUG_OBJECTS := $(patsubst src/%.c, $(OBJDIR)/%_debug.o, $(SRCS))

TARGETS := $(LIBDIR)/libibverbs.so $(LIBDIR)/debug/libibverbs_debug.so

all: build tests

build: $(TARGETS)

$(LIBDIR)/libibverbs.so: $(OBJECTS)
	@printf "Linking %s\n" $@
	@mkdir -p `dirname $@`
	$(CC) -fPIC -shared -o $@ $^ $(LDFLAGS) -Wl,--version-script=version_script.map

$(LIBDIR)/debug/libibverbs_debug.so: $(DEBUG_OBJECTS)
	@printf "Linking %s\n" $@
	@mkdir -p `dirname $@`
	$(CC) -fPIC -shared -o $@ $^ $(LDFLAGS) -Wl,--version-script=version_script.map

$(OBJECTS): $(OBJDIR)/%.o: src/%.c $(HEADERS)
	@printf "Compiling %-35s > %s\n" $< $@
	@mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) -fvisibility=hidden $< -o $@

$(DEBUG_OBJECTS): $(OBJDIR)/%_debug.o: src/%.c $(HEADERS)
	@printf "Compiling debug %-35s > %s\n" $< $@
	@mkdir -p `dirname $@`
	$(CC) -c $(CFLAGS) -DVMRC_DEBUG -fvisibility=hidden $< -o $@

TESTS=tests/check_sanity.c

TESTS_OBJ=$(TESTS:.c=)

tests: $(TESTS_OBJ)

$(TESTS_OBJ): %: %.c build
	$(CC) $(CFLAGS) $< -o $@ -libverbs $(LDFLAGS)

install: build
	mkdir -p $(PREFIX)/lib
	cp $(LIBDIR)/libibverbs.so $(PREFIX)/lib/

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
	rm -rf $(OBJDIR) $(LIBDIR) $(BINDIR) $(INCDIR) $(TESTS_OBJ)