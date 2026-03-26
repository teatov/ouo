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

#define OUO_ER "\e[0m"
#define OUO_EB "\e[1m"
#define OUO_EBR "\e[0;1m"
#define OUO_ED "\e[2m"
#define OUO_EBD "\e[1;2m"
#define OUO_EBRED "\e[1;31m"
#define OUO_EBGRN "\e[1;32m"

#define ouo_abort(err_code, fmt, ...) \
  do { \
    ouo_printerr(fmt "\n", ##__VA_ARGS__); \
    exit(err_code); \
  } while (0)

// Assertions

#define ouo_assert(expr, err_code, fmt, ...) \
  if (!(expr)) ouo_abort(err_code, fmt, ##__VA_ARGS__)

#define ouo_assertf(expr, err_code, fmt, ...) \
  do { \
    if (!(expr)) { \
      ouo_printtrc(); \
      ouo_abort(err_code, fmt, ##__VA_ARGS__); \
    } \
  } while (0)

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
    (da).count = 0; \
    (da).capacity = 0; \
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

#ifdef OUO_DEBUG
#ifndef ouo_printdbg
#define ouo_printdbg(...) fprintf(stderr, __VA_ARGS__)
#endif
#endif // OUO_DEBUG

#ifndef ouo_printtrc
#define ouo_printtrc() \
  fprintf(stderr, OUO_ED OUO_CODEPOS "%s: " OUO_ER, __func__)
#endif

//
// Error handling
//

typedef enum {
  OUO_OK,
  OUO_ERR_NOTE,
  // General
  OUO_ERR_OUT_OF_MEMORY,
  OUO_ERR_USAGE,
  OUO_ERR_READ,
  // Parsing
  OUO_ERR_PARSE_FAIL,
  OUO_ERR_SYNTAX,
  // Compilation
  OUO_ERR_COMPILE_FAIL,
  OUO_ERR_SEMANTIC,
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
// Types
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
  OUO_TYPE_UNKNOWN,
  OUO_TYPE_VOID,
  // Scalar
  OUO_TYPE_INT,
  OUO_TYPE_FLOAT,
  OUO_TYPE_BOOL,
} OuoTypeKind;

//
// Lexing
//

typedef enum {
  OUO_TOK_ILLEGAL,
  OUO_TOK_EOF,
  OUO_TOK_NEWLINE,
  OUO_TOK_IDENT,
  // Keywords
  OUO_TOK_KW_PRINT,
  OUO_TOK_KW_VAR,
  OUO_TOK_KW_INT,
  OUO_TOK_KW_FLOAT,
  OUO_TOK_KW_BOOL,
  // Literals
  OUO_TOK_LIT_INT,
  OUO_TOK_LIT_FLOAT,
  OUO_TOK_LIT_TRUE,
  OUO_TOK_LIT_FALSE,
  // Operators
  OUO_TOK_ASSIGN,
  OUO_TOK_PLUS,
  OUO_TOK_ASTERISK,
  // Punctuation
  OUO_TOK_PAREN_OPN,
  OUO_TOK_PAREN_CLS,
  OUO_TOK_BRACE_OPN,
  OUO_TOK_BRACE_CLS,
  OUO_TOK_COLON,
} OuoTokenKind;

typedef struct {
  OuoTokenKind kind;
  const char *start;
  size_t len;
  size_t line;
  size_t col;
  const char *line_start;
} OuoToken;

#ifdef OUO_DEBUG
/// Prints the token for debugging.
void ouo_tok_dump(OuoToken *tok);
#endif // OUO_DEBUG

//
// Parsing
//

typedef enum {
  OUO_AST_MODULE,
  OUO_AST_IDENT,
  // Literals
  OUO_AST_LIT_INT,
  OUO_AST_LIT_FLOAT,
  OUO_AST_LIT_BOOL,
  // Expressions
  OUO_AST_ASSIGN,
  OUO_AST_BIN_OP,
  OUO_AST_BLOCK,
  // Statements
  OUO_AST_EXPR_STMT,
  OUO_AST_PRINT,
  OUO_AST_DECL_VAR,
} OuoAstKind;

/// Owns memory for any child AST nodes.
typedef struct OuoAst {
  OuoAstKind kind;
  OuoToken tok;
  OuoTypeKind type;

  union {
    // Common
    struct OuoAst *child;

    struct {
      struct OuoAst **items;
      size_t count;
      size_t capacity;
    } children;

    struct {
      OuoToken name;
      size_t sym_idx;
    } ident;

    // Literals
    ouo_int_t lit_int;
    ouo_float_t lit_float;
    bool lit_bool;

    // Expressions
    struct {
      struct OuoAst *target;
      struct OuoAst *value;
    } assign;

    struct {
      struct OuoAst *left;
      OuoTokenKind op;
      struct OuoAst *right;
    } bin_op;

    struct {
      OuoToken name;
      OuoTypeKind type_annotation;
      struct OuoAst *value;
    } decl_var;
  };
} OuoAst;

/// Owns memory for `ast` and `errors.items`.
typedef struct {
  bool failed;
  size_t line;
  OuoAst *ast;
  OuoErrors errors;
} OuoParseResult;

/// Caller owns the result's `ast` and `errors`.
void ouo_parse(const char *src, OuoParseResult *res);

/// Recursively frees the entire AST tree.
void ouo_ast_free(OuoAst *ast);

#ifdef OUO_DEBUG
/// Prints the AST tree for debugging.
void ouo_ast_dump(OuoAst *ast);
#endif // OUO_DEBUG

//
// Compilation
//

typedef struct {
  OuoToken name;
  OuoTypeKind type;
  size_t scope_depth;
} OuoSymbol;

#ifndef OUO_NOEMIT

typedef enum {
  // Objects
  OUO_OP_POP,
  OUO_OP_VAR_GET,
  OUO_OP_VAR_SET,
  OUO_OP_LITERAL,
  // Arithmetic
  OUO_OP_ADD_INT,
  OUO_OP_ADD_FLOAT,
  OUO_OP_MULT_INT,
  OUO_OP_MULT_FLOAT,
  // Control flow
  OUO_OP_RETURN,
  // I/O
  OUO_OP_PRINT,
} OuoOpCode;

struct OuoObject;

/// Owns memory for `items`, `literals.items` and `lines.items`.
typedef struct {
  uint8_t *items;
  size_t count;
  size_t capacity;

  struct {
    struct OuoObject *items;
    size_t count;
    size_t capacity;
  } literals;

  // RLE encoded, length first
  struct {
    size_t *items;
    size_t count;
    size_t capacity;
  } lines;
} OuoChunk;

#endif // OUO_NOEMIT

/// Owns memory for `chunk.items`, `chunk.literals.items`,
/// `chunk.lines.items`, `symbols.items` and `errors.items`.
typedef struct {
  bool failed;
  bool keep_module_scope;
  bool echo;

#ifndef OUO_NOEMIT
  OuoChunk chunk;
#endif

  struct {
    OuoSymbol *items;
    size_t count;
    size_t capacity;
  } symbols;

  OuoErrors errors;
} OuoCompileResult;

/// Caller owns the result's `chunk` and `errors`.
void ouo_compile(OuoAst *ast, OuoCompileResult *res);

#ifndef OUO_NOEMIT

/// Frees bytecode, literals, and lines of the chunk.
void ouo_chunk_free(OuoChunk *chunk);

#ifdef OUO_DEBUG
/// Prints bytecode of the chunk for debugging.
void ouo_chunk_dump(OuoChunk *chunk, const char *name);
#endif // OUO_DEBUG

//
// Virtual machine
//

typedef enum {
  // Pass by value
  OUO_OBJ_INT,
  OUO_OBJ_FLOAT,
  OUO_OBJ_BOOL,
} OuoObjectKind;

typedef struct OuoObject {
  OuoObjectKind kind;

  union {
    // Pass by value
    ouo_int_t v_int;
    ouo_float_t v_float;
    bool v_bool;
  };
} OuoObject;

#define OUO_VM_STACK_SIZE 256

/// Owns memory for `errors.items`.
typedef struct {
  bool failed;

  struct {
    OuoObject items[OUO_VM_STACK_SIZE];
    size_t count;
  } stack;

  OuoError error;
} OuoInterpretResult;

void ouo_interpret(OuoChunk *chunk, OuoInterpretResult *res);

#endif // OUO_NOEMIT

#endif // OUO_H

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef OUO_IMPLEMENTATION
//
// Error handling
//

#define _ouo_err_sprintf(err, fmt, ...) \
  snprintf((err).msg, OUO_ERRMSG_SIZE, fmt, ##__VA_ARGS__)

static const char *_ouo_err_code_str(OuoErrorCode err_code) {
  switch (err_code) {
    case OUO_OK: return "OK :)";
    case OUO_ERR_NOTE: return "NOTE";
    // Parsing
    case OUO_ERR_PARSE_FAIL: return "PARSING FAIL";
    case OUO_ERR_SYNTAX: return "SYNTAX ERROR";
    // Compilation
    case OUO_ERR_COMPILE_FAIL: return "COMPILATION FAIL";
    case OUO_ERR_SEMANTIC: return "SEMANTIC ERROR";
    case OUO_ERR_TYPE: return "TYPE ERROR";
    // Runtime
    case OUO_ERR_RUNTIME: return "RUNTIME ERROR";
    // Generic
    case OUO_ERR_OUT_OF_MEMORY:
    case OUO_ERR_USAGE:
    case OUO_ERR_READ: return "ERROR";
  }
  return "";
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

  ouo_printerr(OUO_ED);
  if (path != NULL) ouo_printerr("%s:", path);
  ouo_printerr("%zu:", err->line);
  if (err->col != 0) ouo_printerr("%zu:", err->col);
  ouo_printerr(OUO_ER "%s %s: " OUO_EBR "%s" OUO_ER,
      err->code == OUO_ERR_NOTE ? OUO_EBD : OUO_EBRED,
      _ouo_err_code_str(err->code), err->msg);

  if (line_len != 0) {
    ouo_printerr("\n%.*s", (int)line_len, line_start);
    if (err->len > 0) {
      ouo_printerr("\n" OUO_ED);
      if (err->col > 0)
        for (size_t i = 0; i < err->col - 1; i++) ouo_printerr(" ");
      for (size_t i = 0; i < err->len; i++) ouo_printerr("^");
      ouo_printerr(OUO_ER);
    }
  }

  ouo_printerr("\n");
}

//
// Types
//

static const char *_ouo_type_kind_str(OuoTypeKind kind) {
  switch (kind) {
    case OUO_TYPE_UNKNOWN: return "unknown";
    case OUO_TYPE_VOID: return "void";
    // Scalar
    case OUO_TYPE_INT: return "int";
    case OUO_TYPE_FLOAT: return "float";
    case OUO_TYPE_BOOL: return "bool";
  }
  return "";
}

//
// Lexing
//

#define _OUO_TOK_FMT_ARGS(tok) \
  (tok).kind == OUO_TOK_EOF           ? 3 \
      : (tok).kind == OUO_TOK_NEWLINE ? 2 \
                                      : (int)(tok).len, \
      (tok).kind == OUO_TOK_EOF       ? "EOF" \
      : (tok).kind == OUO_TOK_NEWLINE ? "LF" \
                                      : (tok).start

typedef struct {
  const char *tok_start;
  const char *curr;

  size_t line;
  size_t col;
  const char *line_start;
} _OuoLexer;

static inline void _ouo_l_init(
    _OuoLexer *l, OuoParseResult *res, const char *src) {
  l->tok_start = src;
  l->curr = src;

  l->line = res->line + 1;
  l->col = 1;
  l->line_start = src;
}

static inline bool _ouo_l_is_eof(_OuoLexer *l) { return *l->curr == '\0'; }

static inline bool _ouo_l_isdigit(char c) { return c >= '0' && c <= '9'; }

static inline bool _ouo_l_isalpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static inline bool _ouo_l_isspace(char c) {
  return c == ' ' || c == '\t' || c == '\r';
}

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
    if (_ouo_l_isspace(c)) _ouo_l_advance(l);
    else return;
  }
}

static inline OuoToken _ouo_l_tok_new(_OuoLexer *l, OuoTokenKind kind) {
  size_t len = (size_t)(l->curr - l->tok_start);
  return (OuoToken){
      .kind = kind,
      .start = l->tok_start,
      .len = len,
      .line = l->line,
      .col = l->col - len,
      .line_start = l->line_start,
  };
}

static inline OuoToken _ouo_l_check_kw(_OuoLexer *l, size_t rest_start,
    size_t rest_len, const char *rest, OuoTokenKind kind) {
  if (l->curr - l->tok_start == (ptrdiff_t)(rest_start + rest_len) &&
      memcmp(l->tok_start + rest_start, rest, rest_len) == 0) {
    return _ouo_l_tok_new(l, kind);
  }
  return _ouo_l_tok_new(l, OUO_TOK_IDENT);
}

static OuoToken _ouo_l_read_word(_OuoLexer *l) {
  while (_ouo_l_isalpha(_ouo_l_peek(l))) _ouo_l_advance(l);

  bool is_long = l->curr - l->tok_start > 1;
  switch (l->tok_start[0]) {
    case 'v': return _ouo_l_check_kw(l, 1, 2, "ar", OUO_TOK_KW_VAR);
    case 'p': return _ouo_l_check_kw(l, 1, 4, "rint", OUO_TOK_KW_PRINT);
    case 'i': return _ouo_l_check_kw(l, 1, 2, "nt", OUO_TOK_KW_INT);
    case 'f':
      if (is_long) {
        switch (l->tok_start[1]) {
          case 'l': return _ouo_l_check_kw(l, 2, 3, "oat", OUO_TOK_KW_FLOAT);
          case 'a': return _ouo_l_check_kw(l, 2, 3, "lse", OUO_TOK_LIT_FALSE);
        }
      }
      break;
    case 'b': return _ouo_l_check_kw(l, 1, 3, "ool", OUO_TOK_KW_BOOL);
    case 't': return _ouo_l_check_kw(l, 1, 3, "rue", OUO_TOK_LIT_TRUE);
  }

  return _ouo_l_tok_new(l, OUO_TOK_IDENT);
}

static OuoToken _ouo_l_read_number(_OuoLexer *l) {
  while (_ouo_l_isdigit(_ouo_l_peek(l))) _ouo_l_advance(l);

  OuoTokenKind kind = OUO_TOK_LIT_INT;
  if (_ouo_l_peek(l) == '.' && _ouo_l_isdigit(_ouo_l_peek_next(l))) {
    kind = OUO_TOK_LIT_FLOAT;
    _ouo_l_advance(l);

    while (_ouo_l_isdigit(_ouo_l_peek(l))) _ouo_l_advance(l);
  }

  return _ouo_l_tok_new(l, kind);
}

static OuoToken _ouo_l_next_token(_OuoLexer *l) {
  _ouo_l_skip_whitespace(l);
  l->tok_start = l->curr;

  if (_ouo_l_is_eof(l)) return _ouo_l_tok_new(l, OUO_TOK_EOF);

  char c = _ouo_l_advance(l);

  if (c == '\n') {
    OuoToken tok = _ouo_l_tok_new(l, OUO_TOK_NEWLINE);
    l->line++;
    l->col = 1;
    l->line_start = l->curr;
    return tok;
  }

  if (_ouo_l_isalpha(c)) return _ouo_l_read_word(l);

  // Literals
  if (_ouo_l_isdigit(c)) return _ouo_l_read_number(l);

  switch (c) {
    // Operators
    case '=': return _ouo_l_tok_new(l, OUO_TOK_ASSIGN);
    case '+': return _ouo_l_tok_new(l, OUO_TOK_PLUS);
    case '*': return _ouo_l_tok_new(l, OUO_TOK_ASTERISK);
    // Punctuation
    case '(': return _ouo_l_tok_new(l, OUO_TOK_PAREN_OPN);
    case ')': return _ouo_l_tok_new(l, OUO_TOK_PAREN_CLS);
    case '{': return _ouo_l_tok_new(l, OUO_TOK_BRACE_OPN);
    case '}': return _ouo_l_tok_new(l, OUO_TOK_BRACE_CLS);
    case ':': return _ouo_l_tok_new(l, OUO_TOK_COLON);
    default: break;
  }

  return _ouo_l_tok_new(l, OUO_TOK_ILLEGAL);
}

static const char *_ouo_tok_kind_str(OuoTokenKind kind) {
  switch (kind) {
    case OUO_TOK_ILLEGAL: return "ILLEGAL";
    case OUO_TOK_EOF: return "EOF";
    case OUO_TOK_NEWLINE: return "NEWLINE";
    case OUO_TOK_IDENT: return "IDENTIFIER";
    // Keywords
    case OUO_TOK_KW_VAR: return "var";
    case OUO_TOK_KW_PRINT: return "print";
    case OUO_TOK_KW_INT: return "int";
    case OUO_TOK_KW_FLOAT: return "float";
    case OUO_TOK_KW_BOOL: return "bool";
    // Literals
    case OUO_TOK_LIT_INT: return "LIT_INT";
    case OUO_TOK_LIT_FLOAT: return "LIT_FLOAT";
    case OUO_TOK_LIT_TRUE: return "true";
    case OUO_TOK_LIT_FALSE: return "false";
    // Operators
    case OUO_TOK_ASSIGN: return "=";
    case OUO_TOK_PLUS: return "+";
    case OUO_TOK_ASTERISK: return "*";
    // Punctuation
    case OUO_TOK_PAREN_OPN: return "(";
    case OUO_TOK_PAREN_CLS: return ")";
    case OUO_TOK_BRACE_OPN: return "{";
    case OUO_TOK_BRACE_CLS: return "}";
    case OUO_TOK_COLON: return ":";
  }
  return "";
}

#ifdef OUO_DEBUG
void ouo_tok_dump(OuoToken *tok) {
  ouo_printdbg(
      "[%s '%.*s'] ", _ouo_tok_kind_str(tok->kind), _OUO_TOK_FMT_ARGS(*tok));
}
#endif // OUO_DEBUG

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
  _OUO_PREC_LOWEST,
  _OUO_PREC_ASSIGN,
  _OUO_PREC_SUM,
  _OUO_PREC_PRODUCT,
  _OUO_PREC_ACCESS,
} _OuoPrecedence;

typedef struct _OuoParseRule {
  _OuoParsePrefixFn prefix_fn;
  _OuoParseInfixFn infix_fn;
  _OuoPrecedence prec;
} _OuoParseRule;

#define _ouo_p_err_append(p, tok, err_code, fmt, ...) \
  do { \
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
  } while (0)

#define _ouo_p_err(p, tok, err_code, fmt, ...) \
  do { \
    if (!(p)->panic_mode) { \
      (p)->res->failed = true; \
      (p)->panic_mode = true; \
      _ouo_p_err_append(p, tok, err_code, fmt, ##__VA_ARGS__); \
    } \
  } while (0)

static inline void _ouo_p_advance(_OuoParser *p) {
  p->curr = p->peek;
  p->peek = _ouo_l_next_token(p->l);
}

static inline void _ouo_p_init(_OuoParser *p, _OuoLexer *l, OuoParseResult *res,
    const _OuoParseRule *rules, size_t rules_count) {
  res->failed = false;
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
  ast->type = OUO_TYPE_UNKNOWN;

  ast->children.items = NULL;
  ast->children.count = 0;
  ast->children.capacity = 0;

  return ast;
}

static OuoAst *_ouo_p_stmt(_OuoParser *p);

static void _ouo_p_stmts(_OuoParser *p, OuoAst *ast, OuoTokenKind end_tok) {
  while (p->curr.kind != OUO_TOK_EOF && p->curr.kind != end_tok) {
    if (p->curr.kind == OUO_TOK_NEWLINE) {
      _ouo_p_advance(p);
      continue;
    }
    OuoAst *stmt = _ouo_p_stmt(p);
    if (stmt != NULL) ouo_da_append(&ast->children, stmt);
    _ouo_p_advance(p);
  }
}

static OuoAst *_ouo_p_module(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_MODULE);
  _ouo_p_stmts(p, ast, OUO_TOK_EOF);
  return ast;
}

static inline OuoTypeKind _ouo_p_type(_OuoParser *p) {
  switch (p->curr.kind) {
    case OUO_TOK_KW_INT: return OUO_TYPE_INT;
    case OUO_TOK_KW_FLOAT: return OUO_TYPE_FLOAT;
    case OUO_TOK_KW_BOOL: return OUO_TYPE_BOOL;
    default:
      _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX, "Expected a type, got '%.*s'.",
          _OUO_TOK_FMT_ARGS(p->curr));
      return OUO_TYPE_UNKNOWN;
  }
}

