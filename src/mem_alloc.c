#include "mem_alloc.h"
#include <assert.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define META_SIZE sizeof(struct block_header)
#define BLOCK_FREE 0
#define BLOCK_ALLOCATED 1
#define MIN_BLOCK_SIZE 8

pthread_mutex_t global_malloc_lock =
    PTHREAD_MUTEX_INITIALIZER; // Mutex lock for thread safety

void *global_base = NULL; // head of the linked list

block_header *find_free_block(block_header **last, size_t size);
block_header *request_space(block_header *last, size_t size);
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t size);
void *my_calloc(size_t no_of_elements, size_t size_of_elements);

block_header *find_free_block(block_header **last, size_t size) {
  block_header *current = global_base;

  while (current && !(current->free && current->size >= size)) {
    *last = current;         // this becomes the new head of the linked list
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
  block->free = BLOCK_ALLOCATED;
  block->debug = 0x12345678;

  return block;
}

void *my_malloc(size_t size) {
  block_header *block;
  if (size <= 0) {
    return NULL;
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
      block->free = BLOCK_ALLOCATED;
      block->debug = 0x7777777;
    }
  }
  pthread_mutex_unlock(&global_malloc_lock);
  return (block + 1); // return pointer after metadata
}

block_header *get_block_addr(void *ptr) { return (block_header *)ptr - 1; }

void my_free(void *ptr) {
  if (!ptr) {
    return;
  }
  pthread_mutex_lock(&global_malloc_lock);
  block_header *block_ptr = get_block_addr(ptr);
  if (block_ptr->free != BLOCK_ALLOCATED) {
    pthread_mutex_unlock(&global_malloc_lock);
    return;
  }
  assert(block_ptr->debug == 0x7777777 || block_ptr->debug == 0x12345678);
  block_ptr->free = BLOCK_FREE;
  block_ptr->debug = 0x55555555;
  pthread_mutex_unlock(&global_malloc_lock);
}

void *my_realloc(void *ptr, size_t size) {
  if (!ptr) {
    return my_malloc(size);
  }
  pthread_mutex_lock(&global_malloc_lock);
  block_header *block_ptr = get_block_addr(ptr);
  if (block_ptr->size >= size) {
    size_t remaining = block_ptr->size - size;

    // Only split if the remainder can hold a new block
    if (remaining >= META_SIZE + MIN_BLOCK_SIZE) {
      block_header *new_block =
          (block_header *)((char *)(block_ptr + 1) +
                           size); // this line generate an error due to void*

      new_block->size = remaining - META_SIZE;
      new_block->free = BLOCK_FREE;
      new_block->next = block_ptr->next;
      new_block->debug = 0xDEADBEEF;

      block_ptr->size = size;
      block_ptr->next = new_block;
    }

    pthread_mutex_unlock(&global_malloc_lock);
    return ptr;
  }

  void *new_ptr;
  pthread_mutex_unlock(&global_malloc_lock);
  new_ptr = my_malloc(size);
  if (!new_ptr) {
    return NULL;
  }
  memcpy(new_ptr, ptr, block_ptr->size);
  my_free(ptr);
  return new_ptr;
}

void *my_calloc(size_t no_of_elements, size_t size_of_elements) {
  size_t size = no_of_elements * size_of_elements;
  if (no_of_elements != 0 && size_of_elements > SIZE_MAX / no_of_elements) {
    return NULL; // Return NULL to signal failure
  }
  void *ptr = my_malloc(size);
  if (ptr != NULL) {
    memset(ptr, 0, size);
  }
  return ptr;
}
