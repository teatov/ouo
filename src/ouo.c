///
/// Standalone ouo interpreter
///

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OUO_IMPLEMENTATION
#include "ouo.h"

static void run(const char *src, const char *path) {
  OuoParseResult parse_res = ouo_parse(src);

  ouo_ast_dump(parse_res.ast);
  ouo_printdbg("\n");

  if (parse_res.failed) {
    OUO_DA_FOREACH(OuoError, err, &parse_res.errors) {
      ouo_err_msg_print(err, src, path);
    }
    goto defer;
  }

defer:
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

static void start_repl() {
  for (;;) {
    ouo_print("ouo > ");
    char *line = read_line();
    if (line == NULL) {
      ouo_print("\n");
      break;
    }

    run(line, NULL);
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

static void run_file(const char *path) {
  char *src = read_file(path);
  run(src, path);
  ouo_free(src);
}

int main(int argc, const char **argv) {
  OuoChunk chunk = {0};

  OuoObject obj = {.kind = OUO_OBJ_FLOAT, .v_float = 5.5};
  size_t constant = _ouo_chunk_add_constant(&chunk, obj);
  _ouo_chunk_write(&chunk, OUO_OP_CONSTANT, 1);
  _ouo_chunk_write(&chunk, (ouo_op_t)constant, 1);

  OuoObject obj2 = {.kind = OUO_OBJ_FLOAT, .v_float = 6.0};
  size_t constant2 = _ouo_chunk_add_constant(&chunk, obj2);
  _ouo_chunk_write(&chunk, OUO_OP_CONSTANT, 1);
  _ouo_chunk_write(&chunk, (ouo_op_t)constant2, 1);

  _ouo_chunk_write(&chunk, OUO_OP_FLOAT_MULT, 1);
  _ouo_chunk_write(&chunk, OUO_OP_RETURN, 2);

  ouo_chunk_dump(&chunk, "main");
  ouo_printdbg("\n");
  OuoInterpretResult interp_res = ouo_interpret(&chunk);

  if (interp_res.failed) ouo_err_msg_print(&interp_res.err, NULL, NULL);

  ouo_chunk_free(&chunk);

  return 0;

  ouo_assert(argc <= 2, OUO_ERR_INCORRECT_USAGE, "Usage: ouo [PATH]");

  if (argc == 1) start_repl();
  else if (argc == 2) run_file(argv[1]);

  return OUO_OK;
}