static inline const _OuoParseRule *_ouo_p_get_rule(
    _OuoParser *p, OuoTokenKind tok) {
  if (tok >= p->rules.count) return &p->rules.items[OUO_TOK_ILLEGAL];
  const _OuoParseRule *rule = &p->rules.items[tok];
  return rule;
}

static OuoAst *_ouo_p_expr(_OuoParser *p, _OuoPrecedence prec) {
  _OuoParsePrefixFn prefix_fn = _ouo_p_get_rule(p, p->curr.kind)->prefix_fn;

  if (prefix_fn == NULL) {
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX,
        "Expected an expression, got '%.*s'.", _OUO_TOK_FMT_ARGS(p->curr));
    return NULL;
  }

  OuoAst *left = prefix_fn(p);

  while (p->peek.kind != OUO_TOK_NEWLINE &&
      prec < _ouo_p_get_rule(p, p->peek.kind)->prec) {
    _OuoParseInfixFn infix_fn = _ouo_p_get_rule(p, p->peek.kind)->infix_fn;

    if (infix_fn == NULL) {
      _ouo_p_err(p, p->peek, OUO_ERR_SYNTAX,
          "Expected an operator, got '%.*s'.", _OUO_TOK_FMT_ARGS(p->peek));
      return left;
    }

    _ouo_p_advance(p);
    left = infix_fn(p, left);
  }

  return left;
}

