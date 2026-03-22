# The compiler to use
CC = gcc

# 1. Warning Flags: Catch common mistakes (uninitialized variables, type mismatches)
# 2. Debugging: -g3 provides maximum debug info for GDB
# 3. Standards: -std=c11 ensures you're using modern C rules
# 4. Security: -fstack-protector adds protection against stack overflows
CFLAGS = -Wall -Wextra -Werror -Wshadow -std=c11 -g3 -fsanitize=address

# The name of the executable
TARGET = my_program

# List of source files
SRCS = testing.c mem_alloc.c
# List of object files (converts .c to .o)
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

# Link the object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
