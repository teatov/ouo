///
/// Language server for ouo
///

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

FILE *log_file;

#define ouo_print(...) \
  do { \
    fprintf(stdout, __VA_ARGS__); \
    if (log_file != NULL) fprintf(log_file, __VA_ARGS__); \
  } while (0)

#define ouo_printerr(...) \
  do { \
    fprintf(stderr, __VA_ARGS__); \
    if (log_file != NULL) { \
      fprintf(log_file, __VA_ARGS__); \
      fflush(log_file); \
    } \
  } while (0)

#undef OUO_DEBUG
#define OUO_DA_INIT_CAPACITY 128
#define OUO_NOEMIT
#define OUO_IMPLEMENTATION
#include "ouo.h"

typedef enum {
  JSON_ILLEGAL,
  JSON_EOF,
  // Punctuation
  JSON_BRACE_OPN,
  JSON_BRACE_CLS,
  JSON_BRACKET_OPN,
  JSON_BRACKET_CLS,
  JSON_COMMA,
  JSON_COLON,
  // Keywords
  JSON_TRUE,
  JSON_FALSE,
  JSON_NULL,
  // Values
  JSON_STRING,
  JSON_NUMBER,
} JsonToken;

static const char *json_tok_str(JsonToken tok) {
  switch (tok) {
    case JSON_ILLEGAL: return "ILLEGAL";
    case JSON_EOF: return "EOF";
    // Punctuation
    case JSON_BRACE_OPN: return "{";
    case JSON_BRACE_CLS: return "}";
    case JSON_BRACKET_OPN: return "[";
    case JSON_BRACKET_CLS: return "]";
    case JSON_COMMA: return ",";
    case JSON_COLON: return ":";
    // Keywords
    case JSON_TRUE: return "true";
    case JSON_FALSE: return "false";
    case JSON_NULL: return "null";
    // Values
    case JSON_STRING: return "STRING";
    case JSON_NUMBER: return "NUMBER";
  }
  return "";
}

//
// JSON parsing
//

static inline bool json_str_eq(OuoStringSlice *json_str, const char *str) {
  return json_str->start != NULL &&
      strncmp(json_str->start, str, json_str->len) == 0;
}

static void json_str_unescaped(OuoString *owned, OuoStringSlice *s) {
  for (const char *c = s->start; c < s->start + s->len; c++) {
    if (*c != '\\' || c + 1 >= s->start + s->len) {
      ouo_da_append(owned, *c);
      continue;
    }
    c++;
    switch (*c) {
      case '"': ouo_da_append(owned, '\"'); break;
      case '\\': ouo_da_append(owned, '\\'); break;
      case '/': ouo_da_append(owned, '/'); break;
      case 'b': ouo_da_append(owned, '\b'); break;
      case 'f': ouo_da_append(owned, '\f'); break;
      case 'n': ouo_da_append(owned, '\n'); break;
      case 'r': ouo_da_append(owned, '\r'); break;
      case 't': ouo_da_append(owned, '\t'); break;
      default:
        ouo_da_append(owned, '\\');
        ouo_da_append(owned, *c);
        break;
    }
  }
  ouo_da_append(owned, '\0');
}

typedef struct {
  const char *tok_start;
  const char *curr;
  JsonToken tok;

  OuoStringSlice string;
  double number;
  bool boolean;

  bool failed;
} JsonParser;

static inline void _jp_init(JsonParser *jp, const char *src) {
  jp->tok_start = src;
  jp->curr = src;
  jp->tok = JSON_ILLEGAL;

  jp->string = (OuoStringSlice){0};
  jp->number = 0.0;
  jp->boolean = false;

  jp->failed = false;
}

#define _jp_err(jp, ...) \
  do { \
    if (!(jp)->failed) { \
      (jp)->failed = true; \
      ouo_printerr("JSON PARSING ERROR: " __VA_ARGS__); \
      ouo_printerr("\n"); \
    } \
  } while (0)

static inline bool jp_str_eq(JsonParser *jp, const char *str) {
  return json_str_eq(&jp->string, str);
}

static inline bool _jp_is_eof(JsonParser *jp) { return *jp->curr == '\0'; }

static inline char _jp_advance(JsonParser *jp) {
  jp->curr++;
  return jp->curr[-1];
}