static OuoAst *_ouo_p_ident(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_IDENT);
  ast->ident.name = p->curr;
  ast->ident.sym_idx = SIZE_MAX;
  return ast;
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

static OuoAst *_ouo_p_lit_bool(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_LIT_BOOL);
  ast->lit_bool = p->curr.kind == OUO_TOK_LIT_TRUE;
  return ast;
}

static OuoAst *_ouo_p_assign(_OuoParser *p, OuoAst *left) {
  OuoToken op = p->curr;
  _ouo_p_advance(p);
  OuoAst *right = _ouo_p_expr(p, _OUO_PREC_LOWEST);

  OuoAst *ast = _ouo_ast_new(&op, OUO_AST_ASSIGN);
  ast->assign.target = left;
  ast->assign.value = right;
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

static OuoAst *_ouo_p_grouping(_OuoParser *p) {
  OuoToken tok = p->curr;
  _ouo_p_advance(p);
  OuoAst *ast = _ouo_p_expr(p, _OUO_PREC_LOWEST);

  if (p->peek.kind != OUO_TOK_PAREN_CLS) {
    bool panic_prev = p->panic_mode;
    _ouo_p_err(p, p->peek, OUO_ERR_SYNTAX, "Expected '%s', got '%.*s'.",
        _ouo_tok_kind_str(OUO_TOK_PAREN_CLS), _OUO_TOK_FMT_ARGS(p->peek));
    if (!panic_prev)
      _ouo_p_err_append(p, tok, OUO_ERR_NOTE, "Grouping starts here.");
  } else {
    _ouo_p_advance(p);
  }

  return ast;
}

static OuoAst *_ouo_p_block(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_BLOCK);
  _ouo_p_advance(p);
  _ouo_p_stmts(p, ast, OUO_TOK_BRACE_CLS);

  if (p->curr.kind != OUO_TOK_BRACE_CLS) {
    bool panic_prev = p->panic_mode;
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX, "Expected '%s', got '%.*s'.",
        _ouo_tok_kind_str(OUO_TOK_BRACE_CLS), _OUO_TOK_FMT_ARGS(p->curr));
    if (!panic_prev)
      _ouo_p_err_append(p, ast->tok, OUO_ERR_NOTE, "Block starts here.");
  }

  return ast;
}

