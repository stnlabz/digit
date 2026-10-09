CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

ABI_DIR ?= ../../../ABI
ABI_INCLUDE := $(ABI_DIR)/includes
DIGIT_INCLUDE := ../../include
CPPFLAGS ?= -Iincludes -I$(DIGIT_INCLUDE) -I$(ABI_INCLUDE)
LDLIBS ?= -pthread -lcrypt -lssl -lcrypto

TARGET := bin/interface.so
TEST_TARGET := bin/test_interface
ACCESS_TEST_TARGET := bin/test_access_policy
SESSION_TEST_TARGET := bin/test_session_store
HTTP_AUTH_TEST_TARGET := bin/test_http_auth
ACCOUNT_AUTH_TEST_TARGET := bin/test_account_auth
CHANNEL_ACL_TEST_TARGET := bin/test_channel_acl
PROJECT_TEST_TARGET := bin/test_project_provision
MILESTONE_TEST_TARGET := bin/test_1_6_0
PROJECT_1510_REGRESSION_TARGET := bin/test_1_5_10
ORG_159_REGRESSION_TARGET := bin/test_1_5_9
CHANNEL_158_REGRESSION_TARGET := bin/test_1_5_8
DASHBOARD_156_REGRESSION_TARGET := bin/test_1_5_6
SA_154_REGRESSION_TARGET := bin/test_1_5_4
PREVIOUS_BUILDER_TEST_TARGET := bin/test_1_5_3
BUILDER_PREVIOUS_TEST_TARGET := bin/test_1_5_2
PREVIOUS_CORPUS_TEST_TARGET := bin/test_1_5_1
CORPUS_BASE_TEST_TARGET := bin/test_1_5_0
CHANNEL_TEST_TARGET := bin/test_1_4_10
ALERT_EXACT_TEST_TARGET := bin/test_1_4_9
ALERT_LIST_TEST_TARGET := bin/test_1_4_8
MESSAGE_TEST_TARGET := bin/test_1_4_7
REPAIR_TEST_TARGET := bin/test_1_4_6
PREVIOUS_MILESTONE_TEST_TARGET := bin/test_1_4_5
UTF8_TEST_TARGET := bin/test_1_4_4
RECORD_TEST_TARGET := bin/test_1_4_3
INTEGRITY_TEST_TARGET := bin/test_1_4_2
KNOWLEDGE_TEST_TARGET := bin/test_1_4_1
COMMUNICATION_TEST_TARGET := bin/test_1_4_0
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
SOURCE := src/grant_store.c src/grant_request.c src/project_list.c src/channel_create_policy.c src/dashboard_response.c src/interface.c src/builder_response.c src/corpus_response.c src/channel_results.c src/alert_results.c src/message_results.c src/http_write.c src/knowledge_query.c src/controlled_communication.c src/interface_audit.c src/http_limits.c src/session_store.c src/http_auth.c src/account_auth.c src/channel_acl.c src/project_provision.c src/project_channel_bridge.c src/security_sa.c
TEST_SOURCE := tests/test_interface.c

DIGIT_PREFIX ?= /opt/digit
INSTALL_DIR := $(DIGIT_PREFIX)/modules/interface
INSTALL_BIN := $(INSTALL_DIR)/bin

.PHONY: all test clean check-abi rebuild install

all: check-abi $(TARGET)

check-abi:
	@test -f "$(ABI_INCLUDE)/module.h" || { echo "Missing external ABI: $(ABI_INCLUDE)/module.h"; exit 1; }
	@test -f "$(DIGIT_INCLUDE)/core_services.h" || { echo "Missing Digit Core service contract: $(DIGIT_INCLUDE)/core_services.h"; exit 1; }

$(TARGET): $(SOURCE) includes/channel_acl.h includes/access_policy.h includes/interface.h includes/session_store.h includes/project_channel_bridge.h includes/project_provision.h includes/security_sa.h includes/project_admin_route.h includes/message_origin.h includes/message_results.h includes/channel_results.h includes/corpus_response.h includes/alert_results.h includes/http_write.h includes/knowledge_query.h $(DIGIT_INCLUDE)/core_services.h | bin
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

$(COMMUNICATION_TEST_TARGET): tests/test_1_4_0.c src/controlled_communication.c includes/controlled_communication.h includes/message_origin.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_0.c src/controlled_communication.c -o $@

$(KNOWLEDGE_TEST_TARGET): tests/test_1_4_1.c src/knowledge_query.c includes/knowledge_query.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_1.c src/knowledge_query.c -o $@

