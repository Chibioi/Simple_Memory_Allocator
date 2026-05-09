CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -Werror \
          -Wpedantic \
          -Wshadow \
          -Wconversion \
          -Wstrict-prototypes \
          -Wmissing-prototypes \
          -Wnull-dereference \
          -Wdouble-promotion \
          -O2 \
          -g3 \
          -fsanitize=address \
          -fno-omit-frame-pointer \
          -std=c11 \
		  -pthread


INCLUDES = -Isrc

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

TARGET = $(BUILD_DIR)/tests

SRC = $(SRC_DIR)/mem_alloc.c
TEST_SRC = $(TEST_DIR)/tests.c

OBJ = $(BUILD_DIR)/mem_alloc.o \
      $(BUILD_DIR)/tests.o

PKG_CFLAGS = $(shell pkg-config --cflags cmocka)
LIBS = $(shell pkg-config --libs cmocka) \
       -lpthread \
       -fsanitize=address

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/mem_alloc.o: $(SRC) $(SRC_DIR)/mem_alloc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PKG_CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/tests.o: $(TEST_SRC) $(SRC_DIR)/mem_alloc.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PKG_CFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) -o $(TARGET)

run: $(TARGET)
	ASAN_OPTIONS=detect_leaks=1 ./$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all

.PHONY: all run clean rebuild