static OuoAst *_ouo_p_expr_stmt(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_EXPR_STMT);
  ast->child = _ouo_p_expr(p, _OUO_PREC_LOWEST);
  return ast;
}

static OuoAst *_ouo_p_print(_OuoParser *p) {
  OuoToken tok = p->curr;
  _ouo_p_advance(p);
  OuoAst *ast = _ouo_ast_new(&tok, OUO_AST_PRINT);
  ast->child = _ouo_p_expr(p, _OUO_PREC_LOWEST);
  return ast;
}

static OuoAst *_ouo_p_decl_var(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_DECL_VAR);

  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_IDENT) {
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX,
        "Expected an identifier, got '%.*s'.", _OUO_TOK_FMT_ARGS(p->curr));
    return ast;
  }

  OuoToken ident = p->curr;
  _ouo_p_advance(p);

  OuoTypeKind type = OUO_TYPE_UNKNOWN;
  if (p->curr.kind == OUO_TOK_COLON) {
    _ouo_p_advance(p);
    type = _ouo_p_type(p);
    if (type == OUO_TYPE_UNKNOWN) return ast;
    _ouo_p_advance(p);
  }

  if (p->curr.kind != OUO_TOK_ASSIGN) {
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX, "Expected '%s', got '%.*s'.",
        _ouo_tok_kind_str(OUO_TOK_ASSIGN), _OUO_TOK_FMT_ARGS(p->curr));
    return ast;
  }

  ast->tok = p->curr;
  _ouo_p_advance(p);
  ast->decl_var.name = ident;
  ast->decl_var.type_annotation = type;
  ast->decl_var.value = _ouo_p_expr(p, _OUO_PREC_LOWEST);
  return ast;
}

static void _ouo_p_synchronize(_OuoParser *p) {
  p->panic_mode = false;
  while (p->peek.kind != OUO_TOK_EOF) {
    if (p->curr.kind == OUO_TOK_NEWLINE || p->curr.kind == OUO_TOK_BRACE_OPN ||
        p->peek.kind == OUO_TOK_BRACE_CLS)
      return;
    _ouo_p_advance(p);
  }
}

static OuoAst *_ouo_p_stmt(_OuoParser *p) {
  OuoAst *ast;
  switch (p->curr.kind) {
    case OUO_TOK_KW_PRINT: ast = _ouo_p_print(p); break;
    case OUO_TOK_KW_VAR: ast = _ouo_p_decl_var(p); break;
    default: ast = _ouo_p_expr_stmt(p); break;
  }

  if (p->peek.kind != OUO_TOK_EOF && p->peek.kind != OUO_TOK_NEWLINE &&
      p->peek.kind != OUO_TOK_BRACE_CLS) {
    _ouo_p_err(p, p->peek, OUO_ERR_SYNTAX, "Expected a new line, got '%.*s'.",
        _OUO_TOK_FMT_ARGS(p->peek));
  }

  if (p->panic_mode) _ouo_p_synchronize(p);
  else if (p->peek.kind == OUO_TOK_NEWLINE) _ouo_p_advance(p);
  return ast;
}

static const _OuoParseRule _ouo_p_rules[] = {
    [OUO_TOK_ILLEGAL] = {NULL, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_IDENT] = {_ouo_p_ident, NULL, _OUO_PREC_LOWEST},
    // Literals
    [OUO_TOK_LIT_INT] = {_ouo_p_lit_int, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_LIT_FLOAT] = {_ouo_p_lit_float, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_LIT_TRUE] = {_ouo_p_lit_bool, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_LIT_FALSE] = {_ouo_p_lit_bool, NULL, _OUO_PREC_LOWEST},
    // Operators
    [OUO_TOK_ASSIGN] = {NULL, _ouo_p_assign, _OUO_PREC_ASSIGN},
    [OUO_TOK_PLUS] = {NULL, _ouo_p_bin_op, _OUO_PREC_SUM},
    [OUO_TOK_ASTERISK] = {NULL, _ouo_p_bin_op, _OUO_PREC_PRODUCT},
    // Punctuation
    [OUO_TOK_PAREN_OPN] = {_ouo_p_grouping, NULL, _OUO_PREC_ACCESS},
    [OUO_TOK_BRACE_OPN] = {_ouo_p_block, NULL, _OUO_PREC_ACCESS},
};

void ouo_parse(const char *src, OuoParseResult *res) {
  _OuoLexer l = {0};
  _ouo_l_init(&l, res, src);

#ifdef OUO_DEBUG
  for (;;) {
    OuoToken tok = _ouo_l_next_token(&l);
    ouo_tok_dump(&tok);
    if (tok.kind == OUO_TOK_EOF) break;
  }
  ouo_printdbg("\n");
  _ouo_l_init(&l, res, src);
#endif

  _OuoParser p = {0};
  _ouo_p_init(&p, &l, res, _ouo_p_rules, ouo_arr_len(_ouo_p_rules));

  res->ast = _ouo_p_module(&p);
  res->line = l.line;

#ifdef OUO_DEBUG
  ouo_ast_dump(res->ast);
  ouo_printdbg("\n");
#endif
}

