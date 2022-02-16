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

const size_t DEFAULT_CHILD_COUNT = 4;

struct Parser {
  Lexer *lexer;
  Token peek;
  bool has_peek;
  Token prev;
};

Parser *parser_init(Lexer *lexer) {
  Parser *out = malloc(sizeof(Parser));
  if (out == NULL) {
    panic("parser_init: failed to allocate memory");
  }
  out->lexer = lexer;
  out->has_peek = false;
  return out;
}

void parser_free(Parser *parser) {
  free(parser);
}

Error parse_peek(Parser *parser, Token *out) {
  if (!parser->has_peek) {
    Error err = lexer_next(parser->lexer, &parser->peek);
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

Error parse_arg(Parser *parser, ASTNode *out) {
  Error err = parse_consume(parser, TOKEN_WORD);
  if (err.type != ERROR_NONE) {
    return err;
  }
  out->type = AST_ARG;
  out->count = 0;
  out->data.string = parser->prev.data.string;

  return (Error){ERROR_NONE};
}

Error parser_parse(Parser *parser, ASTNode *out) {
  // Just initialize this so that the node can be freed even if we error.
  out->count = 0;

  Error err = parse_consume(parser, TOKEN_BUILTIN);
  if (err.type != ERROR_NONE) {
    return err;
  }

  out->type = AST_BUILTIN;
  out->count = 0;
  out->builtin = parser->prev.data.builtin;
  size_t capacity = DEFAULT_CHILD_COUNT;
  out->data.children = malloc(capacity * sizeof(ASTNode));

  // Parse a list of arguments until we see EOF.
  bool is_eof;
  for (;;) {
    err = parse_check(parser, TOKEN_EOF, &is_eof);
    if (err.type != ERROR_NONE) {
      return err;
    }
    if (is_eof) {
      break;
    }
    size_t next_count = out->count + 1;
    while (next_count > capacity) {
      capacity *= 2;
      out->data.children =
          realloc(out->data.children, capacity * sizeof(ASTNode));
    }
    err = parse_arg(parser, out->data.children + out->count);
    if (err.type != ERROR_NONE) {
      return err;
    }
    out->count = next_count;
  }

  return (Error){ERROR_NONE};
}
