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

static OuoErrorCode run(const char *src, const char *path,
    OuoParseResult *p_res, OuoCompileResult *c_res, OuoInterpretResult *i_res) {
  OuoErrorCode err_code = OUO_OK;

  ouo_parse(src, p_res);

  if (p_res->failed) {
    OUO_DA_FOREACH(OuoError, err, &p_res->errors) {
      ouo_err_msg_print(err, src, path);
      err_code = err->code;
    }
    goto parse_defer;
  }

  ouo_compile(p_res->ast, c_res);

parse_defer:
  ouo_p_res_free(p_res);
  if (p_res->failed) return err_code;

  if (c_res->failed) {
    OUO_DA_FOREACH(OuoError, err, &c_res->errors) {
      ouo_err_msg_print(err, src, path);
      err_code = err->code;
    }
    goto compile_defer;
  }

  ouo_interpret(&c_res->chunk, i_res);

compile_defer:
  ouo_c_res_free(c_res);
  if (c_res->failed) return err_code;

  if (i_res->failed) {
    ouo_err_msg_print(&i_res->error, src, path);
    err_code = i_res->error.code;
  }

  return err_code;
}

static char *read_line(void) {
  OuoString buffer = {0};

  int c = '\0';
  while ((c = getchar()) != EOF) {
    if (c == '\n') break;
    ouo_da_append(&buffer, (char)c);
  }

  if (buffer.count == 0 && c == EOF) {
    ouo_da_free(buffer);
    return NULL;
  }

  ouo_da_append(&buffer, '\0');
  return buffer.items;
}

static OuoErrorCode start_repl(void) {
  struct {
    char **items;
    size_t count;
    size_t capacity;
  } lines = {0};

  OuoParseResult p_res = {0};
  OuoCompileResult c_res = {.keep_module_scope = true, .echo = true};
  OuoInterpretResult i_res = {0};

  for (;;) {
    ouo_print("ouo> ");
    char *line = read_line();
    if (line == NULL) {
      ouo_print("\n");
      break;
    }

    run(line, NULL, &p_res, &c_res, &i_res);
    ouo_da_append(&lines, line);
  }

  ouo_c_res_cleanup(&c_res);
  ouo_i_res_cleanup(&i_res);
  OUO_DA_FOREACH(char *, line, &lines) { ouo_free(*line); }
  ouo_da_free(lines);
  return OUO_OK;
}

static char *read_file(const char *path) {
  errno = 0;
  FILE *file = fopen(path, "rb");
  ouo_assert(file != NULL, OUO_ERR_READ, "%s: %s.", path, strerror(errno));

  int seek_res = fseek(file, 0, SEEK_END);
  ouo_assert(seek_res == 0, OUO_ERR_READ, "%s: %s.", path, strerror(errno));

  long file_pos = ftell(file);
  ouo_assert(file_pos >= 0, OUO_ERR_READ, "%s: %s.", path, strerror(errno));

  seek_res = fseek(file, 0, 0);
  ouo_assert(seek_res == 0, OUO_ERR_READ, "%s: %s.", path, strerror(errno));

  size_t file_size = (size_t)file_pos;
  char *buffer = (char *)ouo_malloc(file_size + 1);
  ouo_assert_nomem(buffer);

  size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
  ouo_assert(
      bytes_read == file_size, OUO_ERR_READ, "%s: %s.", path, strerror(errno));

  buffer[bytes_read] = '\0';
  fclose(file);
  return buffer;
}

static OuoErrorCode run_file(const char *path) {
  char *src = read_file(path);

  OuoParseResult p_res = {0};
  OuoCompileResult c_res = {.chunk.name = {.start = path, .len = strlen(path)}};
  OuoInterpretResult i_res = {0};

  OuoErrorCode err_code = run(src, path, &p_res, &c_res, &i_res);

  ouo_c_res_cleanup(&c_res);
  ouo_i_res_cleanup(&i_res);
  ouo_free(src);
  return err_code;
}

int main(int argc, const char **argv) {
  OuoErrorCode err_code = OUO_OK;

  ouo_assert(argc <= 2, OUO_ERR_USAGE, "Usage: ouo [PATH]");
  if (argc == 1) err_code = start_repl();
  else if (argc == 2) err_code = run_file(argv[1]);

  return (int)err_code;
}
