/// Standalone ouo interpreter

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ouo.h"

#define REPL_LINE_SIZE 1024

static void start_repl() {
  char line[REPL_LINE_SIZE];
  for (;;) {
    printf("ouo> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    ouo_hi(line);
  }
}

static char *read_file(const char *path) {
  errno = 0;
  FILE *file = fopen(path, "rb");
  ouo_assertf(file != NULL, OUO_ERR_FILE_NOT_READ, "%s: %s.", path,
              strerror(errno));

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(file_size + 1);
  ouo_assertf(buffer != NULL, OUO_ERR_OUT_OF_MEMORY,
              "%s: Cannot allocate memory.", path);

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  ouo_assertf(bytes_read == file_size, OUO_ERR_FILE_NOT_READ, "%s: %s.", path,
              strerror(errno));

  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}

static void run_file(const char *path) {
  char *src = read_file(path);
  ouo_hi(src);
}

int main(int argc, const char **argv) {
  ouo_assertf(argc <= 2, OUO_ERR_INCORRECT_USAGE, "Usage: ouo [PATH]");

  if (argc == 1) {
    start_repl();
  } else if (argc == 2) {
    run_file(argv[1]);
  }

  return OUO_OK;
}
