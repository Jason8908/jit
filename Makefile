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

.PHONY: all clean
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
