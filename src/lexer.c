#include "ctype.h"

#include "include/lexer.h"

typedef struct CharSlice {
  char const *data;
  size_t len;
} CharSlice;

int charslice_cmp_str(CharSlice slice, char const *str) {
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

extern Lexer lexer_init(char const *input);

Error lexer_next(Lexer *lexer, Token *out) {
  out->type = TOKEN_EOF;
  // We always return, unless we continue
  for (;;) {
    char next = lexer->input[lexer->index];
    if (next == 0) {
      out->type = TOKEN_EOF;
    } else if (isspace(next)) {
      lexer->index++;
      continue;
    } else {
      // Simplest to just assume that everything else starts a word
      size_t start = lexer->index;
      for (; next != 0 && !isspace(next); next = lexer->input[++lexer->index]) {
      }
      size_t len = lexer->index - start;

      CharSlice slice = {.data = lexer->input + start, .len = len};

      if (charslice_cmp_str(slice, "pwd") == 0) {
        out->type = TOKEN_BUILTIN;
        out->data.builtin = BUILTIN_PWD;
      } else {
        return (Error){ERROR_LEXER, {.lexer_error = LEXER_ERROR_UNKNOWN_INPUT}};
      }
    }
    return (Error){ERROR_NONE};
  }
}
