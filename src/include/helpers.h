#ifndef FERRO_LANG_HELPERS
#define FERRO_LANG_HELPERS

#include <stddef.h>

// Initial capacity of a vector.
#define VECTOR_INITIAL_CAPACITY 8

// Generic Vector definition using macros
#define Vector(T)                                                              \
  struct {                                                                     \
    T *data;         /* Pointer to the data array */                           \
    size_t length;   /* Current number of elements */                          \
    size_t capacity; /* Total capacity of the vector */                        \
  }

// Function to initialize a vector.
#define vec_init(T, vec)                                                       \
  do {                                                                         \
    (vec)->length = 0;                                                         \
    (vec)->capacity = VECTOR_INITIAL_CAPACITY;                                 \
    (vec)->data = (T *)malloc(sizeof(T) * (vec)->capacity);                    \
  } while (0)

// Function to resize the vector's internal storage.
#define vec_resize(T, vec, new_capacity)                                       \
  do {                                                                         \
    (vec)->capacity = new_capacity;                                            \
    (vec)->data = (T *)realloc((vec)->data, sizeof(T) * (vec)->capacity);      \
  } while (0)

// Function to push an element to the back of the vector.
#define vec_push(T, vec, value)                                                \
  do {                                                                         \
    if ((vec)->length == (vec)->capacity) {                                    \
      vec_resize(T, vec, (vec)->capacity * 2);                                 \
    }                                                                          \
    (vec)->data[(vec)->length++] = (value);                                    \
  } while (0)

// Function to free the memory allocated by the vector.
#define vec_free(T, vec)                                                       \
  do {                                                                         \
    free((vec)->data);                                                         \
    (vec)->length = 0;                                                         \
    (vec)->capacity = 0;                                                       \
  } while (0)

#endif