static inline void _jp_skip_whitespace(JsonParser *jp) {
  while (!_jp_is_eof(jp) && _ouo_l_isspace(*jp->curr)) _jp_advance(jp);
}

static inline JsonToken _jp_check_keyword(
    JsonParser *jp, size_t rest_len, const char *rest, JsonToken tok) {
  if (!isalpha(*(jp->tok_start + rest_len + 1)) &&
      memcmp(jp->tok_start + 1, rest, rest_len) == 0) {
    jp->curr += rest_len;
    return tok;
  }
  return JSON_ILLEGAL;
}

static bool _jp_get(JsonParser *jp) {
  _jp_skip_whitespace(jp);
  jp->tok_start = jp->curr;

  if (_jp_is_eof(jp)) {
    jp->tok = JSON_EOF;
    _jp_err(jp, "Unexpected 'EOF'.");
    return false;
  }

  char c = _jp_advance(jp);

  switch (c) {
    // Punctuation
    case '{': jp->tok = JSON_BRACE_OPN; break;
    case '}': jp->tok = JSON_BRACE_CLS; break;
    case '[': jp->tok = JSON_BRACKET_OPN; break;
    case ']': jp->tok = JSON_BRACKET_CLS; break;
    case ',': jp->tok = JSON_COMMA; break;
    case ':': jp->tok = JSON_COLON; break;
    // Keywords
    case 't': jp->tok = _jp_check_keyword(jp, 3, "rue", JSON_TRUE); break;
    case 'f': jp->tok = _jp_check_keyword(jp, 4, "alse", JSON_FALSE); break;
    case 'n': jp->tok = _jp_check_keyword(jp, 3, "ull", JSON_NULL); break;
    default: jp->tok = JSON_ILLEGAL; break;
  }

  if (jp->tok != JSON_ILLEGAL) return true;

  if (_ouo_l_isdigit(c)) {
    char *end = NULL;
    jp->number = strtod(jp->tok_start, &end);
    if (jp->tok_start != end) {
      jp->curr = end;
      jp->tok = JSON_NUMBER;
      return true;
    }
  }

  if (c == '"') {
    jp->string.start = jp->curr;
    jp->string.len = 0;
    while (!_jp_is_eof(jp)) {
      char c = _jp_advance(jp);
      if (c == '"') {
        jp->tok = JSON_STRING;
        return true;
      }
      jp->string.len++;
    }
    _jp_err(jp, "Unterminated string.");
    return false;
  }

  _jp_err(jp, "Unexpected '%c'.", c);
  return false;
}

static inline bool _jp_expect(JsonParser *jp, JsonToken tok) {
  if (jp->tok != tok) {
    _jp_err(jp, "Expected '%s', but got '%s'.", json_tok_str(tok),
        json_tok_str(jp->tok));
    return false;
  }
  return true;
}

static inline bool _jp_get_and_expect(JsonParser *jp, JsonToken tok) {
  if (!_jp_get(jp)) return false;
  return _jp_expect(jp, tok);
}

static inline bool jp_object_begin(JsonParser *jp) {
  return _jp_get_and_expect(jp, JSON_BRACE_OPN);
}

static inline bool jp_object_end(JsonParser *jp) {
  return _jp_get_and_expect(jp, JSON_BRACE_CLS);
}

static bool jp_object_member(JsonParser *jp) {
  const char *curr = jp->curr;
  if (!_jp_get(jp)) return false;
  if (jp->tok == JSON_COMMA) {
    if (!_jp_get_and_expect(jp, JSON_STRING)) return false;
    if (!_jp_get_and_expect(jp, JSON_COLON)) return false;
    return true;
  }
  if (jp->tok == JSON_BRACE_CLS) {
    jp->curr = curr;
    return false;
  }
  if (!_jp_expect(jp, JSON_STRING)) return false;
  if (!_jp_get_and_expect(jp, JSON_COLON)) return false;
  return true;
}

static inline bool jp_array_begin(JsonParser *jp) {
  return _jp_get_and_expect(jp, JSON_BRACKET_OPN);
}

static inline bool jp_array_end(JsonParser *jp) {
  return _jp_get_and_expect(jp, JSON_BRACKET_CLS);
}

