///
/// The ouo language
///

#ifndef OUO_H
#define OUO_H

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//
// Helper macros
//

#define OUO_STRINGIZE(X) #X
#define OUO_PP_STRINGIZE(X) OUO_STRINGIZE(X)
#define OUO_CODEPOS __FILE__ ":" OUO_PP_STRINGIZE(__LINE__) ": "

#define ouo_abort(expr, err_code, fmt, ...) \
  do { \
    ouo_printerr(fmt "\n", ##__VA_ARGS__); \
    exit(err_code); \
  } while (0)

#define ouo_assert(expr, err_code, fmt, ...) \
  if (!(expr)) ouo_abort(expr, err_code, fmt, ##__VA_ARGS__)

#define ouo_assertf(expr, err_code, fmt, ...) \
  ouo_assert(expr, err_code, OUO_CODEPOS fmt, ##__VA_ARGS__)

#define ouo_assert_nomem(ptr) \
  ouo_assertf((ptr) != NULL, OUO_ERR_OUT_OF_MEMORY, \
      "Cannot allocate memory for '" #ptr "'.")

// Arrays

#define ouo_arr_len(arr) (sizeof(arr) / sizeof((arr)[0]))

#define OUO_DA_INIT_CAPACITY 8

#define ouo_da_reserve(da, new_capacity) \
  do { \
    if ((new_capacity) > (da)->capacity) { \
      if ((da)->capacity == 0) (da)->capacity = OUO_DA_INIT_CAPACITY; \
      while ((new_capacity) > (da)->capacity) (da)->capacity *= 2; \
      (da)->items = \
          ouo_realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
      ouo_assert_nomem((da)->items); \
    } \
  } while (0)

#define ouo_da_append(da, item) \
  do { \
    ouo_da_reserve(da, (da)->count + 1); \
    (da)->items[(da)->count] = (item); \
    (da)->count++; \
  } while (0)

#define ouo_da_free(da) ouo_free((da).items)

#define OUO_DA_FOREACH(T, item, da) \
  for (T *item = (da)->items; item < (da)->items + (da)->count; ++item)

// Memory management

#ifndef ouo_alloc
  #define ouo_alloc(...) malloc(__VA_ARGS__)
#endif

#ifndef ouo_realloc
  #define ouo_realloc(...) realloc(__VA_ARGS__)
#endif

#ifndef ouo_free
  #define ouo_free(...) free(__VA_ARGS__)
#endif

// Printing

#define ouo_print(...) printf(__VA_ARGS__)
#define ouo_printerr(...) fprintf(stderr, ##__VA_ARGS__)
#define ouo_printdbg(...) fprintf(stderr, ##__VA_ARGS__)

//
// Error handling
//

typedef enum {
  OUO_OK,
  OUO_ERR_OUT_OF_MEMORY,
  OUO_ERR_INCORRECT_USAGE,
  OUO_ERR_FILE_NOT_READ,
  OUO_ERR_PARSING_FAILED,
  OUO_ERR_SYNTAX,
} OuoErrorCode;

#define OUO_ERR_MSG_SIZE 128

typedef struct OuoError {
  OuoErrorCode code;
  const char *start;
  size_t len;
  char msg[OUO_ERR_MSG_SIZE];
} OuoError;

/// Owns memory for `items`.
typedef struct {
  struct OuoError *items;
  size_t count;
  size_t capacity;
} OuoErrors;

/// Prints a formatted error message,
/// pointing at its location in the source code.
/// If `path` is not `NULL`, adds it before the line and column.
void ouo_err_msg_print(OuoError *err, const char *src, const char *path);

//
// Data types
//

#ifndef ouo_int_t
  #define ouo_int_t long
  #define OUO_INT_MIN LONG_MIN
  #define OUO_INT_MAX LONG_MAX
  #define ouo_strtoi(...) strtol(__VA_ARGS__)
  #define OUO_PRId "ld"
#endif

#ifndef ouo_float_t
  #define ouo_float_t double
  #define ouo_strtof(...) strtod(__VA_ARGS__)
  #define OUO_PRIf "f"
#endif

//
// Lexing
//

typedef enum {
  OUO_TOK_EOF,
  OUO_TOK_ILLEGAL,
  // Literals
  OUO_TOK_LIT_INT,
  OUO_TOK_LIT_FLOAT,
  // Operators
  OUO_TOK_PLUS,
  OUO_TOK_ASTERISK,
} OuoTokenKind;

typedef struct {
  OuoTokenKind kind;
  const char *start;
  size_t len;
} OuoToken;

//
// Parsing
//

typedef enum {
  // Literals
  OUO_AST_LIT_INT,
  OUO_AST_LIT_FLOAT,
  // Expressions
  OUO_AST_BIN_OP,
} OuoAstKind;

/// Owns memory for any child AST nodes.
typedef struct OuoAst {
  OuoAstKind kind;
  const char *start;
  size_t len;

  union {
    // Literals
    ouo_int_t lit_int;
    ouo_float_t lit_float;

    // Expressions
    struct {
      struct OuoAst *left;
      OuoTokenKind op;
      struct OuoAst *right;
    } bin_op;
  };
} OuoAst;

/// Owns memory for `ast` and `errors.items`.
typedef struct {
  bool failed;
  OuoAst *ast;
  OuoErrors errors;
} OuoParseResult;

/// Caller owns the result's `ast` and `errors.items`.
OuoParseResult ouo_parse(const char *src);

/// Recursively frees the entire AST tree.
void ouo_ast_free(OuoAst *ast);

/// Prints the AST tree for debugging.
void ouo_ast_dump(OuoAst *ast);

#endif // OUO_H

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef OUO_IMPLEMENTATION

//
// Error handling
//

static const char *_ouo_err_code_str(OuoErrorCode err_code) {
  switch (err_code) {
    case OUO_OK: return "OK :)";
    case OUO_ERR_OUT_OF_MEMORY: return "OUT OF MEMORY";
    case OUO_ERR_PARSING_FAILED: return "PARSING FAILED";
    case OUO_ERR_SYNTAX: return "SYNTAX ERROR";
    default: return "ERROR";
  }
}

void ouo_err_msg_print(OuoError *err, const char *src, const char *path) {
  size_t line = 0, col = 0, line_len = 0;
  const char *line_start = NULL, *line_end = NULL;

  if (err->start != NULL) {
    line_start = src;
    for (const char *p = src; *p != '\0' && p < err->start; p++)
      if (*p == '\n') {
        line++;
        col = 0;
        line_start = p + 1;
      } else col++;

    line_end = line_start;
    while (*line_end != '\0' && *line_end != '\n') line_end++;
    line_len = (size_t)(line_end - line_start);
  }

  if (path != NULL) ouo_printerr("%s:", path);
  ouo_printerr("%zu:%zu: %s: %s\n", line + 1, col + 1,
      _ouo_err_code_str(err->code), err->msg);

  if (line_start != NULL) {
    ouo_printerr("%.*s\n", (int)line_len, line_start);
    for (size_t i = 0; i < col; i++) ouo_printerr(" ");
    for (size_t i = 0; i < err->len; i++) ouo_printerr("^");
  }

  ouo_printerr("\n");
}

//
// Lexing
//

typedef struct {
  const char *start;
  const char *current;
} _OuoLexer;

static inline void _ouo_l_init(_OuoLexer *l, const char *src) {
  l->start = src;
  l->current = src;
}

static inline bool _ouo_l_is_eof(_OuoLexer *l) { return *l->current == '\0'; }

static inline bool _ouo_l_is_digit(char c) { return c >= '0' && c <= '9'; }

static inline char _ouo_l_advance(_OuoLexer *l) {
  l->current++;
  return l->current[-1];
}

static inline char _ouo_l_peek(_OuoLexer *l) { return *l->current; }

static inline char _ouo_l_peek_next(_OuoLexer *l) {
  if (_ouo_l_is_eof(l)) return '\0';
  return l->current[1];
}

static inline void _ouo_l_skip_whitespace(_OuoLexer *l) {
  for (;;) {
    char c = _ouo_l_peek(l);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') _ouo_l_advance(l);
    else return;
  }
}

static inline OuoToken _ouo_l_tok_new(_OuoLexer *l, OuoTokenKind kind) {
  OuoToken tok = {
      .kind = kind,
      .start = l->start,
      .len = (size_t)(l->current - l->start),
  };
  return tok;
}

static OuoToken _ouo_l_read_number(_OuoLexer *l) {
  OuoTokenKind kind = OUO_TOK_LIT_INT;

  while (_ouo_l_is_digit(_ouo_l_peek(l))) _ouo_l_advance(l);

  if (_ouo_l_peek(l) == '.' && _ouo_l_is_digit(_ouo_l_peek_next(l))) {
    kind = OUO_TOK_LIT_FLOAT;
    _ouo_l_advance(l);

    while (_ouo_l_is_digit(_ouo_l_peek(l))) _ouo_l_advance(l);
  }

  return _ouo_l_tok_new(l, kind);
}

static OuoToken _ouo_l_next_token(_OuoLexer *l) {
  _ouo_l_skip_whitespace(l);
  l->start = (l->current);

  if (_ouo_l_is_eof(l)) return _ouo_l_tok_new(l, OUO_TOK_EOF);

  char c = _ouo_l_advance(l);

  // Literals
  if (_ouo_l_is_digit(c)) return _ouo_l_read_number(l);

  switch (c) {
    // Operators
    case '+': return _ouo_l_tok_new(l, OUO_TOK_PLUS);
    case '*': return _ouo_l_tok_new(l, OUO_TOK_ASTERISK);
  }

  return _ouo_l_tok_new(l, OUO_TOK_ILLEGAL);
}

static const char *_ouo_tok_kind_str(OuoTokenKind kind) {
  switch (kind) {
    case OUO_TOK_EOF: return "EOF";
    case OUO_TOK_ILLEGAL: return "ILLEGAL";
    // Literals
    case OUO_TOK_LIT_INT: return "LIT_INT";
    case OUO_TOK_LIT_FLOAT: return "LIT_FLOAT";
    // Operators
    case OUO_TOK_PLUS: return "+";
    case OUO_TOK_ASTERISK: return "*";
  }
  return "";
}

static void _ouo_l_tok_dump(_OuoLexer *l) {
  for (;;) {
    OuoToken tok = _ouo_l_next_token(l);
    ouo_printdbg(
        "[ %s '%.*s'] ", _ouo_tok_kind_str(tok.kind), (int)tok.len, tok.start);
    if (tok.kind == OUO_TOK_EOF) break;
  }
}

//
// Parsing
//

struct OuoParseRule;

typedef struct {
  _OuoLexer *l;
  OuoToken curr;
  OuoToken peek;

  bool failed;
  OuoErrors errors;

  struct {
    const struct OuoParseRule *items;
    size_t count;
  } rules;
} _OuoParser;

typedef OuoAst *(*_OuoParsePrefixFn)(_OuoParser *p);
typedef OuoAst *(*_OuoParseInfixFn)(_OuoParser *p, OuoAst *left);

typedef enum {
  OUO_PREC_LOWEST,
  OUO_PREC_SUM,
  OUO_PREC_PRODUCT,
} _OuoPrecedence;

typedef struct OuoParseRule {
  _OuoParsePrefixFn prefix_fn;
  _OuoParseInfixFn infix_fn;
  _OuoPrecedence prec;
} _OuoParseRule;

#define _ouo_p_err(p, tok, err_code, fmt, ...) \
  do { \
    *p->failed = true; \
    OuoError err = { \
        .code = (err_code), \
        .start = (tok).start, \
        .len = (tok).len, \
        .msg = {0}, \
    }; \
    snprintf(err.msg, OUO_ERR_MSG_SIZE - 1, fmt, ##__VA_ARGS__); \
    ouo_da_append(p->errors, err); \
  } while (0)

#define _ouo_p_assert(p, expr, tok, err_code, fmt, ...) \
  do { \
    if (!(expr)) { \
      _ouo_p_err(p, tok, err_code, fmt, ##__VA_ARGS__); \
      return NULL; \
    } \
  } while (0)

#define _ouo_p_assert_defer(p, expr, tok, err_code, fmt, ...) \
  do { \
    if (!(expr)) { \
      _ouo_p_err(p, tok, err_code, fmt, ##__VA_ARGS__); \
      goto errdefer; \
    } \
  } while (0)

static inline void _ouo_p_advance(_OuoParser *p) {
  p->curr = p->peek;
  p->peek = _ouo_l_next_token(p->l);
}

static inline void _ouo_p_init(_OuoParser *p, _OuoLexer *l,
    const _OuoParseRule *rules, size_t rules_count) {
  p->l = l;
  p->rules.items = rules;
  p->rules.count = rules_count;
  _ouo_p_advance(p);
  _ouo_p_advance(p);
}

static inline OuoAst *_ouo_p_ast_new(_OuoParser *p, OuoAstKind kind) {
  OuoAst *ast = ouo_alloc(sizeof(OuoAst));
  ouo_assert_nomem(ast);
  ast->kind = kind;
  ast->start = p->curr.start;
  ast->len = p->curr.len;
  return ast;
}

static inline const _OuoParseRule *_ouo_p_get_rule(
    _OuoParser *p, OuoTokenKind tok) {
  if (tok >= p->rules.count) return &p->rules.items[OUO_TOK_EOF];
  return &p->rules.items[tok];
}

static OuoAst *_ouo_p_expression(_OuoParser *p, _OuoPrecedence prec) {
  _OuoParsePrefixFn prefix_fn = _ouo_p_get_rule(p, p->curr.kind)->prefix_fn;

  _ouo_p_assert(&p, prefix_fn != NULL, p->curr, OUO_ERR_SYNTAX,
      "Expected an expression, got '%.*s'.", (int)p->curr.len, p->curr.start);

  OuoAst *left = prefix_fn(p);

  while (p->peek.kind != OUO_TOK_EOF &&
         prec <= _ouo_p_get_rule(p, p->peek.kind)->prec) {
    _OuoParseInfixFn infix_fn = _ouo_p_get_rule(p, p->peek.kind)->infix_fn;

    _ouo_p_assert_defer(&p, infix_fn != NULL, p->peek, OUO_ERR_SYNTAX,
        "Expected a binary operator, got '%.*s'.", (int)p->peek.len,
        p->peek.start);

    _ouo_p_advance(p);
    left = infix_fn(p, left);
  }

  return left;

errdefer:
  ouo_ast_free(left);
  return NULL;
}

static OuoAst *_ouo_p_lit_int(_OuoParser *p) {
  errno = 0;
  char *end = NULL;
  ouo_int_t lit = ouo_strtoi(p->curr.start, &end, 10);

  _ouo_p_assert(&p, errno == 0, p->curr, OUO_ERR_PARSING_FAILED,
      "Integer literal value out of range (min %" OUO_PRId ", max %" OUO_PRId
      ").",
      OUO_INT_MIN, OUO_INT_MAX);

  _ouo_p_assert(&p, end == p->curr.start + p->curr.len, p->curr,
      OUO_ERR_PARSING_FAILED,
      "Integer literal length mismatch. Expected %zu, read %zu.", p->curr.len,
      end - p->curr.start);

  OuoAst *ast = _ouo_p_ast_new(p, OUO_AST_LIT_INT);
  ast->lit_int = lit;
  return ast;
}

static OuoAst *_ouo_p_lit_float(_OuoParser *p) {
  errno = 0;
  char *end = NULL;
  ouo_float_t lit = ouo_strtof(p->curr.start, &end);

  _ouo_p_assert(&p, errno == 0, p->curr, OUO_ERR_PARSING_FAILED,
      "Float literal value out of range.");

  _ouo_p_assert(&p, end == p->curr.start + p->curr.len, p->curr,
      OUO_ERR_PARSING_FAILED,
      "Float literal length mismatch. Expected %zu, read %zu.", p->curr.len,
      end - p->curr.start);

  OuoAst *ast = _ouo_p_ast_new(p, OUO_AST_LIT_FLOAT);
  ast->lit_float = lit;
  return ast;
}

static OuoAst *_ouo_p_bin_op(_OuoParser *p, OuoAst *left) {
  OuoTokenKind op = p->curr.kind;
  _OuoPrecedence prec = _ouo_p_get_rule(p, op)->prec;
  _ouo_p_advance(p);
  OuoAst *right = _ouo_p_expression(p, prec);

  OuoAst *ast = _ouo_p_ast_new(p, OUO_AST_BIN_OP);
  ast->bin_op.left = left;
  ast->bin_op.op = op;
  ast->bin_op.right = right;
  return ast;
}

static const _OuoParseRule _ouo_p_rules[] = {
    [OUO_TOK_EOF] = {NULL, NULL, OUO_PREC_LOWEST},
    // Literals
    [OUO_TOK_LIT_INT] = {_ouo_p_lit_int, NULL, OUO_PREC_LOWEST},
    [OUO_TOK_LIT_FLOAT] = {_ouo_p_lit_float, NULL, OUO_PREC_LOWEST},
    // Operators
    [OUO_TOK_PLUS] = {NULL, _ouo_p_bin_op, OUO_PREC_SUM},
    [OUO_TOK_ASTERISK] = {NULL, _ouo_p_bin_op, OUO_PREC_PRODUCT},
};

OuoParseResult ouo_parse(const char *src) {
  _OuoLexer l = {0};
  _ouo_l_init(&l, src);

  _ouo_l_tok_dump(&l);
  ouo_printdbg("\n");

  _ouo_l_init(&l, src);
  _OuoParser p = {0};
  _ouo_p_init(&p, &l, _ouo_p_rules, ouo_arr_len(_ouo_p_rules));

  OuoAst *expr = _ouo_p_expression(&p, OUO_PREC_LOWEST);

  OuoParseResult res = {
      .failed = p.failed,
      .ast = expr,
      .errors = p.errors,
  };

  return res;
}

static const char *_ouo_ast_kind_str(OuoAstKind kind) {
  switch (kind) {
    // Literals
    case OUO_AST_LIT_INT: return "LIT_INT";
    case OUO_AST_LIT_FLOAT: return "LIT_FLOAT";
    // Expressions
    case OUO_AST_BIN_OP: return "BIN_OP";
  }
  return "";
}

void ouo_ast_free(OuoAst *ast) {
  if (ast == NULL) return;

  switch (ast->kind) {
    // Literals
    case OUO_AST_LIT_INT:
    case OUO_AST_LIT_FLOAT: ouo_free(ast); break;
    // Expressions
    case OUO_AST_BIN_OP:
      ouo_ast_free(ast->bin_op.left);
      ouo_ast_free(ast->bin_op.right);
      ouo_free(ast);
      break;
  }
}

void ouo_ast_dump(OuoAst *ast) {
  if (ast == NULL) {
    ouo_printdbg("NULL");
    return;
  }

  ouo_printdbg("( %s ", _ouo_ast_kind_str(ast->kind));

  switch (ast->kind) {
    // Literals
    case OUO_AST_LIT_INT: ouo_printdbg("%" OUO_PRId, ast->lit_int); break;
    case OUO_AST_LIT_FLOAT: ouo_printdbg("%" OUO_PRIf, ast->lit_float); break;
    // Expressions
    case OUO_AST_BIN_OP:
      ouo_ast_dump(ast->bin_op.left);
      ouo_printdbg(" %s ", _ouo_tok_kind_str(ast->bin_op.op));
      ouo_ast_dump(ast->bin_op.right);
      break;
  }

  ouo_printdbg(")");
}

#endif // OUO_IMPLEMENTATION
