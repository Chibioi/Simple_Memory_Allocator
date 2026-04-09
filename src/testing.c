#include "mem_alloc.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  char *data;
} String;

String *create(char *data) {
  String *new = my_malloc(sizeof(String));
  if (!new) {
    puts("Failed to allocate memory for the String");
    return NULL;
  }

  new->data = data;
  return new;
}

int main() {
  printf("--- Test 1: Basic Allocation ---\n");
  String *res = create("Stanley Ezeh");
  if (!res)
    return EXIT_FAILURE;

  printf("Allocated String struct at: %p\n", (void *)res);
  printf("Content: %s\n", res->data);
  printf("%zu bytes\n", sizeof(String));

  // Capture the address before freeing
  void *old_ptr = (void *)res;

  printf("\n--- Test 2: Free and Reuse ---\n");
  my_free(res);

  // This second allocation should (ideally) reuse the same memory block
  String *res2 = create("New Data");
  printf("Second allocation at: %p\n", (void *)res2);

  if ((void *)res2 == old_ptr) {
    printf("SUCCESS: Memory block was reused!\n");
  } else {
    printf("NOTE: New block created (check find_free_block logic).\n");
  }

  my_free(res2);
  return EXIT_SUCCESS;
}
