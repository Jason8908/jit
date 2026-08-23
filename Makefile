NAME      := jit
CC        ?= cc
CPPFLAGS  := -Iinclude
CFLAGS    := -std=c11 -Wall -Wextra -g -O0
LDFLAGS   :=
LDLIBS    :=

SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

SRCS      := $(wildcard $(SRC_DIR)/*.c)
OBJS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS      := $(OBJS:.o=.d)
TARGET    := $(BIN_DIR)/$(NAME)

.PHONY: all clean compile_commands.json
all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

$(BUILD_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

compile_commands.json:
	@printf '[\n' > $@
	@for src in $(SRCS); do \
		printf '  {"directory": "$(CURDIR)", "file": "$(CURDIR)/%s", "command": "$(CC) $(CPPFLAGS) $(CFLAGS) -c %s -o $(BUILD_DIR)/%s.o"},\n' \
			"$$src" "$$src" "$$(basename $$src .c)"; \
	done | sed '$$ s/,$$//' >> $@
	@printf ']\n' >> $@
