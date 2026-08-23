NAME      := jit
CC        ?= cc
CPPFLAGS  := -Iinclude
CFLAGS    := -std=c11 -Wall -Wextra -g -O0
LDFLAGS   :=
LDLIBS    :=

UNAME_S := $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
  LDLIBS += -lcrypto
endif

ifeq ($(SAN),1)
  SANFLAGS := -fsanitize=address,undefined -fno-sanitize-recover=all \
              -fno-omit-frame-pointer
  CFLAGS   += $(SANFLAGS)
  LDFLAGS  += $(SANFLAGS)
  SUFFIX   := -san
endif

SRC_DIR   := src
TEST_DIR  := tests
BUILD_DIR := build$(SUFFIX)
BIN_DIR   := bin$(SUFFIX)

SRCS      := $(wildcard $(SRC_DIR)/*.c)
OBJS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

LIB_SRCS  := $(filter-out $(SRC_DIR)/main.c,$(SRCS))
LIB_OBJS  := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(LIB_SRCS))

TEST_SRCS := $(wildcard $(TEST_DIR)/*_test.c)
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/$(TEST_DIR)/%.o,$(TEST_SRCS))
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/$(TEST_DIR)/%,$(TEST_SRCS))

DEPS      := $(OBJS:.o=.d) $(TEST_OBJS:.o=.d)
TARGET    := $(BIN_DIR)/$(NAME)

RUNNER    ?=
VALGRIND_FLAGS := --leak-check=full --show-leak-kinds=all \
                  --errors-for-leak-kinds=all --track-origins=yes \
                  --error-exitcode=1

.PHONY: all clean test test-valgrind compile_commands.json
all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/$(TEST_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)/$(TEST_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(BIN_DIR)/$(TEST_DIR)/%: $(BUILD_DIR)/$(TEST_DIR)/%.o $(LIB_OBJS) | $(BIN_DIR)/$(TEST_DIR)
	$(CC) $(LDFLAGS) $^ -o $@ $(LDLIBS)

test: $(TEST_BINS)
	@fail=0; for t in $(TEST_BINS); do \
		echo "== $$t"; \
		$(RUNNER) ./$$t || fail=1; \
	done; exit $$fail

test-valgrind:
	@$(MAKE) test RUNNER="valgrind $(VALGRIND_FLAGS)"

-include $(DEPS)

$(BUILD_DIR) $(BIN_DIR) $(BUILD_DIR)/$(TEST_DIR) $(BIN_DIR)/$(TEST_DIR):
	mkdir -p $@

clean:
	rm -rf build bin build-san bin-san

compile_commands.json:
	@printf '[\n' > $@
	@for src in $(SRCS) $(TEST_SRCS); do \
		printf '  {"directory": "$(CURDIR)", "file": "$(CURDIR)/%s", "command": "$(CC) $(CPPFLAGS) $(CFLAGS) -c %s -o $(BUILD_DIR)/%s.o"},\n' \
			"$$src" "$$src" "$$(basename $$src .c)"; \
	done | sed '$$ s/,$$//' >> $@
	@printf ']\n' >> $@

.SECONDARY: $(TEST_OBJS)