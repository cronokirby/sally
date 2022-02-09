#pragma once

#include "include/compiler.h"
#include "include/error.h"
#include "include/parser.h"

/// Represents an interpreter running shell programs.
typedef struct Interpreter Interpreter;

/// Initialize a new interpreter.
///
/// The result can be freed with interpreter_free()
Interpreter *interpreter_init();

/// Free the memory of this interpreter, and the data inside.
void interpreter_free(Interpreter *interpreter);

/// Run the interpreter on a given ast node.
///
/// The interpreter should be reset between different runs.
Error interpreter_run(Interpreter *interpreter, OpBuffer *buf);

/// Reset the state of the interpreter.
///
/// We use resetting, instead of merely creating a new interpreter, in order
/// to reuse memory between runs.
void interpreter_reset(Interpreter *interpreter);
