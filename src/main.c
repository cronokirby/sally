#include "assert.h"
#include "ctype.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "unistd.h"

typedef enum Error {
  ERROR_NONE,
  ERROR_LEXER_UNKNOWN_INPUT,
} Error;

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

/// Represents all of the builtin commands we have
typedef enum Builtin {
  // A builtin which prints the current directory
  BUILTIN_PWD
} Builtin;

typedef enum TokenType {
  /// Represents a builtin command
  TOKEN_BUILTIN,
  /// Represents the end of the input stream
  TOKEN_EOF
} TokenType;

typedef union TokenData {
  /// A builtin command
  Builtin builtin;
} TokenData;

typedef struct Token {
  /// The discriminator tag for this type.
  TokenType type;
  /// The data associated with this type, if any.
  TokenData data;
} Token;

typedef struct Lexer {
  char const *input;
  size_t index;
} Lexer;

Lexer lexer_init(char const *input) {
  assert(input != NULL);
  Lexer ret = {.input = input, .index = 0};
  return ret;
}

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
        return ERROR_LEXER_UNKNOWN_INPUT;
      }
    }
    return ERROR_NONE;
  }
}

/// The number of bytes in our line buffer.
const size_t LINE_BUFFER_SIZE = (1 << 14);

// The prompt to display in the shell.
const char *PROMPT = ">> ";

Error print_working_directory() {
  char buf[1024];
  getcwd(buf, 1024);
  puts(buf);
  return ERROR_NONE;
}

Error handle_builtin(Builtin builtin) {
  switch (builtin) {
  case BUILTIN_PWD: {
    return print_working_directory();
  }
  default:
    assert(false);
  }
  return ERROR_NONE;
}

void handle_line(char const *line) {
  Lexer lexer = lexer_init(line);

  Token token;
  Error err = lexer_next(&lexer, &token);
  if (err != ERROR_NONE) {
    printf("Error: %d\n", err);
    return;
  }

  switch (token.type) {
  case TOKEN_BUILTIN: {
    if ((err = handle_builtin(token.data.builtin)) != ERROR_NONE) {
      printf("Error: %d\n", err);
      return;
    }
    break;
  }
  case TOKEN_EOF: {
    puts("eof");
    break;
  }
  }
}

int main() {
  char line_buffer[LINE_BUFFER_SIZE];

  for (;;) {
    fputs(PROMPT, stdout);
    if (fgets(line_buffer, LINE_BUFFER_SIZE, stdin) == NULL) {
      if (feof(stdin)) {
        break;
      }
      perror("Error reading line:");
      continue;
    }
    handle_line(line_buffer);
  }
}
