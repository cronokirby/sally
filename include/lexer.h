#pragma once

#include "assert.h"
#include "stddef.h"

#include "include/builtin.h"
#include "include/error.h"
#include "include/string_arena.h"
typedef enum TokenType {
  /// Represents a builtin command
  TOKEN_BUILTIN,
  /// Represents some other kind of word.
  ///
  /// These are used as the arguments to builtin commands, or to represent
  /// the invocation of binaries, etc.
  TOKEN_WORD,
  /// The token `>`.
  TOKEN_ANGLE_RIGHT,
  /// The token `|`
  TOKEN_PIPE,
  /// Represents the end of the input stream
  TOKEN_EOF
} TokenType;

typedef union TokenData {
  /// A builtin command
  Builtin builtin;
  /// A handle containing some string data, allocated in an arena.
  StringHandle string;
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
  StringArena *arena;
} Lexer;

inline Lexer lexer_init(char const *input, StringArena *arena) {
  assert(input != NULL);
  Lexer ret = {.input = input, .index = 0, .arena = arena};
  return ret;
}

Error lexer_next(Lexer *lexer, Token *out);
