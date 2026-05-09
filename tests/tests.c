#include <setjmp.h> // jmp_buf
#include <stdarg.h> // va_list
#include <stddef.h> // MUST come first (size_t)

#include <cmocka.h>
#include <stdint.h>
#include <string.h>

#include "../src/mem_alloc.h"

// External globals from allocator
extern void *global_base;

// Helpers
static int reset_allocator(void **state) {
  (void)state;
  global_base = NULL;
  return 0;
}

/*
 * Test: malloc with size 0 returns NULL
 */
static void test_malloc_zero_returns_null(void **state) {
  (void)state;

  void *ptr = my_malloc(0);

  assert_null(ptr);
}

/*
 * Test: malloc allocates memory successfully
 */
static void test_malloc_basic(void **state) {
  (void)state;

  void *ptr = my_malloc(32);

  assert_non_null(ptr);

  memset(ptr, 0xAA, 32);

  my_free(ptr);
}

/*
 * Test: calloc initializes memory to zero
 */
static void test_calloc_zero_initialized(void **state) {
  (void)state;

  int *arr = (int *)my_calloc(10, sizeof(int));

  assert_non_null(arr);

  for (int i = 0; i < 10; i++) {
    assert_int_equal(arr[i], 0);
  }

  my_free(arr);
}

/*
 * Test: calloc overflow protection
 */
static void test_calloc_overflow(void **state) {
  (void)state;

  void *ptr = my_calloc(SIZE_MAX, SIZE_MAX);

  assert_null(ptr);
}

/*
 * Test: free(NULL) should be safe
 */
static void test_free_null_safe(void **state) {
  (void)state;

  my_free(NULL);

  assert_true(1);
}

/*
 * Test: realloc with NULL behaves like malloc
 */
static void test_realloc_null_behaves_like_malloc(void **state) {
  (void)state;

  char *ptr = (char *)my_realloc(NULL, 64);

  assert_non_null(ptr);

  strcpy(ptr, "hello");

  assert_string_equal(ptr, "hello");

  my_free(ptr);
}

/*
 * Test: realloc with size 0 frees memory and returns NULL
 */
static void test_realloc_zero_size(void **state) {
  (void)state;

  char *ptr = (char *)my_malloc(64);

  assert_non_null(ptr);

  void *new_ptr = my_realloc(ptr, 0);

  assert_null(new_ptr);
}

/*
 * Test: realloc expands block and preserves data
 */
static void test_realloc_expand_preserves_data(void **state) {
  (void)state;

  char *ptr = (char *)my_malloc(16);

  assert_non_null(ptr);

  strcpy(ptr, "allocator");

  char *new_ptr = (char *)my_realloc(ptr, 128);

  assert_non_null(new_ptr);

  assert_string_equal(new_ptr, "allocator");

  my_free(new_ptr);
}

/*
 * Test: realloc shrink preserves data
 */
static void test_realloc_shrink_preserves_data(void **state) {
  (void)state;

  char *ptr = (char *)my_malloc(128);

  assert_non_null(ptr);

  strcpy(ptr, "cmocka-test");

  char *new_ptr = (char *)my_realloc(ptr, 16);

  assert_non_null(new_ptr);

  assert_string_equal(new_ptr, "cmocka-test");

  my_free(new_ptr);
}

/*
 * Test: allocated memory is aligned to 8 bytes
 */
static void test_malloc_alignment(void **state) {
  (void)state;

  void *ptr = my_malloc(13);

  assert_non_null(ptr);

  assert_int_equal(((uintptr_t)ptr % 8), 0);

  my_free(ptr);
}

/*
 * Test: large allocation should use mmap
 */
static void test_large_allocation_mmap(void **state) {
  (void)state;

  size_t big_size = 200 * 1024;

  void *ptr = my_malloc(big_size);

  assert_non_null(ptr);

  block_header *header = get_block_addr(ptr);

  assert_int_equal(header->is_mmap, 1);

  my_free(ptr);
}

/*
 * Test: freeing a block marks it free
 */
static void test_free_marks_block_free(void **state) {
  (void)state;

  void *ptr = my_malloc(64);

  assert_non_null(ptr);

  block_header *header = get_block_addr(ptr);

  assert_int_equal(header->free, 1);

  my_free(ptr);

  assert_int_equal(header->free, 0);
}

/*
 * Test: allocator reuses freed block
 */
static void test_reuse_freed_block(void **state) {
  (void)state;

  void *ptr1 = my_malloc(64);

  assert_non_null(ptr1);

  my_free(ptr1);

  void *ptr2 = my_malloc(32);

  assert_non_null(ptr2);

  // allocator should reuse same block
  assert_ptr_equal(ptr1, ptr2);

  my_free(ptr2);
}

/*
 * Test: split block creates a second free block
 */
static void test_split_block(void **state) {
  (void)state;

  void *ptr1 = my_malloc(128);

  assert_non_null(ptr1);

  my_free(ptr1);

  void *ptr2 = my_malloc(32);

  assert_non_null(ptr2);

  block_header *header = get_block_addr(ptr2);

  assert_non_null(header->next);
  assert_int_equal(header->next->free, 0);

  my_free(ptr2);
}

/*
 * Test: coalescing adjacent free blocks
 */
static void test_coalesce_free_blocks(void **state) {
  (void)state;

  void *a = my_malloc(64);
  void *b = my_malloc(64);

  assert_non_null(a);
  assert_non_null(b);

  my_free(a);
  my_free(b);

  block_header *header = get_block_addr(a);

  // after coalescing size should grow
  assert_true(header->size >= (64 + 64));

  // cleanup not required since merged
}

/*
 * Test: multiple allocations are unique
 */
static void test_multiple_allocations_unique(void **state) {
  (void)state;

  void *a = my_malloc(32);
  void *b = my_malloc(32);
  void *c = my_malloc(32);

  assert_non_null(a);
  assert_non_null(b);
  assert_non_null(c);

  assert_ptr_not_equal(a, b);
  assert_ptr_not_equal(b, c);
  assert_ptr_not_equal(a, c);

  my_free(a);
  my_free(b);
  my_free(c);
}

int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test_setup(test_malloc_zero_returns_null, reset_allocator),
      cmocka_unit_test_setup(test_malloc_basic, reset_allocator),
      cmocka_unit_test_setup(test_calloc_zero_initialized, reset_allocator),
      cmocka_unit_test_setup(test_calloc_overflow, reset_allocator),
      cmocka_unit_test_setup(test_free_null_safe, reset_allocator),
      cmocka_unit_test_setup(test_realloc_null_behaves_like_malloc,
                             reset_allocator),
      cmocka_unit_test_setup(test_realloc_zero_size, reset_allocator),
      cmocka_unit_test_setup(test_realloc_expand_preserves_data,
                             reset_allocator),
      cmocka_unit_test_setup(test_realloc_shrink_preserves_data,
                             reset_allocator),
      cmocka_unit_test_setup(test_malloc_alignment, reset_allocator),
      cmocka_unit_test_setup(test_large_allocation_mmap, reset_allocator),
      cmocka_unit_test_setup(test_free_marks_block_free, reset_allocator),
      cmocka_unit_test_setup(test_reuse_freed_block, reset_allocator),
      cmocka_unit_test_setup(test_split_block, reset_allocator),
      cmocka_unit_test_setup(test_coalesce_free_blocks, reset_allocator),
      cmocka_unit_test_setup(test_multiple_allocations_unique, reset_allocator),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
