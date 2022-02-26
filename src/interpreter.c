#include "errno.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/wait.h"
#include "unistd.h"

#include "include/builtin.h"
#include "include/interpreter.h"

int print_working_directory() {
  char buf[1024];
  if (getcwd(buf, 1024) == NULL) {
    return errno;
  }
  if (puts(buf) < 0) {
    return errno;
  }
  return 0;
}

int change_directory(char *dir) {
  if (chdir(dir) < 0) {
    return errno;
  }
  return 0;
}

int launch_command(char *name, char **argv) {
  if (execvp(name, argv) == -1) {
    return errno;
  }
  return 0;
}

typedef enum RunnableType {
  RUNNABLE_COMMAND,
  RUNNABLE_CD,
  RUNNABLE_PWD,
} RunnableType;

typedef struct RunnableDataCommand {
  char *name;
  char **argv;
} RunnableDataCommand;

typedef union RunnableData {
  RunnableDataCommand command;
  char *cd;
} RunnableData;

typedef struct Runnable {
  RunnableType type;
  RunnableData data;
} Runnable;

int runnable_run(Runnable r) {
  switch (r.type) {
  case RUNNABLE_COMMAND: {
    return launch_command(r.data.command.name, r.data.command.argv);
  }
  case RUNNABLE_PWD: {
    return print_working_directory();
  }
  case RUNNABLE_CD: {
    return change_directory(r.data.cd);
  }
  }
  return 0;
}

typedef struct ProcessHandle {
  pid_t pid;
  int err_fd;
} ProcessHandle;

typedef struct ProcessHandleBuf {
  ProcessHandle *buf;
  size_t count;
  size_t capacity;
} ProcessHandleBuf;

const size_t PROCESS_HANDLE_BUF_START_CAPACITY = 2;

ProcessHandleBuf *process_handle_buf_init() {
  ProcessHandleBuf *out = malloc(sizeof(ProcessHandleBuf));
  if (out == NULL) {
    panic("interpreter: failed to allocate");
  }
  out->count = 0;
  out->capacity = PROCESS_HANDLE_BUF_START_CAPACITY;
  out->buf = malloc(out->capacity * sizeof(ProcessHandle));
  if (out->buf == NULL) {
    panic("interpreter: failed to allocate");
  }
  return out;
}

void process_handle_buf_reset(ProcessHandleBuf *buf) {
  buf->count = 0;
}

void process_handle_buf_free(ProcessHandleBuf *buf) {
  free(buf->buf);
  free(buf);
}

void process_handle_buf_push(ProcessHandleBuf *buf, ProcessHandle handle) {
  size_t required = buf->count + 1;
  while (buf->capacity < required) {
    buf->capacity *= 2;
    buf->buf = realloc(buf->buf, buf->capacity * sizeof(ProcessHandle));
  }
  buf->buf[buf->count++] = handle;
}

Error launch(Runnable r, ProcessHandle *handle_out, int redirect_stdout,
             int redirect_stdin) {
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

    if (redirect_stdout != -1) {
      dup2(redirect_stdout, fileno(stdout));
    }
    if (redirect_stdin != -1) {
      dup2(redirect_stdin, fileno(stdin));
    }

    int err_out = runnable_run(r);
    if (err_out != 0) {
      write(err_pipe[1], &err_out, sizeof(int));
      close(err_pipe[1]);
      abort();
    }
    exit(0);
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
  ProcessHandleBuf *process_buf;

  char **argv_buf;
  size_t argv_buf_capacity;
  int last_pipe_fd;
};

Interpreter *interpreter_init(StringArena *arena) {
  Interpreter *out = malloc(sizeof(Interpreter));
  if (out == NULL) {
    panic("interpreter_init: failed to allocate memory");
  }
  out->arena = arena;
  out->string_stack = string_stack_init();
  out->process_buf = process_handle_buf_init();
  out->argv_buf = NULL;
  out->argv_buf_capacity = 0;
  out->last_pipe_fd = -1;
  return out;
}

void interpreter_free(Interpreter *interpreter) {
  string_stack_free(interpreter->string_stack);
  process_handle_buf_free(interpreter->process_buf);
  free(interpreter->argv_buf);
  free(interpreter);
}

Error interpreter_runnable(Interpreter *interpreter, Runnable r, OpFlag flag) {
  int fd;
  if (flag & OP_FLAG_REDIRECT) {
    StringHandle file_h = string_stack_pop(interpreter->string_stack);
    char *file = string_arena_get_str(interpreter->arena, file_h);

    Error err = redirect_stdout(file, &fd);
    if (err.type != ERROR_NONE) {
      return err;
    }
  }

  int redirect_stdin = -1;
  if (flag & OP_FLAG_CONTINUE_PIPE) {
    redirect_stdin = interpreter->last_pipe_fd;
  }
  int redirect_stdout = -1;
  int pipe_fd[2] = {-1, -1};
  if (flag & OP_FLAG_START_PIPE) {
    if (pipe(pipe_fd) < 0) {
      return error_from_errno(errno);
    }
    redirect_stdout = pipe_fd[1];
    interpreter->last_pipe_fd = pipe_fd[0];
  }

  ProcessHandle handle;
  Error err = launch(r, &handle, redirect_stdout, redirect_stdin);
  if (redirect_stdin != -1) {
    close(redirect_stdin);
  }
  // Close the write end of the pipe
  if (pipe_fd[1] != -1) {
    close(pipe_fd[1]);
  }
  if (flag & OP_FLAG_REDIRECT) {
    restore_stdout(fd);
  }
  if (err.type != ERROR_NONE) {
    return err;
  }
  process_handle_buf_push(interpreter->process_buf, handle);
  return (Error){ERROR_NONE};
}

Error interpreter_builtin(Interpreter *interpreter __attribute__((unused)),
                          OpFlag flag, Builtin builtin) {
  Runnable r;
  switch (builtin) {
  case BUILTIN_PWD: {
    r = (Runnable){.type = RUNNABLE_PWD};
    break;
  }
  // We don't use a runnable for CD, since we have no output.
  case BUILTIN_CD: {
    if (string_stack_size(interpreter->string_stack) < 1) {
      return (Error){ERROR_INTERPRETER,
                     {.intepreter_error = INTERPRETER_ERROR_EMPTY_STACK}};
    }
    StringHandle dir_h = string_stack_pop(interpreter->string_stack);
    char *dir = string_arena_get_str(interpreter->arena, dir_h);

    int err = change_directory(dir);
    if (err != 0) {
      return error_from_errno(err);
    }
    return (Error){ERROR_NONE};
  }
  }
  return interpreter_runnable(interpreter, r, flag);
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

  Runnable r = {.type = RUNNABLE_COMMAND,
                .data = {.command = {name, interpreter->argv_buf}}};

  return interpreter_runnable(interpreter, r, flag);
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

Error interpreter_wait(Interpreter *interpreter) {
  for (size_t i = 0; i < interpreter->process_buf->count; i++) {
    Error err = wait_on_handle(interpreter->process_buf->buf + i);
    if (err.type != ERROR_NONE) {
      return err;
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
  return interpreter_wait(interpreter);
}

void interpreter_reset(Interpreter *interpreter) {
  string_stack_reset(interpreter->string_stack);
  process_handle_buf_reset(interpreter->process_buf);
}
