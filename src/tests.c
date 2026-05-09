#include "../src/mem_alloc.h"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define LARGE_ALLOC (128 * 1024 + 100)

void test_basic_malloc() {
  printf("test_basic_malloc...\n");

  void *ptr = my_malloc(64);

  assert(ptr != NULL);

  memset(ptr, 0xAA, 64);

  my_free(ptr);

  printf("PASS\n");
}

void test_zero_malloc() {
  printf("test_zero_malloc...\n");

  void *ptr = my_malloc(0);

  assert(ptr == NULL);

  printf("PASS\n");
}

void test_alignment() {
  printf("test_alignment...\n");

  void *ptr1 = my_malloc(1);
  void *ptr2 = my_malloc(13);
  void *ptr3 = my_malloc(24);

  assert(((uintptr_t)ptr1 % 8) == 0);
  assert(((uintptr_t)ptr2 % 8) == 0);
  assert(((uintptr_t)ptr3 % 8) == 0);

  my_free(ptr1);
  my_free(ptr2);
  my_free(ptr3);

  printf("PASS\n");
}

void test_block_reuse() {
  printf("test_block_reuse...\n");

  void *ptr1 = my_malloc(64);

  my_free(ptr1);

  void *ptr2 = my_malloc(64);

  assert(ptr1 == ptr2);

  my_free(ptr2);

  printf("PASS\n");
}

void test_split_block() {
  printf("test_split_block...\n");

  void *big = my_malloc(256);

  my_free(big);

  void *small = my_malloc(64);

  assert(small == big);

  my_free(small);

  printf("PASS\n");
}

void test_coalesce() {
  printf("test_coalesce...\n");

  void *a = my_malloc(64);
  void *b = my_malloc(64);

  my_free(a);
  my_free(b);

  /*
    After coalescing:
    allocator should reuse merged space
  */

  void *large = my_malloc(120);

  assert(large == a);

  my_free(large);

  printf("PASS\n");
}

void test_realloc_grow() {
  printf("test_realloc_grow...\n");

  char *ptr = my_malloc(16);

  strcpy(ptr, "hello");

  ptr = my_realloc(ptr, 128);

  assert(ptr != NULL);

  assert(strcmp(ptr, "hello") == 0);

  my_free(ptr);

  printf("PASS\n");
}

void test_realloc_shrink() {
  printf("test_realloc_shrink...\n");

  char *ptr = my_malloc(128);

  strcpy(ptr, "allocator");

  char *old = ptr;

  ptr = my_realloc(ptr, 32);

  assert(ptr == old);

  assert(strcmp(ptr, "allocator") == 0);

  my_free(ptr);

  printf("PASS\n");
}

void test_realloc_null() {
  printf("test_realloc_null...\n");

  void *ptr = my_realloc(NULL, 64);

  assert(ptr != NULL);

  my_free(ptr);

  printf("PASS\n");
}

void test_realloc_zero() {
  printf("test_realloc_zero...\n");

  void *ptr = my_malloc(64);

  ptr = my_realloc(ptr, 0);

  assert(ptr == NULL);

  printf("PASS\n");
}

void test_calloc() {
  printf("test_calloc...\n");

  int *arr = my_calloc(10, sizeof(int));

  assert(arr != NULL);

  for (int i = 0; i < 10; i++) {
    assert(arr[i] == 0);
  }

  my_free(arr);

  printf("PASS\n");
}

void test_large_mmap_alloc() {
  printf("test_large_mmap_alloc...\n");

  void *ptr = my_malloc(LARGE_ALLOC);

  assert(ptr != NULL);

  memset(ptr, 0xCC, LARGE_ALLOC);

  my_free(ptr);

  printf("PASS\n");
}

void test_many_allocations() {
  printf("test_many_allocations...\n");

  void *ptrs[1000];

  for (int i = 0; i < 1000; i++) {
    ptrs[i] = my_malloc(32);

    assert(ptrs[i] != NULL);
  }

  for (int i = 0; i < 1000; i++) {
    my_free(ptrs[i]);
  }

  printf("PASS\n");
}

void *thread_func(void *arg) {

  for (int i = 0; i < 10000; i++) {

    void *ptr = my_malloc(64);

    assert(ptr != NULL);

    memset(ptr, 0xAB, 64);

    my_free(ptr);
  }

  return NULL;
}

void test_multithreaded() {
  printf("test_multithreaded...\n");

  pthread_t t1, t2, t3, t4;

  pthread_create(&t1, NULL, thread_func, NULL);
  pthread_create(&t2, NULL, thread_func, NULL);
  pthread_create(&t3, NULL, thread_func, NULL);
  pthread_create(&t4, NULL, thread_func, NULL);

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);
  pthread_join(t3, NULL);
  pthread_join(t4, NULL);

  printf("PASS\n");
}

int main() {

  test_basic_malloc();

  test_zero_malloc();

  test_alignment();

  test_block_reuse();

  test_split_block();

  test_coalesce();

  test_realloc_grow();

  test_realloc_shrink();

  test_realloc_null();

  test_realloc_zero();

  test_calloc();

  test_large_mmap_alloc();

  test_many_allocations();

  test_multithreaded();

  printf("\nALL TESTS PASSED\n");

  return 0;
}
