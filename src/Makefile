CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wshadow -std=c11 -g3 -D_DEFAULT_SOURCE
LDFLAGS = $(CFLAGS)

TARGET = my_program
SRCS = testing.c mem_alloc.c
HEADERS = mem_alloc.h
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# This single rule handles all .c -> .o conversions and tracks header changes
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
