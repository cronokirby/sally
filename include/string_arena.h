#pragma once

#include "stddef.h"

/// A view to a portion of a string.
///
/// This isn't null-terminated, unlike a C string.
typedef struct StringSlice {
  char const *data;
  size_t len;
} StringSlice;

/// Compare a string slice with a null-terminated string.
inline int stringslice_cmp_str(StringSlice slice, char const *str) {
  for (size_t i = 0; i < slice.len && str[i] != 0; ++i) {
    if (slice.data[i] > str[i]) {
      return 1;
    }
    if (slice.data[i] < str[i]) {
      return -1;
    }
  }
  return 0;
}

/// An opaque handle to a string allocated inside the arena.
///
/// We use handles instead of pointers, to avoid stale pointers
/// if the arena memory is relocated.
typedef size_t StringHandle;

/// An arena used to allocate strings.
///
/// Better than individual mallocs, most likely.
typedef struct StringArena StringArena;

/// Initialize a string arena.
StringArena* string_arena_init();

/// Free the memory of a string arena, including the arena itself.
void string_arena_free(StringArena *arena);

/// Allocate a new slice permanently in the arena.
///
/// This returns a handle, which can be exchanged for a short-lived C string
/// with `string_arena_get_str`.
StringHandle string_arena_alloc(StringArena *arena, StringSlice slice);

/// Fetch the null-terminated string associated with a handle.
///
/// This string is only guaranteed to be valid until the next
/// allocation.
char *string_arena_get_str(StringArena *arena, StringHandle handle);