static bool jp_array_item(JsonParser *jp) {
  const char *curr = jp->curr;
  if (!_jp_get(jp)) return false;
  if (jp->tok == JSON_COMMA) return true;
  if (jp->tok == JSON_BRACKET_CLS) {
    jp->curr = curr;
    return false;
  }
  jp->curr = curr;
  return true;
}

static inline bool jp_string(JsonParser *jp) {
  return _jp_get_and_expect(jp, JSON_STRING);
}

static inline bool jp_number(JsonParser *jp) {
  return _jp_get_and_expect(jp, JSON_NUMBER);
}

static bool jp_skip(JsonParser *jp) {
  if (!_jp_get(jp)) return false;
  switch (jp->tok) {
    case JSON_BRACE_OPN:
      while (jp_object_member(jp))
        if (!jp_skip(jp)) return false;
      return jp_object_end(jp);
    case JSON_BRACKET_OPN:
      while (jp_array_item(jp))
        if (!jp_skip(jp)) return false;
      return jp_array_end(jp);
    case JSON_TRUE:
    case JSON_FALSE:
    case JSON_NULL:
    case JSON_STRING:
    case JSON_NUMBER: return true;
    default: break;
  }

  _jp_err(jp, "Unexpected '%s'.", json_tok_str(jp->tok));
  return false;
}

static inline bool jp_end(JsonParser *jp) {
  _jp_skip_whitespace(jp);
  if (!_jp_is_eof(jp)) {
    _jp_err(jp, "Expected 'EOF', but got '%c'.", *jp->curr);
    return false;
  }
  return true;
}

//
// JSON serialization
//

typedef enum {
  JSON_SCOPE_OBJECT,
  JSON_SCOPE_ARRAY,
} JsonScopeKind;

typedef struct {
  JsonScopeKind kind;
  bool tail;
  bool key;
} JsonScope;

typedef struct {
  OuoString res;

  struct {
    JsonScope *items;
    size_t count;
    size_t capacity;
  } scopes;
} JsonSerializer;

#define _js_err(jp, ...) \
  do { \
    ouo_printerr("JSON SERIALIZATION ERROR: " __VA_ARGS__); \
    ouo_printerr("\n"); \
  } while (0)

static inline void _js_init(JsonSerializer *js) {
  js->res.items = NULL;
  js->scopes.items = NULL;
}

static inline JsonScope *_js_current_scope(JsonSerializer *js) {
  if (js->scopes.count > 0) return &js->scopes.items[js->scopes.count - 1];
  return NULL;
}

static inline void _js_scope_push(JsonSerializer *js, JsonScopeKind kind) {
  JsonScope scope = {.kind = kind};
  ouo_da_append(&js->scopes, scope);
}

static inline void _js_scope_pop(JsonSerializer *js) {
  if (js->scopes.count == 0) {
    _js_err(js, "Trying to pop empty scope stack.");
    return;
  }
  js->scopes.count--;
}

static inline void js_raw(JsonSerializer *js, const char *str) {
  ouo_da_append_many(&js->res, str, strlen(str));
}

static inline void js_str_escaped(
    JsonSerializer *js, const char *str, size_t len) {
  js_raw(js, "\"");
  ouo_da_append_many(&js->res, str, len);
  js_raw(js, "\"");
}

static inline void _js_element_begin(JsonSerializer *js) {
  JsonScope *scope = _js_current_scope(js);
  if (scope == NULL) return;
  if (scope->tail && !scope->key) js_raw(js, ",");
}

static inline void _js_element_end(JsonSerializer *js) {
  JsonScope *scope = _js_current_scope(js);
  if (scope == NULL) return;
  scope->tail = true;
  scope->key = false;
}

static inline void js_object_begin(JsonSerializer *js) {
  _js_element_begin(js);
  js_raw(js, "{");
  _js_scope_push(js, JSON_SCOPE_OBJECT);
}

static inline void js_object_end(JsonSerializer *js) {
  js_raw(js, "}");
  _js_scope_pop(js);
  _js_element_end(js);
}

static inline void js_object_member(JsonSerializer *js, const char *key) {
  _js_element_begin(js);
  JsonScope *scope = _js_current_scope(js);
  if (scope == NULL || scope->kind != JSON_SCOPE_OBJECT || scope->key) {
    _js_err(js, "Not in an object scope.");
    return;
  }
  js_str_escaped(js, key, strlen(key));
  js_raw(js, ":");
  scope->key = true;
}

