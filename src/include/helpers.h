#ifndef FERRO_LANG_HELPERS
#define FERRO_LANG_HELPERS

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Generic Vector definition using macros
#define Vector(T)                                                              \
  struct {                                                                     \
    T *data;         /* Pointer to the data array */                           \
    size_t length;   /* Current number of elements */                          \
    size_t capacity; /* Total capacity of the vector */                        \
  }

// Function to resize the vector's internal storage.
#define vec_resize(T, vec, new_capacity)                                       \
  do {                                                                         \
    T *new_data = (T *)realloc((vec)->data, sizeof(T) * (new_capacity));       \
    if (!new_data) {                                                           \
      fprintf(stderr, "Memory allocation failed during vector resize.\n");     \
      exit(1);                                                                 \
    }                                                                          \
    (vec)->data = new_data;                                                    \
    (vec)->capacity = new_capacity;                                            \
  } while (0)

// Function to push an element to the back of the vector.
#define vec_push(T, vec, value)                                                \
  do {                                                                         \
    if ((vec)->length == (vec)->capacity) {                                    \
      if ((vec)->capacity == 0) {                                              \
        vec_resize(T, vec, 1); /* Start with a capacity of 1 */                \
      } else {                                                                 \
        vec_resize(T, vec, (vec)->capacity * 2);                               \
      }                                                                        \
    }                                                                          \
    (vec)->data[(vec)->length++] = (value);                                    \
  } while (0)

// Function to free the memory allocated by the vector.
#define vec_free(T, vec)                                                       \
  do {                                                                         \
    free((vec)->data);                                                         \
    (vec)->data = NULL;                                                        \
    (vec)->length = 0;                                                         \
    (vec)->capacity = 0;                                                       \
  } while (0)

// Function to initialize the vector.
#define vec_init(T, vec)                                                       \
  do {                                                                         \
    (vec)->data = NULL;                                                        \
    (vec)->length = 0;                                                         \
    (vec)->capacity = 0;                                                       \
  } while (0)

#endif
