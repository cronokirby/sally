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

Error change_directory(char *dir) {
  if (chdir(dir) < 0) {
    return error_from_errno(errno);
  }
  return (Error){ERROR_NONE};
}

const size_t STRING_STACK_START_CAPACITY = 32;

typedef struct StringStack {
  StringHandle *buf;
  size_t head;
  size_t capacity;
} StringStack;

StringStack *string_stack_init() {
  StringStack *out = malloc(sizeof(StringStack));
  if (out == NULL) {
    panic("interpreter: failed to allocate");
  }
  out->head = 0;
  out->capacity = STRING_STACK_START_CAPACITY;
  out->buf = malloc(out->capacity * sizeof(StringHandle));
  if (out->buf == NULL) {
    panic("interpreter: failed to allocate");
  }
  return out;
}

void string_stack_free(StringStack *stack) {
  free(stack->buf);
  free(stack);
}

size_t string_stack_size(StringStack *stack) {
  return stack->head;
}

StringHandle string_stack_pop(StringStack *stack) {
  return stack->buf[--stack->head];
}

void string_stack_push(StringStack *stack, StringHandle string) {
  size_t required = stack->head + 1;
  while (stack->capacity < required) {
    stack->capacity *= 2;
    stack->buf = realloc(stack->buf, stack->capacity * sizeof(StringHandle));
  }
  stack->buf[stack->head++] = string;
}

struct Interpreter {
  StringArena *arena;
  StringStack *string_stack;
};

Interpreter *interpreter_init(StringArena *arena) {
  Interpreter *out = malloc(sizeof(Interpreter));
  if (out == NULL) {
    panic("interpreter_init: failed to allocate memory");
  }
  out->arena = arena;
  out->string_stack = string_stack_init();
  return out;
}

void interpreter_free(Interpreter *interpreter) {
  string_stack_free(interpreter->string_stack);
  free(interpreter);
}

Error interpreter_builtin(Interpreter *interpreter __attribute__((unused)),
                          Builtin builtin) {
  switch (builtin) {
  case BUILTIN_PWD: {
    return print_working_directory();
  }
  case BUILTIN_CD: {
    if (string_stack_size(interpreter->string_stack) < 1) {
      return (Error){ERROR_INTERPRETER,
                     {.intepreter_error = INTERPRETER_ERROR_EMPTY_STACK}};
    }
    StringHandle dir_h = string_stack_pop(interpreter->string_stack);
    char *dir = string_arena_get_str(interpreter->arena, dir_h);
    return change_directory(dir);
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
    string_stack_push(interpreter->string_stack, op.data.string);
    break;
  }
  }
  return (Error){ERROR_NONE};
}

Error interpreter_run(Interpreter *interpreter, OpBuffer *buf) {
  for (size_t i = 0; i < buf->len; ++i) {
    Error err = interpreter_op(interpreter, buf->ops[i]);
    if (err.type != ERROR_NONE) {
      return err;
    }
  }
  return (Error){ERROR_NONE};
}

void interpreter_reset(Interpreter *interpreter __attribute__((unused))) {
}