static inline void js_array_begin(JsonSerializer *js) {
  _js_element_begin(js);
  js_raw(js, "[");
  _js_scope_push(js, JSON_SCOPE_ARRAY);
}

static inline void js_array_end(JsonSerializer *js) {
  js_raw(js, "]");
  _js_scope_pop(js);
  _js_element_end(js);
}

static inline void js_string(JsonSerializer *js, OuoStringSlice *value) {
  _js_element_begin(js);
  js_str_escaped(js, value->start, value->len);
  _js_element_end(js);
}

static inline void js_string_raw(JsonSerializer *js, const char *str) {
  OuoStringSlice value = (OuoStringSlice){.start = str, .len = strlen(str)};
  js_string(js, &value);
}

#define JS_BUFFER_SIZE 512

static inline void js_integer(JsonSerializer *js, long value) {
  _js_element_begin(js);
  char str[JS_BUFFER_SIZE] = {0};
  snprintf(str, JS_BUFFER_SIZE, "%ld", value);
  js_raw(js, str);
  _js_element_end(js);
}

static inline void js_boolean(JsonSerializer *js, bool value) {
  _js_element_begin(js);
  js_raw(js, value ? "true" : "false");
  _js_element_end(js);
}

static inline void js_null(JsonSerializer *js) {
  _js_element_begin(js);
  js_raw(js, "null");
  _js_element_end(js);
}

//
// Language server protocol
//

typedef struct {
  long id;
  OuoStringSlice method;

  bool respond;
  bool has_params;
  bool initialized;
  bool shutdown;
  bool exit;

  JsonParser *jp;
  JsonSerializer *js;
} OuoLs;

static inline void _ls_init(OuoLs *ls, JsonParser *jp, JsonSerializer *js) {
  ls->method = (OuoStringSlice){.start = NULL};

  ls->respond = false;
  ls->has_params = false;

  ls->jp = jp;
  ls->js = js;
}

static inline void _ls_begin(OuoLs *ls) {
  ouo_printerr("\n// Send:\n");
  js_object_begin(ls->js);
  {
    js_object_member(ls->js, "jsonrpc");
    js_string_raw(ls->js, "2.0");
  }
}

static inline void _ls_response_begin(OuoLs *ls) {
  _ls_begin(ls);
  js_object_member(ls->js, "id");
  js_integer(ls->js, ls->id);
  js_object_member(ls->js, "result");
}

static inline void _ls_notification_begin(OuoLs *ls, const char *method) {
  _ls_begin(ls);
  js_object_member(ls->js, "method");
  js_string_raw(ls->js, method);
  js_object_member(ls->js, "params");
}

static inline void _ls_end(OuoLs *ls) {
  js_object_end(ls->js);
  js_raw(ls->js, "\0");

  ouo_print("Content-Length: %zu\r\n\r\n%.*s\r\n", ls->js->res.count,
      (int)ls->js->res.count, ls->js->res.items);

  fflush(stdout);
  ouo_da_free(ls->js->scopes);
  ouo_da_free(ls->js->res);
  ouo_printerr("// Flushed!\n");
}

static void _ls_diagnostic(OuoLs *ls, OuoError *err) {
  long line = (long)err->pos.line - 1;
  long col = (long)err->pos.col - 1;

  js_object_begin(ls->js);
  {
    js_object_member(ls->js, "code");
    js_string_raw(ls->js, _ouo_err_code_str(err->code));

    js_object_member(ls->js, "message");
    js_string_raw(ls->js, err->msg);

    js_object_member(ls->js, "range");
    js_object_begin(ls->js);
    {
      js_object_member(ls->js, "start");
      js_object_begin(ls->js);
      {
        js_object_member(ls->js, "line");
        js_integer(ls->js, line);

        js_object_member(ls->js, "character");
        js_integer(ls->js, col);
      }
      js_object_end(ls->js);

      js_object_member(ls->js, "end");
      js_object_begin(ls->js);
      {
        js_object_member(ls->js, "line");
        js_integer(ls->js, line);

        js_object_member(ls->js, "character");
        js_integer(ls->js, col + (long)err->len);
      }
      js_object_end(ls->js);
    }
    js_object_end(ls->js);

    js_object_member(ls->js, "severity");
    js_integer(ls->js, err->code == OUO_ERR_NOTE ? 3 : 1);

    js_object_member(ls->js, "source");
    js_string_raw(ls->js, "ouols");
  }
  js_object_end(ls->js);
}

