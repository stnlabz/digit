CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

ABI_DIR ?= ../../../ABI
ABI_INCLUDE := $(ABI_DIR)/includes
CPPFLAGS ?= -Iincludes -I$(ABI_INCLUDE)

TARGET := bin/corpus.so
TEST_TARGET := bin/test_corpus
SOURCE := src/corpus.c
TEST_SOURCE := tests/test_corpus.c

DIGIT_PREFIX ?= /opt/digit
INSTALL_DIR := $(DIGIT_PREFIX)/modules/corpus
INSTALL_BIN := $(INSTALL_DIR)/bin
CORPUS_DIR := $(DIGIT_PREFIX)/corpus

.PHONY: all test clean check-abi rebuild install

all: check-abi $(TARGET)

check-abi:
	@test -f "$(ABI_INCLUDE)/module.h" || { echo "Missing external ABI: $(ABI_INCLUDE)/module.h"; exit 1; }

$(TARGET): $(SOURCE) includes/corpus.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(SOURCE) -o $@

$(TEST_TARGET): $(TEST_SOURCE) $(SOURCE) includes/corpus.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SOURCE) $(SOURCE) -o $@

test: check-abi $(TEST_TARGET)
	./$(TEST_TARGET)

install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root; run: sudo make install"; exit 1; }
	install -d -o digit -g digit $(INSTALL_DIR) $(INSTALL_BIN) $(CORPUS_DIR)
	install -m 0755 $(TARGET) $(INSTALL_BIN)/corpus.so
	install -m 0644 module.conf $(INSTALL_DIR)/module.conf
	install -m 0644 corpus.conf $(INSTALL_DIR)/corpus.conf
	touch $(CORPUS_DIR)/corpus.tsv
	chown -R digit:digit $(INSTALL_DIR) $(CORPUS_DIR)
	chmod 0640 $(CORPUS_DIR)/corpus.tsv
	@echo "Corpus installed to $(INSTALL_DIR). Digit will qualify the candidate."

bin:
	mkdir -p bin

rebuild: clean all

clean:
	rm -rf bin
