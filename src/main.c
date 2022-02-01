#include "assert.h"
#include "errno.h"
#include "stdbool.h"
#include "stdio.h"
#include "unistd.h"

#include "include/builtin.h"
#include "include/error.h"
#include "include/lexer.h"

/// The number of bytes in our line buffer.
const size_t LINE_BUFFER_SIZE = (1 << 14);

// The prompt to display in the shell.
const char *PROMPT = ">> ";

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

Error handle_builtin(Builtin builtin) {
  switch (builtin) {
  case BUILTIN_PWD: {
    return print_working_directory();
  }
  default:
    assert(false);
  }
  return (Error){ERROR_NONE};
}

void handle_line(char const *line) {
  Lexer lexer = lexer_init(line);

  Token token;
  Error err = lexer_next(&lexer, &token);
  if (err.type != ERROR_NONE) {
    printf("Error: %s\n", error_str(err));
    return;
  }

  switch (token.type) {
  case TOKEN_BUILTIN: {
    err = handle_builtin(token.data.builtin);
    if (err.type != ERROR_NONE) {
      printf("Error: %s\n", error_str(err));
      return;
    }
    break;
  }
  case TOKEN_EOF: {
    puts("eof");
    break;
  }
  }
}

int main() {
  char line_buffer[LINE_BUFFER_SIZE];

  for (;;) {
    fputs(PROMPT, stdout);
    if (fgets(line_buffer, LINE_BUFFER_SIZE, stdin) == NULL) {
      if (feof(stdin)) {
        break;
      }
      perror("Error reading line:");
      continue;
    }
    handle_line(line_buffer);
  }
}
