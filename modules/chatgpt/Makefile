CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
ABI_DIR ?= ../../../ABI
CPPFLAGS ?= -Iincludes -I$(ABI_DIR)/includes
TARGET := bin/chatgpt.so
.PHONY: all test clean check-abi install
all: check-abi $(TARGET)
check-abi:
	@test -f "$(ABI_DIR)/includes/module.h" || { echo "External ABI missing"; exit 1; }
$(TARGET): src/chatgpt.c src/chatgpt_auth.c src/chatgpt_oauth.c src/chatgpt_oauth_codes.c src/chatgpt_oauth_tokens.c includes/chatgpt.h includes/chatgpt_auth.h includes/chatgpt_oauth.h includes/chatgpt_oauth_codes.h includes/chatgpt_oauth_tokens.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared src/chatgpt.c src/chatgpt_auth.c src/chatgpt_oauth.c src/chatgpt_oauth_codes.c src/chatgpt_oauth_tokens.c -o $@ -lcrypto
bin/test_chatgpt: tests/test_chatgpt.c src/chatgpt.c src/chatgpt_oauth.c includes/chatgpt.h includes/chatgpt_oauth.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_chatgpt.c src/chatgpt.c src/chatgpt_oauth.c -o $@
bin/test_chatgpt_auth: tests/test_chatgpt_auth.c src/chatgpt_auth.c includes/chatgpt_auth.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_chatgpt_auth.c src/chatgpt_auth.c -o $@ -lcrypto
bin/test_chatgpt_oauth: tests/test_chatgpt_oauth.c src/chatgpt_oauth.c includes/chatgpt_oauth.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_chatgpt_oauth.c src/chatgpt_oauth.c -o $@
bin/test_chatgpt_oauth_codes: tests/test_chatgpt_oauth_codes.c src/chatgpt_oauth_codes.c src/chatgpt_auth.c includes/chatgpt_oauth_codes.h includes/chatgpt_auth.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_chatgpt_oauth_codes.c src/chatgpt_oauth_codes.c src/chatgpt_auth.c -o $@ -lcrypto
bin/test_chatgpt_oauth_tokens: tests/test_chatgpt_oauth_tokens.c src/chatgpt_oauth_tokens.c includes/chatgpt_oauth_tokens.h | bin
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_chatgpt_oauth_tokens.c src/chatgpt_oauth_tokens.c -o $@ -lcrypto
test: check-abi bin/test_chatgpt bin/test_chatgpt_auth bin/test_chatgpt_oauth bin/test_chatgpt_oauth_codes bin/test_chatgpt_oauth_tokens
	./bin/test_chatgpt
	./bin/test_chatgpt_auth
	./bin/test_chatgpt_oauth
	./bin/test_chatgpt_oauth_codes
	./bin/test_chatgpt_oauth_tokens
bin:
	mkdir -p bin
clean:
	rm -rf bin
install: all
	@test "$$(id -u)" -eq 0 || { echo "install requires root"; exit 1; }
	install -d -o digit -g digit /opt/digit/modules/chatgpt/bin
	install -m 0755 $(TARGET) /opt/digit/modules/chatgpt/bin/chatgpt.so
	install -m 0644 module.conf /opt/digit/modules/chatgpt/module.conf
	chown -R digit:digit /opt/digit/modules/chatgpt
