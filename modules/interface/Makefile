CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

ABI_DIR ?= ../../../ABI
ABI_INCLUDE := $(ABI_DIR)/includes
DIGIT_INCLUDE := ../../include
CPPFLAGS ?= -Iincludes -I$(DIGIT_INCLUDE) -I$(ABI_INCLUDE)
LDLIBS ?= -pthread -lcrypt

TARGET := bin/interface.so
TEST_TARGET := bin/test_interface
ACCESS_TEST_TARGET := bin/test_access_policy
SESSION_TEST_TARGET := bin/test_session_store
HTTP_AUTH_TEST_TARGET := bin/test_http_auth
ACCOUNT_AUTH_TEST_TARGET := bin/test_account_auth
CHANNEL_ACL_TEST_TARGET := bin/test_channel_acl
PROJECT_TEST_TARGET := bin/test_project_provision
MILESTONE_TEST_TARGET := bin/test_1_4_0
STABILIZATION_TEST_TARGET := bin/test_1_3_10
PRIOR_MILESTONE_TEST_TARGET := bin/test_1_3_9
EARLIER_MILESTONE_TEST_TARGET := bin/test_1_3_8
OLDEST_MILESTONE_TEST_TARGET := bin/test_1_3_7
FIRST_MILESTONE_TEST_TARGET := bin/test_1_3_6
BASE_MILESTONE_TEST_TARGET := bin/test_1_3_5
ORIGINAL_MILESTONE_TEST_TARGET := bin/test_1_3_4
PROJECT_BRIDGE_TEST_TARGET := bin/test_project_channel_bridge
SA_TEST_TARGET := bin/test_security_sa
MESSAGE_ORIGIN_TEST_TARGET := bin/test_message_origin
PROJECT_ADMIN_ROUTE_TEST_TARGET := bin/test_project_admin_route
SOURCE := src/interface.c src/controlled_communication.c src/interface_audit.c src/http_limits.c src/session_store.c src/http_auth.c src/account_auth.c src/channel_acl.c src/project_provision.c src/project_channel_bridge.c src/security_sa.c
TEST_SOURCE := tests/test_interface.c

DIGIT_PREFIX ?= /opt/digit
INSTALL_DIR := $(DIGIT_PREFIX)/modules/interface
INSTALL_BIN := $(INSTALL_DIR)/bin

.PHONY: all test clean check-abi rebuild install

all: check-abi $(TARGET)

check-abi:
	@test -f "$(ABI_INCLUDE)/module.h" || { echo "Missing external ABI: $(ABI_INCLUDE)/module.h"; exit 1; }
	@test -f "$(DIGIT_INCLUDE)/core_services.h" || { echo "Missing Digit Core service contract: $(DIGIT_INCLUDE)/core_services.h"; exit 1; }

$(TARGET): $(SOURCE) includes/channel_acl.h includes/access_policy.h includes/interface.h includes/session_store.h includes/project_channel_bridge.h includes/project_provision.h includes/security_sa.h includes/project_admin_route.h includes/message_origin.h $(DIGIT_INCLUDE)/core_services.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared $(SOURCE) -o $@ $(LDLIBS)

$(TEST_TARGET): $(TEST_SOURCE) $(SOURCE) includes/channel_acl.h includes/access_policy.h includes/interface.h includes/session_store.h $(DIGIT_INCLUDE)/core_services.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SOURCE) $(SOURCE) -o $@ $(LDLIBS)

$(ACCESS_TEST_TARGET): tests/test_access_policy.c includes/access_policy.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_access_policy.c -o $@

$(SESSION_TEST_TARGET): tests/test_session_store.c src/session_store.c includes/session_store.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_session_store.c src/session_store.c -o $@

$(HTTP_AUTH_TEST_TARGET): tests/test_http_auth.c src/http_auth.c src/session_store.c includes/http_auth.h includes/session_store.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_http_auth.c src/http_auth.c src/session_store.c -o $@

$(ACCOUNT_AUTH_TEST_TARGET): tests/test_account_auth.c src/account_auth.c includes/account_auth.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_account_auth.c src/account_auth.c -o $@ -lcrypt

$(CHANNEL_ACL_TEST_TARGET): tests/test_channel_acl.c src/channel_acl.c src/security_sa.c src/project_channel_bridge.c src/project_provision.c includes/channel_acl.h includes/access_policy.h includes/project_channel_bridge.h includes/project_provision.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_channel_acl.c src/channel_acl.c src/security_sa.c src/project_channel_bridge.c src/project_provision.c -o $@

$(PROJECT_TEST_TARGET): tests/test_project_provision.c src/project_provision.c includes/project_provision.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_project_provision.c src/project_provision.c src/security_sa.c -o $@

$(ORIGINAL_MILESTONE_TEST_TARGET): tests/test_1_3_4.c src/project_provision.c src/security_sa.c includes/project_provision.h includes/security_sa.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_3_4.c src/project_provision.c src/security_sa.c -o $@

$(BASE_MILESTONE_TEST_TARGET): tests/test_1_3_5.c src/project_provision.c src/security_sa.c includes/project_provision.h includes/security_sa.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_3_5.c src/project_provision.c src/security_sa.c -o $@