void ouo_ast_free(OuoAst *ast) {
  if (ast == NULL) return;

  switch (ast->kind) {
    case OUO_AST_MODULE:
    case OUO_AST_BLOCK:
      OUO_DA_FOREACH(OuoAst *, child, &ast->children) { ouo_ast_free(*child); }
      ouo_da_free(ast->children);
      break;
    case OUO_AST_IDENT: break;
    // Literals
    case OUO_AST_LIT_INT:
    case OUO_AST_LIT_FLOAT:
    case OUO_AST_LIT_BOOL: break;
    // Expressions
    case OUO_AST_ASSIGN:
      ouo_ast_free(ast->assign.target);
      ouo_ast_free(ast->assign.value);
      break;
    case OUO_AST_BIN_OP:
      ouo_ast_free(ast->bin_op.left);
      ouo_ast_free(ast->bin_op.right);
      break;
    // Statements
    case OUO_AST_EXPR_STMT:
    case OUO_AST_PRINT: ouo_ast_free(ast->child); break;
    case OUO_AST_DECL_VAR: ouo_ast_free(ast->decl_var.value); break;
  }

  ouo_free(ast);
}

#ifdef OUO_DEBUG

static const char *_ouo_ast_kind_str(OuoAstKind kind) {
  switch (kind) {
    case OUO_AST_MODULE: return "MODULE";
    case OUO_AST_IDENT: return "IDENT";
    // Literals
    case OUO_AST_LIT_INT: return "LIT_INT";
    case OUO_AST_LIT_FLOAT: return "LIT_FLOAT";
    case OUO_AST_LIT_BOOL: return "LIT_BOOL";
    // Expressions
    case OUO_AST_ASSIGN: return "ASSIGN";
    case OUO_AST_BIN_OP: return "BIN_OP";
    case OUO_AST_BLOCK: return "BLOCK";
    // Statements
    case OUO_AST_EXPR_STMT: return "EXPR_STMT";
    case OUO_AST_PRINT: return "PRINT";
    case OUO_AST_DECL_VAR: return "DECL_VAR";
  }
  return "";
}

void ouo_ast_dump(OuoAst *ast) {
  if (ast == NULL) {
    ouo_printdbg("(NULL)");
    return;
  }

  ouo_printdbg("(%s ", _ouo_ast_kind_str(ast->kind));

  switch (ast->kind) {
    case OUO_AST_MODULE:
    case OUO_AST_BLOCK:
      ouo_printdbg("\n");
      OUO_DA_FOREACH(OuoAst *, stmt, &ast->children) {
        ouo_ast_dump(*stmt);
        ouo_printdbg("\n");
      }
      break;
    case OUO_AST_IDENT:
      ouo_printdbg("%.*s", _OUO_TOK_FMT_ARGS(ast->ident.name));
      break;
    // Literals
    case OUO_AST_LIT_INT: ouo_printdbg("%" OUO_PRId, ast->lit_int); break;
    case OUO_AST_LIT_FLOAT: ouo_printdbg("%" OUO_PRIf, ast->lit_float); break;
    case OUO_AST_LIT_BOOL:
      ouo_printdbg(ast->lit_bool ? "true" : "false");
      break;
    // Expressions
    case OUO_AST_ASSIGN:
      ouo_ast_dump(ast->assign.target);
      ouo_printdbg(" %s ", _ouo_tok_kind_str(OUO_TOK_ASSIGN));
      ouo_ast_dump(ast->assign.value);
      break;
    case OUO_AST_BIN_OP:
      ouo_ast_dump(ast->bin_op.left);
      ouo_printdbg(" %s ", _ouo_tok_kind_str(ast->bin_op.op));
      ouo_ast_dump(ast->bin_op.right);
      break;
    // Statements
    case OUO_AST_EXPR_STMT:
    case OUO_AST_PRINT: ouo_ast_dump(ast->child); break;
    case OUO_AST_DECL_VAR:
      ouo_printdbg("%.*s %s ", _OUO_TOK_FMT_ARGS(ast->decl_var.name),
          _ouo_type_kind_str(ast->decl_var.type_annotation));
      ouo_ast_dump(ast->decl_var.value);
      break;
  }

  ouo_printdbg(")");
}

#endif // OUO_DEBUG

//
// Compilation
//

typedef struct {
  bool panic_mode;
  size_t scope_depth;
  OuoCompileResult *res;
} _OuoCompiler;

#define _ouo_c_err_append(c, tok, err_code, fmt, ...) \
  do { \
    OuoError err = { \
        .code = (err_code), \
        .len = (tok).len, \
        .line = (tok).line, \
        .col = (tok).col, \
        .line_start = (tok).line_start, \
        .msg = {0}, \
    }; \
    _ouo_err_sprintf(err, fmt, ##__VA_ARGS__); \
    ouo_da_append(&(c)->res->errors, err); \
  } while (0)

#define _ouo_c_err(c, tok, err_code, fmt, ...) \
  do { \
    if (!(c)->panic_mode) { \
      (c)->res->failed = true; \
      (c)->panic_mode = true; \
      _ouo_c_err_append(c, tok, err_code, fmt, ##__VA_ARGS__); \
    } \
  } while (0)

static inline void _ouo_c_init(_OuoCompiler *c, OuoCompileResult *res) {
  res->failed = false;
  c->res = res;
}

static inline bool _ouo_tok_eq(OuoToken *a, OuoToken *b) {
  return a->len == b->len && memcmp(a->start, b->start, a->len) == 0;
}

static inline bool _ouo_c_find_sym(
    _OuoCompiler *c, OuoToken *name, size_t *idx) {
  if (c->res->symbols.count == 0) return false;

  for (size_t i = c->res->symbols.count - 1; i >= 0; i--) {
    if (_ouo_tok_eq(name, &c->res->symbols.items[i].name)) {
      *idx = i;
      return true;
    }
    if (i == 0) break;
  }

  return false;
}

static inline bool _ouo_c_sym_check_defined(
    _OuoCompiler *c, OuoToken *tok, OuoSymbol *sym) {
  size_t sym_idx;
  if (_ouo_c_find_sym(c, &sym->name, &sym_idx)) {
    _ouo_c_err(c, *tok, OUO_ERR_SEMANTIC, "Symbol '%.*s' is already defined.",
        _OUO_TOK_FMT_ARGS(sym->name));
    _ouo_c_err_append(c, c->res->symbols.items[sym_idx].name, OUO_ERR_NOTE,
        "Previous definition here.");
    return true;
  }
  return false;
}

static inline void _ouo_c_add_sym(
    _OuoCompiler *c, OuoToken *tok, OuoSymbol *sym) {
  if (sym->type == OUO_TYPE_UNKNOWN) return;

  if (c->res->symbols.count > UINT8_MAX) {
    _ouo_c_err(c, *tok, OUO_ERR_COMPILE_FAIL,
        "Maximum amount of symbols exceeded (max %d).", UINT8_MAX + 1);
    return;
  }

  sym->scope_depth = c->scope_depth;
  ouo_da_append(&c->res->symbols, *sym);
}

// Static analysis

static void _ouo_c_err_ident_undefined(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast->tok, OUO_ERR_SEMANTIC, "Undefined symbol '%.*s'.",
      _OUO_TOK_FMT_ARGS(ast->ident.name));
}

static void _ouo_c_err_assign_type(
    _OuoCompiler *c, OuoToken *tok, OuoTypeKind target, OuoTypeKind value) {
  _ouo_c_err(c, *tok, OUO_ERR_TYPE, "Cannot assign '%s' to '%s'.",
      _ouo_type_kind_str(value), _ouo_type_kind_str(target));
}

static void _ouo_c_err_assign_invalid(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast->tok, OUO_ERR_SEMANTIC,
      "Assignment target can only be a variable.");
}

