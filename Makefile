CC = gcc
UNAME_S := $(shell uname -s)

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
LIB_DIR = lib
TEST_DIR = tests

PKG_CONFIG ?= pkg-config
PKG_NAME = poppler-glib

# Homebrew does not put .pc files on the default pkg-config path.
ifeq ($(UNAME_S),Darwin)
	BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
	ifneq ($(BREW_PREFIX),)
		export PKG_CONFIG_PATH := $(BREW_PREFIX)/lib/pkgconfig:$(PKG_CONFIG_PATH)
	endif
endif

ifeq ($(shell $(PKG_CONFIG) --exists $(PKG_NAME) && echo yes),)
$(error $(PKG_NAME) not found via pkg-config. See README.md for Fedora/macOS install steps)
endif

PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKG_NAME))
PKG_LIBS := $(shell $(PKG_CONFIG) --libs $(PKG_NAME))

CFLAGS = -Wall -Wextra -fPIC -O2 -g -I$(INC_DIR) $(PKG_CFLAGS)
LDLIBS = $(PKG_LIBS) -pthread

ifeq ($(UNAME_S),Darwin)
	SHLIB_EXT = dylib
	SHLIB_FLAGS = -dynamiclib
	POPPLER_LIBDIR := $(shell $(PKG_CONFIG) --variable=libdir $(PKG_NAME))
	GLIB_LIBDIR := $(shell $(PKG_CONFIG) --variable=libdir glib-2.0)
	RPATH_FLAGS = -Wl,-rpath,$(POPPLER_LIBDIR)
	ifneq ($(GLIB_LIBDIR),$(POPPLER_LIBDIR))
		RPATH_FLAGS += -Wl,-rpath,$(GLIB_LIBDIR)
	endif
else
	SHLIB_EXT = so
	SHLIB_FLAGS = -shared
	RPATH_FLAGS =
endif

LDFLAGS = $(SHLIB_FLAGS) $(LDLIBS) $(RPATH_FLAGS)

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

TARGET = $(LIB_DIR)/libengine.$(SHLIB_EXT)
TEST_BIN = $(BUILD_DIR)/test_roundtrip

.PHONY: all test test-python clean compdb

all: $(TARGET) compdb

$(TARGET): $(OBJS)
	@mkdir -p $(LIB_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(OBJS)
	@mkdir -p $(TEST_DIR)/test_data
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(TEST_DIR)/test_roundtrip.c $(OBJS) $(LDLIBS)
	$(TEST_BIN)

test-python: $(TARGET)
	python3 $(TEST_DIR)/test_integration.py

# Keep clangd/IDE includes in sync with this machine's pkg-config paths
compdb:
	@python3 -c "\
import json, os, glob, shlex, subprocess;\
cflags = shlex.split(subprocess.check_output(['$(PKG_CONFIG)', '--cflags', '$(PKG_NAME)'], text=True));\
root = os.getcwd();\
entries = [];\
[entries.append({'directory': root, 'file': src, 'arguments': ['$(CC)', '-Wall', '-Wextra', '-fPIC', '-O2', '-g', '-I$(INC_DIR)'] + cflags + ['-c', src, '-o', '$(BUILD_DIR)/' + os.path.splitext(os.path.basename(src))[0] + '.o'], 'output': '$(BUILD_DIR)/' + os.path.splitext(os.path.basename(src))[0] + '.o'}) for src in sorted(glob.glob('$(SRC_DIR)/*.c'))];\
open('compile_commands.json', 'w').write(json.dumps(entries, indent=2) + '\n')"

clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR) test_roundtrip $(TEST_DIR)/test_data/*.db