$(INTEGRITY_TEST_TARGET): tests/test_1_4_2.c src/knowledge_query.c includes/knowledge_query.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_2.c src/knowledge_query.c -o $@

$(RECORD_TEST_TARGET): tests/test_1_4_3.c src/knowledge_query.c includes/knowledge_query.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_3.c src/knowledge_query.c -o $@

$(UTF8_TEST_TARGET): tests/test_1_4_4.c src/knowledge_query.c includes/knowledge_query.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_4.c src/knowledge_query.c -o $@

$(PREVIOUS_MILESTONE_TEST_TARGET): tests/test_1_4_5.c src/knowledge_query.c includes/knowledge_query.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_5.c src/knowledge_query.c -o $@

$(REPAIR_TEST_TARGET): tests/test_1_4_6.c src/http_write.c src/knowledge_query.c includes/http_write.h includes/knowledge_query.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_6.c src/http_write.c src/knowledge_query.c -o $@

$(MESSAGE_TEST_TARGET): tests/test_1_4_7.c src/message_results.c src/controlled_communication.c includes/message_results.h includes/controlled_communication.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_7.c src/message_results.c src/controlled_communication.c -o $@

$(ALERT_LIST_TEST_TARGET): tests/test_1_4_8.c src/alert_results.c includes/alert_results.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_8.c src/alert_results.c -o $@

$(ALERT_EXACT_TEST_TARGET): tests/test_1_4_9.c src/alert_results.c includes/alert_results.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_9.c src/alert_results.c -o $@

$(CHANNEL_TEST_TARGET): tests/test_1_4_10.c src/channel_results.c includes/channel_results.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_4_10.c src/channel_results.c -o $@

$(CORPUS_BASE_TEST_TARGET): tests/test_1_5_0.c src/corpus_response.c src/knowledge_query.c includes/corpus_response.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_5_0.c src/corpus_response.c src/knowledge_query.c -o $@

$(PREVIOUS_CORPUS_TEST_TARGET): tests/test_1_5_1.c src/corpus_response.c src/knowledge_query.c includes/corpus_response.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_5_1.c src/corpus_response.c src/knowledge_query.c -o $@

$(MILESTONE_TEST_TARGET): tests/test_1_6_0.c src/grant_request.c src/grant_store.c src/project_provision.c src/project_channel_bridge.c src/security_sa.c includes/grant_request.h includes/grant_store.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_6_0.c src/grant_request.c src/grant_store.c src/project_provision.c src/project_channel_bridge.c src/security_sa.c -o $@

$(PROJECT_1510_REGRESSION_TARGET): tests/test_1_5_10.c src/project_list.c src/project_provision.c src/security_sa.c includes/project_list.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_5_10.c src/project_list.c src/project_provision.c src/security_sa.c -o $@

$(ORG_159_REGRESSION_TARGET): tests/test_1_5_9.c src/sa_channel_policy.c includes/sa_channel_policy.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_5_9.c src/sa_channel_policy.c -o $@

$(CHANNEL_158_REGRESSION_TARGET): tests/test_1_5_8.c src/channel_create_policy.c includes/channel_create_policy.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_5_8.c src/channel_create_policy.c -o $@



$(DASHBOARD_156_REGRESSION_TARGET): tests/test_1_5_6.c src/dashboard_response.c includes/dashboard_response.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_5_6.c src/dashboard_response.c -o $@

$(SA_154_REGRESSION_TARGET): tests/test_1_5_4.c src/security_sa.c includes/security_sa.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_5_4.c src/security_sa.c -o $@

$(PREVIOUS_BUILDER_TEST_TARGET): tests/test_1_5_3.c src/builder_response.c includes/builder_response.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_5_3.c src/builder_response.c -o $@

$(BUILDER_PREVIOUS_TEST_TARGET): tests/test_1_5_2.c src/corpus_response.c src/knowledge_query.c includes/corpus_response.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_1_5_2.c src/corpus_response.c src/knowledge_query.c -o $@

$(PROJECT_BRIDGE_TEST_TARGET): tests/test_project_channel_bridge.c src/project_channel_bridge.c src/project_provision.c includes/project_channel_bridge.h includes/security_sa.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_project_channel_bridge.c src/project_channel_bridge.c src/project_provision.c src/security_sa.c -o $@

$(PROJECT_ADMIN_ROUTE_TEST_TARGET): tests/test_project_admin_route.c includes/project_admin_route.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_project_admin_route.c -o $@

$(MESSAGE_ORIGIN_TEST_TARGET): tests/test_message_origin.c includes/message_origin.h $(DIGIT_INCLUDE)/channel.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_message_origin.c -o $@

