CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

ABI_DIR ?= ../../../ABI
ABI_INCLUDE := $(ABI_DIR)/includes
CPPFLAGS ?= -Iincludes -I../corpus/includes -I$(ABI_INCLUDE)

TARGET := bin/interpretation.so
SOURCE := src/interpretation.c

DIGIT_PREFIX ?= /opt/digit
INSTALL_DIR := $(DIGIT_PREFIX)/modules/interpretation
INSTALL_BIN := $(INSTALL_DIR)/bin

.PHONY: all clean check-abi rebuild install test

all: check-abi $(TARGET)

check-abi:
	@test -f "$(ABI_INCLUDE)/module.h" || { echo "Missing external ABI: $(ABI_INCLUDE)/module.h"; exit 1; }

$(TARGET): $(SOURCE) includes/interpretation.h ../corpus/includes/corpus.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(SOURCE) -o $@

bin:
	mkdir -p bin

bin/test_interpretation: tests/test_interpretation.c $(SOURCE) includes/interpretation.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_interpretation.c $(SOURCE) -o $@

test: check-abi bin/test_interpretation
	./bin/test_interpretation

install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root; run: sudo make install"; exit 1; }
	install -d -o digit -g digit $(INSTALL_DIR) $(INSTALL_BIN)
	install -m 0755 $(TARGET) $(INSTALL_BIN)/interpretation.so
	install -m 0644 module.conf $(INSTALL_DIR)/module.conf
	chown -R digit:digit $(INSTALL_DIR)
	@echo "Interpretation installed to $(INSTALL_DIR). Digit will qualify the candidate."

rebuild: clean all

clean:
	rm -rf bin
