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
  case BUILTIN_CD: {
    return (Error){ERROR_NONE};
  }
  }
  return (Error){ERROR_NONE};
}

Error interpreter_op(Interpreter *interpreter, Op op) {
  switch (op.type) {
  case OP_BUILTIN: {
    return interpreter_builtin(interpreter, op.data.builtin);
  }
  case OP_STRING: {
    puts("STRING");
    break;
  }
  }
  return (Error){ERROR_NONE};
}

Error interpreter_run(Interpreter *interpreter, OpBuffer *buf) {
  for (size_t i = 0; i < buf->len; ++i) {
    printf("%ld %d\n", i, buf->ops[i].type);
    Error err = interpreter_op(interpreter, buf->ops[i]);
    if (err.type != ERROR_NONE) {
      return err;
    }
  }
  return (Error){ERROR_NONE};
}

void interpreter_reset(Interpreter *interpreter __attribute__((unused))) {
}
