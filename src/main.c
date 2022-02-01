#include "assert.h"
#include "ctype.h"
#include "errno.h"
#include "error.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

typedef enum ErrorType { ERROR_NONE, ERROR_LEXER, ERROR_UNIX } ErrorType;

typedef enum LexerError { LEXER_ERROR_UNKNOWN_INPUT } LexerError;

char const *lexer_error_str(LexerError err) {
  switch (err) {
  case LEXER_ERROR_UNKNOWN_INPUT: {
    return "Lexer: unknown input";
  }
  }
  return "";
}

typedef union ErrorData {
  LexerError lexer_error;
  int errnum;
} ErrorData;

typedef struct Error {
  ErrorType type;
  ErrorData data;
} Error;

Error error_from_errno(int errnum) {
  return (Error){ERROR_UNIX, {.errnum = errnum}};
}

char const *error_str(Error err) {
  switch (err.type) {
  case ERROR_NONE: {
    return "";
  }
  case ERROR_LEXER:
    return lexer_error_str(err.data.lexer_error);
  case ERROR_UNIX: {
    return strerror(err.data.errnum);
  }
  }
  return "";
}

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
        return (Error){ERROR_LEXER, {.lexer_error = LEXER_ERROR_UNKNOWN_INPUT}};
      }
    }
    return (Error){ERROR_NONE};
  }
}

/// The number of bytes in our line buffer.
const size_t LINE_BUFFER_SIZE = (1 << 14);

// The prompt to display in the shell.
const char *PROMPT = ">> ";

Error print_working_directory() {
  char buf[1024];
  if (getcwd(buf, 1024) == NULL) {
    return error_from_errno(errno);
  }
  if (puts(buf) < 0) {
    return error_from_errno(errno);
  }
  return (Error){ERROR_NONE};
}

Error handle_builtin(Builtin builtin) {
  switch (builtin) {
  case BUILTIN_PWD: {
    return print_working_directory();
  }
  default:
    assert(false);
  }
  return (Error){ERROR_NONE};
}

void handle_line(char const *line) {
  Lexer lexer = lexer_init(line);

  Token token;
  Error err = lexer_next(&lexer, &token);
  if (err.type != ERROR_NONE) {
    printf("Error: %s\n", error_str(err));
    return;
  }

  switch (token.type) {
  case TOKEN_BUILTIN: {
    err = handle_builtin(token.data.builtin);
    if (err.type != ERROR_NONE) {
      printf("Error: %s\n", error_str(err));
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
  foo();
  return 0;
}