$(SA_TEST_TARGET): tests/test_security_sa.c src/security_sa.c includes/security_sa.h | bin
	$(CC) -Iincludes $(CFLAGS) tests/test_security_sa.c src/security_sa.c -o $@

test: check-abi $(MILESTONE_TEST_TARGET) $(PROJECT_1510_REGRESSION_TARGET) $(ORG_159_REGRESSION_TARGET) $(CHANNEL_158_REGRESSION_TARGET) $(DASHBOARD_156_REGRESSION_TARGET) $(SA_154_REGRESSION_TARGET) $(PREVIOUS_BUILDER_TEST_TARGET) $(BUILDER_PREVIOUS_TEST_TARGET) $(PREVIOUS_CORPUS_TEST_TARGET) $(CORPUS_BASE_TEST_TARGET) $(CHANNEL_TEST_TARGET) $(ALERT_EXACT_TEST_TARGET) $(ALERT_LIST_TEST_TARGET) $(MESSAGE_TEST_TARGET) $(REPAIR_TEST_TARGET) $(PREVIOUS_MILESTONE_TEST_TARGET) $(UTF8_TEST_TARGET) $(RECORD_TEST_TARGET) $(INTEGRITY_TEST_TARGET) $(KNOWLEDGE_TEST_TARGET) $(COMMUNICATION_TEST_TARGET) $(STABILIZATION_TEST_TARGET) $(PRIOR_MILESTONE_TEST_TARGET) $(EARLIER_MILESTONE_TEST_TARGET) $(OLDEST_MILESTONE_TEST_TARGET) $(FIRST_MILESTONE_TEST_TARGET) $(BASE_MILESTONE_TEST_TARGET) $(ORIGINAL_MILESTONE_TEST_TARGET) $(TEST_TARGET) $(ACCESS_TEST_TARGET) $(SESSION_TEST_TARGET) $(HTTP_AUTH_TEST_TARGET) $(ACCOUNT_AUTH_TEST_TARGET) $(CHANNEL_ACL_TEST_TARGET) $(PROJECT_TEST_TARGET) $(PROJECT_BRIDGE_TEST_TARGET) $(SA_TEST_TARGET) $(MESSAGE_ORIGIN_TEST_TARGET) $(PROJECT_ADMIN_ROUTE_TEST_TARGET)
	@echo 'DIGIT INTERFACE 1.6.0 — GRANT PARSER MILESTONE'
	./$(MILESTONE_TEST_TARGET)
	@echo 'DIGIT INTERFACE — HISTORICAL REGRESSION'
	@set -eu; passed=0; failed=0; \
	for suite in $(PROJECT_1510_REGRESSION_TARGET) $(ORG_159_REGRESSION_TARGET) $(CHANNEL_158_REGRESSION_TARGET) $(DASHBOARD_156_REGRESSION_TARGET) $(SA_154_REGRESSION_TARGET) $(PREVIOUS_BUILDER_TEST_TARGET) $(BUILDER_PREVIOUS_TEST_TARGET) $(PREVIOUS_CORPUS_TEST_TARGET) $(CORPUS_BASE_TEST_TARGET) $(CHANNEL_TEST_TARGET) $(ALERT_EXACT_TEST_TARGET) $(ALERT_LIST_TEST_TARGET) $(MESSAGE_TEST_TARGET) $(REPAIR_TEST_TARGET) $(PREVIOUS_MILESTONE_TEST_TARGET) $(UTF8_TEST_TARGET) $(RECORD_TEST_TARGET) $(INTEGRITY_TEST_TARGET) $(KNOWLEDGE_TEST_TARGET) $(COMMUNICATION_TEST_TARGET) $(STABILIZATION_TEST_TARGET) $(PRIOR_MILESTONE_TEST_TARGET) $(EARLIER_MILESTONE_TEST_TARGET) $(OLDEST_MILESTONE_TEST_TARGET) $(FIRST_MILESTONE_TEST_TARGET) $(BASE_MILESTONE_TEST_TARGET) $(ORIGINAL_MILESTONE_TEST_TARGET) $(TEST_TARGET) $(ACCESS_TEST_TARGET) $(SESSION_TEST_TARGET) $(HTTP_AUTH_TEST_TARGET) $(ACCOUNT_AUTH_TEST_TARGET) $(CHANNEL_ACL_TEST_TARGET) $(PROJECT_TEST_TARGET) $(PROJECT_BRIDGE_TEST_TARGET) $(SA_TEST_TARGET) $(MESSAGE_ORIGIN_TEST_TARGET) $(PROJECT_ADMIN_ROUTE_TEST_TARGET); do \
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
