CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

ABI_DIR ?= ../../../ABI
ABI_INCLUDE := $(ABI_DIR)/includes
CPPFLAGS ?= -Iincludes -I../source/includes -I../response/includes -I../lesson/includes -I../intent/includes -I../interpretation/includes -I$(ABI_INCLUDE)

TARGET := bin/dispatcher.so
SOURCE := src/dispatcher.c

DIGIT_PREFIX ?= /opt/digit
INSTALL_DIR := $(DIGIT_PREFIX)/modules/dispatcher
INSTALL_BIN := $(INSTALL_DIR)/bin

.PHONY: all clean check-abi rebuild install

all: check-abi $(TARGET)

check-abi:
	@test -f "$(ABI_INCLUDE)/module.h" || { echo "Missing external ABI: $(ABI_INCLUDE)/module.h"; exit 1; }

$(TARGET): $(SOURCE) includes/dispatcher.h ../source/includes/source.h ../response/includes/response.h ../lesson/includes/lesson.h ../intent/includes/intent.h ../interpretation/includes/interpretation.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(SOURCE) -o $@

bin:
	mkdir -p bin

install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root; run: sudo make install"; exit 1; }
	install -d -o digit -g digit $(INSTALL_DIR) $(INSTALL_BIN)
	install -m 0755 $(TARGET) $(INSTALL_BIN)/dispatcher.so
	install -m 0644 module.conf $(INSTALL_DIR)/module.conf
	chown -R digit:digit $(INSTALL_DIR)
	@echo "Dispatcher installed to $(INSTALL_DIR). Digit will qualify the candidate."

rebuild: clean all

clean:
	rm -rf bin
