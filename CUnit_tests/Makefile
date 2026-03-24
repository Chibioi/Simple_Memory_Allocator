# Compiler
CC = gcc
CFLAGS = -Wall -Wextra -pthread -g -I../src
LDFLAGS = -lcunit

# Source files
SRC = ../src/mem_alloc.c
TEST = tests.c

# Output
TARGET = test_malloc

all: $(TARGET)

$(TARGET): $(SRC) $(TEST)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o

rebuild: clean all
