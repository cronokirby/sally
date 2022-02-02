#include "include/parser.h"

#include "stddef.h"

void ast_free(ASTNode *node) {
  for (size_t i = 0; i < node->count; i++) {
    ast_free(node->data.children + i);
  }
  if (node->count > 0) {
    free(node->data.children);
  }
}
