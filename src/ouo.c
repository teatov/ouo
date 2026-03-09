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

static void run(OuoVm *vm, const char *src, const char *path) {
  OuoParseResult parse_res = ouo_parse(src);

#ifdef OUO_DEBUG
  ouo_ast_dump(parse_res.ast);
  ouo_printdbg("\n");
#endif

  if (parse_res.failed) {
    OUO_DA_FOREACH(OuoError, err, &parse_res.errors) {
      ouo_err_msg_print(err, src, path);
    }
    goto parse_defer;
  }

  OuoCompileResult compile_res = ouo_compile(parse_res.ast);

#ifdef OUO_DEBUG
  ouo_chunk_dump(&compile_res.chunk, "main");
  ouo_printdbg("\n");
#endif

  if (compile_res.failed) {
    OUO_DA_FOREACH(OuoError, err, &compile_res.errors) {
      ouo_err_msg_print(err, src, path);
    }
    goto compile_defer;
  }

  ouo_interpret(vm, &compile_res.chunk);

  if (vm->failed) ouo_err_msg_print(&vm->error, NULL, NULL);

compile_defer:
  ouo_da_free(compile_res.errors);
  ouo_chunk_free(&compile_res.chunk);

parse_defer:
  ouo_da_free(parse_res.errors);
  ouo_ast_free(parse_res.ast);
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

static void start_repl(OuoVm *vm) {
  for (;;) {
    ouo_print("ouo> ");
    char *line = read_line();
    if (line == NULL) {
      ouo_print("\n");
      break;
    }

    run(vm, line, NULL);
    ouo_free(line);
  }
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

static void run_file(OuoVm *vm, const char *path) {
  char *src = read_file(path);
  run(vm, src, path);
  ouo_free(src);
}

int main(int argc, const char **argv) {
  ouo_assert(argc <= 2, OUO_ERR_INCORRECT_USAGE, "Usage: ouo [PATH]");

  OuoVm vm = {0};
  if (argc == 1) start_repl(&vm);
  else if (argc == 2) run_file(&vm, argv[1]);

  return OUO_OK;
}
