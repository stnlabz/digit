CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

ABI_DIR ?= ../ABI
ABI_INCLUDE := $(ABI_DIR)/includes
ABI_SRC := $(ABI_DIR)/src

CPPFLAGS ?= -Iinclude -I$(ABI_INCLUDE)
LDLIBS ?= -ldl

TARGET := build/digit
MODULE_DIR := build/modules

DIGIT_SOURCES := \
	src/main.c \
	src/digit.c

ABI_SOURCES := \
	$(ABI_SRC)/abi.c \
	$(ABI_SRC)/module.c \
	$(ABI_SRC)/module_registry.c \
	$(ABI_SRC)/loader_linux.c \
	$(ABI_SRC)/module_discovery_linux.c

DIGIT_OBJECTS := $(DIGIT_SOURCES:src/%.c=build/digit_%.o)
ABI_OBJECTS := $(ABI_SOURCES:$(ABI_SRC)/%.c=build/abi_%.o)
OBJECTS := $(DIGIT_OBJECTS) $(ABI_OBJECTS)

.PHONY: all clean check-abi

all: check-abi $(TARGET) $(MODULE_DIR)

check-abi:
	@test -f "$(ABI_INCLUDE)/abi.h" || { echo "Missing external ABI: $(ABI_INCLUDE)/abi.h"; exit 1; }
	@test -f "$(ABI_SRC)/loader_linux.c" || { echo "Missing Linux ABI loader: $(ABI_SRC)/loader_linux.c"; exit 1; }
	@test -f "$(ABI_SRC)/module_discovery_linux.c" || { echo "Missing Linux ABI discovery: $(ABI_SRC)/module_discovery_linux.c"; exit 1; }

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDLIBS)

build/digit_%.o: src/%.c include/digit.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/abi_%.o: $(ABI_SRC)/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

$(MODULE_DIR): | build
	mkdir -p $(MODULE_DIR)

clean:
	rm -rf build
