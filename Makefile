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

# Enable the ionic (ionic_dv_*) provider interceptors with `make IONIC=1`.
# This defines HAVE_IONIC_DV for both the C sources and the version script
# (via CPPDEFS below), gating the IONIC_* symbol exports. It also defines
# HAVE_MRC_DEVICE_ATTR, since the ionic-capable libmrc uses the newer
# struct mrc_device_attr (older libmrc uses struct mrc_attr).
ifeq ($(IONIC),1)
CFLAGS += -DHAVE_IONIC_DV -DHAVE_MRC_DEVICE_ATTR
endif

SRCS := src/vmrc_symbols.c \
	src/vmrc_ibv_overwrites.c \
	src/vmrc_ht.c

HEADERS := src/include/vmrc_symbols.h \
	src/include/vmrc_ht.h \
	src/include/vmrc_log.h \
	src/include/vmrc_version.h

OBJECTS := $(patsubst src/%.c, $(OBJDIR)/%.o, $(SRCS))
DEBUG_OBJECTS := $(patsubst src/%.c, $(OBJDIR)/%_debug.o, $(SRCS))

# The version script uses #ifdef HAVE_IONIC_DV. ld treats '#' as a comment and
# does NOT preprocess it, so run it through cpp with the same -D defines used for
# the C sources. This gates the IONIC_* version nodes on HAVE_IONIC_DV.
CPPDEFS := $(filter -D%,$(CFLAGS))
# Generated under OBJDIR so `make clean` removes it (avoids a stale script being
# reused when the -D defines change between builds).
VERSION_SCRIPT := $(OBJDIR)/version_script.map

TARGETS := $(LIBDIR)/libibverbs.so $(LIBDIR)/debug/libibverbs_debug.so

all: build tests

build: $(TARGETS)

$(VERSION_SCRIPT): version_script.map
	@mkdir -p `dirname $@`
	$(CC) -E -P -x c $(CPPDEFS) $< -o $@

$(LIBDIR)/libibverbs.so: $(OBJECTS) $(VERSION_SCRIPT)
	@printf "Linking %s\n" $@
	@mkdir -p `dirname $@`
	$(CC) -fPIC -shared -o $@ $(OBJECTS) $(LDFLAGS) -Wl,--version-script=$(VERSION_SCRIPT)

$(LIBDIR)/debug/libibverbs_debug.so: $(DEBUG_OBJECTS) $(VERSION_SCRIPT)
	@printf "Linking %s\n" $@
	@mkdir -p `dirname $@`
	$(CC) -fPIC -shared -o $@ $(DEBUG_OBJECTS) $(LDFLAGS) -Wl,--version-script=$(VERSION_SCRIPT)

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