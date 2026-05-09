#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stddef.h>

// block block_header - provides info about the block
typedef struct block_header {
  size_t size;               // 8 bytes
  struct block_header *next; // 8 bytes
  unsigned int free;    // 4 bytes  // designating 0 as free and 1 as allocated
  unsigned int is_mmap; // 4 bytes
  int debug;            // 4 bytes
} block_header;

block_header *find_free_block(block_header **last, size_t size);
block_header *request_space(block_header *last, size_t size);
block_header *request_space_mmap(size_t size);
void coalesce_free_blocks();
void split_block(block_header *block, size_t size);
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t size);
void *my_calloc(size_t no_of_elements, size_t size_of_elements);

#endif
