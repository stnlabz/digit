CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS ?= -Iinclude

TARGET := build/digit
SOURCES := src/main.c src/digit.c
OBJECTS := $(SOURCES:src/%.c=build/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

build/%.o: src/%.c include/digit.h | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build
