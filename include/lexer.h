#pragma once

#include "assert.h"
#include "stddef.h"

#include "include/builtin.h"
#include "include/error.h"

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

inline Lexer lexer_init(char const *input) {
  assert(input != NULL);
  Lexer ret = {.input = input, .index = 0};
  return ret;
}

Error lexer_next(Lexer *lexer, Token *out);
