///
/// The ouo language
///

#ifndef OUO_H
#define OUO_H

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Helper macros
//

#define OUO_STRINGIZE(X) #X
#define OUO_PP_STRINGIZE(X) OUO_STRINGIZE(X)
#define OUO_CODEPOS __FILE__ ":" OUO_PP_STRINGIZE(__LINE__) ": "

#define ouo_abort(err_code, fmt, ...) \
  do { \
    ouo_printerr(fmt "\n", ##__VA_ARGS__); \
    exit(err_code); \
  } while (0)

#define ouo_assert(expr, err_code, fmt, ...) \
  if (!(expr)) ouo_abort(err_code, fmt, ##__VA_ARGS__)

#define ouo_assertf(expr, err_code, fmt, ...) \
  ouo_assert(expr, err_code, OUO_CODEPOS fmt, ##__VA_ARGS__)

#define ouo_assert_nomem(ptr) \
  ouo_assertf((ptr) != NULL, OUO_ERR_OUT_OF_MEMORY, \
      "Cannot allocate memory for '" #ptr "'.")

// Arrays

#define ouo_arr_len(arr) (sizeof(arr) / sizeof((arr)[0]))

#ifndef OUO_DA_INIT_CAPACITY
#define OUO_DA_INIT_CAPACITY 8
#endif

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

#define ouo_da_append_many(da, new_items, new_items_count) \
  do { \
    ouo_da_reserve((da), (da)->count + (new_items_count)); \
    ouo_memcpy((da)->items + (da)->count, (new_items), \
        (new_items_count) * sizeof(*(da)->items)); \
    (da)->count += (new_items_count); \
  } while (0)

#define ouo_da_free(da) \
  do { \
    ouo_free((da).items); \
    (da).items = NULL; \
  } while (0)

#define OUO_DA_FOREACH(T, item, da) \
  for (T * (item) = (da)->items; (item) < (da)->items + (da)->count; ++(item))

// Memory management

#ifndef ouo_malloc
#define ouo_malloc(...) malloc(__VA_ARGS__)
#endif

#ifndef ouo_realloc
#define ouo_realloc(...) realloc(__VA_ARGS__)
#endif

#ifndef ouo_memcpy
#define ouo_memcpy(...) memcpy(__VA_ARGS__)
#endif

#ifndef ouo_free
#define ouo_free(...) free(__VA_ARGS__)
#endif

// Printing

#ifndef ouo_print
#define ouo_print(...) fprintf(stdout, __VA_ARGS__)
#endif

#ifndef ouo_printerr
#define ouo_printerr(...) fprintf(stderr, __VA_ARGS__)
#endif

#ifndef ouo_printdbg
#define ouo_printdbg(...) fprintf(stderr, __VA_ARGS__)
#endif

//
// Error handling
//

typedef enum {
  OUO_OK,
  // General
  OUO_ERR_OUT_OF_MEMORY,
  OUO_ERR_USAGE,
  OUO_ERR_READ,
  // Parsing
  OUO_ERR_PARSE_FAIL,
  OUO_ERR_SYNTAX,
  // Compilation
  OUO_ERR_COMPILE_FAIL,
  OUO_ERR_TYPE,
  // Runtime
  OUO_ERR_RUNTIME,
} OuoErrorCode;

#define OUO_ERRMSG_SIZE 128

typedef struct OuoError {
  OuoErrorCode code;

  size_t len;
  size_t line;
  size_t col;
  const char *line_start;

  char msg[OUO_ERRMSG_SIZE];
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
#endif // ouo_int_t

#ifndef ouo_float_t
#define ouo_float_t double
#define ouo_strtof(...) strtod(__VA_ARGS__)
#define OUO_PRIf "f"
#endif // ouo_float_t

typedef enum {
  OUO_TYPE_VOID,
  // Scalar
  OUO_TYPE_INT,
  OUO_TYPE_FLOAT,
} OuoTypeKind;

//
// Lexing
//

typedef enum {
  OUO_TOK_ILLEGAL,
  OUO_TOK_EOF,
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
  size_t line;
  size_t col;
  const char *line_start;
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
  OuoToken tok;
  OuoTypeKind type;

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

/// Caller owns the result's `ast` and `errors`.
OuoParseResult ouo_parse(const char *src);

/// Recursively frees the entire AST tree.
void ouo_ast_free(OuoAst *ast);

/// Prints the AST tree for debugging.
void ouo_ast_dump(OuoAst *ast);

//
// Compilation
//

#ifndef OUO_NOEMIT

typedef enum {
  // Objects
  OUO_OP_CONSTANT,
  // Arithmetic
  OUO_OP_INT_ADD,
  OUO_OP_FLOAT_ADD,
  OUO_OP_INT_MULT,
  OUO_OP_FLOAT_MULT,
  // Control flow
  OUO_OP_RETURN,
} OuoOpCode;

struct OuoObject;

/// Owns memory for `items`, `constants.items` and `lines.items`.
typedef struct {
  uint8_t *items;
  size_t count;
  size_t capacity;

  struct {
    struct OuoObject *items;
    size_t count;
    size_t capacity;
  } constants;

  struct {
    size_t *items;
    size_t count;
    size_t capacity;
  } lines;
} OuoChunk;

#endif // OUO_NOEMIT

/// Owns memory for `chunk.items`, `chunk.constants.items`,
/// `chunk.lines.items` and `errors.items`.
typedef struct {
  bool failed;
#ifndef OUO_NOEMIT
  OuoChunk chunk;
#endif
  OuoErrors errors;
} OuoCompileResult;

/// Caller owns the result's `chunk` and `errors`.
OuoCompileResult ouo_compile(OuoAst *ast);

#ifndef OUO_NOEMIT

/// Frees bytecode, constants, and lines of the chunk.
void ouo_chunk_free(OuoChunk *chunk);

/// Prints bytecode of the chunk for debugging.
void ouo_chunk_dump(OuoChunk *chunk, const char *name);

//
// Virtual machine
//

typedef enum {
  // Pass by value
  OUO_OBJ_INT,
  OUO_OBJ_FLOAT,
} OuoObjectKind;

typedef struct OuoObject {
  OuoObjectKind kind;

  union {
    // Pass by value
    ouo_int_t v_int;
    ouo_float_t v_float;
  };
} OuoObject;

/// Owns memory for `errors.items`.
typedef struct {
  bool failed;
  OuoError error;
} OuoInterpretResult;

OuoInterpretResult ouo_interpret(OuoChunk *chunk);

#endif // OUO_NOEMIT

#endif // OUO_H

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef OUO_IMPLEMENTATION

//
// Error handling
//

#define _OUO_ER "\e[0m"
#define _OUO_EB "\e[1m"
#define _OUO_EBR "\e[0;1m"
#define _OUO_ED "\e[2m"
#define _OUO_EBRED "\e[1;31m"

#define _ouo_err_sprintf(err, fmt, ...) \
  snprintf((err).msg, OUO_ERRMSG_SIZE, fmt, ##__VA_ARGS__)

static const char *_ouo_err_code_str(OuoErrorCode err_code) {
  switch (err_code) {
    case OUO_OK: return "OK :)";
    // Parsing
    case OUO_ERR_PARSE_FAIL: return "PARSING FAIL";
    case OUO_ERR_SYNTAX: return "SYNTAX ERROR";
    // Compilation
    case OUO_ERR_COMPILE_FAIL: return "COMPILATION FAIL";
    case OUO_ERR_TYPE: return "TYPE ERROR";
    // Runtime
    case OUO_ERR_RUNTIME: return "RUNTIME ERROR";
    default: return "ERROR";
  }
}

void ouo_err_msg_print(OuoError *err, const char *src, const char *path) {
  const char *line_start = err->line_start;
  size_t line_len = 0;

  if (line_start == NULL && err->line != 0 && src != NULL) {
    size_t line = 1;
    for (const char *p = src; *p != '\0'; p++) {
      if (line == err->line) {
        line_start = p;
        break;
      }
      if (*p == '\n') line++;
    }
  }

  if (line_start != NULL && src != NULL) {
    const char *line_end = line_start;
    while (*line_end != '\0' && *line_end != '\n') line_end++;
    line_len = (size_t)(line_end - line_start);
  }

  ouo_printerr(_OUO_ED);
  if (path != NULL) ouo_printerr("%s:", path);
  ouo_printerr("%zu:", err->line);
  if (err->col != 0) ouo_printerr("%zu:", err->col);
  ouo_printerr(_OUO_EBR _OUO_EBRED " %s: " _OUO_EBR "%s" _OUO_ER,
      _ouo_err_code_str(err->code), err->msg);

  if (line_len != 0) {
    ouo_printerr(_OUO_ED "\n%5zu | " _OUO_ER "%.*s", err->line, (int)line_len,
        line_start);
    if (err->len > 0) {
      ouo_printerr(_OUO_ED "\n      | " _OUO_ER _OUO_EB);
      for (size_t i = 0; i < err->col - 1; i++) ouo_printerr(" ");
      for (size_t i = 0; i < err->len; i++) ouo_printerr("^");
      ouo_printerr(_OUO_ER);
    }
  }

  ouo_printerr("\n");
}

//
// Data types
//

static const char *_ouo_type_kind_str(OuoTypeKind kind) {
  switch (kind) {
    case OUO_TYPE_VOID: return "void";
    // Scalar
    case OUO_TYPE_INT: return "int";
    case OUO_TYPE_FLOAT: return "float";
  }
  return "";
}

//
// Lexing
//

typedef struct {
  const char *start;
  const char *curr;

  size_t line;
  size_t col;
  const char *line_start;
} _OuoLexer;

static inline void _ouo_l_init(_OuoLexer *l, const char *src) {
  l->start = src;
  l->curr = src;

  l->line = 1;
  l->col = 1;
  l->line_start = src;
}

static inline bool _ouo_l_is_eof(_OuoLexer *l) { return *l->curr == '\0'; }

static inline bool _ouo_l_is_digit(char c) { return c >= '0' && c <= '9'; }

static inline char _ouo_l_advance(_OuoLexer *l) {
  l->col++;
  l->curr++;
  return l->curr[-1];
}

static inline char _ouo_l_peek(_OuoLexer *l) { return *l->curr; }

static inline char _ouo_l_peek_next(_OuoLexer *l) {
  if (_ouo_l_is_eof(l)) return '\0';
  return l->curr[1];
}

static inline void _ouo_l_skip_whitespace(_OuoLexer *l) {
  for (;;) {
    char c = _ouo_l_peek(l);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      _ouo_l_advance(l);
      if (c == '\n') {
        l->line++;
        l->col = 1;
        l->line_start = l->curr;
      }
    } else return;
  }
}

static inline OuoToken _ouo_l_tok_new(_OuoLexer *l, OuoTokenKind kind) {
  size_t len = (size_t)(l->curr - l->start);
  return (OuoToken){
      .kind = kind,
      .start = l->start,
      .len = len,
      .line = l->line,
      .col = l->col - len,
      .line_start = l->line_start,
  };
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
  l->start = l->curr;

  if (_ouo_l_is_eof(l)) return _ouo_l_tok_new(l, OUO_TOK_EOF);

  char c = _ouo_l_advance(l);

  // Literals
  if (_ouo_l_is_digit(c)) return _ouo_l_read_number(l);

  switch (c) {
    // Operators
    case '+': return _ouo_l_tok_new(l, OUO_TOK_PLUS);
    case '*': return _ouo_l_tok_new(l, OUO_TOK_ASTERISK);
    default: break;
  }

  return _ouo_l_tok_new(l, OUO_TOK_ILLEGAL);
}

static const char *_ouo_tok_kind_str(OuoTokenKind kind) {
  switch (kind) {
    case OUO_TOK_ILLEGAL: return "ILLEGAL";
    case OUO_TOK_EOF: return "EOF";
    // Literals
    case OUO_TOK_LIT_INT: return "LIT_INT";
    case OUO_TOK_LIT_FLOAT: return "LIT_FLOAT";
    // Operators
    case OUO_TOK_PLUS: return "+";
    case OUO_TOK_ASTERISK: return "*";
  }
  return "";
}

//
// Parsing
//

struct _OuoParseRule;

typedef struct {
  _OuoLexer *l;
  OuoToken curr;
  OuoToken peek;

  bool panic_mode;
  OuoParseResult *res;

  struct {
    const struct _OuoParseRule *items;
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

typedef struct _OuoParseRule {
  _OuoParsePrefixFn prefix_fn;
  _OuoParseInfixFn infix_fn;
  _OuoPrecedence prec;
} _OuoParseRule;

#define _ouo_p_err(p, tok, err_code, fmt, ...) \
  do { \
    if (!(p)->panic_mode) { \
      (p)->res->failed = true; \
      (p)->panic_mode = true; \
      OuoError err = { \
          .code = (err_code), \
          .len = (tok).len == 0 ? 1 : (tok).len, \
          .line = (tok).line, \
          .col = (tok).col, \
          .line_start = (tok).line_start, \
          .msg = {0}, \
      }; \
      _ouo_err_sprintf(err, fmt, ##__VA_ARGS__); \
      ouo_da_append(&(p)->res->errors, err); \
    } \
  } while (0)

static inline void _ouo_p_advance(_OuoParser *p) {
  p->curr = p->peek;
  p->peek = _ouo_l_next_token(p->l);
}

static inline void _ouo_p_init(_OuoParser *p, _OuoLexer *l, OuoParseResult *res,
    const _OuoParseRule *rules, size_t rules_count) {
  p->l = l;
  p->res = res;
  p->rules.items = rules;
  p->rules.count = rules_count;
  _ouo_p_advance(p);
  _ouo_p_advance(p);
}

static inline OuoAst *_ouo_ast_new(OuoToken *tok, OuoAstKind kind) {
  OuoAst *ast = ouo_malloc(sizeof(OuoAst));
  ouo_assert_nomem(ast);
  ast->kind = kind;
  ast->tok = *tok;
  ast->type = OUO_TYPE_VOID;
  return ast;
}

static inline const _OuoParseRule *_ouo_p_get_rule(
    _OuoParser *p, OuoTokenKind tok) {
  if (tok >= p->rules.count) return &p->rules.items[OUO_TOK_EOF];
  return &p->rules.items[tok];
}

static OuoAst *_ouo_p_expr(_OuoParser *p, _OuoPrecedence prec) {
  _OuoParsePrefixFn prefix_fn = _ouo_p_get_rule(p, p->curr.kind)->prefix_fn;

  if (prefix_fn == NULL) {
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX,
        "Expected an expression, got '%.*s'.", (int)p->curr.len, p->curr.start);
    return NULL;
  }

  OuoAst *left = prefix_fn(p);

  while (p->peek.kind != OUO_TOK_EOF &&
         prec <= _ouo_p_get_rule(p, p->peek.kind)->prec) {
    _OuoParseInfixFn infix_fn = _ouo_p_get_rule(p, p->peek.kind)->infix_fn;

    if (infix_fn == NULL) {
      _ouo_p_err(p, p->peek, OUO_ERR_SYNTAX,
          "Expected a binary operator, got '%.*s'.", (int)p->peek.len,
          p->peek.start);
      goto errdefer;
    }

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

  if (errno != 0) {
    _ouo_p_err(p, p->curr, OUO_ERR_PARSE_FAIL,
        "Integer literal value out of range (min %" OUO_PRId ", max %" OUO_PRId
        ").",
        OUO_INT_MIN, OUO_INT_MAX);
    return NULL;
  }

  if (end != p->curr.start + p->curr.len) {
    _ouo_p_err(p, p->curr, OUO_ERR_PARSE_FAIL,
        "Integer literal length mismatch. Expected %zu, read %zu.", p->curr.len,
        end - p->curr.start);
    return NULL;
  }

  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_LIT_INT);
  ast->lit_int = lit;
  return ast;
}

static OuoAst *_ouo_p_lit_float(_OuoParser *p) {
  errno = 0;
  char *end = NULL;
  ouo_float_t lit = ouo_strtof(p->curr.start, &end);

  if (errno != 0) {
    _ouo_p_err(
        p, p->curr, OUO_ERR_PARSE_FAIL, "Float literal value out of range.");
    return NULL;
  }

  if (end != p->curr.start + p->curr.len) {
    _ouo_p_err(p, p->curr, OUO_ERR_PARSE_FAIL,
        "Float literal length mismatch. Expected %zu, read %zu.", p->curr.len,
        end - p->curr.start);
    return NULL;
  }

  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_LIT_FLOAT);
  ast->lit_float = lit;
  return ast;
}

static OuoAst *_ouo_p_bin_op(_OuoParser *p, OuoAst *left) {
  OuoToken op = p->curr;
  _OuoPrecedence prec = _ouo_p_get_rule(p, op.kind)->prec;
  _ouo_p_advance(p);
  OuoAst *right = _ouo_p_expr(p, prec);

  OuoAst *ast = _ouo_ast_new(&op, OUO_AST_BIN_OP);
  ast->bin_op.left = left;
  ast->bin_op.op = op.kind;
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

#ifdef OUO_DEBUG
  for (;;) {
    OuoToken tok = _ouo_l_next_token(&l);
    ouo_printdbg(
        "[%s '%.*s'] ", _ouo_tok_kind_str(tok.kind), (int)tok.len, tok.start);
    if (tok.kind == OUO_TOK_EOF) break;
  }
  ouo_printdbg("\n");
  _ouo_l_init(&l, src);
#endif

  OuoParseResult res = {0};
  _OuoParser p = {0};
  _ouo_p_init(&p, &l, &res, _ouo_p_rules, ouo_arr_len(_ouo_p_rules));

  res.ast = _ouo_p_expr(&p, OUO_PREC_LOWEST);

#ifdef OUO_DEBUG
  ouo_ast_dump(res.ast);
  ouo_printdbg("\n");
#endif

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

  ouo_printdbg("(%s ", _ouo_ast_kind_str(ast->kind));

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

//
// Compilation
//

typedef struct {
  bool panic_mode;
  OuoCompileResult *res;
} _OuoCompiler;

#define _ouo_c_err(c, ast, err_code, fmt, ...) \
  do { \
    if (!(c)->panic_mode) { \
      (c)->res->failed = true; \
      (c)->panic_mode = true; \
      OuoError err = { \
          .code = (err_code), \
          .len = (ast)->tok.len, \
          .line = (ast)->tok.line, \
          .col = (ast)->tok.col, \
          .line_start = (ast)->tok.line_start, \
          .msg = {0}, \
      }; \
      _ouo_err_sprintf(err, fmt, ##__VA_ARGS__); \
      ouo_da_append(&(c)->res->errors, err); \
    } \
  } while (0)

static inline void _ouo_c_init(_OuoCompiler *c, OuoCompileResult *res) {
  c->res = res;
}

// Static analysis

static void _ouo_c_err_bin_op_type(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast, OUO_ERR_TYPE,
      "Operation '%s' does not support '%s' and '%s'.",
      _ouo_tok_kind_str(ast->bin_op.op),
      _ouo_type_kind_str(ast->bin_op.left->type),
      _ouo_type_kind_str(ast->bin_op.right->type));
}

static void _ouo_c_err_bin_op_unknown(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast, OUO_ERR_COMPILE_FAIL, "Unknown binary operator '%s'.",
      _ouo_tok_kind_str(ast->bin_op.op));
}

static inline bool _ouo_ast_bin_op_is_int(OuoAst *ast) {
  return ast->bin_op.left->type == OUO_TYPE_INT &&
         ast->bin_op.right->type == OUO_TYPE_INT;
}

static inline bool _ouo_ast_bin_op_is_float(OuoAst *ast) {
  return ast->bin_op.left->type == OUO_TYPE_FLOAT &&
         ast->bin_op.right->type == OUO_TYPE_FLOAT;
}

static bool _ouo_c_ast_analyze(_OuoCompiler *c, OuoAst *ast) {
  bool failed = false;

  switch (ast->kind) {
    // Literals
    case OUO_AST_LIT_INT: ast->type = OUO_TYPE_INT; break;
    case OUO_AST_LIT_FLOAT: ast->type = OUO_TYPE_FLOAT; break;

    // Expressions
    case OUO_AST_BIN_OP:
      switch (ast->bin_op.op) {
        // Arithmetic
        case OUO_TOK_PLUS:
        case OUO_TOK_ASTERISK:
          if (_ouo_ast_bin_op_is_int(ast)) ast->type = OUO_TYPE_INT;
          else if (_ouo_ast_bin_op_is_float(ast)) ast->type = OUO_TYPE_FLOAT;
          else {
            failed = true;
            _ouo_c_err_bin_op_type(c, ast);
          }
          break;

        default: _ouo_c_err_bin_op_unknown(c, ast); break;
      }
      break;
  }

  return failed;
}

#ifndef OUO_NOEMIT

// Bytecode emitting

static void _ouo_c_err_bin_op_unanalyzed(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast, OUO_ERR_COMPILE_FAIL,
      "Unanalyzed binary operation for '%s %s %s'.",
      _ouo_type_kind_str(ast->bin_op.left->type),
      _ouo_tok_kind_str(ast->bin_op.op),
      _ouo_type_kind_str(ast->bin_op.right->type));
}

static inline void _ouo_c_chunk_write(
    _OuoCompiler *c, uint8_t byte, size_t line) {
  ouo_da_append(&c->res->chunk, byte);
  ouo_da_append(&c->res->chunk.lines, line);
}

static inline size_t _ouo_c_chunk_add_constant(
    _OuoCompiler *c, OuoObject *obj) {
  ouo_da_append(&c->res->chunk.constants, *obj);
  return c->res->chunk.constants.count - 1;
}

static inline void _ouo_c_emit_byte(
    _OuoCompiler *c, OuoAst *ast, uint8_t byte) {
  _ouo_c_chunk_write(c, byte, ast->tok.line);
}

static inline void _ouo_c_emit_bytes(
    _OuoCompiler *c, OuoAst *ast, uint8_t byte1, uint8_t byte2) {
  _ouo_c_emit_byte(c, ast, byte1);
  _ouo_c_emit_byte(c, ast, byte2);
}

static void _ouo_c_emit_constant(_OuoCompiler *c, OuoAst *ast, OuoObject *obj) {
  size_t constant = _ouo_c_chunk_add_constant(c, obj);

  if (constant > UINT8_MAX) {
    _ouo_c_err(c, ast, OUO_ERR_COMPILE_FAIL,
        "Maximum amount of constants exceeded (max %d).", UINT8_MAX + 1);
    return;
  }

  _ouo_c_emit_bytes(c, ast, OUO_OP_CONSTANT, (uint8_t)constant);
}

static void _ouo_c_ast_emit(_OuoCompiler *c, OuoAst *ast) {
  switch (ast->kind) {
    // Literals
    case OUO_AST_LIT_INT: {
      OuoObject obj = {
          .kind = OUO_OBJ_INT,
          .v_int = ast->lit_int,
      };
      _ouo_c_emit_constant(c, ast, &obj);
      break;
    }

    case OUO_AST_LIT_FLOAT: {
      OuoObject obj = {
          .kind = OUO_OBJ_FLOAT,
          .v_float = ast->lit_float,
      };
      _ouo_c_emit_constant(c, ast, &obj);
      break;
    }

    // Expressions
    case OUO_AST_BIN_OP:
      switch (ast->bin_op.op) {
        // Arithmetic
        case OUO_TOK_PLUS:
          if (_ouo_ast_bin_op_is_int(ast))
            _ouo_c_emit_byte(c, ast, OUO_OP_INT_ADD);
          else if (_ouo_ast_bin_op_is_float(ast))
            _ouo_c_emit_byte(c, ast, OUO_OP_FLOAT_ADD);
          else _ouo_c_err_bin_op_unanalyzed(c, ast);
          break;

        case OUO_TOK_ASTERISK:
          if (_ouo_ast_bin_op_is_int(ast))
            _ouo_c_emit_byte(c, ast, OUO_OP_INT_MULT);
          else if (_ouo_ast_bin_op_is_float(ast))
            _ouo_c_emit_byte(c, ast, OUO_OP_FLOAT_MULT);
          else _ouo_c_err_bin_op_unanalyzed(c, ast);
          break;

        default: _ouo_c_err_bin_op_unknown(c, ast); break;
      }
      break;
  }
}

#endif // OUO_NOEMIT

static void _ouo_c_ast(_OuoCompiler *c, OuoAst *ast) {
  if (ast == NULL) return;

  switch (ast->kind) {
    // Literals
    case OUO_AST_LIT_INT:
    case OUO_AST_LIT_FLOAT: break;
    // Expressions
    case OUO_AST_BIN_OP:
      _ouo_c_ast(c, ast->bin_op.left);
      _ouo_c_ast(c, ast->bin_op.right);
      break;
  }

  bool failed = _ouo_c_ast_analyze(c, ast);
  if (failed) return;

#ifndef OUO_NOEMIT
  _ouo_c_ast_emit(c, ast);
#endif
}

OuoCompileResult ouo_compile(OuoAst *ast) {
  OuoCompileResult res = {0};
  _OuoCompiler c = {0};
  _ouo_c_init(&c, &res);

  _ouo_c_ast(&c, ast);

#ifndef OUO_NOEMIT

  _ouo_c_chunk_write(&c, OUO_OP_RETURN,
      res.chunk.lines.count > 0
          ? res.chunk.lines.items[res.chunk.lines.count - 1]
          : 0);

#ifdef OUO_DEBUG
  ouo_chunk_dump(&res.chunk, "main");
  ouo_printdbg("\n");
#endif

#endif // OUO_NOEMIT

  return res;
}

#ifndef OUO_NOEMIT

void ouo_chunk_free(OuoChunk *chunk) {
  ouo_da_free(chunk->constants);
  ouo_da_free(chunk->lines);
  ouo_da_free(*chunk);
}

static const char *_ouo_op_code_str(OuoOpCode op_code) {
  switch (op_code) {
    // Objects
    case OUO_OP_CONSTANT: return "CONSTANT";
    // Arithmetic
    case OUO_OP_INT_ADD: return "INT_ADD";
    case OUO_OP_FLOAT_ADD: return "FLOAT_ADD";
    case OUO_OP_INT_MULT: return "INT_MULT";
    case OUO_OP_FLOAT_MULT: return "FLOAT_MULT";
    // Control flow
    case OUO_OP_RETURN: return "RETURN";
  }
  return "";
}

static void _ouo_obj_dump(OuoObject *obj) {
  switch (obj->kind) {
    case OUO_OBJ_INT: ouo_printdbg("%" OUO_PRId, obj->v_int); break;
    case OUO_OBJ_FLOAT: ouo_printdbg("%" OUO_PRIf, obj->v_float); break;
  }
}

static ptrdiff_t _ouo_chunk_op_dump(OuoChunk *chunk, uint8_t *ip) {
  uint8_t *ip_prev = ip;
  ptrdiff_t i = ip - chunk->items;

  ouo_printdbg("%04ld ", i);
  if (i > 0 && chunk->lines.items[i] == chunk->lines.items[i - 1])
    ouo_printdbg("   | ");
  else ouo_printdbg("%4zu ", chunk->lines.items[i]);

  OuoOpCode op_code = (OuoOpCode)*ip;
  ouo_printdbg("%-16s", _ouo_op_code_str(op_code));

  switch (op_code) {
    case OUO_OP_CONSTANT: {
      uint8_t constant = *(++ip);
      ouo_printdbg("%4d ", constant);
      _ouo_obj_dump(&chunk->constants.items[constant]);
      break;
    }
    default: break;
  }

  return ip - ip_prev;
}

void ouo_chunk_dump(OuoChunk *chunk, const char *name) {
  ouo_printdbg("%s:\n", name);

  OUO_DA_FOREACH(uint8_t, ip, chunk) {
    ip += _ouo_chunk_op_dump(chunk, ip);
    ouo_printdbg("\n");
  }

  ouo_printdbg("---------------------------------------\n");
}

//
// Virtual machine
//

#define OUO_VM_STACK_SIZE 256

typedef struct {
  OuoChunk *chunk;
  OuoObject stack[OUO_VM_STACK_SIZE];
  OuoObject *stack_top;
  OuoObject *stack_max;

  OuoInterpretResult *res;
} _OuoVm;

#define _ouo_vm_err(vm, line_num, err_code, fmt, ...) \
  do { \
    (vm)->res->failed = true; \
    OuoError error = { \
        .code = (err_code), \
        .line = (line_num), \
        .line_start = NULL, \
        .msg = {0}, \
    }; \
    _ouo_err_sprintf(error, fmt, ##__VA_ARGS__); \
    (vm)->res->error = error; \
  } while (0)

static inline void _ouo_vm_init(
    _OuoVm *vm, OuoInterpretResult *res, OuoChunk *chunk) {
  vm->res = res;
  vm->chunk = chunk;
  vm->stack_top = vm->stack;
  vm->stack_max = &vm->stack[OUO_VM_STACK_SIZE];
}

static inline size_t _ouo_vm_get_line(_OuoVm *vm, const uint8_t *ip) {
  return vm->chunk->lines.items[ip - vm->chunk->items];
}

static inline void _ouo_vm_stack_push(_OuoVm *vm, uint8_t *ip, OuoObject obj) {
  if (vm->stack_top == vm->stack_max) {
    _ouo_vm_err(vm, _ouo_vm_get_line(vm, ip), OUO_ERR_RUNTIME,
        "Maximum stack size exceeded (max %d).", OUO_VM_STACK_SIZE);
    return;
  }

  *vm->stack_top = obj;
  vm->stack_top++;
}

static inline OuoObject *_ouo_vm_stack_pop(_OuoVm *vm) {
  ouo_assertf(
      vm->stack_top > vm->stack, OUO_ERR_RUNTIME, "Trying to pop empty stack.");
  vm->stack_top--;
  return vm->stack_top;
}

#define _ouo_obj_new_int(v) \
  ((OuoObject){ \
      .kind = OUO_OBJ_INT, \
      .v_int = (v), \
  })

#define _ouo_obj_new_float(v) \
  ((OuoObject){ \
      .kind = OUO_OBJ_FLOAT, \
      .v_float = (v), \
  })

#define _ouo_vm_read_constant(vm, ip) ((vm)->chunk->constants.items[*(++(ip))])

#define _OUO_VM_BIN_OP(vm, ip, T, OP) \
  do { \
    ouo_##T##_t b = _ouo_vm_stack_pop((vm))->v_##T; \
    ouo_##T##_t a = _ouo_vm_stack_pop((vm))->v_##T; \
    _ouo_vm_stack_push((vm), (ip), _ouo_obj_new_##T(a OP b)); \
  } while (0)

static void _ouo_vm_run(_OuoVm *vm) {
  OUO_DA_FOREACH(uint8_t, ip, vm->chunk) {
    if (vm->res->failed) return;

#ifdef OUO_DEBUG
    if (vm->stack_top != vm->stack) {
      for (OuoObject *slot = vm->stack; slot < vm->stack_top; slot++) {
        ouo_printdbg("[");
        _ouo_obj_dump(slot);
        ouo_printdbg("] ");
      }
      ouo_printdbg("\n");
    }

    _ouo_chunk_op_dump(vm->chunk, ip);
    ouo_printdbg("\n");
#endif

    OuoOpCode op_code = (OuoOpCode)*ip;
    switch (op_code) {
      // Objects
      case OUO_OP_CONSTANT: {
        OuoObject constant = _ouo_vm_read_constant(vm, ip);
        _ouo_vm_stack_push(vm, ip, constant);
        break;
      }

      // Arithmetic
      case OUO_OP_INT_ADD: _OUO_VM_BIN_OP(vm, ip, int, +); break;
      case OUO_OP_FLOAT_ADD: _OUO_VM_BIN_OP(vm, ip, float, +); break;
      case OUO_OP_INT_MULT: _OUO_VM_BIN_OP(vm, ip, int, *); break;
      case OUO_OP_FLOAT_MULT: _OUO_VM_BIN_OP(vm, ip, float, *); break;

      // Control flow
      case OUO_OP_RETURN: {
        OuoObject *obj = _ouo_vm_stack_pop(vm);
        _ouo_obj_dump(obj);
        ouo_printdbg("\n");
        return;
      }
    }
  }
}

OuoInterpretResult ouo_interpret(OuoChunk *chunk) {
  OuoInterpretResult res = {0};
  _OuoVm vm = {0};
  _ouo_vm_init(&vm, &res, chunk);

  _ouo_vm_run(&vm);

  return res;
}

#endif // OUO_NOEMIT

#endif // OUO_IMPLEMENTATION
