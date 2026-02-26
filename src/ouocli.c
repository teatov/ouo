/*
 * Standalone ouo interpreter
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ouo.h"

static void run(const char *src, const char *path) {
  OuoParseResult parse_res = ouo_parse(src);

  ouo_ast_print(parse_res.ast);
  printf("\n");

  if (parse_res.failed) {
    ouo_da_foreach(OuoError, err, &parse_res.errors) {
      ouo_err_print(err, src, path);
    }
    goto defer;
  }

defer:
  ouo_da_free(parse_res.errors);
  ouo_ast_free(parse_res.ast);
}

#define REPL_LINE_SIZE 1024

static void start_repl() {
  char line[REPL_LINE_SIZE];
  for (;;) {
    printf("ouo> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    run(line, NULL);
  }
}

static char *read_file(const char *path) {
  errno = 0;
  FILE *file = fopen(path, "rb");
  ouo_assertf(file != NULL, OUO_ERR_FILE_NOT_READ, "%s: %s.", path,
              strerror(errno));

  fseek(file, 0L, SEEK_END);
  size_t file_size = (size_t)ftell(file);
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
  run(src, path);
  free(src);
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
