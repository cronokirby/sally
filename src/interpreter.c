#include "errno.h"
#include "unistd.h"

#include "include/builtin.h"
#include "include/interpreter.h"

Error print_working_directory() {
  char buf[1024];
  if (getcwd(buf, 1024) == NULL) {
    return error_from_errno(errno);
  }
  if (puts(buf) < 0) {
    return error_from_errno(errno);
  }
  return (Error){ERROR_NONE};
}

struct Interpreter {};

Interpreter *interpreter_init() {
  Interpreter *out = malloc(sizeof(Interpreter));
  if (out == NULL) {
    panic("interpreter_init: failed to allocate memory");
  }
  return out;
}

void interpreter_free(Interpreter *interpreter) {
  free(interpreter);
}

Error interpreter_builtin(Interpreter *interpreter __attribute__((unused)),
                          Builtin builtin) {
  switch (builtin) {
  case BUILTIN_PWD: {
    return print_working_directory();
  }
  }
  return (Error){ERROR_NONE};
}

Error interpreter_run(Interpreter *interpreter, ASTNode *node) {
  switch (node->type) {
  case AST_BUILTIN: {
    return interpreter_builtin(interpreter, node->builtin);
  }
  }
  return (Error){ERROR_NONE};
}

void interpreter_reset(Interpreter *interpreter __attribute__((unused))) {
}
