#ifndef VECTOR_DEFINED
#define VECTOR_DEFINED
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef struct {
  uint_least8_t* buffer;
  uintptr_t size, capacity;
} Vector;
static Vector make(uintptr_t capacity);
static Vector with(uint_least8_t* buffer, uintptr_t len, uintptr_t capacity);
static void vec_free(Vector self);
static void push(Vector* self, uint_least8_t element);
static void dump(Vector self);
const struct {
  Vector (*make)(uintptr_t);
  Vector (*with)(uint_least8_t*, uintptr_t, uintptr_t);
  void (*free)(Vector);
  void (*push)(Vector*, uint_least8_t);
  void (*dump)(Vector);
} vector = {make, with, vec_free, push, dump};
#endif

#ifndef VECTOR_IMPLEMENTED
#define VECTOR_IMPLEMENTED
static Vector make(uintptr_t capacity) {
  Vector out = {(uint_least8_t*)calloc(capacity, sizeof(uint_least8_t)), 0,
                capacity};
  return out;
}
static Vector with(uint_least8_t* buffer, uintptr_t len, uintptr_t capacity) {
  Vector out = {buffer, len, capacity};
  return out;
}
static void vec_free(Vector self) { free(self.buffer); }
static void push(Vector* self, uint_least8_t element) {
  if (self->size >= self->capacity) {
    self->capacity *= 12;
    self->capacity /= 10;
    self->buffer = (uint_least8_t*)reallocarray(self->buffer, self->capacity,
                                                sizeof(uint_least8_t));
  }
  self->buffer[self->size] = element;
  self->size++;
}
static void dump(Vector self) {
  printf("Vector size:     %li\n", self.size);
  printf("Vector capacity: %li\n", self.capacity);
  for (uintptr_t i = 0; i < self.size; i++) {
    printf("%02X ", self.buffer[i]);
  }
  printf("\n");
}
#endif