static void _ls_analyze(OuoLs *ls, OuoString *src, OuoStringSlice *uri) {
  _ls_notification_begin(ls, "textDocument/publishDiagnostics");

  js_object_begin(ls->js);
  {
    js_object_member(ls->js, "diagnostics");
    js_array_begin(ls->js);
    {
      OuoParseResult p_res = {0};
      ouo_parse(src->items, &p_res);

      if (p_res.failed) {
        OUO_DA_FOREACH(OuoError, err, &p_res.errors) {
          _ls_diagnostic(ls, err);
        }
      } else {
        OuoCompileResult c_res = {0};
        ouo_compile(p_res.ast, &c_res);

        if (c_res.failed) {
          OUO_DA_FOREACH(OuoError, err, &c_res.errors) {
            _ls_diagnostic(ls, err);
          }
        }

        ouo_da_free(c_res.errors);
      }

      ouo_ast_free(p_res.ast);
      ouo_da_free(p_res.errors);
    }
    js_array_end(ls->js);

    js_object_member(ls->js, "uri");
    if (uri->start != NULL) js_string(ls->js, uri);
    else js_string_raw(ls->js, "");
  }
  js_object_end(ls->js);
  _ls_end(ls);
}

static bool _ls_initialize(OuoLs *ls) {
  while (jp_object_member(ls->jp)) {
    if (jp_str_eq(ls->jp, "processId")) {
      if (!jp_number(ls->jp)) return false;
    } else if (!jp_skip(ls->jp)) return false;
  }

  _ls_response_begin(ls);

  js_object_begin(ls->js);
  {
    js_object_member(ls->js, "capabilities");
    js_object_begin(ls->js);
    {
      js_object_member(ls->js, "textDocumentSync");
      js_object_begin(ls->js);
      {
        js_object_member(ls->js, "openClose");
        js_boolean(ls->js, true);
        js_object_member(ls->js, "change");
        js_integer(ls->js, 1);
      }
      js_object_end(ls->js);
    }
    js_object_end(ls->js);
  }
  js_object_end(ls->js);

  _ls_end(ls);
  return true;
}

static bool _ls_did_open(OuoLs *ls, bool change) {
  OuoString src = {0};
  OuoStringSlice uri = {0};

  while (jp_object_member(ls->jp)) {
    if (jp_str_eq(ls->jp, "textDocument")) {
      if (!jp_object_begin(ls->jp)) return false;

      while (jp_object_member(ls->jp)) {
        if (jp_str_eq(ls->jp, "uri")) {
          if (!jp_string(ls->jp)) return false;
          uri = ls->jp->string;
        } else if (!change && jp_str_eq(ls->jp, "text")) {
          if (!jp_string(ls->jp)) return false;
          json_str_unescaped(&src, &ls->jp->string);
        } else if (!jp_skip(ls->jp)) return false;
      }

      if (!jp_object_end(ls->jp)) return false;
    } else if (change && jp_str_eq(ls->jp, "contentChanges")) {
      if (!jp_array_begin(ls->jp)) return false;

      while (jp_array_item(ls->jp)) {
        if (!jp_object_begin(ls->jp)) return false;

        while (jp_object_member(ls->jp)) {
          if (jp_str_eq(ls->jp, "text")) {
            if (!jp_string(ls->jp)) return false;
            json_str_unescaped(&src, &ls->jp->string);
          } else {
            _jp_err(ls->jp, "Unexpected object key %.*s",
                (int)ls->jp->string.len, ls->jp->string.start);
            return false;
          }
        }

        if (!jp_object_end(ls->jp)) return false;
      }

      if (!jp_array_end(ls->jp)) return false;
    } else if (!jp_skip(ls->jp)) return false;
  }

  if (src.items == NULL) {
    ouo_printerr("src is empty...\n");
    return false;
  }

  _ls_analyze(ls, &src, &uri);
  ouo_da_free(src);
  return true;
}

