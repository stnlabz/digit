CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
ABI_DIR ?= ../../../ABI
CPPFLAGS ?= -Iincludes -I../dispatcher/includes -I$(ABI_DIR)/includes
LDLIBS ?= -lcurl -ljson-c -pthread
TARGET := bin/slack.so
TEST_TARGET := bin/test_slack
PREFIX ?= /opt/digit
.PHONY: all test clean check-abi install
all: check-abi $(TARGET)
check-abi:
	@test -f "$(ABI_DIR)/includes/module.h" || { echo "Missing external ABI"; exit 1; }
bin:
	mkdir -p bin
$(TARGET): src/slack.c includes/slack.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $< -o $@ $(LDLIBS)
$(TEST_TARGET): tests/test_slack.c src/slack.c includes/slack.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDLIBS)
test: check-abi $(TEST_TARGET)
	./$(TEST_TARGET)
install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root"; exit 1; }
	install -d -o digit -g digit -m 0750 $(PREFIX)/auth/slack
	install -d -o digit -g digit -m 0755 $(PREFIX)/modules/slack/bin
	install -m 0755 $(TARGET) $(PREFIX)/modules/slack/bin/slack.so
	install -m 0644 module.conf $(PREFIX)/modules/slack/module.conf
	@if test ! -e $(PREFIX)/auth/slack/slack.conf; then install -o digit -g digit -m 0600 slack.conf.example $(PREFIX)/auth/slack/slack.conf; fi
clean:
	rm -rf bin
