#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#include <stddef.h>

// block header - provides info about the block
typedef struct block_meta {
  size_t size;
  struct block_meta *next;
  unsigned int free; // designating 0 as free and 1 as allocated
  int debug;
} header;

header *find_free_block(header **last, size_t size);
header *request_space(header *last, size_t size);
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t size);
void *my_calloc(size_t no_of_elements, size_t size_of_elements);

#endif
