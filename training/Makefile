CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

BIN_DIR := bin
BUILDER := $(BIN_DIR)/training_builder
VALIDATOR := $(BIN_DIR)/training_validate
PROMOTER := $(BIN_DIR)/training_promote
SEED := seed.jsonl
CANDIDATES := candidates.jsonl

.PHONY: all validate validate-candidates test clean

all: $(BUILDER) $(VALIDATOR) $(PROMOTER)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILDER): tools/training_builder.c | $(BIN_DIR)
	$(CC) $(CFLAGS) tools/training_builder.c -o $(BUILDER)

$(VALIDATOR): tools/training_validate.c | $(BIN_DIR)
	$(CC) $(CFLAGS) tools/training_validate.c -o $(VALIDATOR)

$(PROMOTER): tools/training_promote.c | $(BIN_DIR)
	$(CC) $(CFLAGS) tools/training_promote.c -o $(PROMOTER)

validate: $(VALIDATOR)
	./$(VALIDATOR) $(SEED)

validate-candidates: $(VALIDATOR)
	./$(VALIDATOR) $(CANDIDATES)

test: all validate
	@rm -f $(BIN_DIR)/test.jsonl $(BIN_DIR)/promoted.jsonl
	./$(BUILDER) $(BIN_DIR)/test.jsonl "Digit will enter safe mode on fatal Core failure." "What will Digit do?" "I will enter safe mode."
	./$(VALIDATOR) $(BIN_DIR)/test.jsonl
	./$(PROMOTER) $(BIN_DIR)/test.jsonl $(BIN_DIR)/promoted.jsonl
	./$(VALIDATOR) $(BIN_DIR)/promoted.jsonl
	@! ./$(BUILDER) $(BIN_DIR)/test.jsonl "" "Question" "Answer" >/dev/null 2>&1
	@printf '%s\n' '{"context":"","question":"Question","answer":"Answer"}' > $(BIN_DIR)/invalid.jsonl
	@! ./$(VALIDATOR) $(BIN_DIR)/invalid.jsonl >/dev/null 2>&1
	@echo "Training tool tests passed."

clean:
	rm -rf $(BIN_DIR)
