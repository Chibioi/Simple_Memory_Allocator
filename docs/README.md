<!-- markdownlint-disable MD013 -->
# mem_alloc — Custom Memory Allocator in C

A custom heap memory allocator implemented in C, providing drop-in replacements for `malloc`, `free`, `realloc`, and `calloc`. It manages a linked list of memory blocks using `sbrk()` for heap expansion, with thread safety via POSIX mutexes.

---

## Features

- **`my_malloc(size)`** — Allocates a block of at least `size` bytes. Reuses freed blocks when possible; requests new heap space via `sbrk()` otherwise.
- **`my_free(ptr)`** — Marks a block as free for future reuse. Handles `NULL` safely.
- **`my_realloc(ptr, size)`** — Resizes an existing allocation. Splits the block in place when shrinking (if the remainder is large enough), or allocates a new block and copies data when growing.
- **`my_calloc(n, size)`** — Allocates `n * size` zero-initialized bytes. Includes overflow protection.
- **Thread safety** — All allocator operations are protected by a `pthread_mutex_t` global lock.

---

## Project Structure

```
.
├── src/
│   ├── mem_alloc.h       # Public API and block_header struct definition
│   └── mem_alloc.c       # Allocator implementation
├── tests/
│   ├── tests.c           # CUnit test suite
│   └── Makefile          # Build system for tests
└── testing.c             # Manual integration test (basic allocation & reuse)
```

---

## How It Works

Each allocated region is preceded by a `block_header` struct in memory:

```c
typedef struct block_header {
    size_t size;               // Usable size of the block (excluding header)
    struct block_header *next; // Next block in the linked list
    unsigned int free;         // 0 = free, 1 = allocated
    int debug;                 // Magic value for integrity checks
} block_header;
```

### Allocation Strategy

1. On the first allocation, `my_malloc` calls `sbrk()` to extend the heap and initialises `global_base` (the head of the list).
2. On subsequent allocations, `find_free_block()` performs a **first-fit** linear scan of the list.
3. If no suitable free block is found, `request_space()` extends the heap again.

### Realloc Splitting

When shrinking via `my_realloc`, the leftover space is carved into a new free `block_header` and inserted into the list — but only when the remainder is at least `sizeof(block_header) + MIN_BLOCK_SIZE` (8) bytes, avoiding unusably small fragments.

### Debug Values

| Value        | Meaning                          |
|--------------|----------------------------------|
| `0x12345678` | Freshly allocated via `sbrk()`   |
| `0x7777777`  | Reused from the free list        |
| `0x55555555` | Freed block                      |
| `0xDEADBEEF` | Block created by realloc split   |

---

## Building & Running

### Prerequisites

- GCC
- POSIX threads (`-pthread`)
- [CUnit](http://cunit.sourceforge.net/) (`libcunit`) for the test suite

On Debian/Ubuntu:
```bash
sudo apt install libcunit1 libcunit1-dev
```

### Run the Test Suite

```bash
cd tests
make run
```

Or step by step:
```bash
make          # compiles test_malloc binary
./test_malloc # runs the CUnit suite
make clean    # removes build artefacts
make rebuild  # clean + build in one step
```

### Run the Manual Test

```bash
gcc -Wall -pthread -o testing testing.c src/mem_alloc.c
./testing
```

---

## Test Suite

Tests are written with **CUnit** and cover:

| Test | Description |
|------|-------------|
| `malloc basic` | Allocates 32 bytes and writes/verifies a pattern |
| `malloc zero` | `my_malloc(0)` must return `NULL` |
| `calloc` | 10-element int array is zero-initialised |
| `free null` | `my_free(NULL)` must not crash |
| `realloc grow` | Grows a 10-byte block to 20; data preserved |
| `realloc shrink` | Shrinks a 20-byte block to 5; prefix preserved |
| `realloc null` | `my_realloc(NULL, n)` behaves like `my_malloc(n)` |
| `multiple allocations` | Three independent allocations all succeed |
| `calloc overflow` | `my_calloc(SIZE_MAX, 2)` returns `NULL` |

---

## Known Limitations

- The free list is a simple **singly linked list**; large heaps will have O(n) allocation time. Doubly linked list can be implemented for O(1) coalescing.
- find_free_block() uses the first-fit allocation algorithm but improvements can be made by implementing best-fit, AVL trees and segregated lists to improve reuse significantly.

---
## Updates

1. Added block coalescing to avoid fragmentation in memory.
2. Added the macros ALIGN8(x) (((x) + 7) & ~7) for better memory alignment to improve CPU performance. 
3. Added block splitting to my malloc()
4. Implemented mmap() for large allocations while still keeping sbrk() for small alloactions making it thread-safe.

---
## License

This project is for educational purposes.
