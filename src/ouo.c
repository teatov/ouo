///
/// Standalone ouo interpreter
///

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #define OUO_DEBUG
#define OUO_IMPLEMENTATION
#include "ouo.h"

static OuoErrorCode run(const char *src, const char *path) {
  OuoErrorCode err_code = OUO_OK;

  OuoParseResult parse_res = ouo_parse(src);

  if (parse_res.failed) {
    OUO_DA_FOREACH(OuoError, err, &parse_res.errors) {
      ouo_err_msg_print(err, src, path);
      err_code = err->code;
    }
    goto parse_defer;
  }

  OuoCompileResult compile_res = ouo_compile(parse_res.ast);

parse_defer:
  ouo_ast_free(parse_res.ast);
  ouo_da_free(parse_res.errors);
  if (parse_res.failed) return err_code;

  if (compile_res.failed) {
    OUO_DA_FOREACH(OuoError, err, &compile_res.errors) {
      ouo_err_msg_print(err, src, path);
      err_code = err->code;
    }
    goto compile_defer;
  }

  OuoInterpretResult interpret_res = ouo_interpret(&compile_res.chunk);

compile_defer:
  ouo_chunk_free(&compile_res.chunk);
  ouo_da_free(compile_res.errors);
  if (compile_res.failed) return err_code;

  if (interpret_res.failed) {
    ouo_err_msg_print(&interpret_res.error, src, path);
    err_code = interpret_res.error.code;
  }

  return err_code;
}

static char *read_line(void) {
  struct {
    char *items;
    size_t count;
    size_t capacity;
  } buffer = {0};

  int c;
  while ((c = getchar()) != EOF) {
    ouo_da_append(&buffer, (char)c);
    if (c == '\n') break;
  }

  if (buffer.count == 0 && c == EOF) {
    ouo_da_free(buffer);
    return NULL;
  }

  ouo_da_append(&buffer, '\0');
  return buffer.items;
}

static OuoErrorCode start_repl(void) {
  for (;;) {
    ouo_print("ouo> ");
    char *line = read_line();
    if (line == NULL) {
      ouo_print("\n");
      break;
    }

    run(line, NULL);
    ouo_free(line);
  }

  return OUO_OK;
}

static char *read_file(const char *path) {
  errno = 0;
  FILE *file = fopen(path, "rb");
  ouo_assert(
      file != NULL, OUO_ERR_FILE_NOT_READ, "%s: %s.", path, strerror(errno));

  fseek(file, 0L, SEEK_END);
  size_t file_size = (size_t)ftell(file);
  rewind(file);

  char *buffer = (char *)ouo_alloc(file_size + 1);
  ouo_assert_nomem(buffer);

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  ouo_assert(bytes_read == file_size, OUO_ERR_FILE_NOT_READ, "%s: %s.", path,
      strerror(errno));

  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}

static OuoErrorCode run_file(const char *path) {
  char *src = read_file(path);
  OuoErrorCode err_code = run(src, path);
  ouo_free(src);
  return err_code;
}

int main(int argc, const char **argv) {
  OuoErrorCode err_code = OUO_OK;

  ouo_assert(argc <= 2, OUO_ERR_INCORRECT_USAGE, "Usage: ouo [PATH]");
  if (argc == 1) err_code = start_repl();
  else if (argc == 2) err_code = run_file(argv[1]);

  return (int)err_code;
}