static void _ouo_c_err_bin_op_type(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast->tok, OUO_ERR_TYPE,
      "Operation '%s' does not support '%s' and '%s'.",
      _ouo_tok_kind_str(ast->bin_op.op),
      _ouo_type_kind_str(ast->bin_op.left->type),
      _ouo_type_kind_str(ast->bin_op.right->type));
}

static void _ouo_c_err_bin_op_unknown(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL, "Unknown binary operator '%s'.",
      _ouo_tok_kind_str(ast->bin_op.op));
}

static void _ouo_c_err_stmt_type(
    _OuoCompiler *c, OuoToken *tok, OuoTypeKind type) {
  _ouo_c_err(c, *tok, OUO_ERR_TYPE, "Type of '%.*s' cannot be '%s'.",
      _OUO_TOK_FMT_ARGS(*tok), _ouo_type_kind_str(type));
}

static void _ouo_c_err_var_void(_OuoCompiler *c, OuoToken *tok) {
  _ouo_c_err(c, *tok, OUO_ERR_TYPE, "A variabe cannot be '%s'.",
      _ouo_type_kind_str(OUO_TYPE_VOID));
}

static inline bool _ouo_ast_bin_op_is(OuoAst *ast, OuoTypeKind type) {
  return ast->bin_op.left->type == type && ast->bin_op.right->type == type;
}

static void _ouo_c_ast_analyze(_OuoCompiler *c, OuoAst *ast) {
  switch (ast->kind) {
    case OUO_AST_MODULE: ast->type = OUO_TYPE_VOID; break;
    case OUO_AST_IDENT: {
      size_t sym_idx;
      if (_ouo_c_find_sym(c, &ast->ident.name, &sym_idx)) {
        ast->type = c->res->symbols.items[sym_idx].type;
        ast->ident.sym_idx = sym_idx;
      } else _ouo_c_err_ident_undefined(c, ast);
      break;
    }

    // Literals
    case OUO_AST_LIT_INT: ast->type = OUO_TYPE_INT; break;
    case OUO_AST_LIT_FLOAT: ast->type = OUO_TYPE_FLOAT; break;
    case OUO_AST_LIT_BOOL: ast->type = OUO_TYPE_BOOL; break;

    // Expressions
    case OUO_AST_ASSIGN: {
      ast->type = OUO_TYPE_VOID;
      switch (ast->assign.target->kind) {
        case OUO_AST_IDENT:
          if (ast->assign.value->type != ast->assign.target->type)
            _ouo_c_err_assign_type(c, &ast->tok, ast->assign.target->type,
                ast->assign.value->type);
          break;
        default: _ouo_c_err_assign_invalid(c, ast); break;
      }
      break;
    }
    case OUO_AST_BIN_OP:
      switch (ast->bin_op.op) {
        // Arithmetic
        case OUO_TOK_PLUS:
        case OUO_TOK_ASTERISK:
          if (_ouo_ast_bin_op_is(ast, OUO_TYPE_INT)) ast->type = OUO_TYPE_INT;
          else if (_ouo_ast_bin_op_is(ast, OUO_TYPE_FLOAT))
            ast->type = OUO_TYPE_FLOAT;
          else _ouo_c_err_bin_op_type(c, ast);
          break;

        default: _ouo_c_err_bin_op_unknown(c, ast); break;
      }
      break;
    case OUO_AST_BLOCK: ast->type = OUO_TYPE_VOID; break;

    // Statements
    case OUO_AST_EXPR_STMT: ast->type = ast->child->type; break;
    case OUO_AST_PRINT:
      ast->type = OUO_TYPE_VOID;
      if (ast->child->type == OUO_TYPE_VOID)
        _ouo_c_err_stmt_type(c, &ast->tok, ast->child->type);
      break;
    case OUO_AST_DECL_VAR: {
      ast->type = OUO_TYPE_VOID;
      bool failed = false;
      bool panic_prev = c->panic_mode;
      c->panic_mode = false;

      if (ast->decl_var.value->type == OUO_TYPE_VOID ||
          ast->decl_var.type_annotation == OUO_TYPE_VOID) {
        _ouo_c_err_var_void(c, &ast->tok);
        failed = true;
      } else if (ast->decl_var.type_annotation != OUO_TYPE_UNKNOWN &&
          ast->decl_var.value->type != OUO_TYPE_UNKNOWN &&
          ast->decl_var.value->type != ast->decl_var.type_annotation) {
        _ouo_c_err_assign_type(c, &ast->tok, ast->decl_var.type_annotation,
            ast->decl_var.value->type);
        failed = true;
      } else if (ast->decl_var.value->type == OUO_TYPE_UNKNOWN) {
        failed = true;
      }

      OuoSymbol sym = {
          .name = ast->decl_var.name,
          .type = ast->decl_var.value->type,
      };
      if (_ouo_c_sym_check_defined(c, &ast->decl_var.name, &sym)) failed = true;
      if (!failed) _ouo_c_add_sym(c, &ast->decl_var.name, &sym);
      c->panic_mode = panic_prev;
    } break;
  }
}

#ifndef OUO_NOEMIT

// Bytecode emission

static inline void _ouo_c_chunk_write(
    _OuoCompiler *c, uint8_t byte, size_t line) {
  ouo_da_append(&c->res->chunk, byte);

  size_t lines_count = c->res->chunk.lines.count;
  if (lines_count == 0 || c->res->chunk.lines.items[lines_count - 1] != line) {
    ouo_da_append(&c->res->chunk.lines, 1);
    ouo_da_append(&c->res->chunk.lines, line);
  } else {
    c->res->chunk.lines.items[lines_count - 2]++;
  }
}

static inline size_t _ouo_chunk_get_line(OuoChunk *chunk, const uint8_t *ip) {
  size_t ip_idx = (size_t)(ip - chunk->items);
  size_t ip_idx_curr = 0;
  for (size_t i = 0; i < chunk->lines.count; i += 2) {
    ip_idx_curr += chunk->lines.items[i];
    if (ip_idx_curr > ip_idx) return chunk->lines.items[i + 1];
  }
  return 0;
}

static inline size_t _ouo_c_chunk_add_lit(_OuoCompiler *c, OuoObject *obj) {
  ouo_da_append(&c->res->chunk.literals, *obj);
  return c->res->chunk.literals.count - 1;
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

static void _ouo_c_emit_lit(_OuoCompiler *c, OuoAst *ast, OuoObject *obj) {
  if (c->res->chunk.literals.count > UINT8_MAX) {
    _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
        "Maximum amount of literals exceeded (max %d).", UINT8_MAX + 1);
    return;
  }

  size_t lit_idx = _ouo_c_chunk_add_lit(c, obj);
  _ouo_c_emit_bytes(c, ast, OUO_OP_LITERAL, (uint8_t)lit_idx);
}

