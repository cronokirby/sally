#include "include/parser.h"

#include "stdbool.h"
#include "stddef.h"

void ast_free(ASTNode *node) {
  for (size_t i = 0; i < node->count; i++) {
    ast_free(node->data.children + i);
  }
  if (node->count > 0) {
    free(node->data.children);
  }
}

struct Parser {
  Lexer *lexer;
  Token peek;
  bool has_peek;
  Token prev;
};

void parser_init(Parser *parser, Lexer *lexer) {
  parser->lexer = lexer;
  parser->has_peek = false;
}

Error parse_peek(Parser *parser, Token *out) {
  if (!parser->has_peek) {
    Error err = lexer_next(&parser->lexer, &parser->peek);
    if (err.type != ERROR_NONE) {
      return err;
    }
    parser->has_peek = true;
  }
  *out = parser->peek;
  return (Error){ERROR_NONE};
}

void parse_advance(Parser *parser) {
  parser->has_peek = false;
  parser->prev = parser->peek;
}

Error parse_check(Parser *parser, TokenType type, bool *out) {
  Token peek;
  Error err = parse_peek(parser, &peek);
  if (err.type != ERROR_NONE) {
    return err;
  }
  *out = peek.type == type;
  return (Error){ERROR_NONE};
}

Error parse_consume(Parser *parser, TokenType type) {
  bool success;
  Error err = parse_check(parser, type, &success);
  if (err.type != ERROR_NONE) {
    return err;
  }
  if (!success) {
    return (Error){ERROR_PARSER,
                   {.parser_error = PARSER_ERROR_UNEXPECTED_TOKEN}};
  }
  parse_advance(parser);
  return (Error){ERROR_NONE};
}

Error parser_parse(Parser *parser, ASTNode *out) {
  Error err = parse_consume(parser, TOKEN_BUILTIN);
  if (err.type != ERROR_NONE) {
    return err;
  }

  out->type = AST_BUILTIN;
  out->count = 0;
  out->builtin = parser->prev.data.builtin;

  return (Error){ERROR_NONE};
}
