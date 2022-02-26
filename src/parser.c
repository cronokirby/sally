#include "include/parser.h"

#include "stdbool.h"
#include "stddef.h"

void ast_free(ASTNode *node) {
  for (size_t i = 0; i < node->count; i++) {
    ast_free(node->children + i);
  }
  if (node->count > 0) {
    free(node->children);
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

Error parse_command(Parser *parser, ASTNode *out) {
  Error err;

  Token peek;
  if ((err = parse_peek(parser, &peek)).type != ERROR_NONE) {
    return err;
  }

  switch (peek.type) {
  case TOKEN_BUILTIN: {
    parse_advance(parser);

    out->type = AST_BUILTIN;
    out->builtin = peek.data.builtin;
    break;
  }
  case TOKEN_WORD: {
    parse_advance(parser);

    out->type = AST_COMMAND;
    out->data.string = peek.data.string;
    break;
  }
  default: {
    return (Error){ERROR_PARSER,
                   {.parser_error = PARSER_ERROR_UNEXPECTED_TOKEN}};
  }
  }

  out->count = 0;
  size_t capacity = DEFAULT_CHILD_COUNT;
  out->children = malloc(capacity * sizeof(ASTNode));

  // Parse a list of arguments while we see words.
  bool is_word;
  for (;;) {
    err = parse_check(parser, TOKEN_WORD, &is_word);
    if (err.type != ERROR_NONE) {
      return err;
    }
    if (!is_word) {
      break;
    }
    size_t next_count = out->count + 1;
    while (next_count > capacity) {
      capacity *= 2;
      out->children = realloc(out->children, capacity * sizeof(ASTNode));
    }
    err = parse_arg(parser, out->children + out->count);
    if (err.type != ERROR_NONE) {
      return err;
    }
    out->count = next_count;
  }

  // If we see a `>`, then we know that there's a redirection, and expect an
  // arg.
  bool is_angle_right;
  err = parse_check(parser, TOKEN_ANGLE_RIGHT, &is_angle_right);
  if (err.type != ERROR_NONE) {
    return err;
  }
  if (!is_angle_right) {
    return (Error){ERROR_NONE};
  }
  parse_advance(parser);

  // A bit of annoying book-keeping. We've already written a node for the
  // command, but now that node needs to become one of two children.
  ASTNode *children = malloc(2 * sizeof(ASTNode));
  memcpy(children, out, sizeof(ASTNode));
  err = parse_arg(parser, children + 1);
  if (err.type != ERROR_NONE) {
    return err;
  }

  out->type = AST_REDIRECT;
  out->count = 2;
  out->children = children;

  return (Error){ERROR_NONE};
}

Error parse_pipes(Parser *parser, ASTNode *out) {
  size_t capacity = 2;
  size_t count = 0;
  ASTNode *children = malloc(capacity * sizeof(ASTNode));

  Error err = parse_command(parser, children + count);
  if (err.type != ERROR_NONE) {
    return err;
  }
  count++;

  bool is_pipe;
  for (;;) {
    err = parse_check(parser, TOKEN_PIPE, &is_pipe);
    if (err.type != ERROR_NONE) {
      return err;
    }
    if (!is_pipe) {
      break;
    }
    parse_advance(parser);

    size_t required = count + 1;
    while (capacity < required) {
      capacity *= 2;
      children = realloc(children, capacity * sizeof(ASTNode));
    }

    err = parse_command(parser, children + count);
    if (err.type != ERROR_NONE) {
      return err;
    }
    count++;
  }

  if (count <= 1) {
    memcpy(out, children, sizeof(ASTNode));
    free(children);
    return (Error){ERROR_NONE};
  }

  out->type = AST_PIPE;
  out->count = count;
  out->children = children;

  return (Error){ERROR_NONE};
}

Error parser_parse(Parser *parser, ASTNode *out) {
  // Just initialize this so that the node can be freed even if we error.
  out->count = 0;
  return parse_pipes(parser, out);
}