static void _ouo_c_ast_emit(_OuoCompiler *c, OuoAst *ast) {
  switch (ast->kind) {
    case OUO_AST_MODULE: break;
    case OUO_AST_IDENT:
      _ouo_c_emit_bytes(c, ast, OUO_OP_VAR_GET, (uint8_t)ast->ident.sym_idx);
      break;

    // Literals
    case OUO_AST_LIT_INT:
      _ouo_c_emit_lit(c, ast,
          &(OuoObject){
              .kind = OUO_OBJ_INT,
              .v_int = ast->lit_int,
          });
      break;
    case OUO_AST_LIT_FLOAT:
      _ouo_c_emit_lit(c, ast,
          &(OuoObject){
              .kind = OUO_OBJ_FLOAT,
              .v_float = ast->lit_float,
          });
      break;
    case OUO_AST_LIT_BOOL:
      _ouo_c_emit_lit(c, ast,
          &(OuoObject){
              .kind = OUO_OBJ_BOOL,
              .v_bool = ast->lit_bool,
          });
      break;

    // Expressions
    case OUO_AST_ASSIGN:
      switch (ast->assign.target->kind) {
        case OUO_AST_IDENT:
          _ouo_c_emit_bytes(c, ast, OUO_OP_VAR_SET,
              (uint8_t)ast->assign.target->ident.sym_idx);
          break;
        default: _ouo_c_err_assign_invalid(c, ast); break;
      }
      break;
    case OUO_AST_BIN_OP:
      switch (ast->bin_op.op) {
        // Arithmetic
        case OUO_TOK_PLUS:
          if (_ouo_ast_bin_op_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_ADD_INT);
          else if (_ouo_ast_bin_op_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_ADD_FLOAT);
          else _ouo_c_err_bin_op_type(c, ast);
          break;

        case OUO_TOK_ASTERISK:
          if (_ouo_ast_bin_op_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_MULT_INT);
          else if (_ouo_ast_bin_op_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_MULT_FLOAT);
          else _ouo_c_err_bin_op_type(c, ast);
          break;

        default: _ouo_c_err_bin_op_unknown(c, ast); break;
      }
      break;
    case OUO_AST_BLOCK: break;

    // Statements
    case OUO_AST_EXPR_STMT:
      if (ast->type != OUO_TYPE_VOID) {
        if (c->res->echo && c->scope_depth == 0)
          _ouo_c_emit_byte(c, ast, OUO_OP_PRINT);
        _ouo_c_emit_byte(c, ast, OUO_OP_POP);
      }
      break;
    case OUO_AST_PRINT:
      _ouo_c_emit_byte(c, ast, OUO_OP_PRINT);
      _ouo_c_emit_byte(c, ast, OUO_OP_POP);
      break;
    case OUO_AST_DECL_VAR: break;
  }
}

#endif // OUO_NOEMIT

static inline void _ouo_c_scope_begin(_OuoCompiler *c) { c->scope_depth++; }

static inline void _ouo_c_scope_end(_OuoCompiler *c,
#ifndef OUO_NOEMIT
    OuoAst *ast
#else
    OuoAst *_
#endif
) {
  c->scope_depth--;
  while (c->res->symbols.count > 0 &&
      c->res->symbols.items[c->res->symbols.count - 1].scope_depth >
          c->scope_depth) {
    c->res->symbols.count--;
#ifndef OUO_NOEMIT
    _ouo_c_emit_byte(c, ast, OUO_OP_POP);
#endif
  }
}

static void _ouo_c_ast(_OuoCompiler *c, OuoAst *ast, bool noemit) {
  if (ast == NULL) return;
  bool new_scope = false;

  switch (ast->kind) {
    case OUO_AST_MODULE:
    case OUO_AST_BLOCK:
      if (ast->kind == OUO_AST_BLOCK || !c->res->keep_module_scope) {
        new_scope = true;
        _ouo_c_scope_begin(c);
      }
      bool panic_prev = c->panic_mode;
      c->panic_mode = false;
      OUO_DA_FOREACH(OuoAst *, stmt, &ast->children) {
        _ouo_c_ast(c, *stmt, noemit);
        c->panic_mode = false;
      }
      c->panic_mode = panic_prev;
      break;
    case OUO_AST_IDENT: break;
    // Literals
    case OUO_AST_LIT_INT:
    case OUO_AST_LIT_FLOAT:
    case OUO_AST_LIT_BOOL: break;
    // Expressions
    case OUO_AST_ASSIGN:
      _ouo_c_ast(c, ast->assign.target, true);
      _ouo_c_ast(c, ast->assign.value, noemit);
      break;
    case OUO_AST_BIN_OP:
      _ouo_c_ast(c, ast->bin_op.left, noemit);
      _ouo_c_ast(c, ast->bin_op.right, noemit);
      break;
    // Statements
    case OUO_AST_EXPR_STMT:
    case OUO_AST_PRINT: _ouo_c_ast(c, ast->child, noemit); break;
    case OUO_AST_DECL_VAR: _ouo_c_ast(c, ast->decl_var.value, noemit); break;
  }

  _ouo_c_ast_analyze(c, ast);
  if (c->res->failed || noemit) return;

#ifndef OUO_NOEMIT
  _ouo_c_ast_emit(c, ast);
#endif

  if (new_scope) _ouo_c_scope_end(c, ast);
}

void ouo_compile(OuoAst *ast, OuoCompileResult *res) {
  _OuoCompiler c = {0};
  _ouo_c_init(&c, res);

  _ouo_c_ast(&c, ast, false);

#ifdef OUO_DEBUG
  for (size_t i = 0; i < res->symbols.count; i++) {
    OuoSymbol sym = res->symbols.items[i];
    ouo_printdbg("[%zu %.*s '%s' (%zu)] ", i, _OUO_TOK_FMT_ARGS(sym.name),
        _ouo_type_kind_str(sym.type), sym.scope_depth);
  }
  ouo_printdbg("\n");

#ifndef OUO_NOEMIT
  ouo_chunk_dump(&res->chunk, "main");
  ouo_printdbg("\n");
#endif // OUO_NOEMIT
#endif // OUO_DEBUG
}

#ifndef OUO_NOEMIT

void ouo_chunk_free(OuoChunk *chunk) {
  ouo_da_free(chunk->literals);
  ouo_da_free(chunk->lines);
  ouo_da_free(*chunk);
}

static inline void _ouo_obj_print(OuoObject *obj) {
  switch (obj->kind) {
    case OUO_OBJ_INT: ouo_print("%" OUO_PRId, obj->v_int); break;
    case OUO_OBJ_FLOAT: ouo_print("%" OUO_PRIf, obj->v_float); break;
    case OUO_OBJ_BOOL: ouo_print(obj->v_bool ? "true" : "false"); break;
  }
}

#ifdef OUO_DEBUG

static inline void _ouo_obj_dump(OuoObject *obj) {
  if (obj == NULL) {
    ouo_printdbg("(NULL)");
    return;
  }
  _ouo_obj_print(obj);
  fflush(stdout);
}

static const char *_ouo_op_code_str(OuoOpCode op_code) {
  switch (op_code) {
    // Objects
    case OUO_OP_POP: return "POP";
    case OUO_OP_VAR_GET: return "VAR_GET";
    case OUO_OP_VAR_SET: return "VAR_SET";
    case OUO_OP_LITERAL: return "LITERAL";
    // Arithmetic
    case OUO_OP_ADD_INT: return "INT_ADD";
    case OUO_OP_ADD_FLOAT: return "FLOAT_ADD";
    case OUO_OP_MULT_INT: return "INT_MULT";
    case OUO_OP_MULT_FLOAT: return "FLOAT_MULT";
    // Control flow
    case OUO_OP_RETURN: return "RETURN";
    // I/O
    case OUO_OP_PRINT: return "PRINT";
  }
  return "";
}

