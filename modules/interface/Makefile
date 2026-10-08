CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

ABI_DIR ?= ../../../ABI
ABI_INCLUDE := $(ABI_DIR)/includes
DIGIT_INCLUDE := ../../include
CPPFLAGS ?= -Iincludes -I$(DIGIT_INCLUDE) -I$(ABI_INCLUDE)
LDLIBS ?= -pthread

TARGET := bin/interface.so
TEST_TARGET := bin/test_interface
ACCESS_TEST_TARGET := bin/test_access_policy
SESSION_TEST_TARGET := bin/test_session_store
HTTP_AUTH_TEST_TARGET := bin/test_http_auth
SOURCE := src/interface.c src/session_store.c src/http_auth.c
TEST_SOURCE := tests/test_interface.c

DIGIT_PREFIX ?= /opt/digit
INSTALL_DIR := $(DIGIT_PREFIX)/modules/interface
INSTALL_BIN := $(INSTALL_DIR)/bin

.PHONY: all test clean check-abi rebuild install

all: check-abi $(TARGET)

check-abi:
	@test -f "$(ABI_INCLUDE)/module.h" || { echo "Missing external ABI: $(ABI_INCLUDE)/module.h"; exit 1; }
	@test -f "$(DIGIT_INCLUDE)/core_services.h" || { echo "Missing Digit Core service contract: $(DIGIT_INCLUDE)/core_services.h"; exit 1; }

$(TARGET): $(SOURCE) includes/interface.h includes/session_store.h $(DIGIT_INCLUDE)/core_services.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(SOURCE) -o $@ $(LDLIBS)

$(TEST_TARGET): $(TEST_SOURCE) $(SOURCE) includes/interface.h includes/session_store.h $(DIGIT_INCLUDE)/core_services.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SOURCE) $(SOURCE) -o $@ $(LDLIBS)

$(ACCESS_TEST_TARGET): tests/test_access_policy.c includes/access_policy.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_access_policy.c -o $@

$(SESSION_TEST_TARGET): tests/test_session_store.c src/session_store.c includes/session_store.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_session_store.c src/session_store.c -o $@

$(HTTP_AUTH_TEST_TARGET): tests/test_http_auth.c src/http_auth.c src/session_store.c includes/http_auth.h includes/session_store.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_http_auth.c src/http_auth.c src/session_store.c -o $@

test: check-abi $(TEST_TARGET) $(ACCESS_TEST_TARGET) $(SESSION_TEST_TARGET) $(HTTP_AUTH_TEST_TARGET)
	./$(TEST_TARGET)
	./$(ACCESS_TEST_TARGET)
	./$(SESSION_TEST_TARGET)
	./$(HTTP_AUTH_TEST_TARGET)

install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root; run: sudo make install"; exit 1; }
	install -d -o digit -g digit $(INSTALL_DIR) $(INSTALL_BIN)
	install -m 0755 $(TARGET) $(INSTALL_BIN)/interface.so
	install -m 0644 module.conf $(INSTALL_DIR)/module.conf
	install -m 0644 interface.conf $(INSTALL_DIR)/interface.conf
	chown -R digit:digit $(INSTALL_DIR)
	@echo "Interface installed to $(INSTALL_DIR). Digit will qualify the candidate."

bin:
	mkdir -p bin

rebuild: clean all

clean:
	rm -rf bin