static bool _ls_handle_method(OuoLs *ls) {
  if (json_str_eq(&ls->method, "initialize")) return _ls_initialize(ls);

  if (json_str_eq(&ls->method, "textDocument/didOpen"))
    return _ls_did_open(ls, false);

  if (json_str_eq(&ls->method, "textDocument/didChange"))
    return _ls_did_open(ls, true);

  if (json_str_eq(&ls->method, "initialized")) {
    ls->initialized = true;
    ouo_printerr("client initialized!\n");
  }

  if (json_str_eq(&ls->method, "shutdown")) {
    ls->shutdown = true;
    ouo_printerr("shutdown!\n");
  }

  if (json_str_eq(&ls->method, "exit")) {
    ls->exit = true;
    ouo_printerr("exit!\n");
  }

  if (ls->has_params) {
    while (jp_object_member(ls->jp))
      if (!jp_skip(ls->jp)) return false;
  }

  if (ls->respond) {
    _ls_response_begin(ls);
    js_null(ls->js);
    _ls_end(ls);
  }

  return true;
}

static bool _ls_handle_request(OuoLs *ls) {
  if (!jp_object_begin(ls->jp)) return false;
  while (jp_object_member(ls->jp)) {
    if (jp_str_eq(ls->jp, "jsonrpc")) {
      if (!jp_string(ls->jp)) return false;
      if (!jp_str_eq(ls->jp, "2.0")) {
        ouo_printerr("Unknown JSON-RPC version '%.*s'.\n",
            (int)ls->jp->string.len, ls->jp->string.start);
        return false;
      }
    } else if (jp_str_eq(ls->jp, "id")) {
      if (!jp_number(ls->jp)) return false;
      ls->id = (long)ls->jp->number;
      ls->respond = true;
    } else if (jp_str_eq(ls->jp, "method")) {
      if (!jp_string(ls->jp)) return false;
      ls->method.start = ls->jp->string.start;
      ls->method.len = ls->jp->string.len;
    } else if (jp_str_eq(ls->jp, "params")) {
      if (!jp_object_begin(ls->jp)) return false;
      ls->has_params = true;
      break;
    } else if (!jp_skip(ls->jp)) return false;
  }

  if (!_ls_handle_method(ls)) return false;

  if (ls->has_params) {
    if (!jp_object_end(ls->jp)) return false;
  }
  if (!jp_object_end(ls->jp)) return false;
  if (!jp_end(ls->jp)) return false;

  return true;
}

static void ls_handle(OuoLs *ls, const char *body) {
  JsonParser jp = {0};
  JsonSerializer js = {0};
  _jp_init(&jp, body);
  _js_init(&js);
  _ls_init(ls, &jp, &js);

  bool result = _ls_handle_request(ls);
  if (!result) ouo_printerr("GOT ERRORS!\n");
}

int main(int argc, const char **argv) {
  log_file = NULL;
  if (argc > 1) {
    if (argc != 3 || strcmp(argv[1], "--log") != 0) {
      ouo_printerr("Usage: ouo [--log PATH]\n");
      return OUO_ERR_USAGE;
    }
    log_file = fopen(argv[2], "w");
    if (log_file == NULL) ouo_printerr("%s: %s.", argv[2], strerror(errno));
  }

  ouo_printerr("ouols starting...\n");
  OuoLs ls = {0};

  for (;;) {
    unsigned long content_len = 0;
    char header[512];

    while (fgets(header, sizeof(header), stdin)) {
      if (strcmp(header, "\r\n") == 0) break;
      if (strncmp(header, "Content-Length: ", 16) == 0) {
        char *end = NULL;
        content_len = strtoul(header + 16, &end, 10);
      }
    }

    if (content_len == 0) {
      if (feof(stdin)) break;
      continue;
    }

    ouo_printerr("\n// Receive:\nContent-Length: %lu\n\n", content_len);

    char *body = ouo_malloc(content_len + 1);
    ouo_assert_nomem(body);
    if (fread(body, 1, content_len, stdin) != (size_t)content_len) {
      ouo_printerr("Error reading body.\n");
      ouo_free(body);
      break;
    }
    body[content_len] = '\0';

    ouo_printerr("%s\n", body);

    ls_handle(&ls, body);

    ouo_free(body);
  }

  ouo_printerr("\ndone!\n");
  if (log_file != NULL) fclose(log_file);
  return OUO_OK;
}
