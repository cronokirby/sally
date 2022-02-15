#include "sys/types.h"

#include "include/compiler.h"

void op_buffer_push(OpBuffer *buf, Op op) {
  size_t required = buf->len + 1;
  if (required > buf->capacity) {
    size_t new_capacity = buf->capacity;
    while (new_capacity < required) {
      new_capacity *= 2;
    }
    Op *new_ops = realloc(buf->ops, sizeof(Op) * new_capacity);
    if (new_ops == NULL) {
      panic("compiler: failed to allocate memory for opcodes");
    }
    buf->capacity = new_capacity;
  }
  buf->ops[buf->len++] = op;
}

const size_t OP_BUFFER_START_SIZE = 2048;

OpBuffer *op_buffer_init() {
  OpBuffer *out = malloc(sizeof(OpBuffer));
  out->ops = malloc(sizeof(Op) * OP_BUFFER_START_SIZE);
  if (out->ops == NULL) {
    panic("op_buffer_init: failed to allocate memory");
  }
  out->len = 0;
  out->capacity = OP_BUFFER_START_SIZE;

  return out;
}

void op_buffer_reset(OpBuffer *buf) {
  buf->len = 0;
}

void op_buffer_free(OpBuffer *buf) {
  free(buf->ops);
  free(buf);
}

Error handle_node(ASTNode *input, OpBuffer *out) {
  switch (input->type) {
  case AST_BUILTIN: {
    for (ssize_t i = input->count - 1; i >= 0; i--) {
      Error err = handle_node(input->data.children + i, out);
      if (err.type != ERROR_NONE) {
        return err;
      }
    }
    op_buffer_push(out, (Op){OP_BUILTIN, {.builtin = input->builtin}});
    break;
  }
  case AST_ARG: {
    op_buffer_push(out, (Op){OP_STRING, {.string = input->data.string}});
    break;
  }
  }
  return (Error){ERROR_NONE};
}

Error compile(ASTNode *input, OpBuffer *out) {
  op_buffer_init(out);

  return handle_node(input, out);
}
