#include "mem_alloc.h"
#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define META_SIZE sizeof(struct block_header)
#define BLOCK_FREE 0
#define BLOCK_ALLOCATED 1
#define MMAP_THRESHOLD (128 * 1024)
#define MIN_BLOCK_SIZE 8
#define ALIGN8(x)                                                              \
  (((x) + 7) & ~7) // Alignment of addresses that is valid for the CPU to use
                   // for data types

pthread_mutex_t global_malloc_lock =
    PTHREAD_MUTEX_INITIALIZER; // Mutex lock for thread safety

void *global_base = NULL; // head of the linked list

block_header *find_free_block(block_header **last, size_t size) {
  block_header *current = global_base;

  while (current && !(current->free && current->size >= size)) {
    *last =
        current; // this points to the very last valid block of the linked list
    current = current->next; // immediate next block
  }

  return current;
}

block_header *request_space(block_header *last, size_t size) {
  block_header *block;
  // block = sbrk(0);
  block = sbrk(size + META_SIZE);

  if (block == (void *)-1) {
    puts("sbrk() failed!!");
    return NULL;
  }

  // assert((void *)block == block);
  if (last) {
    last->next = block;
  }

  block->size = size;
  block->next = NULL;
  block->is_mmap = 0;
  block->free = BLOCK_ALLOCATED;
  block->debug = 0x12345678;

  return block;
}

// mmap allocation helper
block_header *request_space_mmap(size_t size) {
  size_t total_size = size + META_SIZE; // get the total size to be allocated
  block_header *block =
      mmap(NULL, total_size, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0); // allocate block with mmap
  if (block == MAP_FAILED) {
    perror("mmap failed");
    return NULL;
  }
  block->size = size;
  block->next = NULL;
  block->free = BLOCK_ALLOCATED;
  block->is_mmap = 1;
  block->debug = 0xABCDEF;

  return block;
}

// take one free block and split it into an allocated block and a remaining free
// block
void split_block(block_header *block, size_t size) {
  size_t remaining = block->size - size;

  // not enough room for another block
  if (remaining < META_SIZE + MIN_BLOCK_SIZE) {
    return;
  }

  // get the range (start - end) of the remaining free block
  block_header *new_block = (block_header *)((char *)(block + 1) + size);

  // Initialize the new free block
  new_block->size = remaining - META_SIZE;
  new_block->free = BLOCK_FREE;
  new_block->is_mmap = 0;
  new_block->debug =
      0xDEADBEEF; // just a debug marker which helps to detect corruption
  new_block->next = block->next; // link to the list
  block->size = size;            // shrink the original
  block->next = new_block;
}

void *my_malloc(size_t size) {
  block_header *block;

  if (size == 0) {
    return NULL;
  }
  size = ALIGN8(size);
  // using mmap if size is greater than mmap() threshold
  if (size >= MMAP_THRESHOLD) {

    block = request_space_mmap(size);

    if (!block) {
      return NULL;
    }
    return (block + 1);
  }

  pthread_mutex_lock(&global_malloc_lock);

  if (!global_base) {
    block = request_space(NULL, size);
    if (!block) {
      pthread_mutex_unlock(&global_malloc_lock);
      return NULL;
    }
    global_base = block;
  } else {
    block_header *last = global_base;
    block = find_free_block(&last, size);
    // If a suitable block is not found request new block
    if (!block) {
      block = request_space(last, size);
      if (!block) {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
      }
    } else {
      split_block(block, size);
    }
    block->free = BLOCK_ALLOCATED;
  }
  block->debug = 0x7777777;
  pthread_mutex_unlock(&global_malloc_lock);
  return (block ? (block + 1) : NULL); // return pointer after metadata
}

block_header *get_block_addr(void *ptr) { return (block_header *)ptr - 1; }

void coalesce_free_blocks() {
  block_header *current = global_base; // head of the linked list
  while (current && current->next) {
    char *end_of_current =
        (char *)(current + 1) + current->size; // to check for adjacency
    // check if the current and next block are free
    if (current->free == BLOCK_FREE && current->next->free == BLOCK_FREE &&
        end_of_current == (char *)current->next) {
      // Merge current free block with the next free block
      current->size += META_SIZE + current->next->size;
      current->next = current->next->next; // because blocks have been merged
      continue; // stay on current block to allow repeated merges
    }
    current = current->next;
  }
}

void my_free(void *ptr) {
  if (!ptr) {
    return;
  }
  pthread_mutex_lock(&global_malloc_lock);
  block_header *block_ptr = get_block_addr(ptr);
  if (block_ptr->is_mmap) {
    size_t total_size = block_ptr->size + META_SIZE;
    pthread_mutex_unlock(&global_malloc_lock);
    munmap(block_ptr, total_size);
    return;
  }
  assert(block_ptr->free == BLOCK_ALLOCATED);
  assert(block_ptr->debug == 0x7777777 || block_ptr->debug == 0x12345678);
  
  block_ptr->free = BLOCK_FREE;
  block_ptr->debug = 0x55555555;

  // merge adjacent free blocks
  coalesce_free_blocks();
  pthread_mutex_unlock(&global_malloc_lock);
}

void *my_realloc(void *ptr, size_t size) {
  size = ALIGN8(size);
  if (!ptr) {
    return my_malloc(size);
  }
  if (size == 0) {
    my_free(ptr);
    return NULL;
  }
  pthread_mutex_lock(&global_malloc_lock);
  block_header *block_ptr = get_block_addr(ptr);
  if (block_ptr->is_mmap) {

    pthread_mutex_unlock(&global_malloc_lock);

    if (block_ptr->size >= size) {
      return ptr;
    }

    void *new_ptr = my_malloc(size);

    if (!new_ptr) {
      return NULL;
    }

    memcpy(new_ptr, ptr, block_ptr->size);

    my_free(ptr);

    return new_ptr;
  }

  if (block_ptr->size >= size) {
    size_t remaining = block_ptr->size - size;

    // Only split if the remainder can hold a new block
    if (remaining >= META_SIZE + MIN_BLOCK_SIZE) {
      block_header *new_block =
          (block_header *)((char *)(block_ptr + 1) +
                           size); // this line generate an error due to void*

      new_block->size = remaining - META_SIZE;
      new_block->free = BLOCK_FREE;
      new_block->is_mmap = 0;
      new_block->next = block_ptr->next;
      new_block->debug = 0xDEADBEEF;

      block_ptr->size = size;
      block_ptr->next = new_block;
    }

    pthread_mutex_unlock(&global_malloc_lock);
    return ptr;
  }

  void *new_ptr;
  size_t old_size = block_ptr->size;
  pthread_mutex_unlock(&global_malloc_lock);
  new_ptr = my_malloc(size);
  if (!new_ptr) {
    return NULL;
  }
  memcpy(new_ptr, ptr, old_size);
  my_free(ptr);
  return new_ptr;
}

void *my_calloc(size_t no_of_elements, size_t size_of_elements) {
  if (no_of_elements != 0 && size_of_elements > SIZE_MAX / no_of_elements) {
    return NULL; // Return NULL to signal failure
  }
  size_t size = no_of_elements * size_of_elements;
  void *ptr = my_malloc(size);
  if (ptr != NULL) {
    memset(ptr, 0, size);
  }
  return ptr;
}
