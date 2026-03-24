#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

// Your allocator functions
void *my_malloc(size_t size);
void my_free(void *ptr);
void *my_realloc(void *ptr, size_t size);
void *my_calloc(size_t n, size_t size);

/* ---------- Test Cases ---------- */

// Test malloc basic functionality
void test_malloc_basic(void) {
  void *ptr = my_malloc(32);
  CU_ASSERT_PTR_NOT_NULL(ptr);

  if (ptr) {
    memset(ptr, 0xAA, 32);
    unsigned char *p = (unsigned char *)ptr;
    for (int i = 0; i < 32; i++) {
      CU_ASSERT_EQUAL(p[i], 0xAA);
    }
    my_free(ptr);
  }
}

// malloc(0)
void test_malloc_zero(void) {
  void *ptr = my_malloc(0);
  CU_ASSERT_PTR_NULL(ptr);
}

// calloc initializes memory to zero
void test_calloc(void) {
  int *arr = (int *)my_calloc(10, sizeof(int));
  CU_ASSERT_PTR_NOT_NULL(arr);

  if (arr) {
    for (int i = 0; i < 10; i++) {
      CU_ASSERT_EQUAL(arr[i], 0);
    }
    my_free(arr);
  }
}

// free(NULL)
void test_free_null(void) {
  my_free(NULL);
  CU_PASS("free(NULL) did not crash");
}

// realloc growing
void test_realloc_grow(void) {
  char *ptr = (char *)my_malloc(10);
  CU_ASSERT_PTR_NOT_NULL(ptr);

  if (ptr) {
    strcpy(ptr, "hello");

    ptr = (char *)my_realloc(ptr, 20);
    CU_ASSERT_PTR_NOT_NULL(ptr);

    if (ptr) {
      CU_ASSERT_STRING_EQUAL(ptr, "hello");
      my_free(ptr);
    }
  }
}

// realloc shrinking
void test_realloc_shrink(void) {
  char *ptr = (char *)my_malloc(20);
  CU_ASSERT_PTR_NOT_NULL(ptr);

  if (ptr) {
    strcpy(ptr, "hello world");

    ptr = (char *)my_realloc(ptr, 5);
    CU_ASSERT_PTR_NOT_NULL(ptr);

    if (ptr) {
      CU_ASSERT_NSTRING_EQUAL(ptr, "hello", 5);
      my_free(ptr);
    }
  }
}

// realloc NULL behaves like malloc
void test_realloc_null(void) {
  char *ptr = (char *)my_realloc(NULL, 15);
  CU_ASSERT_PTR_NOT_NULL(ptr);

  if (ptr) {
    my_free(ptr);
  }
}

// multiple allocations
void test_multiple_allocations(void) {
  void *a = my_malloc(16);
  void *b = my_malloc(32);
  void *c = my_malloc(64);

  CU_ASSERT_PTR_NOT_NULL(a);
  CU_ASSERT_PTR_NOT_NULL(b);
  CU_ASSERT_PTR_NOT_NULL(c);

  if (a)
    my_free(a);
  if (b)
    my_free(b);
  if (c)
    my_free(c);
}

// calloc overflow protection
void test_calloc_overflow(void) {
  void *ptr = my_calloc(SIZE_MAX, 2);
  CU_ASSERT_PTR_NULL(ptr);
}

/* ---------- Test Runner ---------- */

int main() {
  if (CU_initialize_registry() != CUE_SUCCESS)
    return CU_get_error();

  CU_pSuite suite = CU_add_suite("CustomMallocSuite", NULL, NULL);

  if (!suite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  // Add tests
  CU_add_test(suite, "malloc basic", test_malloc_basic);
  CU_add_test(suite, "malloc zero", test_malloc_zero);
  CU_add_test(suite, "calloc", test_calloc);
  CU_add_test(suite, "free null", test_free_null);
  CU_add_test(suite, "realloc grow", test_realloc_grow);
  CU_add_test(suite, "realloc shrink", test_realloc_shrink);
  CU_add_test(suite, "realloc null", test_realloc_null);
  CU_add_test(suite, "multiple allocations", test_multiple_allocations);
  CU_add_test(suite, "calloc overflow", test_calloc_overflow);

  // Run tests
  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();

  CU_cleanup_registry();
  return 0;
}
