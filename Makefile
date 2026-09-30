CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

ABI_DIR ?= ../ABI
ABI_INCLUDE := $(ABI_DIR)/includes
ABI_SRC := $(ABI_DIR)/src

CPPFLAGS ?= -Iinclude -I$(ABI_INCLUDE)
LDLIBS ?= -ldl

TARGET := build/digit
TEST_MODULE_DIR := build/tests/modules
SACRIFICIAL_DIR := $(TEST_MODULE_DIR)/sacrificial
SACRIFICIAL_SO := $(SACRIFICIAL_DIR)/sacrificial.so
SACRIFICIAL_CONF := $(SACRIFICIAL_DIR)/module.conf
TEST_MODULE_MANAGER := build/test_module_manager
TEST_HOTLOAD := build/test_hotload
TEST_QUALIFICATION := build/test_qualification
TEST_QUALIFICATION_STORE := build/test_qualification_store
TEST_AUTHORITY := build/test_authority
TEST_SACRIFICIAL := build/test_sacrificial
TEST_RUNTIME := build/test_runtime
TEST_AUDIT := build/test_audit

PREFIX ?= /opt/digit
BINDIR := $(PREFIX)/bin
MODULEDIR := $(PREFIX)/modules
STATEDIR := $(PREFIX)/state
LOGDIR := $(PREFIX)/logs
SYSTEMD_UNIT := /etc/systemd/system/digit.service

DIGIT_SOURCES := \
	src/main.c \
	src/digit.c \
	src/module_manager.c \
	src/hotload.c \
	src/runtime.c \
	src/qualification.c \
	src/qualification_store.c \
	src/authority.c \
	src/audit.c

ABI_SOURCES := \
	$(ABI_SRC)/abi.c \
	$(ABI_SRC)/module.c \
	$(ABI_SRC)/module_registry.c \
	$(ABI_SRC)/loader_linux.c \
	$(ABI_SRC)/module_discovery_linux.c

MODULE_MANAGER_POLICY_SOURCES := \
	src/module_manager.c \
	src/qualification.c \
	src/qualification_store.c \
	src/authority.c

DIGIT_OBJECTS := $(DIGIT_SOURCES:src/%.c=build/digit_%.o)
ABI_OBJECTS := $(ABI_SOURCES:$(ABI_SRC)/%.c=build/abi_%.o)
OBJECTS := $(DIGIT_OBJECTS) $(ABI_OBJECTS)

.PHONY: all clean check-abi test install

all: check-abi $(TARGET)

check-abi:
	@test -f "$(ABI_INCLUDE)/abi.h" || { echo "Missing external ABI: $(ABI_INCLUDE)/abi.h"; exit 1; }
	@test -f "$(ABI_SRC)/loader_linux.c" || { echo "Missing Linux ABI loader: $(ABI_SRC)/loader_linux.c"; exit 1; }
	@test -f "$(ABI_SRC)/module_discovery_linux.c" || { echo "Missing Linux ABI discovery: $(ABI_SRC)/module_discovery_linux.c"; exit 1; }

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDLIBS)

build/digit_%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/abi_%.o: $(ABI_SRC)/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(SACRIFICIAL_SO): tests/modules/sacrificial/sacrificial.c | $(SACRIFICIAL_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $< -o $@

$(SACRIFICIAL_CONF): tests/modules/sacrificial/module.conf | $(SACRIFICIAL_DIR)
	cp $< $@

$(TEST_MODULE_MANAGER): tests/test_module_manager.c $(MODULE_MANAGER_POLICY_SOURCES) $(ABI_SOURCES) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_module_manager.c $(MODULE_MANAGER_POLICY_SOURCES) $(ABI_SOURCES) -o $@ $(LDLIBS)

$(TEST_HOTLOAD): tests/test_hotload.c src/hotload.c $(MODULE_MANAGER_POLICY_SOURCES) $(ABI_SOURCES) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_hotload.c src/hotload.c $(MODULE_MANAGER_POLICY_SOURCES) $(ABI_SOURCES) -o $@ $(LDLIBS)

$(TEST_QUALIFICATION): tests/test_qualification.c src/qualification.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_qualification.c src/qualification.c -o $@

$(TEST_QUALIFICATION_STORE): tests/test_qualification_store.c src/qualification.c src/qualification_store.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_qualification_store.c src/qualification.c src/qualification_store.c -o $@

$(TEST_AUTHORITY): tests/test_authority.c src/authority.c src/qualification.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_authority.c src/authority.c src/qualification.c -o $@

$(TEST_SACRIFICIAL): tests/test_sacrificial.c $(ABI_SRC)/loader_linux.c $(SACRIFICIAL_SO) $(SACRIFICIAL_CONF) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_sacrificial.c $(ABI_SRC)/loader_linux.c -o $@ $(LDLIBS)

$(TEST_RUNTIME): tests/test_runtime.c src/runtime.c src/hotload.c $(MODULE_MANAGER_POLICY_SOURCES) $(ABI_SOURCES) | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_runtime.c src/runtime.c src/hotload.c $(MODULE_MANAGER_POLICY_SOURCES) $(ABI_SOURCES) -o $@ $(LDLIBS)

$(TEST_AUDIT): tests/test_audit.c src/audit.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_audit.c src/audit.c -o $@

test: check-abi $(TEST_MODULE_MANAGER) $(TEST_HOTLOAD) $(TEST_QUALIFICATION) $(TEST_QUALIFICATION_STORE) $(TEST_AUTHORITY) $(TEST_SACRIFICIAL) $(TEST_RUNTIME) $(TEST_AUDIT)
	./$(TEST_MODULE_MANAGER)
	./$(TEST_HOTLOAD)
	./$(TEST_QUALIFICATION)
	./$(TEST_QUALIFICATION_STORE)
	./$(TEST_AUTHORITY)
	./$(TEST_SACRIFICIAL)
	./$(TEST_RUNTIME)
	@mkdir -p /opt/digit/logs 2>/dev/null || true
	./$(TEST_AUDIT)

install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root; run: sudo make install"; exit 1; }
	@id digit >/dev/null 2>&1 || useradd --system --home-dir $(PREFIX) --shell /usr/sbin/nologin digit
	install -d -o digit -g digit $(PREFIX) $(BINDIR) $(MODULEDIR) $(STATEDIR) $(LOGDIR)
	install -m 0755 $(TARGET) $(BINDIR)/digit
	chown digit:digit $(BINDIR)/digit
	@if test -e "$(SYSTEMD_UNIT)"; then \
		echo "Digit systemd service already exists; leaving it unchanged."; \
	else \
		install -m 0644 digit.service $(SYSTEMD_UNIT); \
		systemctl daemon-reload; \
		systemctl enable digit.service; \
	fi
	@systemctl restart digit.service
	@echo "Digit installed to $(PREFIX) and service is running."

build:
	mkdir -p build

$(TEST_MODULE_DIR): | build
	mkdir -p $(TEST_MODULE_DIR)

$(SACRIFICIAL_DIR): | $(TEST_MODULE_DIR)
	mkdir -p $(SACRIFICIAL_DIR)

clean:
	rm -rf build
