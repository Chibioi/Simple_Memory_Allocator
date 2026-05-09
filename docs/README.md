<!-- markdownlint-disable MD013 -->
# mem_alloc — Custom Memory Allocator in C

A custom heap memory allocator implemented in C, providing drop-in replacements for `malloc`, `free`, `realloc`, and `calloc`. It manages a doubly linked list of memory blocks using `sbrk()` for small heap allocations and `mmap()` for large ones, with thread safety via POSIX mutexes.

---

## Features

- **`my_malloc(size)`** — Allocates a block of at least `size` bytes. Reuses freed blocks (with splitting) when possible; requests new heap space via `sbrk()` otherwise. Falls back to `mmap()` for allocations ≥ 128 KB.
- **`my_free(ptr)`** — Marks a block as free and coalesces adjacent free blocks to reduce fragmentation. Handles `NULL` safely. Uses `munmap()` for blocks originally allocated via `mmap()`.
- **`my_realloc(ptr, size)`** — Resizes an existing allocation. Splits the block in place when shrinking (if the remainder is large enough), or allocates a new block and copies data when growing.
- **`my_calloc(n, size)`** — Allocates `n * size` zero-initialized bytes. Includes overflow protection.
- **Thread safety** — All sbrk-based allocator operations are protected by a `pthread_mutex_t` global lock. mmap allocations are issued outside the lock since `mmap()` is independently thread-safe.

---

## Project Structure

```
.
├── build/
│   ├── mem_alloc.o       # Compiled allocator object
│   ├── tests             # Test binary
│   └── tests.o           # Compiled test object
├── docs/
│   └── README.md         # Project documentation
├── Makefile              # Build system
├── src/
│   ├── mem_alloc.c       # Allocator implementation
│   └── mem_alloc.h       # Public API and block_header struct definition
└── tests/
    └── tests.c           # cmocka test suite
```

---

## How It Works

Each allocated region is preceded by a `block_header` struct in memory:

```c
typedef struct block_header {
    size_t size;               // Usable size of the block (excluding header)
    struct block_header *next; // Next block in the doubly linked list
    struct block_header *prev; // Previous block in the doubly linked list
    unsigned int free;         // 0 = free, 1 = allocated
    unsigned int is_mmap;      // 1 = allocated via mmap, 0 = via sbrk
    uint32_t debug;            // Magic value for integrity checks
} block_header;
```

### Allocation Strategy

1. Requests ≥ 128 KB bypass the heap entirely and are served via `mmap()` (anonymous, private mapping). These blocks are freed with `munmap()` and are not part of the linked list.
2. On the first small allocation, `my_malloc` calls `sbrk()` to extend the heap and initialises `global_base` (the head of the list).
3. On subsequent small allocations, `find_free_block()` performs a **first-fit** linear scan of the doubly linked list.
4. If a free block is found that is larger than needed, `split_block()` carves it into an allocated block and a new smaller free block.
5. If no suitable free block exists, `request_space()` extends the heap via `sbrk()`.

### Block Coalescing

When a block is freed, `coalesce_free_blocks()` performs a linear scan and merges any adjacent free blocks into a single larger block, reducing heap fragmentation. A second coalescing pass is triggered if the freed block has a free predecessor.

### Realloc Splitting

When shrinking via `my_realloc`, the leftover space is carved into a new free `block_header` and inserted into the list — but only when the remainder is at least `sizeof(block_header) + MIN_BLOCK_SIZE` (8) bytes, avoiding unusably small fragments.

### Alignment

All allocation sizes are rounded up to the nearest 8-byte boundary via the `ALIGN8(x)` macro, ensuring addresses are valid for all standard C data types on the target CPU.

### Debug Values

| Value        | Meaning                          |
|--------------|----------------------------------|
| `0x12345678` | Freshly allocated via `sbrk()`   |
| `0x7777777`  | Reused from the free list        |
| `0x55555555` | Freed block                      |
| `0xDEADBEEF` | Block created by split           |
| `0xABCDEF`   | Allocated via `mmap()`           |

---

## Building & Running

### Prerequisites

