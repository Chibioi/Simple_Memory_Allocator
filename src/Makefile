CC = gcc

CFLAGS = -Wall -Wextra -Werror -Wshadow \
          -std=c11 -g3 -pthread \
          -O2 -fsanitize=address \
          -D_DEFAULT_SOURCE

LDFLAGS = -pthread -fsanitize=address

TARGET = test_alloc

SRCS = testing.c mem_alloc.c
HEADERS = mem_alloc.h

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
