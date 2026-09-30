CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iincludes -I../../ABI/includes

MODULE = response
BUILD_DIR = build
BIN_DIR = bin
SO = $(BUILD_DIR)/$(MODULE).so
TEST = $(BIN_DIR)/test_$(MODULE)

.PHONY: all clean test install

all: $(SO)

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

$(SO): src/response.c includes/response.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared src/response.c -o $@

$(TEST): tests/test_response.c src/response.c includes/response.h | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_response.c src/response.c -o $@

test: $(TEST)
	./$(TEST)

install: $(SO)
	install -d /opt/digit/modules/response
	install -m 0755 $(SO) /opt/digit/modules/response/response.so
	install -m 0644 module.conf /opt/digit/modules/response/module.conf

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
