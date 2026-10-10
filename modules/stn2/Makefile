CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
ABI_DIR ?= ../../../ABI
CPPFLAGS ?= -Iincludes -I$(ABI_DIR)/includes
LDLIBS ?= -lcurl -pthread
TARGET := bin/stn2.so
TEST_TARGET := bin/test_stn2
PREFIX ?= /opt/digit
.PHONY: all test clean install check-abi
all: check-abi $(TARGET)
check-abi:
	@test -f "$(ABI_DIR)/includes/module.h" || { echo "Missing external ABI"; exit 1; }
bin:
	mkdir -p bin
$(TARGET): src/stn2.c includes/stn2.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $< -o $@ $(LDLIBS)
$(TEST_TARGET): tests/test_stn2.c | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
test: check-abi $(TEST_TARGET)
	./$(TEST_TARGET)
install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root"; exit 1; }
	install -d -o digit -g digit -m 0750 $(PREFIX)/auth/stn2
	install -d -o digit -g digit $(PREFIX)/modules/stn2/bin
	install -m 0755 $(TARGET) $(PREFIX)/modules/stn2/bin/stn2.so
	install -m 0644 module.conf $(PREFIX)/modules/stn2/module.conf
	@if test ! -e $(PREFIX)/auth/stn2/stn2.conf; then install -o digit -g digit -m 0600 stn2.conf.example $(PREFIX)/auth/stn2/stn2.conf; fi
clean:
	rm -rf bin
