#include "stdint.h"
#include "stdio.h"

/// The number of bytes in our line buffer.
const size_t LINE_BUFFER_SIZE = (1 << 14);

// The prompt to display in the shell.
const char *PROMPT = ">> ";

void handle_line(char const *line) {
  fputs(line, stdout);
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
