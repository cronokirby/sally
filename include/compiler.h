#pragma once

#include "include/builtin.h"
#include "include/error.h"
#include "include/parser.h"

/// The different kinds of operations in our bytecode.
typedef enum OpType { OP_BUILTIN } OpType;

/// The variants of data held in a bytecode operation.
typedef union OpData {
  Builtin builtin;
} OpData;

/// Represents a single operation in our bytecode.
///
/// We have a stack-based language, and operations represent individual
/// manipulations of that stack, and its environment.
typedef struct Op {
  OpType type;
  OpData data;
} Op;

/// Represents a linear buffer of operations.
typedef struct OpBuffer {
  Op *ops;
  size_t len;
  size_t capacity;
} OpBuffer;

/// Allocate memory for a new OpBuffer.
///
/// The result of this operation should be freed with op_buffer_free.
OpBuffer *op_buffer_init();

/// Reset an OpBuffer, making it empty, but reusing its memory.
void op_buffer_reset(OpBuffer *buf);

/// Free the memory of an Opbuffer, including the pointer itself.
void op_buffer_free(OpBuffer *buf);

/// compile a syntax tree into a linear buffer of stack operations.
Error compile(ASTNode *input, OpBuffer *out);
