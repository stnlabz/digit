CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
ABI_DIR ?= ../../../ABI
CPPFLAGS ?= -Iincludes -I$(ABI_DIR)/includes
TARGET := bin/arithmetic.so
TEST_TARGET := bin/test_arithmetic
SOURCE := src/arithmetic.c
TEST_SOURCE := tests/test_arithmetic.c
DIGIT_PREFIX ?= /opt/digit
INSTALL_DIR := $(DIGIT_PREFIX)/modules/arithmetic
.PHONY: all test clean check-abi install
all: check-abi $(TARGET)
check-abi:
	@test -f "$(ABI_DIR)/includes/module.h" || { echo "Missing external ABI"; exit 1; }
bin:
	mkdir -p bin
$(TARGET): $(SOURCE) includes/arithmetic.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(SOURCE) -o $@
$(TEST_TARGET): $(TEST_SOURCE) $(SOURCE) includes/arithmetic.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SOURCE) $(SOURCE) -o $@
test: check-abi $(TEST_TARGET)
	./$(TEST_TARGET)
install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root"; exit 1; }
	install -d -o digit -g digit $(INSTALL_DIR) $(INSTALL_DIR)/bin
	install -m 0755 $(TARGET) $(INSTALL_DIR)/bin/arithmetic.so
	install -m 0644 module.conf $(INSTALL_DIR)/module.conf
	chown -R digit:digit $(INSTALL_DIR)
clean:
	rm -rf bin
