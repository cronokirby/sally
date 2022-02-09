#include "assert.h"
#include "errno.h"
#include "stdbool.h"
#include "stdio.h"

#include "include/builtin.h"
#include "include/compiler.h"
#include "include/error.h"
#include "include/interpreter.h"
#include "include/lexer.h"
#include "include/parser.h"

/// The number of bytes in our line buffer.
const size_t LINE_BUFFER_SIZE = (1 << 14);

// The prompt to display in the shell.
const char *PROMPT = ">> ";

Error handle_line(Interpreter *interpreter, OpBuffer *op_buffer,
                  char const *line) {
  interpreter_reset(interpreter);

  Error error = (Error){ERROR_NONE};

  Lexer lexer = lexer_init(line);
  Parser *parser = parser_init(&lexer);

  ASTNode node;
  error = parser_parse(parser, &node);
  if (error.type != ERROR_NONE) {
    goto err;
  }

  error = compile(&node, op_buffer);
  if (error.type != ERROR_NONE) {
    goto err;
  }

  error = interpreter_run(interpreter, op_buffer);

err:
  parser_free(parser);
  return error;
}

int main() {
  char line_buffer[LINE_BUFFER_SIZE];
  Interpreter *interpreter = interpreter_init();
  OpBuffer *op_buffer = op_buffer_init();

  for (;;) {
    fputs(PROMPT, stdout);
    if (fgets(line_buffer, LINE_BUFFER_SIZE, stdin) == NULL) {
      if (feof(stdin)) {
        break;
      }
      perror("Error reading line:");
      continue;
    }
    Error error = handle_line(interpreter, op_buffer, line_buffer);
    if (error.type != ERROR_NONE) {
      fputs(error_str(error), stderr);
      fputc('\n', stderr);
    }
  }

  interpreter_free(interpreter);
  op_buffer_free(op_buffer);
}
