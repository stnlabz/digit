CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Iincludes -I../../../ABI/includes
PREFIX ?= /opt/digit
all: bin/memory.so
bin:
	mkdir -p bin
bin/memory.so: src/memory.c includes/memory.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared src/memory.c -o $@
bin/test_memory: tests/test_memory.c src/memory.c includes/memory.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_memory.c src/memory.c -o $@
test: bin/test_memory
	./bin/test_memory
install: all
	@test "$$(id -u)" -eq 0 || { echo "root required"; exit 1; }
	install -d -o digit -g digit -m 0700 $(PREFIX)/state/memory
	install -d -o digit -g digit -m 0755 $(PREFIX)/modules/memory/bin
	install -m 0755 bin/memory.so $(PREFIX)/modules/memory/bin/memory.so
	install -m 0644 module.conf $(PREFIX)/modules/memory/module.conf
clean:
	rm -rf bin
.PHONY: all test clean install
