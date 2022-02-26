#include "errno.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/wait.h"
#include "unistd.h"

#include "include/builtin.h"
#include "include/interpreter.h"

typedef struct ProcessHandle {
  pid_t pid;
  int err_fd;
} ProcessHandle;

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

Error launch(char const *name, char *const *argv, ProcessHandle *handle_out) {
  if (fflush(stdout) == -1) {
    return error_from_errno(errno);
  }

  int err_pipe[2];
  if (pipe(err_pipe) == -1) {
    return error_from_errno(errno);
  }
  if (fcntl(err_pipe[1], F_SETFD, fcntl(err_pipe[1], F_GETFD) | FD_CLOEXEC) ==
      -1) {
    return error_from_errno(errno);
  }
  pid_t pid = fork();
  if (pid == -1) {
    return error_from_errno(errno);
  }
  if (pid == 0) {
    close(err_pipe[0]);
    if (execvp(name, argv) == -1) {
      write(err_pipe[1], &errno, sizeof(int));
      close(err_pipe[1]);
      abort();
    }
  } else {
    close(err_pipe[1]);
    handle_out->pid = pid;
    handle_out->err_fd = err_pipe[0];
  }
  return (Error){ERROR_NONE};
}

Error wait_on_handle(ProcessHandle *handle) {
  int exec_err;
  int count;
  for (count = -1; count == -1;
       count = read(handle->err_fd, &exec_err, sizeof(int))) {
    if (errno == EAGAIN || errno == EINTR) {
      continue;
    }
  }
  close(handle->err_fd);
  if (wait(&handle->pid) == -1) {
    return error_from_errno(errno);
  }
  if (count > 0) {
    return error_from_errno(exec_err);
  }
  return (Error){ERROR_NONE};
}

Error redirect_stdout(char *file_name, int *fd_out) {
  FILE *fp = fopen(file_name, "w");
  if (fp == NULL) {
    return error_from_errno(errno);
  }

  Error err;

  fflush(stdout);
  *fd_out = dup(fileno(stdout));
  if (*fd_out == -1) {
    err = error_from_errno(errno);
    goto err0;
  }

  if (dup2(fileno(fp), fileno(stdout)) < 0) {
    err = error_from_errno(errno);
    goto err1;
  }

  fclose(fp);
  return (Error){ERROR_NONE};

err1:
  close(*fd_out);
err0:
  fclose(fp);
  return err;
}

void restore_stdout(int fd) {
  fflush(stdout);
  dup2(fd, fileno(stdout));
  close(fd);
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

void string_stack_reset(StringStack *stack) {
  stack->head = 0;
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

  char **argv_buf;
  size_t argv_buf_capacity;
};

Interpreter *interpreter_init(StringArena *arena) {
  Interpreter *out = malloc(sizeof(Interpreter));
  if (out == NULL) {
    panic("interpreter_init: failed to allocate memory");
  }
  out->arena = arena;
  out->string_stack = string_stack_init();
  out->argv_buf = NULL;
  out->argv_buf_capacity = 0;
  return out;
}

void interpreter_free(Interpreter *interpreter) {
  string_stack_free(interpreter->string_stack);
  free(interpreter->argv_buf);
  free(interpreter);
}

Error interpreter_builtin(Interpreter *interpreter __attribute__((unused)),
                          OpFlag flag, Builtin builtin) {
  switch (builtin) {
  case BUILTIN_PWD: {
    Error err;

    int fd;
    if (flag & OP_FLAG_REDIRECT) {
      StringHandle file_h = string_stack_pop(interpreter->string_stack);
      char *file = string_arena_get_str(interpreter->arena, file_h);
      err = redirect_stdout(file, &fd);
      if (err.type != ERROR_NONE) {
        return err;
      }
    }

    err = print_working_directory();

    if (flag & OP_FLAG_REDIRECT) {
      restore_stdout(fd);
    }

    return err;
  }
  case BUILTIN_CD: {
    if (string_stack_size(interpreter->string_stack) < 1) {
      return (Error){ERROR_INTERPRETER,
                     {.intepreter_error = INTERPRETER_ERROR_EMPTY_STACK}};
    }
    StringHandle dir_h = string_stack_pop(interpreter->string_stack);
    char *dir = string_arena_get_str(interpreter->arena, dir_h);

    int fd;
    if (flag & OP_FLAG_REDIRECT) {
      StringHandle file_h = string_stack_pop(interpreter->string_stack);
      char *file = string_arena_get_str(interpreter->arena, file_h);

      Error err = redirect_stdout(file, &fd);
      if (err.type != ERROR_NONE) {
        return err;
      }
    }

    Error err = change_directory(dir);

    if (flag & OP_FLAG_REDIRECT) {
      restore_stdout(fd);
    }
    return err;
  }
  }
  return (Error){ERROR_NONE};
}

Error interpreter_command(Interpreter *interpreter __attribute__((unused)),
                          OpFlag flag, char *name, size_t arg_count) {
  if (arg_count + 2 > interpreter->argv_buf_capacity) {
    interpreter->argv_buf_capacity *= 2;
    interpreter->argv_buf =
        realloc(interpreter->argv_buf, (arg_count + 2) * sizeof(char *));
  }
  interpreter->argv_buf[0] = name;
  for (size_t i = 1; i < arg_count + 1; ++i) {
    StringHandle handle = string_stack_pop(interpreter->string_stack);
    interpreter->argv_buf[i] = string_arena_get_str(interpreter->arena, handle);
  }
  interpreter->argv_buf[arg_count + 1] = NULL;

  int fd;
  if (flag & OP_FLAG_REDIRECT) {
    StringHandle file_h = string_stack_pop(interpreter->string_stack);
    char *file = string_arena_get_str(interpreter->arena, file_h);

    Error err = redirect_stdout(file, &fd);
    if (err.type != ERROR_NONE) {
      return err;
    }
  }

  ProcessHandle handle;
  Error err = launch(name, interpreter->argv_buf, &handle);
  if (flag & OP_FLAG_REDIRECT) {
    restore_stdout(fd);
  }
  if (err.type != ERROR_NONE) {
    return err;
  }
  return wait_on_handle(&handle);
}

Error interpreter_op(Interpreter *interpreter, Op op) {
  switch (op.type) {
  case OP_BUILTIN: {
    return interpreter_builtin(interpreter, op.flag, op.data.builtin);
  }
  case OP_STRING: {
    string_stack_push(interpreter->string_stack, op.data.string);
    break;
  }
  case OP_COMMAND: {
    char *name = string_arena_get_str(interpreter->arena, op.data.command.name);
    return interpreter_command(interpreter, op.flag, name,
                               op.data.command.arg_count);
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

void interpreter_reset(Interpreter *interpreter) {
  string_stack_reset(interpreter->string_stack);
}