- GCC
- POSIX threads (`-pthread`)
- [cmocka](https://cmocka.org/) for the test suite

On Debian/Ubuntu:
```bash
sudo apt install libcmocka0 libcmocka-dev pkg-config
```

### Run the Test Suite

```bash
cd tests
make run
```

Or step by step:
```bash
make          # compiles build/tests binary
./build/tests # runs the cmocka suite
make clean    # removes the build/ directory
make rebuild  # clean + build in one step
```

The test binary is compiled with AddressSanitizer (`-fsanitize=address`) and run with leak detection enabled:

```bash
ASAN_OPTIONS=detect_leaks=1 ./build/tests
```

### Run the Manual Test

```bash
gcc -Wall -pthread -o testing testing.c src/mem_alloc.c
./testing
```

---

## Test Suite

Tests are written with **cmocka** and cover:

| Test | Description |
|------|-------------|
| `test_malloc_zero_returns_null` | `my_malloc(0)` must return `NULL` |
| `test_malloc_basic` | Allocates 32 bytes and writes/verifies a pattern |
| `test_calloc_zero_initialized` | 10-element int array is zero-initialised |
| `test_calloc_overflow` | `my_calloc(SIZE_MAX, SIZE_MAX)` returns `NULL` |
| `test_free_null_safe` | `my_free(NULL)` must not crash |
| `test_realloc_null_behaves_like_malloc` | `my_realloc(NULL, n)` behaves like `my_malloc(n)` |
| `test_realloc_zero_size` | `my_realloc(ptr, 0)` frees memory and returns `NULL` |
| `test_realloc_expand_preserves_data` | Grows a block; original data is intact |
| `test_realloc_shrink_preserves_data` | Shrinks a block; original data is intact |
| `test_malloc_alignment` | Returned pointer is 8-byte aligned |
| `test_large_allocation_mmap` | Allocation ≥ 128 KB sets `is_mmap = 1` |
| `test_free_marks_block_free` | Freed block has `free == 0` |
| `test_reuse_freed_block` | Second malloc reuses the previously freed block |
| `test_split_block` | Allocating from a large free block leaves a remainder free block |
| `test_coalesce_free_blocks` | Freeing two adjacent blocks merges them into one |
| `test_multiple_allocations_unique` | Three concurrent allocations return distinct pointers |

Each test runs with a `reset_allocator` setup function that resets `global_base = NULL` for a clean heap state.

---

## Known Limitations

- `find_free_block()` uses a **first-fit** linear scan; large heaps will have O(n) allocation time. Potential improvements include best-fit, segregated free lists, or AVL-tree-indexed bins.
- The global mutex serialises all sbrk-path allocations; a per-size-class lock or lock-free structure would improve multi-threaded throughput.

---

## Updates

1. Migrated test framework from **CUnit** to **cmocka**; expanded test suite from 9 to 16 tests.
2. Upgraded the block list from a singly linked list to a **doubly linked list** (`prev` pointer added to `block_header`), enabling O(1) backward coalescing.
3. Added **block coalescing** (`coalesce_free_blocks`) to merge adjacent free blocks and reduce fragmentation.
4. Added **block splitting** in `my_malloc()` and `my_realloc()` to reduce wasted space when reusing large free blocks.
5. Added `ALIGN8(x)` macro to enforce 8-byte alignment on all allocations.
6. Added **mmap-backed allocation** for requests ≥ 128 KB (`MMAP_THRESHOLD`), bypassing the sbrk heap and its global lock for large blocks.
7. Build system now uses `pkg-config` for cmocka discovery, outputs artefacts to `build/`, and compiles with AddressSanitizer + full warning flags (`-Wall -Wextra -Werror -Wpedantic` and more).

---

## Problems Faced During Updates

### 1. Linked list corruption when implementing split block
- `next`/`prev` pointers were being overwritten incorrectly.
- Duplicate assignments and unsafe pointer usage broke list structure.

### 2. Free/reuse state was inconsistent
- Blocks were marked allocated too early and too often.
- Split blocks were reused when still in an inconsistent state.
- `find_free_block()` sometimes saw invalid `free` metadata.

### 3. `find_free_block()` relied on broken assumptions
- Assumed list structure was always valid.
- Skipped or misread blocks because metadata and structure were out of sync.

### 4. Coalescing never reliably triggered
- Adjacent free blocks were not consistently both marked `FREE`.
- List corruption meant blocks that should merge were never detected as adjacent.

### 5. Memory reuse failed due to stale/incorrect metadata
- Freed blocks were not reliably reusable.
- Allocator sometimes treated valid free blocks as invalid.

### 6. Double state updates in `my_malloc()`
- Free flags were set multiple times in the wrong order.
- Splitting and marking allocation happened in a conflicting order.

### 7. Structural and logical state were mixed incorrectly
- Pointer structure (`next`/`prev`) and allocation state (`free`/allocated) were not kept in sync.
- Operations like split, free, and coalesce partially updated one but not the other.

### 8. Heap invariants were not stable across operations
- After a few alloc/free cycles, the list no longer represented a consistent heap.

---

## License

This project is for educational purposes.
