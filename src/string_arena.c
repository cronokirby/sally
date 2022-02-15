#include "assert.h"
#include "stdlib.h"
#include "string.h"

#include "include/error.h"
#include "include/string_arena.h"


extern inline int stringslice_cmp_str(StringSlice slice, char const *str);

struct StringArena {
  char *buffer;
  size_t start;
  size_t size;
};

const size_t STRING_ARENA_DEFAULT_SIZE = 1 << 14;

void string_arena_init(StringArena *arena) {
  arena->buffer = malloc(STRING_ARENA_DEFAULT_SIZE);
  if (arena->buffer == NULL) {
    panic("Fatal: Failed to allocate memory");
  }
  arena->start = 0;
  arena->size = STRING_ARENA_DEFAULT_SIZE;
}

void string_arena_free(StringArena *arena) {
  free(arena->buffer);
  free(arena);
}

void string_arena_resize(StringArena *arena, size_t min) {
  size_t new_size = arena->size;
  while (new_size < min) {
    new_size *= 2;
  }
  arena->buffer = realloc(arena->buffer, new_size);
  if (arena->buffer == NULL) {
    panic("Fatal: Failed to allocate memory");
  }
  arena->size = new_size;
}

StringHandle string_arena_alloc(StringArena *arena, StringSlice slice) {
  size_t old_start = arena->start;
  size_t remaining = arena->size - old_start;
  size_t required = slice.len + 1;
  if (remaining < required) {
    string_arena_resize(arena, arena->size + required);
  }
  // Make sure to add a zero after the string
  memcpy(arena->buffer + old_start, slice.data, slice.len);
  arena->buffer[old_start + slice.len] = 0;
  arena->start += required;
  return old_start;
}

char *string_arena_get_str(StringArena *arena, StringHandle handle) {
  assert(handle < arena->size);
  return arena->buffer + handle;
}
