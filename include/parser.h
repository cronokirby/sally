#pragma once

#include "stdint.h"

#include "include/builtin.h"
#include "include/error.h"
#include "include/lexer.h"
#include "include/string_arena.h"

/// Represents one of the variants in our AST.
typedef enum ASTType {
  /// Represents a builtin command for our shell.
  AST_BUILTIN,
  /// Represents an arbitrary command that isn't builtin.
  AST_COMMAND,
  /// Represent an individual argument for some command.
  AST_ARG,
  /// Represents a redirection to a certain file.
  AST_REDIRECT,
  /// Represents the piping between two processes.
  AST_PIPE
} ASTType;

/// Represents one of the nodes in our AST.
typedef struct ASTNode ASTNode;

/// Represents one of the kinds of data our AST can handle.
typedef union ASTData {
  StringHandle string;
} ASTData;

struct ASTNode {
  /// The variant this node is.
  ASTType type;
  /// The number if children under this node, if relevant.
  uint64_t count;
  /// The builtin associated with this node, if any.
  Builtin builtin;
  /// The children beneath this node, if any.
  ASTNode *children;
  /// Additional data associated with this node.
  ASTData data;
};

/// Free all of the memory allocated inside of an AST.
///
/// This recursively traverses the tree to free all nodes contained inside.
void ast_free(ASTNode *node);

/// Represents a parser, which uses the tokens produced by the lexer to make an
/// AST.
typedef struct Parser Parser;

/// Initialize a parser, with a given lexer.
///
/// The result can be freed with `parser_free`.
Parser *parser_init(Lexer *lexer);

/// This frees the data inside parser, and the memory of parser as well;
void parser_free(Parser *parser);

/// Parse data, producing a full AST.
Error parser_parse(Parser *parser, ASTNode *out);
