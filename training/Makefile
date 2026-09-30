CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

BIN_DIR := bin
BUILDER := $(BIN_DIR)/training_builder
VALIDATOR := $(BIN_DIR)/training_validate
SEED := seed.jsonl

.PHONY: all validate test clean

all: $(BUILDER) $(VALIDATOR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILDER): tools/training_builder.c | $(BIN_DIR)
	$(CC) $(CFLAGS) tools/training_builder.c -o $(BUILDER)

$(VALIDATOR): tools/training_validate.c | $(BIN_DIR)
	$(CC) $(CFLAGS) tools/training_validate.c -o $(VALIDATOR)

validate: $(VALIDATOR)
	./$(VALIDATOR) $(SEED)

test: all validate
	@rm -f $(BIN_DIR)/test.jsonl
	./$(BUILDER) $(BIN_DIR)/test.jsonl "Digit will enter safe mode on fatal Core failure." "What will Digit do?" "I will enter safe mode."
	./$(VALIDATOR) $(BIN_DIR)/test.jsonl
	@! ./$(BUILDER) $(BIN_DIR)/test.jsonl "" "Question" "Answer" >/dev/null 2>&1
	@printf '%s\n' '{"context":"","question":"Question","answer":"Answer"}' > $(BIN_DIR)/invalid.jsonl
	@! ./$(VALIDATOR) $(BIN_DIR)/invalid.jsonl >/dev/null 2>&1
	@echo "Training tool tests passed."

clean:
	rm -rf $(BIN_DIR)
