#pragma once

#include "string.h"

typedef enum ErrorType { ERROR_NONE, ERROR_LEXER, ERROR_UNIX } ErrorType;

typedef enum LexerError { LEXER_ERROR_UNKNOWN_INPUT } LexerError;

char const *lexer_error_str(LexerError err);

typedef union ErrorData {
  LexerError lexer_error;
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