$(FIRST_MILESTONE_TEST_TARGET): tests/test_1_3_6.c src/channel_acl.c src/project_channel_bridge.c src/project_provision.c src/security_sa.c includes/channel_acl.h includes/project_channel_bridge.h includes/project_provision.h includes/security_sa.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_3_6.c src/channel_acl.c src/project_channel_bridge.c src/project_provision.c src/security_sa.c -o $@

$(OLDEST_MILESTONE_TEST_TARGET): tests/test_1_3_7.c src/channel_acl.c src/project_channel_bridge.c src/project_provision.c src/security_sa.c includes/channel_acl.h includes/project_channel_bridge.h includes/project_provision.h includes/security_sa.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_3_7.c src/channel_acl.c src/project_channel_bridge.c src/project_provision.c src/security_sa.c -o $@

$(EARLIER_MILESTONE_TEST_TARGET): tests/test_1_3_8.c src/http_limits.c includes/http_limits.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_3_8.c src/http_limits.c -o $@

$(PRIOR_MILESTONE_TEST_TARGET): tests/test_1_3_9.c src/interface_audit.c includes/interface_audit.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_3_9.c src/interface_audit.c -o $@

$(STABILIZATION_TEST_TARGET): tests/test_1_3_10.c src/http_limits.c src/interface_audit.c src/security_sa.c src/project_provision.c includes/http_limits.h includes/interface_audit.h includes/security_sa.h includes/project_provision.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_3_10.c src/http_limits.c src/interface_audit.c src/security_sa.c src/project_provision.c -o $@

$(MILESTONE_TEST_TARGET): tests/test_1_4_0.c src/controlled_communication.c includes/controlled_communication.h includes/message_origin.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_0.c src/controlled_communication.c -o $@

$(PROJECT_BRIDGE_TEST_TARGET): tests/test_project_channel_bridge.c src/project_channel_bridge.c src/project_provision.c includes/project_channel_bridge.h includes/security_sa.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_project_channel_bridge.c src/project_channel_bridge.c src/project_provision.c src/security_sa.c -o $@

$(PROJECT_ADMIN_ROUTE_TEST_TARGET): tests/test_project_admin_route.c includes/project_admin_route.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_project_admin_route.c -o $@

$(MESSAGE_ORIGIN_TEST_TARGET): tests/test_message_origin.c includes/message_origin.h $(DIGIT_INCLUDE)/channel.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_message_origin.c -o $@

$(SA_TEST_TARGET): tests/test_security_sa.c src/security_sa.c includes/security_sa.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_security_sa.c src/security_sa.c -o $@

test: check-abi $(MILESTONE_TEST_TARGET) $(STABILIZATION_TEST_TARGET) $(PRIOR_MILESTONE_TEST_TARGET) $(EARLIER_MILESTONE_TEST_TARGET) $(OLDEST_MILESTONE_TEST_TARGET) $(FIRST_MILESTONE_TEST_TARGET) $(BASE_MILESTONE_TEST_TARGET) $(ORIGINAL_MILESTONE_TEST_TARGET) $(TEST_TARGET) $(ACCESS_TEST_TARGET) $(SESSION_TEST_TARGET) $(HTTP_AUTH_TEST_TARGET) $(ACCOUNT_AUTH_TEST_TARGET) $(CHANNEL_ACL_TEST_TARGET) $(PROJECT_TEST_TARGET) $(PROJECT_BRIDGE_TEST_TARGET) $(SA_TEST_TARGET) $(MESSAGE_ORIGIN_TEST_TARGET) $(PROJECT_ADMIN_ROUTE_TEST_TARGET)
	@echo 'DIGIT INTERFACE 1.4.0 — MILESTONE'
	./$(MILESTONE_TEST_TARGET)
	@echo 'DIGIT INTERFACE — HISTORICAL REGRESSION'
	@set -eu; passed=0; failed=0; \
	for suite in $(STABILIZATION_TEST_TARGET) $(PRIOR_MILESTONE_TEST_TARGET) $(EARLIER_MILESTONE_TEST_TARGET) $(OLDEST_MILESTONE_TEST_TARGET) $(FIRST_MILESTONE_TEST_TARGET) $(BASE_MILESTONE_TEST_TARGET) $(ORIGINAL_MILESTONE_TEST_TARGET) $(TEST_TARGET) $(ACCESS_TEST_TARGET) $(SESSION_TEST_TARGET) $(HTTP_AUTH_TEST_TARGET) $(ACCOUNT_AUTH_TEST_TARGET) $(CHANNEL_ACL_TEST_TARGET) $(PROJECT_TEST_TARGET) $(PROJECT_BRIDGE_TEST_TARGET) $(SA_TEST_TARGET) $(MESSAGE_ORIGIN_TEST_TARGET) $(PROJECT_ADMIN_ROUTE_TEST_TARGET); do \
	  output=$$(mktemp); \
	  if ./$$suite >"$$output" 2>&1; then \
	    passed=$$((passed + 1)); \
	  else \
	    failed=$$((failed + 1)); \
	    echo "FAIL: $$suite"; cat "$$output"; \
	  fi; \
	  rm -f "$$output"; \
	done; \
	echo "Historical suites: $$passed passed, $$failed failed"; \
	test "$$failed" -eq 0

install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root; run: sudo make install"; exit 1; }
	install -d -o digit -g digit $(INSTALL_DIR) $(INSTALL_BIN)
	install -d -o digit -g digit -m 0700 $(DIGIT_PREFIX)/state/auth $(DIGIT_PREFIX)/state/projects
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
