///
/// Language server for ouo
///

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#define OUO_IMPLEMENTATION
#include "ouo.h"

typedef enum {
  JSON_ILLEGAL,
  JSON_EOF,
  // Punctuation
  JSON_BRACE_OP,
  JSON_BRACE_CL,
  JSON_BRACKET_OP,
  JSON_BRACKET_CL,
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
    case JSON_BRACE_OP: return "{";
    case JSON_BRACE_CL: return "}";
    case JSON_BRACKET_OP: return "[";
    case JSON_BRACKET_CL: return "]";
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

///
/// JSON parsing
///

typedef struct {
  const char *start;
  size_t len;
} JsonString;

static inline bool json_strcmp(JsonString *json_str, const char *str) {
  return strncmp(json_str->start, str, json_str->len) == 0;
}

typedef struct {
  const char *start;
  const char *curr;
  JsonToken tok;

  JsonString string;
  double number;
  bool boolean;

  bool failed;
} JsonParser;

static inline void _jp_init(JsonParser *jp, const char *src) {
  jp->start = src;
  jp->curr = src;
  jp->tok = JSON_ILLEGAL;

  jp->string = (JsonString){0};
  jp->number = 0.0;
  jp->boolean = false;

  jp->failed = false;
}

#define _jp_err(jp, fmt, ...) \
  do { \
    if (!(jp)->failed) { \
      (jp)->failed = true; \
      ouo_printerr("JSON PARSING ERROR: " fmt "\n", ##__VA_ARGS__); \
    } \
  } while (0)

static inline bool jp_strcmp(JsonParser *jp, const char *str) {
  return json_strcmp(&jp->string, str);
}

static inline bool _jp_is_eof(JsonParser *jp) { return *jp->curr == '\0'; }

static inline char _jp_advance(JsonParser *jp) {
  jp->curr++;
  return jp->curr[-1];
}

static inline void _jp_skip_whitespace(JsonParser *jp) {
  while (!_jp_is_eof(jp) && isspace(*jp->curr)) _jp_advance(jp);
}

static inline JsonToken _jp_check_keyword(
    JsonParser *jp, size_t len, const char *rest, JsonToken tok) {
  if (!isalpha(*(jp->start + len + 1)) &&
      memcmp(jp->start + 1, rest, len) == 0) {
    jp->curr += len;
    return tok;
  }
  return JSON_ILLEGAL;
}

static inline bool _jp_get(JsonParser *jp) {
  _jp_skip_whitespace(jp);
  jp->start = jp->curr;

  if (_jp_is_eof(jp)) {
    jp->tok = JSON_EOF;
    _jp_err(jp, "Unexpected 'EOF'.");
    return false;
  }

  char c = _jp_advance(jp);

  switch (c) {
    // Punctuation
    case '{': jp->tok = JSON_BRACE_OP; break;
    case '}': jp->tok = JSON_BRACE_CL; break;
    case '[': jp->tok = JSON_BRACKET_OP; break;
    case ']': jp->tok = JSON_BRACKET_CL; break;
    case ',': jp->tok = JSON_COMMA; break;
    case ':': jp->tok = JSON_COLON; break;
    // Keywords
    case 't': jp->tok = _jp_check_keyword(jp, 3, "rue", JSON_TRUE); break;
    case 'f': jp->tok = _jp_check_keyword(jp, 4, "alse", JSON_FALSE); break;
    case 'n': jp->tok = _jp_check_keyword(jp, 3, "ull", JSON_NULL); break;
    default: jp->tok = JSON_ILLEGAL; break;
  }

  if (jp->tok != JSON_ILLEGAL) return true;

  if (isdigit(c)) {
    char *end = NULL;
    jp->number = strtod(jp->start, &end);
    if (jp->start != end) {
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
      switch (c) {
        case '"': {
          jp->tok = JSON_STRING;
          return true;
        }
        default: jp->string.len++; break;
      }
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
  return _jp_get_and_expect(jp, JSON_BRACE_OP);
}

static inline bool jp_object_end(JsonParser *jp) {
  return _jp_get_and_expect(jp, JSON_BRACE_CL);
}

static bool jp_object_member(JsonParser *jp) {
  const char *curr = jp->curr;
  if (!_jp_get(jp)) return false;
  if (jp->tok == JSON_COMMA) {
    if (!_jp_get_and_expect(jp, JSON_STRING)) return false;
    if (!_jp_get_and_expect(jp, JSON_COLON)) return false;
    return true;
  }
  if (jp->tok == JSON_BRACE_CL) {
    jp->curr = curr;
    return false;
  }
  if (!_jp_expect(jp, JSON_STRING)) return false;
  if (!_jp_get_and_expect(jp, JSON_COLON)) return false;
  return true;
}

static inline void jp_object_member_unknown(JsonParser *jp) {
  _jp_err(jp, "Unknown member '%.*s'.", (int)jp->string.len, jp->string.start);
}

static inline bool jp_string(JsonParser *jp) {
  return _jp_get_and_expect(jp, JSON_STRING);
}

static inline bool jp_number(JsonParser *jp) {
  return _jp_get_and_expect(jp, JSON_NUMBER);
}

static bool jp_any(JsonParser *jp) {
  if (!_jp_get(jp)) return false;
  switch (jp->tok) {
    case JSON_BRACE_OP:
      while (jp_object_member(jp))
        if (!jp_any(jp)) return false;
      if (!jp_object_end(jp)) return false;
      break;
    case JSON_BRACKET_OP:
      while (!_jp_is_eof(jp) && *jp->curr != ']') _jp_advance(jp);
      _jp_advance(jp);
      break;
    case JSON_TRUE:
    case JSON_FALSE:
    case JSON_NULL:
    case JSON_STRING:
    case JSON_NUMBER: return true;
    default: _jp_err(jp, "Unexpected '%s'.", json_tok_str(jp->tok)); break;
  }
  return true;
}

static inline bool jp_end(JsonParser *jp) {
  _jp_skip_whitespace(jp);
  if (!_jp_is_eof(jp)) {
    _jp_err(jp, "Expected 'EOF', but got '%c'.", *jp->curr);
    return false;
  }
  return true;
}

///
/// JSON serializing
///

typedef struct {
  char *items;
  size_t count;
  size_t capacity;
} JsonSerializer;

static inline void _js_init(JsonSerializer *js) { js->items = NULL; }

static void js_raw(JsonSerializer *js, const char *str) {
  ouo_da_append_many(js, str, strlen(str));
}

#define JS_BUFFER_SIZE 512

static void js_int(JsonSerializer *js, long v) {
  char str[JS_BUFFER_SIZE] = {0};
  snprintf(str, JS_BUFFER_SIZE, "%ld", v);
  js_raw(js, str);
}

///
/// Language server
///

typedef struct {
  bool is_notif;
  bool has_params;
} OuoLs;

static inline void _ls_init(OuoLs *ls) {
  ls->is_notif = true;
  ls->has_params = false;
}

static bool _ls_handle_initialize(
    OuoLs *_, JsonParser *jp, JsonSerializer *js) {
  bool result = false;

  while (jp_object_member(jp)) {
    if (jp_strcmp(jp, "processId")) {
      if (!jp_number(jp)) goto defer;
    } else if (!jp_any(jp)) goto defer;
  }

  result = true;
defer:
  js_raw(js, "{\"capabilities\":{}}");
  return result;
}

static bool _ls_handle_method(
    OuoLs *ls, JsonParser *jp, JsonSerializer *js, JsonString *method) {
  if (json_strcmp(method, "initialize"))
    return _ls_handle_initialize(ls, jp, js);
  else if (ls->has_params) {
    while (jp_object_member(jp))
      if (!jp_any(jp)) return false;
  }

  if (!ls->is_notif) js_raw(js, "null");
  return true;
}

static bool ls_handle_request(
    OuoLs *ls, JsonParser *jp, JsonSerializer *js, const char *body) {
  _ls_init(ls);
  _jp_init(jp, body);
  _js_init(js);

  long id;
  JsonString method;

  if (!jp_object_begin(jp)) return false;
  while (jp_object_member(jp)) {
    if (jp_strcmp(jp, "jsonrpc")) {
      if (!jp_string(jp)) return false;
      if (!jp_strcmp(jp, "2.0")) {
        ouo_printerr("Unknown JSON-RPC version '%.*s'.\n", (int)jp->string.len,
            jp->string.start);
        return false;
      }
    } else if (jp_strcmp(jp, "id")) {
      if (!jp_number(jp)) return false;
      id = (long)jp->number;
      ls->is_notif = false;
    } else if (jp_strcmp(jp, "method")) {
      if (!jp_string(jp)) return false;
      method.start = jp->string.start;
      method.len = jp->string.len;
    } else if (jp_strcmp(jp, "params")) {
      if (!jp_object_begin(jp)) return false;
      ls->has_params = true;
      break;
    } else {
      jp_object_member_unknown(jp);
      return false;
    }
  }

  bool result = false;

  if (!ls->is_notif) {
    js_raw(js, "{\"jsonrpc\":\"2.0\",\"id\":");
    js_int(js, id);
    js_raw(js, ",\"result\":");
  }

  if (!_ls_handle_method(ls, jp, js, &method)) goto defer;

  if (ls->has_params) {
    if (!jp_object_end(jp)) goto defer;
  }
  if (!jp_object_end(jp)) goto defer;
  if (!jp_end(jp)) goto defer;

  result = true;
defer:
  if (!ls->is_notif) {
    js_raw(js, "}");
    ouo_da_append(js, '\0');
  }
  return result;
}

int main(void) {
  ouo_printdbg("ouols starting...\n");

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

    ouo_printdbg("Content-Length: %lu\n", content_len);

    char *body = ouo_alloc(content_len + 1);
    if (fread(body, 1, content_len, stdin) != (size_t)content_len) {
      ouo_printerr("Error reading body.\n");
      free(body);
      break;
    }
    body[content_len] = '\0';

    ouo_printdbg("Received: %s\n", body);

    JsonParser jp = {0};
    JsonSerializer js = {0};

    ls_handle_request(&ls, &jp, &js, body);

    if (js.items != NULL) {
      fprintf(stdout, "Content-Length: %zu\r\n\r\n%s\r\n", strlen(js.items),
          js.items);

      fflush(stdout);
      ouo_da_free(js);
    }

    ouo_free(body);
  }

  return 0;
}
