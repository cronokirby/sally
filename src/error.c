#include "include/error.h"

char const *lexer_error_str(LexerError err) {
  switch (err) {
  case LEXER_ERROR_UNKNOWN_INPUT: {
    return "Lexer: unknown input";
  }
  }
  return "";
}

char const *parser_error_Str(ParserError err) {
  switch (err) {
  case PARSER_ERROR_UNEXPECTED_TOKEN: {
    return "Parser: unexpected token";
  }
  }
  return "";
}

extern inline Error error_from_errno(int errnum);

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

extern inline void panic(char const *str);
