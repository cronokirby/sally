#include "assert.h"
#include "errno.h"
#include "stdbool.h"
#include "stdio.h"

#include "include/builtin.h"
#include "include/error.h"
#include "include/interpreter.h"
#include "include/lexer.h"
#include "include/parser.h"

/// The number of bytes in our line buffer.
const size_t LINE_BUFFER_SIZE = (1 << 14);

// The prompt to display in the shell.
const char *PROMPT = ">> ";

Error handle_line(Interpreter *interpreter, char const *line) {
  interpreter_reset(interpreter);

  Error error = (Error){ERROR_NONE};

  Lexer lexer = lexer_init(line);
  Parser *parser = parser_init(&lexer);

  ASTNode node;
  error = parser_parse(parser, &node);
  if (error.type != ERROR_NONE) {
    goto err;
  }

  error = interpreter_run(interpreter, &node);

err:
  free(parser);
  return error;
}

int main() {
  char line_buffer[LINE_BUFFER_SIZE];
  Interpreter *interpreter = interpreter_init();

  for (;;) {
    fputs(PROMPT, stdout);
    if (fgets(line_buffer, LINE_BUFFER_SIZE, stdin) == NULL) {
      if (feof(stdin)) {
        break;
      }
      perror("Error reading line:");
      continue;
    }
    Error error = handle_line(interpreter, line_buffer);
    if (error.type != ERROR_NONE) {
      fputs(error_str(error), stderr);
      fputc('\n', stderr);
    }
  }

  interpreter_free(interpreter);
}
