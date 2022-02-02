#pragma once

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

typedef enum ErrorType {
  ERROR_NONE,
  ERROR_LEXER,
  ERROR_PARSER,
  ERROR_UNIX
} ErrorType;

typedef enum LexerError { LEXER_ERROR_UNKNOWN_INPUT } LexerError;

char const *lexer_error_str(LexerError err);

typedef enum ParserError { PARSER_ERROR_UNEXPECTED_TOKEN } ParserError;

char const *parser_error_Str(ParserError err);

typedef union ErrorData {
  LexerError lexer_error;
  ParserError parser_error;
  int errnum;
} ErrorData;

typedef struct Error {
  ErrorType type;
  ErrorData data;
} Error;

inline Error error_from_errno(int errnum) {
  return (Error){ERROR_UNIX, {.errnum = errnum}};
}

char const *error_str(Error err);

/// panic exits the program immediately with an error.
///
/// The intention is for errors from which no recovery is possible, such as
/// failures to allocate memory.
inline void panic(char const *str) {
  fputs(str, stderr);
  exit(1);
}
