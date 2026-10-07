CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude

TARGET_DIR = bin
BUILD_DIR = build
SRC_DIR = src

TARGET = $(TARGET_DIR)/vcu_powertrain
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/vcu_core.c $(SRC_DIR)/can_manager.c \
	$(SRC_DIR)/ipc_can_db.c
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(TARGET_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	@echo "[SUCCESS] Binary generated at: $(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET_DIR)
	@echo "[CLEANED] Artifacts wiped."

test: all
	./$(TARGET) --simulate

.PHONY: all clean test
