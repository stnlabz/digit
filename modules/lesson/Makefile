CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

ABI_DIR ?= ../../../ABI
ABI_INCLUDE := $(ABI_DIR)/includes
CPPFLAGS ?= -Iincludes -I../corpus_builder/includes -I$(ABI_INCLUDE)

TARGET := bin/lesson.so
SOURCE := src/lesson.c

DIGIT_PREFIX ?= /opt/digit
INSTALL_DIR := $(DIGIT_PREFIX)/modules/lesson
INSTALL_BIN := $(INSTALL_DIR)/bin
LESSON_DIR := $(DIGIT_PREFIX)/lessons

.PHONY: all clean check-abi rebuild install

all: check-abi $(TARGET)

check-abi:
	@test -f "$(ABI_INCLUDE)/module.h" || { echo "Missing external ABI: $(ABI_INCLUDE)/module.h"; exit 1; }

$(TARGET): $(SOURCE) includes/lesson.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(SOURCE) -o $@

bin:
	mkdir -p bin

install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root; run: sudo make install"; exit 1; }
	install -d -o digit -g digit $(INSTALL_DIR) $(INSTALL_BIN) $(LESSON_DIR)
	install -m 0755 $(TARGET) $(INSTALL_BIN)/lesson.so
	install -m 0644 module.conf $(INSTALL_DIR)/module.conf
	chown -R digit:digit $(INSTALL_DIR) $(LESSON_DIR)
	@echo "Lesson installed to $(INSTALL_DIR). Lessons belong in $(LESSON_DIR)/*.txt."

rebuild: clean all

clean:
	rm -rf bin
