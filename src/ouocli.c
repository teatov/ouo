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
  if (file == NULL) {
    fprintf(stderr, "%s: %s.\n", path, strerror(errno));
    exit(OUO_ERR_FILE_NOT_READ);
  }

  fseek(file, 0L, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char *buffer = (char *)malloc(file_size + 1);
  if (buffer == NULL) {
    fprintf(stderr, "%s: Cannot allocate memory.\n", path);
    exit(OUO_ERR_OUT_OF_MEMORY);
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "%s: %s.\n", path, strerror(errno));
    exit(OUO_ERR_FILE_NOT_READ);
  }

  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}

static void run_file(const char *path) {
  char *src = read_file(path);
  ouo_hi(src);
}

int main(int argc, const char **argv) {
  if (argc == 1) {
    start_repl();
  } else if (argc == 2) {
    run_file(argv[1]);
  } else {
    fprintf(stderr, "Usage: ouo [path]\n");
    exit(OUO_ERR_INCORRECT_USAGE);
  }

  return OUO_OK;
}