static ptrdiff_t _ouo_chunk_op_dump(OuoChunk *chunk, uint8_t *ip) {
  uint8_t *ip_prev = ip;
  ptrdiff_t ip_idx = ip - chunk->items;

  ouo_printdbg("%04ld ", ip_idx);
  size_t line_curr = _ouo_chunk_get_line(chunk, ip);
  if (ip_idx > 0 && line_curr == _ouo_chunk_get_line(chunk, ip - 1))
    ouo_printdbg("   | ");
  else ouo_printdbg("%4zu ", line_curr);

  OuoOpCode op_code = (OuoOpCode)*ip;
  ouo_printdbg("%-16s", _ouo_op_code_str(op_code));

  switch (op_code) {
    // Objects
    case OUO_OP_POP: break;
    case OUO_OP_VAR_GET:
    case OUO_OP_VAR_SET: {
      uint8_t sym_idx = *(++ip);
      ouo_printdbg("%4d ", sym_idx);
      break;
    }
    case OUO_OP_LITERAL: {
      uint8_t lit_idx = *(++ip);
      ouo_printdbg("%4d '", lit_idx);
      _ouo_obj_dump(&chunk->literals.items[lit_idx]);
      ouo_printdbg("'");
      break;
    }
    // Arithmetic
    case OUO_OP_ADD_INT:
    case OUO_OP_ADD_FLOAT:
    case OUO_OP_MULT_INT:
    case OUO_OP_MULT_FLOAT: break;
    // Control flow
    case OUO_OP_RETURN: break;
    // I/O
    case OUO_OP_PRINT: break;
  }

  return ip - ip_prev;
}

void ouo_chunk_dump(OuoChunk *chunk, const char *name) {
  ouo_printdbg("%s:\n", name);
  if (chunk->items == NULL) {
    ouo_printdbg("(NULL)\n");
    return;
  }

  for (size_t i = 0; i < chunk->lines.count; i += 2)
    ouo_printdbg("%zu-%zu ", chunk->lines.items[i], chunk->lines.items[i + 1]);
  ouo_printdbg("\n");

  OUO_DA_FOREACH(uint8_t, ip, chunk) {
    ip += _ouo_chunk_op_dump(chunk, ip);
    ouo_printdbg("\n");
  }

  ouo_printdbg("---------------------------------------\n");
}

#endif // OUO_DEBUG

//
// Virtual machine
//

typedef struct {
  OuoChunk *chunk;
  OuoInterpretResult *res;
} _OuoVm;

#define _ouo_vm_err(vm, ip, err_code, fmt, ...) \
  do { \
    (vm)->res->failed = true; \
    OuoError error = { \
        .code = (err_code), \
        .line = _ouo_chunk_get_line((vm)->chunk, (ip)), \
        .line_start = NULL, \
        .msg = {0}, \
    }; \
    _ouo_err_sprintf(error, fmt, ##__VA_ARGS__); \
    (vm)->res->error = error; \
  } while (0)

static inline void _ouo_vm_init(
    _OuoVm *vm, OuoInterpretResult *res, OuoChunk *chunk) {
  res->failed = false;
  vm->res = res;
  vm->chunk = chunk;
}

static inline void _ouo_vm_stack_push(_OuoVm *vm, uint8_t *ip, OuoObject *obj) {
  if (vm->res->stack.count + 1 >= OUO_VM_STACK_SIZE) {
    _ouo_vm_err(vm, ip, OUO_ERR_RUNTIME,
        "Maximum stack size exceeded (max %d).", OUO_VM_STACK_SIZE);
    return;
  }

  vm->res->stack.items[vm->res->stack.count] = *obj;
  vm->res->stack.count++;
}

static inline OuoObject *_ouo_vm_stack_pop(_OuoVm *vm, uint8_t *ip) {
  if (vm->res->stack.count == 0) {
    _ouo_vm_err(vm, ip, OUO_ERR_RUNTIME, "Trying to pop empty stack.");
    return &vm->res->stack.items[0];
  }

  vm->res->stack.count--;
  return &vm->res->stack.items[vm->res->stack.count];
}

static inline OuoObject *_ouo_vm_stack_peek(
    _OuoVm *vm, uint8_t *ip, size_t offset) {
  if (offset + 1 > vm->res->stack.count) {
    _ouo_vm_err(vm, ip, OUO_ERR_RUNTIME,
        "Trying peek beyond the stack (offset %td, stack size %ld).",
        offset + 1, vm->res->stack.count);
    return &vm->res->stack.items[0];
  }

  return &vm->res->stack.items[vm->res->stack.count - offset - 1];
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

#define _ouo_vm_read_byte(vm, ip) *(++(ip))

#define _ouo_vm_read_lit(vm, ip) \
  ((vm)->chunk->literals.items[_ouo_vm_read_byte(vm, ip)])

#define _OUO_VM_BIN_OP(vm, ip, T, OP) \
  do { \
    ouo_##T##_t b = _ouo_vm_stack_pop((vm), (ip))->v_##T; \
    ouo_##T##_t a = _ouo_vm_stack_pop((vm), (ip))->v_##T; \
    _ouo_vm_stack_push((vm), (ip), &_ouo_obj_new_##T(a OP b)); \
  } while (0)

static void _ouo_vm_run(_OuoVm *vm) {
  OUO_DA_FOREACH(uint8_t, ip, vm->chunk) {
    if (vm->res->failed) return;

#ifdef OUO_DEBUG
    _ouo_chunk_op_dump(vm->chunk, ip);
    ouo_printdbg("\n");
#endif

    OuoOpCode op_code = (OuoOpCode)*ip;
    switch (op_code) {
      // Objects
      case OUO_OP_POP: _ouo_vm_stack_pop(vm, ip); break;
      case OUO_OP_VAR_GET: {
        uint8_t idx = _ouo_vm_read_byte(vm, ip);
        _ouo_vm_stack_push(vm, ip, &vm->res->stack.items[idx]);
        break;
      }
      case OUO_OP_VAR_SET: {
        uint8_t idx = _ouo_vm_read_byte(vm, ip);
        vm->res->stack.items[idx] = *_ouo_vm_stack_pop(vm, ip);
        break;
      }
      case OUO_OP_LITERAL: {
        OuoObject lit = _ouo_vm_read_lit(vm, ip);
        _ouo_vm_stack_push(vm, ip, &lit);
        break;
      }

      // Arithmetic
      case OUO_OP_ADD_INT: _OUO_VM_BIN_OP(vm, ip, int, +); break;
      case OUO_OP_ADD_FLOAT: _OUO_VM_BIN_OP(vm, ip, float, +); break;
      case OUO_OP_MULT_INT: _OUO_VM_BIN_OP(vm, ip, int, *); break;
      case OUO_OP_MULT_FLOAT: _OUO_VM_BIN_OP(vm, ip, float, *); break;

      // Control flow
      case OUO_OP_RETURN: _ouo_vm_stack_pop(vm, ip); return;

      // I/O
      case OUO_OP_PRINT:
        _ouo_obj_print(_ouo_vm_stack_peek(vm, ip, 0));
        ouo_print("\n");
        break;
    }

#ifdef OUO_DEBUG
    if (vm->res->stack.count != 0) {
      OUO_DA_FOREACH(OuoObject, slot, &vm->res->stack) {
        ouo_printdbg("[");
        _ouo_obj_dump(slot);
        ouo_printdbg("] ");
      }
      ouo_printdbg("\n");
    }
#endif
  }
}

void ouo_interpret(OuoChunk *chunk, OuoInterpretResult *res) {
  _OuoVm vm = {0};
  _ouo_vm_init(&vm, res, chunk);

  _ouo_vm_run(&vm);
}

#endif // OUO_NOEMIT

#endif // OUO_IMPLEMENTATION
