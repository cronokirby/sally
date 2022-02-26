#include "ctype.h"

#include "include/lexer.h"

extern Lexer lexer_init(char const *input, StringArena *arena);

Error lexer_next(Lexer *lexer, Token *out) {
  out->type = TOKEN_EOF;
  // We always return, unless we continue
  for (;;) {
    char next = lexer->input[lexer->index];
    if (next == 0) {
      out->type = TOKEN_EOF;
    } else if (next == '>') {
      out->type = TOKEN_ANGLE_RIGHT;
      lexer->index++;
    } else if (next == '|') {
      out->type = TOKEN_PIPE;
      lexer->index++;
    } else if (isspace(next)) {
      lexer->index++;
      continue;
    } else {
      // Simplest to just assume that everything else starts a word
      size_t start = lexer->index;
      for (; next != 0 && !isspace(next); next = lexer->input[++lexer->index]) {
      }
      size_t len = lexer->index - start;

      StringSlice slice = {.data = lexer->input + start, .len = len};

      if (stringslice_cmp_str(slice, "pwd") == 0) {
        out->type = TOKEN_BUILTIN;
        out->data.builtin = BUILTIN_PWD;
      } else if (stringslice_cmp_str(slice, "cd") == 0) {
        out->type = TOKEN_BUILTIN;
        out->data.builtin = BUILTIN_CD;
      } else {
        StringHandle handle = string_arena_alloc(lexer->arena, slice);
        out->type = TOKEN_WORD;
        out->data.string = handle;
      }
    }
    return (Error){ERROR_NONE};
  }
}
