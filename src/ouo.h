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

#define OUO_ER "\x1b[0m"
#define OUO_EB "\x1b[1m"
#define OUO_EBR "\x1b[0;1m"
#define OUO_ED "\x1b[2m"
#define OUO_EBD "\x1b[1;2m"
#define OUO_EBRED "\x1b[1;31m"
#define OUO_EBGRN "\x1b[1;32m"

#define ouo_abort(err_code, ...) \
  do { \
    ouo_printerr(__VA_ARGS__); \
    ouo_printerr("\n"); \
    exit(err_code); \
  } while (0)

// Assertions

#define ouo_assert(expr, err_code, ...) \
  if (!(expr)) ouo_abort(err_code, __VA_ARGS__)

#define ouo_assertf(expr, err_code, ...) \
  do { \
    if (!(expr)) { \
      fprintf(stderr, OUO_ED OUO_CODEPOS "%s: " OUO_ER, __func__); \
      ouo_abort(err_code, __VA_ARGS__); \
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

#ifndef ouo_bool_t
#define ouo_bool_t bool
#endif // ouo_bool_t

/// Owns memory for `items`.
typedef struct {
  char *items;
  size_t count;
  size_t capacity;
} OuoString;

typedef struct {
  const char *start;
  size_t len;
} OuoStringSlice;

#define OUO_STR_FMT(str) (int)(str).count, (str).items
#define OUO_STRSL_FMT(str) (int)(str).len, (str).start

#define ouo_str_slice_eq(a, b) \
  ((a)->len == (b)->len && memcmp((a)->start, (b)->start, (a)->len) == 0)

typedef enum {
  OUO_TYPE_UNKNOWN,
  OUO_TYPE_VOID,
  // Scalar
  OUO_TYPE_INT,
  OUO_TYPE_FLOAT,
  OUO_TYPE_BOOL,
  OUO_TYPE_STR,
  // Funtion
  OUO_TYPE_FN,
} OuoTypeKind;

struct OuoType;

typedef struct {
  OuoStringSlice name;
  struct OuoType *type;
} OuoNameType;

typedef struct {
  OuoNameType *items;
  size_t count;
  size_t capacity;
} OuoNameTypes;

typedef struct OuoType {
  OuoTypeKind kind;

  union {
    struct {
      OuoNameTypes args;
      struct OuoType *return_type;
    } t_fn;
  } as;
} OuoType;

//
// Error handling
//

// line and col start from 1
typedef struct {
  size_t line;
  size_t col;
  const char *line_start;
} OuoCodePosition;

typedef enum {
  OUO_OK,
  OUO_ERR_NOTE,
  // General
  OUO_ERR_OUT_OF_MEMORY,
  OUO_ERR_DOUBLE_FREE,
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

#define OUO_ERRMSG_SIZE 256

typedef struct OuoError {
  OuoErrorCode code;
  size_t len;
  OuoCodePosition pos;
  char msg[OUO_ERRMSG_SIZE];
  OuoStringSlice fn_name;
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
// Lexing
//

typedef enum {
  OUO_TOK_ILLEGAL,
  OUO_TOK_EOF,
  OUO_TOK_NEWLINE,
  OUO_TOK_IDENT,
  // Keywords
  OUO_TOK_KW_INT,
  OUO_TOK_KW_FLOAT,
  OUO_TOK_KW_BOOL,
  OUO_TOK_KW_STR,
  OUO_TOK_KW_VOID,
  OUO_TOK_KW_OR,
  OUO_TOK_KW_AND,
  OUO_TOK_KW_IF,
  OUO_TOK_KW_ELSE,
  OUO_TOK_KW_WHILE,
  OUO_TOK_KW_PRINT,
  OUO_TOK_KW_VAR,
  OUO_TOK_KW_FN,
  // Literals
  OUO_TOK_LIT_INT,
  OUO_TOK_LIT_FLOAT,
  OUO_TOK_LIT_TRUE,
  OUO_TOK_LIT_FALSE,
  OUO_TOK_LIT_STR,
  // Operators
  OUO_TOK_ASSIGN,
  OUO_TOK_PLUS,
  OUO_TOK_MINUS,
  OUO_TOK_ASTERISK,
  OUO_TOK_SLASH,
  OUO_TOK_EQ,
  OUO_TOK_NEQ,
  OUO_TOK_LT,
  OUO_TOK_LT_EQ,
  OUO_TOK_GT,
  OUO_TOK_GT_EQ,
  OUO_TOK_BANG,
  // Punctuation
  OUO_TOK_COMMA,
  OUO_TOK_PAREN_OPN,
  OUO_TOK_PAREN_CLS,
  OUO_TOK_BRACE_OPN,
  OUO_TOK_BRACE_CLS,
  OUO_TOK_COLON,
  OUO_TOK_ARROW,
} OuoTokenKind;

typedef struct {
  OuoTokenKind kind;
  OuoStringSlice str;
  OuoCodePosition pos;
} OuoToken;

#define OUO_TOK_FMT(tok) \
  (tok).kind == OUO_TOK_EOF           ? 3 \
      : (tok).kind == OUO_TOK_NEWLINE ? 2 \
                                      : (int)(tok).str.len, \
      (tok).kind == OUO_TOK_EOF       ? "EOF" \
      : (tok).kind == OUO_TOK_NEWLINE ? "LF" \
                                      : (tok).str.start

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
  OUO_AST_LIT_STR,
  OUO_AST_LIT_TYPE,
  // Expressions
  OUO_AST_ASSIGN,
  OUO_AST_BINARY,
  OUO_AST_UNARY,
  OUO_AST_CALL,
  OUO_AST_BLOCK,
  OUO_AST_IF,
  OUO_AST_WHILE,
  // Statements
  OUO_AST_EXPR_STMT,
  OUO_AST_PRINT,
  OUO_AST_DECL_VAR,
  OUO_AST_DECL_FN,
} OuoAstKind;

struct OuoAst;

typedef struct {
  OuoToken name;
  struct OuoAst *type_annot;
} OuoAstNameType;

typedef struct {
  OuoAstNameType *items;
  size_t count;
  size_t capacity;
} OuoAstNameTypes;

struct OuoSymbol;

#ifndef OUO_NOEMIT
struct OuoChunk;
#endif

/// Owns memory for any child `ast` nodes and their `chunk` fields.
typedef struct OuoAst {
  OuoAstKind kind;
  OuoToken tok;
  OuoType type;

  // Common
  struct {
    struct OuoAst **items;
    size_t count;
    size_t capacity;
  } children;

  union {
    struct {
      OuoToken name;
      struct OuoSymbol *sym;
    } ident;

    // Literals
    ouo_int_t lit_int;
    ouo_float_t lit_float;
    ouo_bool_t lit_bool;
    OuoString lit_str;
    OuoType lit_type;

    // Expressions
    struct {
      struct OuoAst *target;
      struct OuoAst *value;
    } assign;

    struct {
      struct OuoAst *left;
      OuoTokenKind op;
      struct OuoAst *right;
    } binary;

    struct {
      OuoTokenKind op;
      struct OuoAst *right;
    } unary;

    struct {
      struct OuoAst *target;
    } call;

    struct {
      struct OuoAst *condition;
      struct OuoAst *then_branch;
      struct OuoAst *else_branch;
    } if_expr;

    struct {
      struct OuoAst *condition;
      struct OuoAst *body;
    } while_expr;

    // Statements
    struct {
      struct OuoAst *expr;
      bool pop;
    } expr_stmt;

    struct {
      OuoToken name;
      struct OuoAst *type_annot;
      struct OuoAst *value;
    } decl_var;

    struct {
      OuoToken name;
      OuoAstNameTypes args;
      struct OuoAst *return_type_annot;
      struct OuoAst *body;
#ifndef OUO_NOEMIT
      struct OuoChunk *chunk;
#endif
    } decl_fn;
  } as;
} OuoAst;

/// Owns memory for `ast` and `errors`.
typedef struct {
  bool failed;
  size_t line;
  OuoAst *ast;
  OuoErrors errors;
} OuoParseResult;

/// Caller owns the parse result's `ast` and `errors`.
void ouo_parse(const char *src, OuoParseResult *res);

/// Frees the parse result's `ast` and `errors`.
void ouo_p_res_free(OuoParseResult *res);

//
// Compilation
//

typedef struct OuoSymbol {
  size_t idx;
  OuoToken name;
  OuoType *type;
  bool is_global;
  size_t scope_depth;
} OuoSymbol;

#ifndef OUO_NOEMIT

typedef enum {
  // Objects
  OUO_OP_POP,
  OUO_OP_GET,
  OUO_OP_SET,
  OUO_OP_GET_GLOBAL,
  OUO_OP_LIT,
  // Arithmetic
  OUO_OP_ADD_INT,
  OUO_OP_ADD_FLOAT,
  OUO_OP_ADD_STR,
  OUO_OP_SUB_INT,
  OUO_OP_SUB_FLOAT,
  OUO_OP_MULT_INT,
  OUO_OP_MULT_FLOAT,
  OUO_OP_DIV_INT,
  OUO_OP_DIV_FLOAT,
  OUO_OP_NEG_INT,
  OUO_OP_NEG_FLOAT,
  // Comparison
  OUO_OP_EQ_INT,
  OUO_OP_EQ_FLOAT,
  OUO_OP_EQ_BOOL,
  OUO_OP_NEQ_INT,
  OUO_OP_NEQ_FLOAT,
  OUO_OP_NEQ_BOOL,
  OUO_OP_LT_INT,
  OUO_OP_LT_FLOAT,
  OUO_OP_LT_EQ_INT,
  OUO_OP_LT_EQ_FLOAT,
  OUO_OP_GT_INT,
  OUO_OP_GT_FLOAT,
  OUO_OP_GT_EQ_INT,
  OUO_OP_GT_EQ_FLOAT,
  // Logic
  OUO_OP_NOT,
  // Control flow
  OUO_OP_JUMP,
  OUO_OP_JUMP_IF_FALSE,
  OUO_OP_LOOP,
  OUO_OP_RETURN,
  // Input/output
  OUO_OP_PRINT,
} OuoOpCode;

struct OuoObject;

/// Owns memory for `items`.
typedef struct {
  struct OuoObject *items;
  size_t count;
  size_t capacity;
} OuoObjects;

/// Owns memory for `literals`, `globals`, `bytecode` and `lines`.
typedef struct OuoChunk {
  OuoStringSlice name;

  OuoObjects literals;
  OuoObjects globals;

  struct {
    uint8_t *items;
    size_t count;
    size_t capacity;
  } bytecode;

  // RLE encoded, length first
  struct {
    size_t *items;
    size_t count;
    size_t capacity;
  } lines;
} OuoChunk;

#endif // OUO_NOEMIT

/// Owns memory for `items`.
typedef struct {
  OuoSymbol *items;
  size_t count;
  size_t capacity;
} OuoChunkSymbols;

/// Owns memory for `chunk`, `local_syms`, `global_syms`, `types` and `errors`.
typedef struct {
  bool failed;
  bool keep_module_scope;
  bool echo;

#ifndef OUO_NOEMIT
  OuoChunk chunk;
#endif

  OuoChunkSymbols local_syms;
  OuoChunkSymbols global_syms;

  struct {
    OuoType *items;
    size_t count;
    size_t capacity;
  } types;

  OuoErrors errors;
} OuoCompileResult;

/// Caller owns the compile result's `chunk`, `local_syms`,
/// `global_syms` and `errors`.
void ouo_compile(OuoAst *ast, OuoCompileResult *res);

/// Frees the compile result's `chunk` (except for `chunk.globals`)
/// and `errors`.
void ouo_c_res_free(OuoCompileResult *res);

/// Frees the compile result's `chunk.globals`, `local_syms`,
/// `global_syms` and `types`.
void ouo_c_res_cleanup(OuoCompileResult *res);

#ifndef OUO_NOEMIT

//
// Virtual machine
//

typedef enum {
  // Copy-on-write
  OUO_OBJ_INT,
  OUO_OBJ_FLOAT,
  OUO_OBJ_BOOL,
  // Reference-counted
  OUO_OBJ_STR,
  OUO_OBJ_FN,
} OuoObjectKind;

typedef struct {
  size_t count;
} OuoRc;

typedef struct OuoObject {
  OuoObjectKind kind;

  union {
    // Copy-on-write
    ouo_int_t v_int;
    ouo_float_t v_float;
    ouo_bool_t v_bool;
    // Reference-counted
    OuoRc *ref;
  } as;
} OuoObject;

/// Owns memory for `str`.
typedef struct {
  OuoRc ref;
  OuoString str;
} OuoRcStr;

/// Owns memory for `chunk`.
typedef struct {
  OuoRc ref;
  size_t arity;
  OuoChunk chunk;
} OuoRcFn;

#define OUO_FRAMES_SIZE 64
#define OUO_VM_STACK_SIZE (OUO_FRAMES_SIZE * UINT8_MAX)

/// Owns memory for any reference-counted objects on the `stack`.
typedef struct {
  bool failed;

  struct {
    OuoObject items[OUO_VM_STACK_SIZE];
    size_t count;
  } stack;

  OuoError error;
} OuoInterpretResult;

/// Caller owns the interpret result's `errors`.
void ouo_interpret(OuoChunk *chunk, OuoInterpretResult *res);

/// Frees the interpret result's `stack`.
void ouo_i_res_cleanup(OuoInterpretResult *res);

#endif // OUO_NOEMIT

#endif // OUO_H

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef OUO_IMPLEMENTATION
//
// Data types
//

static void _ouo_type_free(OuoType *type) {
  if (type->kind == OUO_TYPE_FN) ouo_da_free(type->as.t_fn.args);
}

static inline bool _ouo_type_is(OuoType *a, OuoType *b) {
  if (a->kind != b->kind) return false;
  switch (a->kind) {
    case OUO_TYPE_UNKNOWN:
    case OUO_TYPE_VOID: break;
    // Scalar
    case OUO_TYPE_INT:
    case OUO_TYPE_FLOAT:
    case OUO_TYPE_BOOL:
    case OUO_TYPE_STR: break;
    // Funtion
    case OUO_TYPE_FN:
      if (!_ouo_type_is(a->as.t_fn.return_type, b->as.t_fn.return_type))
        return false;
      if (a->as.t_fn.args.count != b->as.t_fn.args.count) return false;
      for (size_t i = 0; i < a->as.t_fn.args.count; i++)
        if (!_ouo_type_is(
                a->as.t_fn.args.items[i].type, b->as.t_fn.args.items[i].type))
          return false;
      break;
  }
  return true;
}

static const char *_ouo_type_kind_str(OuoTypeKind kind) {
  switch (kind) {
    case OUO_TYPE_UNKNOWN: return "unknown";
    case OUO_TYPE_VOID: return "void";
    // Scalar
    case OUO_TYPE_INT: return "int";
    case OUO_TYPE_FLOAT: return "float";
    case OUO_TYPE_BOOL: return "bool";
    case OUO_TYPE_STR: return "str";
    // Funtion
    case OUO_TYPE_FN: return "fn";
  }
  return "";
}

static OuoString _ouo_type_str(OuoType *type) {
  OuoString str = {0};
  const char *kind_str = _ouo_type_kind_str(type->kind);
  size_t kind_strlen = strlen(kind_str);
  ouo_da_append_many(&str, kind_str, kind_strlen);

  if (type->kind == OUO_TYPE_FN) {
    ouo_da_append_many(&str, "(", 1);
    for (size_t i = 0; i < type->as.t_fn.args.count; i++) {
      OuoNameType *arg = &type->as.t_fn.args.items[i];
      ouo_da_append_many(&str, arg->name.start, arg->name.len);
      ouo_da_append_many(&str, ": ", 2);

      OuoString arg_str = _ouo_type_str(arg->type);
      ouo_da_append_many(&str, arg_str.items, arg_str.count);
      ouo_da_free(arg_str);

      if (i < type->as.t_fn.args.count - 1) ouo_da_append_many(&str, ", ", 2);
    }
    ouo_da_append_many(&str, "): ", 3);

    OuoString return_type_str = _ouo_type_str(type->as.t_fn.return_type);
    ouo_da_append_many(&str, return_type_str.items, return_type_str.count);
    ouo_da_free(return_type_str);
  }

  return str;
}

static inline uint8_t _ouo_utf8_cp_len(const char *p) {
  unsigned char c = (unsigned char)p[0];
  if ((c & 0x80) == 0x00) return 1;
  else if ((c & 0xE0) == 0xC0) return 2;
  else if ((c & 0xF0) == 0xE0) return 3;
  else if ((c & 0xF8) == 0xF0) return 4;
  else return 1;
}

//
// Error handling
//

#ifdef OUO_DEBUG
#define _OUO_ERRCODEPOS OUO_CODEPOS
#else
#define _OUO_ERRCODEPOS
#endif

#define _ouo_err_sprintf(err, ...) \
  snprintf((err).msg, OUO_ERRMSG_SIZE, _OUO_ERRCODEPOS __VA_ARGS__)

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
    case OUO_ERR_DOUBLE_FREE:
    case OUO_ERR_USAGE:
    case OUO_ERR_READ: return "ERROR";
  }
  return "";
}

void ouo_err_msg_print(OuoError *err, const char *src, const char *path) {
  const char *line_start = err->pos.line_start;
  size_t line_len = 0;

  if (line_start == NULL && err->pos.line != 0 && src != NULL) {
    size_t line = 1;
    for (const char *p = src; *p != '\0'; p++) {
      if (line == err->pos.line) {
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
  ouo_printerr("%zu:", err->pos.line);
  if (err->pos.col != 0) ouo_printerr("%zu:", err->pos.col);
  if (err->fn_name.start != NULL && err->fn_name.len > 0)
    ouo_printerr(" %.*s:", OUO_STRSL_FMT(err->fn_name));
  ouo_printerr(OUO_ER "%s %s: " OUO_EBR "%s" OUO_ER,
      err->code == OUO_ERR_NOTE ? OUO_EBD : OUO_EBRED,
      _ouo_err_code_str(err->code), err->msg);

  if (line_len != 0) {
    ouo_printerr("\n%.*s", (int)line_len, line_start);
    if (err->len > 0) {
      ouo_printerr("\n" OUO_ED);
      if (err->pos.col > 0)
        for (size_t i = 0; i < err->pos.col - 1; i++) ouo_printerr(" ");
      for (size_t i = 0; i < err->len; i++) ouo_printerr("^");
      ouo_printerr(OUO_ER);
    }
  }

  ouo_printerr("\n");
}

//
// Lexing
//

typedef struct {
  const char *tok_start;
  const char *curr;
  OuoCodePosition pos;
} _OuoLexer;

static inline void _ouo_l_init(
    _OuoLexer *l, OuoParseResult *res, const char *src) {
  l->tok_start = src;
  l->curr = src;

  l->pos.line = res->line + 1;
  l->pos.col = 1;
  l->pos.line_start = src;
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
  l->pos.col++;
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

static inline OuoToken _ouo_l_tok_len_new(
    _OuoLexer *l, OuoTokenKind kind, size_t len) {
  return (OuoToken){
      .kind = kind,
      .str = {.start = l->tok_start, .len = len},
      .pos = {.line = l->pos.line,
          .col = l->pos.col - len,
          .line_start = l->pos.line_start},
  };
}

static inline OuoToken _ouo_l_tok_new(_OuoLexer *l, OuoTokenKind kind) {
  return _ouo_l_tok_len_new(l, kind, (size_t)(l->curr - l->tok_start));
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
    case 'i':
      if (is_long) switch (l->tok_start[1]) {
          case 'n': return _ouo_l_check_kw(l, 2, 1, "t", OUO_TOK_KW_INT);
          case 'f': return _ouo_l_check_kw(l, 1, 1, "f", OUO_TOK_KW_IF);
        }
      break;
    case 'f':
      if (is_long) switch (l->tok_start[1]) {
          case 'l': return _ouo_l_check_kw(l, 2, 3, "oat", OUO_TOK_KW_FLOAT);
          case 'a': return _ouo_l_check_kw(l, 2, 3, "lse", OUO_TOK_LIT_FALSE);
          case 'n': return _ouo_l_check_kw(l, 1, 1, "n", OUO_TOK_KW_FN);
        }
      break;
    case 'b': return _ouo_l_check_kw(l, 1, 3, "ool", OUO_TOK_KW_BOOL);
    case 's': return _ouo_l_check_kw(l, 1, 2, "tr", OUO_TOK_KW_STR);
    case 'v':
      if (is_long) switch (l->tok_start[1]) {
          case 'o': return _ouo_l_check_kw(l, 2, 2, "id", OUO_TOK_KW_VOID);
          case 'a': return _ouo_l_check_kw(l, 2, 1, "r", OUO_TOK_KW_VAR);
        }
      break;
    case 'o': return _ouo_l_check_kw(l, 1, 1, "r", OUO_TOK_KW_OR);
    case 'a': return _ouo_l_check_kw(l, 1, 2, "nd", OUO_TOK_KW_AND);
    case 'e': return _ouo_l_check_kw(l, 1, 3, "lse", OUO_TOK_KW_ELSE);
    case 'w': return _ouo_l_check_kw(l, 1, 4, "hile", OUO_TOK_KW_WHILE);
    case 'p': return _ouo_l_check_kw(l, 1, 4, "rint", OUO_TOK_KW_PRINT);
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

static OuoToken _ouo_l_read_string(_OuoLexer *l) {
  while (_ouo_l_peek(l) != '"' && !_ouo_l_is_eof(l)) _ouo_l_advance(l);
  if (!_ouo_l_is_eof(l)) _ouo_l_advance(l);
  return _ouo_l_tok_new(l, OUO_TOK_LIT_STR);
}

static inline bool _ouo_l_check_peek(_OuoLexer *l, char exp) {
  if (_ouo_l_peek(l) != exp) return false;
  _ouo_l_advance(l);
  return true;
}

static OuoToken _ouo_l_next_token(_OuoLexer *l) {
  _ouo_l_skip_whitespace(l);
  l->tok_start = l->curr;

  if (_ouo_l_is_eof(l)) return _ouo_l_tok_new(l, OUO_TOK_EOF);

  char c = _ouo_l_advance(l);

  if (c == '\n') {
    OuoToken tok = _ouo_l_tok_new(l, OUO_TOK_NEWLINE);
    l->pos.line++;
    l->pos.col = 1;
    l->pos.line_start = l->curr;
    return tok;
  }

  if (_ouo_l_isalpha(c)) return _ouo_l_read_word(l);

  // Literals
  if (_ouo_l_isdigit(c)) return _ouo_l_read_number(l);

  switch (c) {
    case '"': return _ouo_l_read_string(l);
    // Operators
    case '=':
      return _ouo_l_tok_new(l,
          _ouo_l_check_peek(l, '=')       ? OUO_TOK_EQ
              : _ouo_l_check_peek(l, '>') ? OUO_TOK_ARROW
                                          : OUO_TOK_ASSIGN);
    case '+': return _ouo_l_tok_new(l, OUO_TOK_PLUS);
    case '-': return _ouo_l_tok_new(l, OUO_TOK_MINUS);
    case '*': return _ouo_l_tok_new(l, OUO_TOK_ASTERISK);
    case '/': return _ouo_l_tok_new(l, OUO_TOK_SLASH);
    case '!':
      return _ouo_l_tok_new(
          l, _ouo_l_check_peek(l, '=') ? OUO_TOK_NEQ : OUO_TOK_BANG);
    case '<':
      return _ouo_l_tok_new(
          l, _ouo_l_check_peek(l, '=') ? OUO_TOK_LT_EQ : OUO_TOK_LT);
    case '>':
      return _ouo_l_tok_new(
          l, _ouo_l_check_peek(l, '=') ? OUO_TOK_GT_EQ : OUO_TOK_GT);
    // Punctuation
    case ',': return _ouo_l_tok_new(l, OUO_TOK_COMMA);
    case '(': return _ouo_l_tok_new(l, OUO_TOK_PAREN_OPN);
    case ')': return _ouo_l_tok_new(l, OUO_TOK_PAREN_CLS);
    case '{': return _ouo_l_tok_new(l, OUO_TOK_BRACE_OPN);
    case '}': return _ouo_l_tok_new(l, OUO_TOK_BRACE_CLS);
    case ':': return _ouo_l_tok_new(l, OUO_TOK_COLON);
    default: break;
  }

  // Illegal
  size_t len = _ouo_utf8_cp_len(l->tok_start);
  for (size_t i = 0; i < len - 1; i++) _ouo_l_advance(l);
  return _ouo_l_tok_len_new(l, OUO_TOK_ILLEGAL, len);
}

static const char *_ouo_tok_kind_str(OuoTokenKind kind) {
  switch (kind) {
    case OUO_TOK_ILLEGAL: return "ILLEGAL";
    case OUO_TOK_EOF: return "EOF";
    case OUO_TOK_NEWLINE: return "NEWLINE";
    case OUO_TOK_IDENT: return "IDENT";
    // Keywords
    case OUO_TOK_KW_INT: return "int";
    case OUO_TOK_KW_FLOAT: return "float";
    case OUO_TOK_KW_BOOL: return "bool";
    case OUO_TOK_KW_STR: return "str";
    case OUO_TOK_KW_VOID: return "void";
    case OUO_TOK_KW_OR: return "or";
    case OUO_TOK_KW_AND: return "and";
    case OUO_TOK_KW_IF: return "if";
    case OUO_TOK_KW_ELSE: return "else";
    case OUO_TOK_KW_WHILE: return "while";
    case OUO_TOK_KW_PRINT: return "print";
    case OUO_TOK_KW_VAR: return "var";
    case OUO_TOK_KW_FN: return "fn";
    // Literals
    case OUO_TOK_LIT_INT: return "LIT_INT";
    case OUO_TOK_LIT_FLOAT: return "LIT_FLOAT";
    case OUO_TOK_LIT_TRUE: return "LIT_TRUE";
    case OUO_TOK_LIT_FALSE: return "LIT_FALSE";
    case OUO_TOK_LIT_STR: return "LIT_STR";
    // Operators
    case OUO_TOK_ASSIGN: return "=";
    case OUO_TOK_PLUS: return "+";
    case OUO_TOK_MINUS: return "-";
    case OUO_TOK_ASTERISK: return "*";
    case OUO_TOK_SLASH: return "/";
    case OUO_TOK_EQ: return "==";
    case OUO_TOK_NEQ: return "!=";
    case OUO_TOK_LT: return "<";
    case OUO_TOK_LT_EQ: return "<=";
    case OUO_TOK_GT: return ">";
    case OUO_TOK_GT_EQ: return ">=";
    case OUO_TOK_BANG: return "!";
    // Punctuation
    case OUO_TOK_COMMA: return ",";
    case OUO_TOK_PAREN_OPN: return "(";
    case OUO_TOK_PAREN_CLS: return ")";
    case OUO_TOK_BRACE_OPN: return "{";
    case OUO_TOK_BRACE_CLS: return "}";
    case OUO_TOK_COLON: return ":";
    case OUO_TOK_ARROW: return "=>";
  }
  return "";
}

#ifdef OUO_DEBUG
static void _ouo_tok_dump(OuoToken *tok) {
  ouo_printdbg("[%s '%.*s'] ", _ouo_tok_kind_str(tok->kind), OUO_TOK_FMT(*tok));
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
  bool ignore_newline;

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
  _OUO_PREC_OR,
  _OUO_PREC_AND,
  _OUO_PREC_EQUALITY,
  _OUO_PREC_RELATION,
  _OUO_PREC_SUM,
  _OUO_PREC_PRODUCT,
  _OUO_PREC_UNARY,
  _OUO_PREC_ACCESS,
} _OuoPrecedence;

typedef struct _OuoParseRule {
  _OuoParsePrefixFn prefix_fn;
  _OuoParseInfixFn infix_fn;
  _OuoPrecedence prec;
} _OuoParseRule;

#define _ouo_p_err_append(p, tok, err_code, ...) \
  do { \
    OuoError err = { \
        .code = (err_code), \
        .len = (tok).str.len == 0 ? 1 : (tok).str.len, \
        .pos = (tok).pos, \
        .msg = {0}, \
    }; \
    _ouo_err_sprintf(err, __VA_ARGS__); \
    ouo_da_append(&(p)->res->errors, err); \
  } while (0)

#define _ouo_p_err(p, tok, err_code, ...) \
  do { \
    if (!(p)->panic_mode) { \
      (p)->res->failed = true; \
      (p)->panic_mode = true; \
      _ouo_p_err_append(p, tok, err_code, __VA_ARGS__); \
    } \
  } while (0)

#define _ouo_p_err_unexpected(p, tok, exp) \
  _ouo_p_err(p, tok, OUO_ERR_SYNTAX, "Expected '%s', got '%.*s'.", \
      _ouo_tok_kind_str(exp), OUO_TOK_FMT(tok))

static OuoAst *_ouo_p_stmt(_OuoParser *p, bool exp_newline);

#ifdef OUO_DEBUG
static void _ouo_ast_dump(OuoAst *ast);
#endif

static inline void _ouo_p_advance(_OuoParser *p) {
  p->curr = p->peek;
  p->peek = _ouo_l_next_token(p->l);
  if (p->ignore_newline) {
    while (p->curr.kind == OUO_TOK_NEWLINE) {
      p->curr = p->peek;
      p->peek = _ouo_l_next_token(p->l);
    }
    while (p->peek.kind == OUO_TOK_NEWLINE) p->peek = _ouo_l_next_token(p->l);
  }
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
  ast->type.kind = OUO_TYPE_UNKNOWN;

  ast->children.items = NULL;
  ast->children.count = 0;
  ast->children.capacity = 0;

  return ast;
}

static void _ouo_p_stmts(_OuoParser *p, OuoAst *ast, OuoTokenKind end_tok) {
  while (p->curr.kind != OUO_TOK_EOF) {
    if (p->curr.kind == end_tok) return;
    if (p->curr.kind == OUO_TOK_NEWLINE) {
      _ouo_p_advance(p);
      continue;
    }
    OuoAst *stmt = _ouo_p_stmt(p, true);
    if (stmt != NULL) ouo_da_append(&ast->children, stmt);
    _ouo_p_advance(p);
  }
}

static OuoAst *_ouo_p_module(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_MODULE);
  _ouo_p_stmts(p, ast, OUO_TOK_EOF);
  return ast;
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
        "Expected an expression, got '%.*s'.", OUO_TOK_FMT(p->curr));
    return NULL;
  }

  OuoAst *left = prefix_fn(p);

  while (p->peek.kind != OUO_TOK_NEWLINE &&
      prec < _ouo_p_get_rule(p, p->peek.kind)->prec) {
    _OuoParseInfixFn infix_fn = _ouo_p_get_rule(p, p->peek.kind)->infix_fn;

    if (infix_fn == NULL) {
      _ouo_p_err(p, p->peek, OUO_ERR_SYNTAX,
          "Expected an operator, got '%.*s'.", OUO_TOK_FMT(p->peek));
      return left;
    }

    _ouo_p_advance(p);
    left = infix_fn(p, left);
  }

  return left;
}

static OuoAst *_ouo_p_ident(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_IDENT);
  ast->as.ident.name = p->curr;
  ast->as.ident.sym = NULL;
  return ast;
}

static OuoAst *_ouo_p_lit_int(_OuoParser *p) {
  errno = 0;
  char *end = NULL;
  ouo_int_t lit = ouo_strtoi(p->curr.str.start, &end, 10);

  if (errno != 0) {
    _ouo_p_err(p, p->curr, OUO_ERR_PARSE_FAIL,
        "Integer literal value out of range (min %" OUO_PRId ", max %" OUO_PRId
        ").",
        OUO_INT_MIN, OUO_INT_MAX);
    return NULL;
  }

  if (end != p->curr.str.start + p->curr.str.len) {
    _ouo_p_err(p, p->curr, OUO_ERR_PARSE_FAIL,
        "Integer literal length mismatch. Expected %zu, read %zu.",
        p->curr.str.len, end - p->curr.str.start);
    return NULL;
  }

  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_LIT_INT);
  ast->as.lit_int = lit;
  return ast;
}

static OuoAst *_ouo_p_lit_float(_OuoParser *p) {
  errno = 0;
  char *end = NULL;
  ouo_float_t lit = ouo_strtof(p->curr.str.start, &end);

  if (errno != 0) {
    _ouo_p_err(
        p, p->curr, OUO_ERR_PARSE_FAIL, "Float literal value out of range.");
    return NULL;
  }

  if (end != p->curr.str.start + p->curr.str.len) {
    _ouo_p_err(p, p->curr, OUO_ERR_PARSE_FAIL,
        "Float literal length mismatch. Expected %zu, read %zu.",
        p->curr.str.len, end - p->curr.str.start);
    return NULL;
  }

  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_LIT_FLOAT);
  ast->as.lit_float = lit;
  return ast;
}

static OuoAst *_ouo_p_lit_bool(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_LIT_BOOL);
  ast->as.lit_bool = p->curr.kind == OUO_TOK_LIT_TRUE;
  return ast;
}

static OuoAst *_ouo_p_lit_str(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_LIT_STR);
  ast->as.lit_str.items = NULL;
  ast->as.lit_str.count = 0;
  ast->as.lit_str.capacity = 0;
  ouo_da_append_many(&ast->as.lit_str, p->curr.str.start, p->curr.str.len);
  return ast;
}

static inline OuoAst *_ouo_p_lit_type(_OuoParser *p) {
  OuoTypeKind type_kind = OUO_TYPE_UNKNOWN;
  switch (p->curr.kind) {
    case OUO_TOK_KW_INT: type_kind = OUO_TYPE_INT; break;
    case OUO_TOK_KW_FLOAT: type_kind = OUO_TYPE_FLOAT; break;
    case OUO_TOK_KW_BOOL: type_kind = OUO_TYPE_BOOL; break;
    case OUO_TOK_KW_STR: type_kind = OUO_TYPE_STR; break;
    case OUO_TOK_KW_VOID: type_kind = OUO_TYPE_VOID; break;
    default:
      _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX, "Expected a type, got '%.*s'.",
          OUO_TOK_FMT(p->curr));
      return NULL;
  }

  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_LIT_TYPE);
  ast->as.lit_type.kind = type_kind;
  return ast;
}

static OuoAst *_ouo_p_assign(_OuoParser *p, OuoAst *left) {
  OuoToken op = p->curr;
  _ouo_p_advance(p);
  OuoAst *right = _ouo_p_expr(p, _OUO_PREC_LOWEST);

  OuoAst *ast = _ouo_ast_new(&op, OUO_AST_ASSIGN);
  ast->as.assign.target = left;
  ast->as.assign.value = right;
  return ast;
}

static OuoAst *_ouo_p_binary(_OuoParser *p, OuoAst *left) {
  OuoToken op = p->curr;
  _OuoPrecedence prec = _ouo_p_get_rule(p, op.kind)->prec;
  _ouo_p_advance(p);
  OuoAst *right = _ouo_p_expr(p, prec);

  OuoAst *ast = _ouo_ast_new(&op, OUO_AST_BINARY);
  ast->as.binary.left = left;
  ast->as.binary.op = op.kind;
  ast->as.binary.right = right;
  return ast;
}

static OuoAst *_ouo_p_unary(_OuoParser *p) {
  OuoToken op = p->curr;
  _ouo_p_advance(p);
  OuoAst *right = _ouo_p_expr(p, _OUO_PREC_UNARY);

  OuoAst *ast = _ouo_ast_new(&op, OUO_AST_UNARY);
  ast->as.unary.op = op.kind;
  ast->as.unary.right = right;
  return ast;
}

static void _ouo_p_exprs(_OuoParser *p, OuoAst *ast, OuoTokenKind end_tok) {
  while (p->peek.kind != OUO_TOK_EOF) {
    if (p->peek.kind == end_tok) return;
    _ouo_p_advance(p);

    OuoAst *expr = _ouo_p_expr(p, _OUO_PREC_LOWEST);
    ouo_da_append(&ast->children, expr);

    if (p->peek.kind == OUO_TOK_COMMA) _ouo_p_advance(p);
    else if (p->peek.kind != end_tok) {
      _ouo_p_err_unexpected(p, p->peek, OUO_TOK_COMMA);
      return;
    }
  }
}

static OuoAst *_ouo_p_call(_OuoParser *p, OuoAst *left) {
  OuoToken op = p->curr;
  bool ignore_newline_prev = p->ignore_newline;
  p->ignore_newline = true;

  OuoAst *ast = _ouo_ast_new(&op, OUO_AST_CALL);
  ast->as.call.target = left;
  _ouo_p_exprs(p, ast, OUO_TOK_PAREN_CLS);

  p->ignore_newline = ignore_newline_prev;
  if (p->peek.kind != OUO_TOK_PAREN_CLS) {
    _ouo_p_err_unexpected(p, p->peek, OUO_TOK_PAREN_CLS);
    return ast;
  }
  _ouo_p_advance(p);
  return ast;
}

static OuoAst *_ouo_p_grouping(_OuoParser *p) {
  bool ignore_newline_prev = p->ignore_newline;
  p->ignore_newline = true;

  OuoToken tok = p->curr;
  _ouo_p_advance(p);
  OuoAst *ast = _ouo_p_expr(p, _OUO_PREC_LOWEST);

  p->ignore_newline = ignore_newline_prev;
  if (p->peek.kind != OUO_TOK_PAREN_CLS) {
    bool panic_prev = p->panic_mode;
    _ouo_p_err_unexpected(p, p->peek, OUO_TOK_PAREN_CLS);
    if (!panic_prev)
      _ouo_p_err_append(p, tok, OUO_ERR_NOTE, "Grouping starts here.");
    return ast;
  }

  _ouo_p_advance(p);
  return ast;
}

static OuoAst *_ouo_p_block(_OuoParser *p) {
  bool ignore_newline_prev = p->ignore_newline;
  p->ignore_newline = false;

  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_BLOCK);

  _ouo_p_advance(p);
  _ouo_p_stmts(p, ast, OUO_TOK_BRACE_CLS);

  p->ignore_newline = ignore_newline_prev;
  if (p->curr.kind != OUO_TOK_BRACE_CLS) {
    bool panic_prev = p->panic_mode;
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_BRACE_CLS);
    if (!panic_prev)
      _ouo_p_err_append(p, ast->tok, OUO_ERR_NOTE, "Block starts here.");
    return ast;
  }

  return ast;
}

static OuoAst *_ouo_p_if(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_IF);
  ast->as.if_expr.condition = NULL;
  ast->as.if_expr.then_branch = NULL;
  ast->as.if_expr.else_branch = NULL;

  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_PAREN_OPN) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_PAREN_OPN);
    return ast;
  }

  bool ignore_newline_prev = p->ignore_newline;
  p->ignore_newline = true;

  _ouo_p_advance(p);
  ast->as.if_expr.condition = _ouo_p_expr(p, _OUO_PREC_LOWEST);

  p->ignore_newline = ignore_newline_prev;
  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_PAREN_CLS) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_PAREN_CLS);
    return ast;
  }

  _ouo_p_advance(p);
  ast->as.if_expr.then_branch = _ouo_p_stmt(p, false);

  if (p->peek.kind == OUO_TOK_KW_ELSE) {
    _ouo_p_advance(p);
    _ouo_p_advance(p);
    ast->as.if_expr.else_branch = _ouo_p_stmt(p, false);
  }

  return ast;
}

static OuoAst *_ouo_p_while(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_WHILE);
  ast->as.while_expr.condition = NULL;
  ast->as.while_expr.body = NULL;

  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_PAREN_OPN) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_PAREN_OPN);
    return ast;
  }

  bool ignore_newline_prev = p->ignore_newline;
  p->ignore_newline = true;

  _ouo_p_advance(p);
  ast->as.while_expr.condition = _ouo_p_expr(p, _OUO_PREC_LOWEST);

  p->ignore_newline = ignore_newline_prev;
  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_PAREN_CLS) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_PAREN_CLS);
    return ast;
  }

  _ouo_p_advance(p);
  ast->as.while_expr.body = _ouo_p_stmt(p, false);
  return ast;
}

static OuoAst *_ouo_p_expr_stmt(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_EXPR_STMT);
  ast->as.expr_stmt.expr = _ouo_p_expr(p, _OUO_PREC_LOWEST);
  ast->as.expr_stmt.pop = false;
  return ast;
}

static OuoAst *_ouo_p_print(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_PRINT);
  _ouo_p_advance(p);
  ast->as.expr_stmt.expr = _ouo_p_expr(p, _OUO_PREC_LOWEST);
  return ast;
}

static OuoAst *_ouo_p_decl_var(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_DECL_VAR);
  ast->as.decl_var.type_annot = NULL;
  ast->as.decl_var.value = NULL;

  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_IDENT) {
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX,
        "Expected an identifier, got '%.*s'.", OUO_TOK_FMT(p->curr));
    return ast;
  }

  OuoToken ident = p->curr;
  _ouo_p_advance(p);

  if (p->curr.kind == OUO_TOK_COLON) {
    _ouo_p_advance(p);
    ast->as.decl_var.type_annot = _ouo_p_lit_type(p);
    if (ast->as.decl_var.type_annot == NULL) return ast;
    _ouo_p_advance(p);
  }

  if (p->curr.kind != OUO_TOK_ASSIGN) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_ASSIGN);
    return ast;
  }

  ast->tok = p->curr;
  _ouo_p_advance(p);
  ast->as.decl_var.name = ident;
  ast->as.decl_var.value = _ouo_p_expr(p, _OUO_PREC_LOWEST);
  return ast;
}

static void _ouo_p_name_types(
    _OuoParser *p, OuoAstNameTypes *nts, OuoTokenKind end_tok) {
  while (p->peek.kind != OUO_TOK_EOF) {
    if (p->peek.kind == end_tok) return;

    if (p->peek.kind != OUO_TOK_IDENT) {
      _ouo_p_err(p, p->peek, OUO_ERR_SYNTAX,
          "Expected an identifier, got '%.*s'.", OUO_TOK_FMT(p->peek));
      return;
    }

    OuoToken ident = p->peek;
    _ouo_p_advance(p);

    if (p->peek.kind != OUO_TOK_COLON) {
      _ouo_p_err_unexpected(p, p->peek, OUO_TOK_COLON);
      return;
    }
    _ouo_p_advance(p);
    _ouo_p_advance(p);

    OuoAst *type = _ouo_p_lit_type(p);
    if (type == NULL) return;

    OuoAstNameType nt = (OuoAstNameType){.name = ident, .type_annot = type};
    ouo_da_append(nts, nt);

    if (p->peek.kind == OUO_TOK_COMMA) _ouo_p_advance(p);
    else if (p->peek.kind != end_tok) {
      _ouo_p_err_unexpected(p, p->peek, OUO_TOK_COMMA);
      return;
    }
  }
}

static OuoAst *_ouo_p_decl_fn(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_DECL_FN);
  ast->as.decl_fn.args.items = NULL;
  ast->as.decl_fn.args.count = 0;
  ast->as.decl_fn.args.capacity = 0;

  ast->as.decl_fn.return_type_annot = NULL;
  ast->as.decl_fn.body = NULL;
#ifndef OUO_NOEMIT
  ast->as.decl_fn.chunk = ouo_malloc(sizeof(OuoChunk));
  ouo_assert_nomem(ast->as.decl_fn.chunk);
#endif

  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_IDENT) {
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX,
        "Expected an identifier, got '%.*s'.", OUO_TOK_FMT(p->curr));
    return ast;
  }

  OuoToken ident = p->curr;
  _ouo_p_advance(p);

  if (p->curr.kind != OUO_TOK_PAREN_OPN) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_PAREN_OPN);
    return ast;
  }
  bool ignore_newline_prev = p->ignore_newline;
  p->ignore_newline = true;

  _ouo_p_name_types(p, &ast->as.decl_fn.args, OUO_TOK_PAREN_CLS);

  p->ignore_newline = ignore_newline_prev;
  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_PAREN_CLS) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_PAREN_CLS);
    return ast;
  }
  _ouo_p_advance(p);

  if (p->curr.kind == OUO_TOK_COLON) {
    _ouo_p_advance(p);
    ast->as.decl_fn.return_type_annot = _ouo_p_lit_type(p);
    if (ast->as.decl_fn.return_type_annot == NULL) return ast;
    _ouo_p_advance(p);
  }

  if (p->curr.kind != OUO_TOK_ARROW) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_ARROW);
    return ast;
  }

  ast->tok = p->curr;
  _ouo_p_advance(p);
  ast->as.decl_fn.name = ident;
  ast->as.decl_fn.body = _ouo_p_expr(p, _OUO_PREC_LOWEST);
  return ast;
}

static void _ouo_p_synchronize(_OuoParser *p) {
  p->panic_mode = false;
  while (p->peek.kind != OUO_TOK_EOF) {
    if (p->curr.kind == OUO_TOK_NEWLINE || p->peek.kind == OUO_TOK_BRACE_OPN ||
        p->peek.kind == OUO_TOK_BRACE_CLS)
      return;
    _ouo_p_advance(p);
  }
}

static OuoAst *_ouo_p_stmt(_OuoParser *p, bool exp_newline) {
  OuoAst *ast;
  switch (p->curr.kind) {
    case OUO_TOK_KW_PRINT: ast = _ouo_p_print(p); break;
    case OUO_TOK_KW_VAR: ast = _ouo_p_decl_var(p); break;
    case OUO_TOK_KW_FN: ast = _ouo_p_decl_fn(p); break;
    default: ast = _ouo_p_expr_stmt(p); break;
  }

  if (exp_newline && p->peek.kind != OUO_TOK_EOF &&
      p->peek.kind != OUO_TOK_NEWLINE && p->peek.kind != OUO_TOK_BRACE_CLS) {
    _ouo_p_err(p, p->peek, OUO_ERR_SYNTAX, "Expected a new line, got '%.*s'.",
        OUO_TOK_FMT(p->peek));
  } else if (exp_newline && p->peek.kind == OUO_TOK_NEWLINE) {
    _ouo_p_advance(p);
  }

  if (p->panic_mode) _ouo_p_synchronize(p);
  return ast;
}

static const _OuoParseRule _ouo_p_rules[] = {
    [OUO_TOK_ILLEGAL] = {NULL, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_IDENT] = {_ouo_p_ident, NULL, _OUO_PREC_LOWEST},
    // Keywords
    [OUO_TOK_KW_OR] = {NULL, _ouo_p_binary, _OUO_PREC_OR},
    [OUO_TOK_KW_AND] = {NULL, _ouo_p_binary, _OUO_PREC_AND},
    [OUO_TOK_KW_IF] = {_ouo_p_if, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_KW_WHILE] = {_ouo_p_while, NULL, _OUO_PREC_LOWEST},
    // Literals
    [OUO_TOK_LIT_INT] = {_ouo_p_lit_int, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_LIT_FLOAT] = {_ouo_p_lit_float, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_LIT_TRUE] = {_ouo_p_lit_bool, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_LIT_FALSE] = {_ouo_p_lit_bool, NULL, _OUO_PREC_LOWEST},
    [OUO_TOK_LIT_STR] = {_ouo_p_lit_str, NULL, _OUO_PREC_LOWEST},
    // Operators
    [OUO_TOK_ASSIGN] = {NULL, _ouo_p_assign, _OUO_PREC_ASSIGN},
    [OUO_TOK_PLUS] = {NULL, _ouo_p_binary, _OUO_PREC_SUM},
    [OUO_TOK_MINUS] = {_ouo_p_unary, _ouo_p_binary, _OUO_PREC_SUM},
    [OUO_TOK_ASTERISK] = {NULL, _ouo_p_binary, _OUO_PREC_PRODUCT},
    [OUO_TOK_SLASH] = {NULL, _ouo_p_binary, _OUO_PREC_PRODUCT},
    [OUO_TOK_EQ] = {NULL, _ouo_p_binary, _OUO_PREC_EQUALITY},
    [OUO_TOK_NEQ] = {NULL, _ouo_p_binary, _OUO_PREC_EQUALITY},
    [OUO_TOK_LT] = {NULL, _ouo_p_binary, _OUO_PREC_RELATION},
    [OUO_TOK_LT_EQ] = {NULL, _ouo_p_binary, _OUO_PREC_RELATION},
    [OUO_TOK_GT] = {NULL, _ouo_p_binary, _OUO_PREC_RELATION},
    [OUO_TOK_GT_EQ] = {NULL, _ouo_p_binary, _OUO_PREC_RELATION},
    [OUO_TOK_BANG] = {_ouo_p_unary, NULL, _OUO_PREC_LOWEST},
    // Punctuation
    [OUO_TOK_PAREN_OPN] = {_ouo_p_grouping, _ouo_p_call, _OUO_PREC_ACCESS},
    [OUO_TOK_BRACE_OPN] = {_ouo_p_block, NULL, _OUO_PREC_LOWEST},
};

void ouo_parse(const char *src, OuoParseResult *res) {
  _OuoLexer l = {0};
  _ouo_l_init(&l, res, src);

#ifdef OUO_DEBUG
  for (;;) {
    OuoToken tok = _ouo_l_next_token(&l);
    _ouo_tok_dump(&tok);
    if (tok.kind == OUO_TOK_EOF) break;
  }
  ouo_printdbg("\n");
  _ouo_l_init(&l, res, src);
#endif

  _OuoParser p = {0};
  _ouo_p_init(&p, &l, res, _ouo_p_rules, ouo_arr_len(_ouo_p_rules));

  res->ast = _ouo_p_module(&p);
  res->line = l.pos.line;

#ifdef OUO_DEBUG
  _ouo_ast_dump(res->ast);
  ouo_printdbg("\n");
#endif
}

static void _ouo_ast_free(OuoAst *ast) {
  if (ast == NULL) return;

  switch (ast->kind) {
    case OUO_AST_MODULE:
    case OUO_AST_BLOCK:
      OUO_DA_FOREACH(OuoAst *, child, &ast->children) { _ouo_ast_free(*child); }
      ouo_da_free(ast->children);
      break;
    case OUO_AST_IDENT: break;
    // Literals
    case OUO_AST_LIT_INT:
    case OUO_AST_LIT_FLOAT:
    case OUO_AST_LIT_BOOL: break;
    case OUO_AST_LIT_STR: ouo_da_free(ast->as.lit_str); break;
    case OUO_AST_LIT_TYPE: _ouo_type_free(&ast->as.lit_type); break;
    // Expressions
    case OUO_AST_ASSIGN:
      _ouo_ast_free(ast->as.assign.target);
      _ouo_ast_free(ast->as.assign.value);
      break;
    case OUO_AST_BINARY:
      _ouo_ast_free(ast->as.binary.left);
      _ouo_ast_free(ast->as.binary.right);
      break;
    case OUO_AST_UNARY: _ouo_ast_free(ast->as.unary.right); break;
    case OUO_AST_CALL:
      _ouo_ast_free(ast->as.call.target);
      OUO_DA_FOREACH(OuoAst *, child, &ast->children) { _ouo_ast_free(*child); }
      ouo_da_free(ast->children);
      break;
    case OUO_AST_IF:
      _ouo_ast_free(ast->as.if_expr.condition);
      _ouo_ast_free(ast->as.if_expr.then_branch);
      _ouo_ast_free(ast->as.if_expr.else_branch);
      break;
    case OUO_AST_WHILE:
      _ouo_ast_free(ast->as.while_expr.condition);
      _ouo_ast_free(ast->as.while_expr.body);
      break;
    // Statements
    case OUO_AST_EXPR_STMT:
    case OUO_AST_PRINT: _ouo_ast_free(ast->as.expr_stmt.expr); break;
    case OUO_AST_DECL_VAR:
      if (ast->as.decl_var.type_annot != NULL)
        _ouo_ast_free(ast->as.decl_var.type_annot);
      _ouo_ast_free(ast->as.decl_var.value);
      break;
    case OUO_AST_DECL_FN:
#ifndef OUO_NOEMIT
      ouo_free(ast->as.decl_fn.chunk);
#endif
      OUO_DA_FOREACH(OuoAstNameType, arg, &ast->as.decl_fn.args) {
        _ouo_ast_free(arg->type_annot);
      }
      ouo_da_free(ast->as.decl_fn.args);
      if (ast->as.decl_fn.return_type_annot != NULL)
        _ouo_ast_free(ast->as.decl_fn.return_type_annot);
      _ouo_ast_free(ast->as.decl_fn.body);
      break;
  }

  ouo_free(ast);
}

void ouo_p_res_free(OuoParseResult *res) {
  _ouo_ast_free(res->ast);
  ouo_da_free(res->errors);
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
    case OUO_AST_LIT_STR: return "LIT_STR";
    case OUO_AST_LIT_TYPE: return "LIT_TYPE";
    // Expressions
    case OUO_AST_ASSIGN: return "ASSIGN";
    case OUO_AST_BINARY: return "BINARY";
    case OUO_AST_UNARY: return "UNARY";
    case OUO_AST_CALL: return "CALL";
    case OUO_AST_BLOCK: return "BLOCK";
    case OUO_AST_IF: return "IF";
    case OUO_AST_WHILE: return "WHILE";
    // Statements
    case OUO_AST_EXPR_STMT: return "EXPR_STMT";
    case OUO_AST_PRINT: return "PRINT";
    case OUO_AST_DECL_VAR: return "DECL_VAR";
    case OUO_AST_DECL_FN: return "DECL_FN";
  }
  return "";
}

static void _ouo_ast_dump(OuoAst *ast) {
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
        _ouo_ast_dump(*stmt);
        ouo_printdbg("\n");
      }
      break;
    case OUO_AST_IDENT:
      ouo_printdbg("%.*s", OUO_TOK_FMT(ast->as.ident.name));
      break;
    // Literals
    case OUO_AST_LIT_INT: ouo_printdbg("%" OUO_PRId, ast->as.lit_int); break;
    case OUO_AST_LIT_FLOAT:
      ouo_printdbg("%" OUO_PRIf, ast->as.lit_float);
      break;
    case OUO_AST_LIT_BOOL:
      ouo_printdbg(ast->as.lit_bool ? "true" : "false");
      break;
    case OUO_AST_LIT_STR:
      ouo_printdbg("%.*s", OUO_STR_FMT(ast->as.lit_str));
      break;
    case OUO_AST_LIT_TYPE: {
      OuoString type_str = _ouo_type_str(&ast->as.lit_type);
      ouo_printdbg("%.*s", OUO_STR_FMT(type_str));
      ouo_da_free(type_str);
      break;
    }
    // Expressions
    case OUO_AST_ASSIGN:
      _ouo_ast_dump(ast->as.assign.target);
      ouo_printdbg(" %s ", _ouo_tok_kind_str(OUO_TOK_ASSIGN));
      _ouo_ast_dump(ast->as.assign.value);
      break;
    case OUO_AST_BINARY:
      _ouo_ast_dump(ast->as.binary.left);
      ouo_printdbg(" %s ", _ouo_tok_kind_str(ast->as.binary.op));
      _ouo_ast_dump(ast->as.binary.right);
      break;
    case OUO_AST_UNARY:
      ouo_printdbg("%s ", _ouo_tok_kind_str(ast->as.unary.op));
      _ouo_ast_dump(ast->as.unary.right);
      break;
    case OUO_AST_CALL:
      _ouo_ast_dump(ast->as.call.target);
      ouo_printdbg(" (");
      OUO_DA_FOREACH(OuoAst *, expr, &ast->children) { _ouo_ast_dump(*expr); }
      ouo_printdbg(")");
      break;
    case OUO_AST_IF:
      _ouo_ast_dump(ast->as.if_expr.condition);
      ouo_printdbg(" then ");
      _ouo_ast_dump(ast->as.if_expr.then_branch);
      if (ast->as.if_expr.else_branch != NULL) {
        ouo_printdbg(" else ");
        _ouo_ast_dump(ast->as.if_expr.else_branch);
      }
      break;
    case OUO_AST_WHILE:
      _ouo_ast_dump(ast->as.while_expr.condition);
      _ouo_ast_dump(ast->as.while_expr.body);
      break;
    // Statements
    case OUO_AST_EXPR_STMT:
    case OUO_AST_PRINT:
      ouo_printdbg("%d ", ast->as.expr_stmt.pop);
      _ouo_ast_dump(ast->as.expr_stmt.expr);
      break;
    case OUO_AST_DECL_VAR:
      ouo_printdbg("%.*s ", OUO_TOK_FMT(ast->as.decl_var.name));
      _ouo_ast_dump(ast->as.decl_var.type_annot);
      ouo_printdbg(" ");
      _ouo_ast_dump(ast->as.decl_var.value);
      break;
    case OUO_AST_DECL_FN:
      ouo_printdbg("%.*s (", OUO_TOK_FMT(ast->as.decl_fn.name));
      OUO_DA_FOREACH(OuoAstNameType, arg, &ast->as.decl_fn.args) {
        ouo_printdbg("(%.*s ", OUO_TOK_FMT(arg->name));
        _ouo_ast_dump(arg->type_annot);
        ouo_printdbg(")");
      }
      ouo_printdbg(") ");
      _ouo_ast_dump(ast->as.decl_fn.return_type_annot);
      ouo_printdbg(" ");
      _ouo_ast_dump(ast->as.decl_fn.body);
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
  bool noemit;
  OuoCompileResult *res;
} _OuoCompiler;

#define _ouo_c_err_append(c, tok, err_code, ...) \
  do { \
    OuoError err = { \
        .code = (err_code), \
        .len = (tok).str.len, \
        .pos = (tok).pos, \
        .msg = {0}, \
    }; \
    _ouo_err_sprintf(err, __VA_ARGS__); \
    ouo_da_append(&(c)->res->errors, err); \
  } while (0)

#define _ouo_c_err(c, tok, err_code, ...) \
  do { \
    if (!(c)->panic_mode) { \
      (c)->res->failed = true; \
      (c)->panic_mode = true; \
      _ouo_c_err_append(c, tok, err_code, __VA_ARGS__); \
    } \
  } while (0)

static inline void _ouo_c_compile(_OuoCompiler *c, OuoAst *ast);

#ifndef OUO_NOEMIT
static inline bool _ouo_obj_is_rc(OuoObject *obj);
static inline void _ouo_obj_rc_ref(OuoObject *obj);
static inline bool _ouo_obj_rc_deref(OuoObject *obj);

static void _ouo_chunk_free(OuoChunk *chunk);
#ifdef OUO_DEBUG
static void _ouo_chunk_dump(OuoChunk *chunk);
#endif // OUO_DEBUG
#endif // OUO_NOEMIT

static inline void _ouo_c_init(_OuoCompiler *c, OuoCompileResult *res) {
  res->failed = false;
  c->res = res;
}

static inline bool _ouo_chunk_find_sym(
    OuoChunkSymbols *syms, OuoToken *name, OuoSymbol **res_sym) {
  if (syms->count == 0) return false;

  for (size_t i = syms->count - 1; i >= 0; i--) {
    if (ouo_str_slice_eq(&name->str, &syms->items[i].name.str)) {
      if (res_sym != NULL) *res_sym = &syms->items[i];
      return true;
    }
    if (i == 0) break;
  }

  return false;
}

static inline bool _ouo_c_find_sym(
    _OuoCompiler *c, OuoToken *name, OuoSymbol **res_sym) {
  if (_ouo_chunk_find_sym(&c->res->local_syms, name, res_sym)) return true;
  else if (_ouo_chunk_find_sym(&c->res->global_syms, name, res_sym))
    return true;
  return false;
}

static inline OuoSymbol *_ouo_c_chunk_add_sym(
    _OuoCompiler *c, OuoChunkSymbols *syms, OuoToken *name, OuoType *type) {
  OuoSymbol *found_sym = NULL;
  if (_ouo_c_find_sym(c, name, &found_sym)) {
    _ouo_c_err(c, *name, OUO_ERR_SEMANTIC, "Symbol '%.*s' is already defined.",
        OUO_TOK_FMT(found_sym->name));
    _ouo_c_err_append(
        c, found_sym->name, OUO_ERR_NOTE, "Previous definition here.");
    return NULL;
  }

  if (c->res->failed && c->res->keep_module_scope) return NULL;

  OuoSymbol sym = (OuoSymbol){
      .idx = syms->count,
      .name = *name,
      .type = type,
      .is_global = false,
      .scope_depth = c->scope_depth,
  };
  ouo_da_append(syms, sym);
  return &syms->items[syms->count - 1];
}

static inline OuoSymbol *_ouo_c_add_local_sym(
    _OuoCompiler *c, OuoToken *name, OuoType *type) {
  if (c->res->local_syms.count > UINT8_MAX) {
    _ouo_c_err(c, *name, OUO_ERR_COMPILE_FAIL,
        "Maximum amount of local symbols exceeded (max %d).", UINT8_MAX);
    return NULL;
  }

  return _ouo_c_chunk_add_sym(c, &c->res->local_syms, name, type);
}

static inline OuoSymbol *_ouo_c_add_global_sym(
    _OuoCompiler *c, OuoToken *name, OuoType *type) {
  if (c->res->global_syms.count > UINT8_MAX) {
    _ouo_c_err(c, *name, OUO_ERR_COMPILE_FAIL,
        "Maximum amount of global symbols exceeded (max %d).", UINT8_MAX);
    return NULL;
  }

  OuoSymbol *sym = _ouo_c_chunk_add_sym(c, &c->res->global_syms, name, type);
  if (sym != NULL) sym->is_global = true;
  return sym;
}

static inline OuoType *_ouo_c_add_type(_OuoCompiler *c, OuoType *type) {
  ouo_da_append(&c->res->types, *type);
  return &c->res->types.items[c->res->types.count - 1];
}

// Static analysis

static void _ouo_c_err_todo(_OuoCompiler *c, OuoAst *ast, const char *msg) {
  _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL, "TODO: %s", msg);
}

static void _ouo_c_err_ident_no_sym(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
      "Identifier '%.*s' has no symbol.", OUO_TOK_FMT(ast->as.ident.name));
}

static void _ouo_c_err_ident_undefined(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast->tok, OUO_ERR_SEMANTIC, "Undefined symbol '%.*s'.",
      OUO_TOK_FMT(ast->as.ident.name));
}

static void _ouo_c_err_assign_type(
    _OuoCompiler *c, OuoToken *tok, OuoType *target_type, OuoType *value_type) {
  OuoString value_type_str = _ouo_type_str(value_type);
  OuoString target_type_str = _ouo_type_str(target_type);

  _ouo_c_err(c, *tok, OUO_ERR_TYPE, "Cannot assign '%.*s' to '%.*s'.",
      OUO_STR_FMT(value_type_str), OUO_STR_FMT(target_type_str));

  ouo_da_free(value_type_str);
  ouo_da_free(target_type_str);
}

static void _ouo_c_err_assign_invalid(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast->tok, OUO_ERR_SEMANTIC,
      "Assignment target can only be a variable.");
}

static void _ouo_c_err_binary_type(_OuoCompiler *c, OuoAst *ast) {
  OuoString left_type_str = _ouo_type_str(&ast->as.binary.left->type);
  OuoString right_type_str = _ouo_type_str(&ast->as.binary.right->type);

  _ouo_c_err(c, ast->tok, OUO_ERR_TYPE,
      "Operation '%s' does not support '%.*s' and '%.*s'.",
      _ouo_tok_kind_str(ast->as.binary.op), OUO_STR_FMT(left_type_str),
      OUO_STR_FMT(right_type_str));

  ouo_da_free(left_type_str);
  ouo_da_free(right_type_str);
}

static void _ouo_c_err_binary_unknown(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL, "Unknown binary operator '%s'.",
      _ouo_tok_kind_str(ast->as.binary.op));
}

static void _ouo_c_err_unary_type(_OuoCompiler *c, OuoAst *ast) {
  OuoString type_str = _ouo_type_str(&ast->as.unary.right->type);

  _ouo_c_err(c, ast->tok, OUO_ERR_TYPE,
      "Operation '%s' does not support '%.*s'.",
      _ouo_tok_kind_str(ast->as.unary.op), OUO_STR_FMT(type_str));

  ouo_da_free(type_str);
}

static void _ouo_c_err_unary_unknown(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL, "Unknown unary operator '%s'.",
      _ouo_tok_kind_str(ast->as.unary.op));
}

static void _ouo_c_err_if_condition_type(_OuoCompiler *c, OuoAst *ast) {
  OuoString type_str = _ouo_type_str(&ast->as.if_expr.condition->type);

  _ouo_c_err(c, ast->as.if_expr.condition->tok, OUO_ERR_TYPE,
      "Condition can only be '%s', got '%.*s'.",
      _ouo_type_kind_str(OUO_TYPE_BOOL), OUO_STR_FMT(type_str));

  ouo_da_free(type_str);
}

static void _ouo_c_err_if_branch_type(
    _OuoCompiler *c, OuoAst *ast, OuoType *then_type, OuoType *else_type) {
  OuoString then_type_str = _ouo_type_str(then_type);
  OuoString else_type_str = _ouo_type_str(else_type);

  _ouo_c_err(c, ast->tok, OUO_ERR_TYPE,
      "All branches must evaluate to the same type (then is '%.*s', else is "
      "'%.*s').",
      OUO_STR_FMT(then_type_str), OUO_STR_FMT(else_type_str));

  ouo_da_free(then_type_str);
  ouo_da_free(else_type_str);
}

static void _ouo_c_err_stmt_type(
    _OuoCompiler *c, OuoToken *tok, OuoType *type) {
  OuoString type_str = _ouo_type_str(type);

  _ouo_c_err(c, *tok, OUO_ERR_TYPE, "Type of '%.*s' cannot be '%.*s'.",
      OUO_TOK_FMT(*tok), OUO_STR_FMT(type_str));

  ouo_da_free(type_str);
}

static void _ouo_c_err_var_type(_OuoCompiler *c, OuoAst *ast, OuoType *type) {
  OuoString type_str = _ouo_type_str(type);

  _ouo_c_err(c, ast->tok, OUO_ERR_TYPE, "A variabe cannot be '%.*s'.",
      OUO_STR_FMT(type_str));

  ouo_da_free(type_str);
}

static void _ouo_c_err_fn_type(_OuoCompiler *c, OuoAst *ast) {
  OuoString return_type_str =
      _ouo_type_str(&ast->as.decl_fn.return_type_annot->as.lit_type);
  OuoString body_type_str = _ouo_type_str(&ast->as.decl_fn.body->type);

  _ouo_c_err(c, ast->tok, OUO_ERR_TYPE,
      "Function returns '%.*s', but got '%.*s'.", OUO_STR_FMT(return_type_str),
      OUO_STR_FMT(body_type_str));

  ouo_da_free(return_type_str);
  ouo_da_free(body_type_str);
}

static inline bool _ouo_ast_binary_is(OuoAst *ast, OuoTypeKind type_kind) {
  return ast->as.binary.left->type.kind == type_kind &&
      ast->as.binary.right->type.kind == type_kind;
}

static inline bool _ouo_ast_unary_is(OuoAst *ast, OuoTypeKind type_kind) {
  return ast->as.unary.right->type.kind == type_kind;
}

static void _ouo_c_ast_analyze(_OuoCompiler *c, OuoAst *ast) {
  switch (ast->kind) {
    case OUO_AST_MODULE: ast->type.kind = OUO_TYPE_VOID; break;
    case OUO_AST_IDENT: {
      OuoSymbol *sym = NULL;
      if (_ouo_c_find_sym(c, &ast->as.ident.name, &sym)) {
        ast->type = *sym->type;
        ast->as.ident.sym = sym;
      } else _ouo_c_err_ident_undefined(c, ast);
      break;
    }

    // Literals
    case OUO_AST_LIT_INT: ast->type.kind = OUO_TYPE_INT; break;
    case OUO_AST_LIT_FLOAT: ast->type.kind = OUO_TYPE_FLOAT; break;
    case OUO_AST_LIT_BOOL: ast->type.kind = OUO_TYPE_BOOL; break;
    case OUO_AST_LIT_STR: ast->type.kind = OUO_TYPE_STR; break;
    case OUO_AST_LIT_TYPE: ast->type.kind = OUO_TYPE_VOID; break;

    // Expressions
    case OUO_AST_ASSIGN:
      ast->type.kind = OUO_TYPE_VOID;
      switch (ast->as.assign.target->kind) {
        case OUO_AST_IDENT:
          if (ast->as.ident.sym == NULL) {
            _ouo_c_err_ident_no_sym(c, ast);
            break;
          }
          if (!_ouo_type_is(
                  &ast->as.assign.value->type, &ast->as.assign.target->type) ||
              ast->as.assign.target->as.ident.sym->is_global)
            _ouo_c_err_assign_type(c, &ast->tok, &ast->as.assign.target->type,
                &ast->as.assign.value->type);
          break;
        default: _ouo_c_err_assign_invalid(c, ast); break;
      }
      break;
    case OUO_AST_BINARY:
      switch (ast->as.binary.op) {
        // Arithmetic
        case OUO_TOK_PLUS:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            ast->type.kind = OUO_TYPE_INT;
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            ast->type.kind = OUO_TYPE_FLOAT;
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_STR))
            ast->type.kind = OUO_TYPE_STR;
          else _ouo_c_err_binary_type(c, ast);
          break;
        case OUO_TOK_MINUS:
        case OUO_TOK_ASTERISK:
        case OUO_TOK_SLASH:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            ast->type.kind = OUO_TYPE_INT;
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            ast->type.kind = OUO_TYPE_FLOAT;
          else _ouo_c_err_binary_type(c, ast);
          break;

        // Comparison
        case OUO_TOK_EQ:
        case OUO_TOK_NEQ:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT) ||
              _ouo_ast_binary_is(ast, OUO_TYPE_FLOAT) ||
              _ouo_ast_binary_is(ast, OUO_TYPE_BOOL))
            ast->type.kind = OUO_TYPE_BOOL;
          else _ouo_c_err_binary_type(c, ast);
          break;
        case OUO_TOK_LT:
        case OUO_TOK_LT_EQ:
        case OUO_TOK_GT:
        case OUO_TOK_GT_EQ:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT) ||
              _ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            ast->type.kind = OUO_TYPE_BOOL;
          else _ouo_c_err_binary_type(c, ast);
          break;

        // Logic
        case OUO_TOK_KW_OR:
        case OUO_TOK_KW_AND:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_BOOL))
            ast->type.kind = OUO_TYPE_BOOL;
          else _ouo_c_err_binary_type(c, ast);
          break;

        default: _ouo_c_err_binary_unknown(c, ast); break;
      }
      break;
    case OUO_AST_UNARY:
      switch (ast->as.unary.op) {
        // Arithmetic
        case OUO_TOK_MINUS:
          if (_ouo_ast_unary_is(ast, OUO_TYPE_INT))
            ast->type.kind = OUO_TYPE_INT;
          else if (_ouo_ast_unary_is(ast, OUO_TYPE_FLOAT))
            ast->type.kind = OUO_TYPE_FLOAT;
          else _ouo_c_err_unary_type(c, ast);
          break;

        // Logic
        case OUO_TOK_BANG:
          if (_ouo_ast_unary_is(ast, OUO_TYPE_BOOL))
            ast->type.kind = OUO_TYPE_BOOL;
          else _ouo_c_err_unary_type(c, ast);
          break;

        default: _ouo_c_err_unary_unknown(c, ast); break;
      }
      break;
    case OUO_AST_CALL: _ouo_c_err_todo(c, ast, "call analyze"); break;
    case OUO_AST_BLOCK: ast->type.kind = OUO_TYPE_VOID; break;
    case OUO_AST_IF: {
      if (ast->as.if_expr.condition->type.kind != OUO_TYPE_BOOL) {
        bool panic_prev = c->panic_mode;
        c->panic_mode = false;
        _ouo_c_err_if_condition_type(c, ast);
        c->panic_mode = panic_prev;
      }

      OuoType *else_type = ast->as.if_expr.else_branch != NULL
          ? &ast->as.if_expr.else_branch->type
          : &(OuoType){.kind = OUO_TYPE_VOID};

      if (_ouo_type_is(&ast->as.if_expr.then_branch->type, else_type))
        ast->type = ast->as.if_expr.then_branch->type;
      else
        _ouo_c_err_if_branch_type(
            c, ast, &ast->as.if_expr.then_branch->type, else_type);
      break;
    }
    case OUO_AST_WHILE: {
      ast->type.kind = OUO_TYPE_VOID;
      if (ast->as.while_expr.condition->type.kind != OUO_TYPE_BOOL) {
        bool panic_prev = c->panic_mode;
        c->panic_mode = false;
        _ouo_c_err_if_condition_type(c, ast);
        c->panic_mode = panic_prev;
      }
      break;
    }

    // Statements
    case OUO_AST_EXPR_STMT: ast->type = ast->as.expr_stmt.expr->type; break;
    case OUO_AST_PRINT:
      ast->type.kind = OUO_TYPE_VOID;
      if (ast->as.expr_stmt.expr->type.kind == OUO_TYPE_VOID)
        _ouo_c_err_stmt_type(c, &ast->tok, &ast->as.expr_stmt.expr->type);
      break;
    case OUO_AST_DECL_VAR: {
      ast->type.kind = OUO_TYPE_VOID;
      bool panic_prev = c->panic_mode;
      c->panic_mode = false;

      OuoType *value_type = &ast->as.decl_var.value->type;
      OuoType *type = ast->as.decl_var.type_annot != NULL
          ? &ast->as.decl_var.type_annot->as.lit_type
          : value_type;

      if (type->kind == OUO_TYPE_VOID) {
        _ouo_c_err_var_type(c, ast, type);
      } else if (value_type->kind != OUO_TYPE_UNKNOWN &&
          !_ouo_type_is(value_type, type)) {
        _ouo_c_err_assign_type(c, &ast->tok, type, value_type);
      }

      _ouo_c_add_local_sym(c, &ast->as.decl_var.name, type);
      c->panic_mode = panic_prev;
      break;
    }
    case OUO_AST_DECL_FN: {
      ast->type.kind = OUO_TYPE_VOID;
      bool panic_prev = c->panic_mode;
      c->panic_mode = false;

      OuoType *body_type = &ast->as.decl_fn.body->type;
      OuoType *return_type = ast->as.decl_fn.return_type_annot != NULL
          ? &ast->as.decl_fn.return_type_annot->as.lit_type
          : body_type;

      if (body_type->kind != OUO_TYPE_UNKNOWN &&
          !_ouo_type_is(body_type, return_type))
        _ouo_c_err_fn_type(c, ast);

      OuoType *type = _ouo_c_add_type(c,
          &(OuoType){
              .kind = OUO_TYPE_FN,
              .as.t_fn = {.return_type = _ouo_c_add_type(c, return_type),
                  .args = {.count = 0}},
          });

      OUO_DA_FOREACH(OuoAstNameType, arg, &ast->as.decl_fn.args) {
        OuoNameType arg_type = {
            .name = arg->name.str,
            .type = _ouo_c_add_type(c, &arg->type_annot->as.lit_type),
        };
        ouo_da_append(&type->as.t_fn.args, arg_type);
      }

      _ouo_c_add_global_sym(c, &ast->as.decl_fn.name, type);
      c->panic_mode = panic_prev;
      break;
    }
  }
}

#ifndef OUO_NOEMIT

// Bytecode emission

static inline void _ouo_c_chunk_write(
    _OuoCompiler *c, uint8_t byte, size_t line) {
  ouo_da_append(&c->res->chunk.bytecode, byte);

  size_t lines_count = c->res->chunk.lines.count;
  if (lines_count == 0 || c->res->chunk.lines.items[lines_count - 1] != line) {
    ouo_da_append(&c->res->chunk.lines, 1);
    ouo_da_append(&c->res->chunk.lines, line);
  } else {
    c->res->chunk.lines.items[lines_count - 2]++;
  }
}

static inline size_t _ouo_chunk_get_line(OuoChunk *chunk, const uint8_t *ip) {
  size_t ip_idx = (size_t)(ip - chunk->bytecode.items);
  size_t ip_idx_curr = 0;
  for (size_t i = 0; i < chunk->lines.count; i += 2) {
    ip_idx_curr += chunk->lines.items[i];
    if (ip_idx_curr > ip_idx) return chunk->lines.items[i + 1];
  }
  return 0;
}

static inline void _ouo_c_emit_byte(
    _OuoCompiler *c, OuoAst *ast, uint8_t byte) {
  _ouo_c_chunk_write(c, byte, ast->tok.pos.line);
}

static inline void _ouo_c_emit_bytes2(
    _OuoCompiler *c, OuoAst *ast, uint8_t byte1, uint8_t byte2) {
  _ouo_c_emit_byte(c, ast, byte1);
  _ouo_c_emit_byte(c, ast, byte2);
}

static inline size_t _ouo_chunk_objs_add(OuoObjects *objs, OuoObject *obj) {
  if (_ouo_obj_is_rc(obj)) _ouo_obj_rc_ref(obj);
  ouo_da_append(objs, *obj);
  return objs->count - 1;
}

static inline void _ouo_c_emit_lit(
    _OuoCompiler *c, OuoAst *ast, OuoObject *obj) {
  if (c->res->chunk.literals.count > UINT8_MAX) {
    _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
        "Maximum amount of literals exceeded (max %d).", UINT8_MAX);
    return;
  }

  size_t lit_idx = _ouo_chunk_objs_add(&c->res->chunk.literals, obj);
  _ouo_c_emit_bytes2(c, ast, OUO_OP_LIT, (uint8_t)lit_idx);
}

static inline size_t _ouo_c_add_global(
    _OuoCompiler *c, OuoAst *ast, OuoObject *obj) {
  if (c->res->chunk.globals.count > UINT8_MAX) {
    _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
        "Maximum amount of globals exceeded (max %d).", UINT8_MAX);
    return SIZE_MAX;
  }

  return _ouo_chunk_objs_add(&c->res->chunk.globals, obj);
}

static inline size_t _ouo_c_emit_jump(
    _OuoCompiler *c, OuoAst *ast, uint8_t op) {
  _ouo_c_emit_byte(c, ast, op);
  _ouo_c_emit_byte(c, ast, UINT8_MAX);
  _ouo_c_emit_byte(c, ast, UINT8_MAX);
  return c->res->chunk.bytecode.count - 2;
}

static inline void _ouo_c_patch_jump(
    _OuoCompiler *c, OuoAst *ast, size_t op_idx) {
  size_t jump = c->res->chunk.bytecode.count - op_idx - 2;
  if (jump > UINT16_MAX) {
    _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
        "Maximum jump offset exceeded (max %d, got %zu).", UINT16_MAX, jump);
    return;
  }

  c->res->chunk.bytecode.items[op_idx] = (jump >> 8) & 0xFF;
  c->res->chunk.bytecode.items[op_idx + 1] = jump & 0xFF;
}

static inline void _ouo_c_emit_loop(
    _OuoCompiler *c, OuoAst *ast, size_t op_idx) {
  size_t jump = c->res->chunk.bytecode.count - op_idx + 3;
  if (jump > UINT16_MAX) {
    _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
        "Maximum loop offset exceeded (max %d, got %zu).", UINT16_MAX, jump);
    return;
  }

  _ouo_c_emit_byte(c, ast, OUO_OP_LOOP);
  _ouo_c_emit_byte(c, ast, (jump >> 8) & 0xFF);
  _ouo_c_emit_byte(c, ast, jump & 0xFF);
}

// Copy-on-write
#define _ouo_obj_new_int(v) ((OuoObject){.kind = OUO_OBJ_INT, .as.v_int = (v)})

#define _ouo_obj_new_float(v) \
  ((OuoObject){.kind = OUO_OBJ_FLOAT, .as.v_float = (v)})

#define _ouo_obj_new_bool(v) \
  ((OuoObject){.kind = OUO_OBJ_BOOL, .as.v_bool = (v)})

// Reference-counted
static inline OuoRc *_ouo_rc_new(size_t size) {
  OuoRc *rc = ouo_malloc(size);
  ouo_assert_nomem(rc);
  rc->count = 0;
  return rc;
}

static inline OuoRcStr *_ouo_rc_new_str(void) {
  OuoRcStr *rc = (OuoRcStr *)_ouo_rc_new(sizeof(OuoRcStr));
  rc->str.items = NULL;
  rc->str.count = 0;
  rc->str.capacity = 0;
  return rc;
}

#define _ouo_obj_new_str(rc) \
  ((OuoObject){.kind = OUO_OBJ_STR, .as.ref = (OuoRc *)(rc)})

static inline OuoRcFn *_ouo_rc_new_fn(void) {
  OuoRcFn *rc = (OuoRcFn *)_ouo_rc_new(sizeof(OuoRcFn));
  rc->arity = 0;
  rc->chunk = (OuoChunk){0};
  return rc;
}

#define _ouo_obj_new_fn(rc) \
  ((OuoObject){.kind = OUO_OBJ_FN, .as.ref = (OuoRc *)(rc)})

static void _ouo_c_ast_emit(_OuoCompiler *c, OuoAst *ast) {
  switch (ast->kind) {
    case OUO_AST_MODULE: break;
    case OUO_AST_IDENT: {
      OuoSymbol *sym = ast->as.ident.sym;
      if (sym == NULL) {
        _ouo_c_err_ident_no_sym(c, ast);
        break;
      }
      if (sym->is_global && sym->type->kind == OUO_TYPE_FN)
        _ouo_c_emit_bytes2(c, ast, OUO_OP_GET_GLOBAL, (uint8_t)sym->idx);
      else _ouo_c_emit_bytes2(c, ast, OUO_OP_GET, (uint8_t)sym->idx);
      break;
    }

    // Literals
    case OUO_AST_LIT_INT:
      _ouo_c_emit_lit(c, ast, &_ouo_obj_new_int(ast->as.lit_int));
      break;
    case OUO_AST_LIT_FLOAT:
      _ouo_c_emit_lit(c, ast, &_ouo_obj_new_float(ast->as.lit_float));
      break;
    case OUO_AST_LIT_BOOL:
      _ouo_c_emit_lit(c, ast, &_ouo_obj_new_bool(ast->as.lit_bool));
      break;
    case OUO_AST_LIT_STR: {
      OuoRcStr *rc = _ouo_rc_new_str();
      ouo_da_append_many(
          &rc->str, ast->as.lit_str.items + 1, ast->as.lit_str.count - 2);
      _ouo_c_emit_lit(c, ast, &_ouo_obj_new_str(rc));
      break;
    }
    case OUO_AST_LIT_TYPE: break;

    // Expressions
    case OUO_AST_ASSIGN:
      switch (ast->as.assign.target->kind) {
        case OUO_AST_IDENT: {
          OuoSymbol *sym = ast->as.assign.target->as.ident.sym;
          if (sym == NULL) {
            _ouo_c_err_ident_no_sym(c, ast);
            break;
          }
          if (sym->is_global) {
            _ouo_c_err_assign_invalid(c, ast);
            break;
          }
          _ouo_c_emit_bytes2(c, ast, OUO_OP_SET, (uint8_t)sym->idx);
          break;
        }
        default: _ouo_c_err_assign_invalid(c, ast); break;
      }
      break;
    case OUO_AST_BINARY:
      switch (ast->as.binary.op) {
        // Arithmetic
        case OUO_TOK_PLUS:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_ADD_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_ADD_FLOAT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_STR))
            _ouo_c_emit_byte(c, ast, OUO_OP_ADD_STR);
          else _ouo_c_err_binary_type(c, ast);
          break;

        case OUO_TOK_MINUS:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_SUB_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_SUB_FLOAT);
          else _ouo_c_err_binary_type(c, ast);
          break;

        case OUO_TOK_ASTERISK:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_MULT_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_MULT_FLOAT);
          else _ouo_c_err_binary_type(c, ast);
          break;

        case OUO_TOK_SLASH:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_DIV_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_DIV_FLOAT);
          else _ouo_c_err_binary_type(c, ast);
          break;

        // Comparison
        case OUO_TOK_EQ:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_EQ_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_EQ_FLOAT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_BOOL))
            _ouo_c_emit_byte(c, ast, OUO_OP_EQ_BOOL);
          else _ouo_c_err_binary_type(c, ast);
          break;

        case OUO_TOK_NEQ:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_NEQ_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_NEQ_FLOAT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_BOOL))
            _ouo_c_emit_byte(c, ast, OUO_OP_NEQ_BOOL);
          else _ouo_c_err_binary_type(c, ast);
          break;

        case OUO_TOK_LT:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_LT_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_LT_FLOAT);
          else _ouo_c_err_binary_type(c, ast);
          break;

        case OUO_TOK_LT_EQ:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_LT_EQ_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_LT_EQ_FLOAT);
          else _ouo_c_err_binary_type(c, ast);
          break;

        case OUO_TOK_GT:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_GT_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_GT_FLOAT);
          else _ouo_c_err_binary_type(c, ast);
          break;

        case OUO_TOK_GT_EQ:
          if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_GT_EQ_INT);
          else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_GT_EQ_FLOAT);
          else _ouo_c_err_binary_type(c, ast);
          break;

        // Logic
        case OUO_TOK_KW_OR: break;
        case OUO_TOK_KW_AND: break;

        default: _ouo_c_err_binary_unknown(c, ast); break;
      }
      break;
    case OUO_AST_UNARY:
      switch (ast->as.unary.op) {
        // Arithmetic
        case OUO_TOK_MINUS:
          if (_ouo_ast_unary_is(ast, OUO_TYPE_INT))
            _ouo_c_emit_byte(c, ast, OUO_OP_NEG_INT);
          else if (_ouo_ast_unary_is(ast, OUO_TYPE_FLOAT))
            _ouo_c_emit_byte(c, ast, OUO_OP_NEG_FLOAT);
          else _ouo_c_err_unary_type(c, ast);
          break;

        // Logic
        case OUO_TOK_BANG:
          if (_ouo_ast_unary_is(ast, OUO_TYPE_BOOL))
            _ouo_c_emit_byte(c, ast, OUO_OP_NOT);
          else _ouo_c_err_unary_type(c, ast);
          break;

        default: _ouo_c_err_unary_unknown(c, ast); break;
      }
      break;
    case OUO_AST_CALL: _ouo_c_err_todo(c, ast, "call emit"); break;
    case OUO_AST_BLOCK: break;
    case OUO_AST_IF: break;
    case OUO_AST_WHILE: break;

    // Statements
    case OUO_AST_EXPR_STMT:
      if (ast->as.expr_stmt.pop && ast->type.kind != OUO_TYPE_VOID) {
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
    case OUO_AST_DECL_FN: {
      if (ast->as.decl_fn.args.count > UINT8_MAX) {
        _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
            "Maximum amount of arguments exceeded (max %d).", UINT8_MAX);
        break;
      }
      OuoRcFn *rc = _ouo_rc_new_fn();
      rc->arity = ast->as.decl_fn.args.count;
      rc->chunk = *ast->as.decl_fn.chunk;
      _ouo_c_add_global(c, ast, &_ouo_obj_new_fn(rc));
      break;
    }
  }
}

#endif // OUO_NOEMIT

static inline void _ouo_c_scope_begin(_OuoCompiler *c) { c->scope_depth++; }

static inline bool _ouo_c_chunk_scope_has_syms(
    _OuoCompiler *c, OuoChunkSymbols *syms) {
  return syms->count > 0 &&
      syms->items[syms->count - 1].scope_depth > c->scope_depth;
}

static inline void _ouo_c_chunk_scope_pop(
    _OuoCompiler *c, OuoAst *ast, OuoChunkSymbols *syms, bool emit) {
  while (_ouo_c_chunk_scope_has_syms(c, syms)) {
    syms->count--;
#ifndef OUO_NOEMIT
    if (emit) _ouo_c_emit_byte(c, ast, OUO_OP_POP);
#else
    (void)ast;
    (void)emit;
#endif
  }
}

static inline void _ouo_c_scope_end(_OuoCompiler *c, OuoAst *ast) {
  c->scope_depth--;
  _ouo_c_chunk_scope_pop(c, ast, &c->res->local_syms, true);
  _ouo_c_chunk_scope_pop(c, ast, &c->res->global_syms, false);
}

static inline bool _ouo_ast_is_global(OuoAst *ast) {
  return ast->kind == OUO_AST_DECL_FN;
}

static void _ouo_c_ast_visit(_OuoCompiler *c, OuoAst *ast) {
  if (ast == NULL) return;
  bool new_scope = false;

  switch (ast->kind) {
    case OUO_AST_MODULE:
    case OUO_AST_BLOCK: {
      bool panic_prev = c->panic_mode;
      c->panic_mode = false;
      if (ast->kind == OUO_AST_BLOCK || !c->res->keep_module_scope) {
        new_scope = true;
        _ouo_c_scope_begin(c);
      }

      OUO_DA_FOREACH(OuoAst *, stmt_p, &ast->children) {
        OuoAst *stmt = *stmt_p;
        if (_ouo_ast_is_global(stmt)) {
          _ouo_c_ast_visit(c, stmt);
          c->panic_mode = false;
        }
      }

      OUO_DA_FOREACH(OuoAst *, stmt_p, &ast->children) {
        OuoAst *stmt = *stmt_p;
        if (!_ouo_ast_is_global(stmt)) {
          if ((stmt)->kind == OUO_AST_EXPR_STMT)
            (stmt)->as.expr_stmt.pop = true;
          _ouo_c_ast_visit(c, stmt);
          c->panic_mode = false;
        }
      }

      c->panic_mode = panic_prev;
      break;
    }
    case OUO_AST_IDENT: break;

    // Literals
    case OUO_AST_LIT_INT:
    case OUO_AST_LIT_FLOAT:
    case OUO_AST_LIT_BOOL:
    case OUO_AST_LIT_STR:
    case OUO_AST_LIT_TYPE: break;

    // Expressions
    case OUO_AST_ASSIGN: {
      bool noemit_prev = c->noemit;
      c->noemit = true;
      _ouo_c_ast_visit(c, ast->as.assign.target);
      c->noemit = noemit_prev;
      _ouo_c_ast_visit(c, ast->as.assign.value);
      break;
    }
    case OUO_AST_BINARY:
      switch (ast->as.binary.op) {
        case OUO_TOK_KW_OR: {
          _ouo_c_ast_visit(c, ast->as.binary.left);

#ifndef OUO_NOEMIT
          size_t else_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP_IF_FALSE);
          size_t end_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP);
          _ouo_c_patch_jump(c, ast, else_jump);
          _ouo_c_emit_byte(c, ast, OUO_OP_POP);
#endif

          _ouo_c_ast_visit(c, ast->as.binary.right);

#ifndef OUO_NOEMIT
          _ouo_c_patch_jump(c, ast, end_jump);
#endif
          break;
        }
        case OUO_TOK_KW_AND: {
          _ouo_c_ast_visit(c, ast->as.binary.left);

#ifndef OUO_NOEMIT
          size_t end_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP_IF_FALSE);
          _ouo_c_emit_byte(c, ast, OUO_OP_POP);
#endif

          _ouo_c_ast_visit(c, ast->as.binary.right);

#ifndef OUO_NOEMIT
          _ouo_c_patch_jump(c, ast, end_jump);
#endif
          break;
        }
        default:
          _ouo_c_ast_visit(c, ast->as.binary.left);
          _ouo_c_ast_visit(c, ast->as.binary.right);
          break;
      }
      break;
    case OUO_AST_UNARY: _ouo_c_ast_visit(c, ast->as.unary.right); break;
    case OUO_AST_CALL: _ouo_c_err_todo(c, ast, "call visit"); break;
    case OUO_AST_IF: {
      c->panic_mode = false;
      _ouo_c_ast_visit(c, ast->as.if_expr.condition);
      bool panic_prev = c->panic_mode;

#ifndef OUO_NOEMIT
      size_t then_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP_IF_FALSE);
      _ouo_c_emit_byte(c, ast, OUO_OP_POP);
#endif

      c->panic_mode = false;
      _ouo_c_scope_begin(c);
      _ouo_c_ast_visit(c, ast->as.if_expr.then_branch);
      _ouo_c_scope_end(c, ast);

#ifndef OUO_NOEMIT
      size_t else_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP);
      _ouo_c_patch_jump(c, ast, then_jump);
      _ouo_c_emit_byte(c, ast, OUO_OP_POP);
#endif

      if (ast->as.if_expr.else_branch != NULL) {
        c->panic_mode = false;
        _ouo_c_scope_begin(c);
        _ouo_c_ast_visit(c, ast->as.if_expr.else_branch);
        _ouo_c_scope_end(c, ast);
      }

#ifndef OUO_NOEMIT
      _ouo_c_patch_jump(c, ast, else_jump);
#endif
      c->panic_mode = panic_prev;
      break;
    }
    case OUO_AST_WHILE: {
#ifndef OUO_NOEMIT
      size_t loop_start = c->res->chunk.bytecode.count;
#endif

      c->panic_mode = false;
      _ouo_c_ast_visit(c, ast->as.while_expr.condition);
      bool panic_prev = c->panic_mode;

#ifndef OUO_NOEMIT
      size_t exit_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP_IF_FALSE);
      _ouo_c_emit_byte(c, ast, OUO_OP_POP);
#endif

      c->panic_mode = false;
      _ouo_c_scope_begin(c);
      _ouo_c_ast_visit(c, ast->as.while_expr.body);
      _ouo_c_scope_end(c, ast);

#ifndef OUO_NOEMIT
      _ouo_c_emit_loop(c, ast, loop_start);
      _ouo_c_patch_jump(c, ast, exit_jump);
      _ouo_c_emit_byte(c, ast, OUO_OP_POP);
#endif
      c->panic_mode = panic_prev;
      break;
    }

    // Statements
    case OUO_AST_EXPR_STMT:
    case OUO_AST_PRINT: _ouo_c_ast_visit(c, ast->as.expr_stmt.expr); break;
    case OUO_AST_DECL_VAR:
      if (ast->as.decl_var.type_annot != NULL)
        _ouo_c_ast_visit(c, ast->as.decl_var.type_annot);
      _ouo_c_ast_visit(c, ast->as.decl_var.value);
      break;
    case OUO_AST_DECL_FN: {
      OUO_DA_FOREACH(OuoAstNameType, arg, &ast->as.decl_fn.args) {
        _ouo_c_ast_visit(c, arg->type_annot);
      }
      if (ast->as.decl_fn.return_type_annot != NULL)
        _ouo_c_ast_visit(c, ast->as.decl_fn.return_type_annot);

      OuoCompileResult fn_res = {0};
#ifndef OUO_NOEMIT
      fn_res.chunk.name = ast->as.decl_fn.name.str;
#endif

      _OuoCompiler fn_c = {0};
      _ouo_c_init(&fn_c, &fn_res);

      _ouo_c_scope_begin(&fn_c);
      OUO_DA_FOREACH(OuoAstNameType, arg, &ast->as.decl_fn.args) {
        _ouo_c_add_local_sym(&fn_c, &arg->name, &arg->type_annot->as.lit_type);
      }

      _ouo_c_compile(&fn_c, ast->as.decl_fn.body);
      _ouo_c_scope_end(&fn_c, ast);

      if (fn_res.failed) {
        c->res->failed = true;
        OUO_DA_FOREACH(OuoError, err, &fn_res.errors) {
          ouo_da_append(&c->res->errors, *err);
        }
      }

#ifndef OUO_NOEMIT
      if (!fn_res.failed) *ast->as.decl_fn.chunk = fn_res.chunk;
      else _ouo_chunk_free(&fn_res.chunk);
#endif

      ouo_da_free(fn_res.errors);
      ouo_da_free(fn_res.local_syms);
      ouo_da_free(fn_res.global_syms);
      break;
    }
  }

  _ouo_c_ast_analyze(c, ast);
  if (c->res->failed || c->noemit) return;

#ifndef OUO_NOEMIT
  _ouo_c_ast_emit(c, ast);
#endif

  if (new_scope) _ouo_c_scope_end(c, ast);
}

static inline void _ouo_c_compile(_OuoCompiler *c, OuoAst *ast) {
  _ouo_c_ast_visit(c, ast);

#ifdef OUO_DEBUG
  ouo_printdbg("local syms: ");
  for (size_t i = 0; i < c->res->local_syms.count; i++) {
    OuoSymbol sym = c->res->local_syms.items[i];
    OuoString type_str = _ouo_type_str(sym.type);
    ouo_printdbg("[%zu '%.*s' %.*s (%zu)] ", i, OUO_STR_FMT(type_str),
        OUO_TOK_FMT(sym.name), sym.scope_depth);
    ouo_da_free(type_str);
  }
  ouo_printdbg("\nglobal syms: ");
  for (size_t i = 0; i < c->res->global_syms.count; i++) {
    OuoSymbol sym = c->res->global_syms.items[i];
    OuoString type_str = _ouo_type_str(sym.type);
    ouo_printdbg("[%zu '%.*s' %.*s (%zu)] ", i, OUO_STR_FMT(type_str),
        OUO_TOK_FMT(sym.name), sym.scope_depth);
    ouo_da_free(type_str);
  }
  ouo_printdbg("\n");

#ifndef OUO_NOEMIT
  _ouo_chunk_dump(&c->res->chunk);
  ouo_printdbg("\n");
#endif // OUO_NOEMIT
#endif // OUO_DEBUG
}

void ouo_compile(OuoAst *ast, OuoCompileResult *res) {
  _OuoCompiler c = {0};
  _ouo_c_init(&c, res);

  _ouo_c_compile(&c, ast);
}

void ouo_c_res_free(OuoCompileResult *res) {
#ifndef OUO_NOEMIT
  _ouo_chunk_free(&res->chunk);
#endif
  ouo_da_free(res->errors);
}

void ouo_c_res_cleanup(OuoCompileResult *res) {
  ouo_da_free(res->local_syms);
  ouo_da_free(res->global_syms);

  OUO_DA_FOREACH(OuoType, type, &res->types) { _ouo_type_free(type); }
  ouo_da_free(res->types);

#ifndef OUO_NOEMIT
  OUO_DA_FOREACH(OuoObject, obj, &res->chunk.globals) {
    if (_ouo_obj_is_rc(obj)) _ouo_obj_rc_deref(obj);
  }
  ouo_da_free(res->chunk.globals);
#endif
}

#ifndef OUO_NOEMIT

static void _ouo_chunk_free(OuoChunk *chunk) {
#ifdef OUO_DEBUG
  ouo_printdbg("freeing %.*s...\n", OUO_STRSL_FMT(chunk->name));
#endif

  OUO_DA_FOREACH(OuoObject, lit, &chunk->literals) {
    if (_ouo_obj_is_rc(lit)) _ouo_obj_rc_deref(lit);
  }
  ouo_da_free(chunk->literals);
  ouo_da_free(chunk->bytecode);
  ouo_da_free(chunk->lines);
}

#define _ouo_chunk_read_byte(ip) *(++(ip))
#define _ouo_chunk_read_bytes2(ip) (ip += 2, (uint16_t)((ip[-1] << 8) | ip[0]))

static inline void _ouo_obj_print(OuoObject *obj) {
  switch (obj->kind) {
    // Copy-on-write
    case OUO_OBJ_INT: ouo_print("%" OUO_PRId, obj->as.v_int); break;
    case OUO_OBJ_FLOAT: ouo_print("%" OUO_PRIf, obj->as.v_float); break;
    case OUO_OBJ_BOOL: ouo_print(obj->as.v_bool ? "true" : "false"); break;
    // Reference-counted
    case OUO_OBJ_STR:
      ouo_print("%.*s", OUO_STR_FMT(((OuoRcStr *)obj->as.ref)->str));
      break;
    case OUO_OBJ_FN: {
      OuoRcFn *fn = (OuoRcFn *)obj->as.ref;
      ouo_print("fn %.*s(%zu)", OUO_STRSL_FMT(fn->chunk.name), fn->arity);
      break;
    }
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
    case OUO_OP_GET: return "GET";
    case OUO_OP_SET: return "SET";
    case OUO_OP_GET_GLOBAL: return "GET_GLOBAL";
    case OUO_OP_LIT: return "LIT";

    // Arithmetic
    case OUO_OP_ADD_INT: return "ADD_INT";
    case OUO_OP_ADD_FLOAT: return "ADD_FLOAT";
    case OUO_OP_ADD_STR: return "ADD_STR";
    case OUO_OP_SUB_INT: return "SUB_INT";
    case OUO_OP_SUB_FLOAT: return "SUB_FLOAT";
    case OUO_OP_MULT_INT: return "MULT_INT";
    case OUO_OP_MULT_FLOAT: return "MULT_FLOAT";
    case OUO_OP_DIV_INT: return "DIV_INT";
    case OUO_OP_DIV_FLOAT: return "DIV_FLOAT";

    case OUO_OP_NEG_INT: return "NEG_INT";
    case OUO_OP_NEG_FLOAT: return "NEG_FLOAT";

    // Comparison
    case OUO_OP_EQ_INT: return "EQ_INT";
    case OUO_OP_EQ_FLOAT: return "EQ_FLOAT";
    case OUO_OP_EQ_BOOL: return "EQ_BOOL";
    case OUO_OP_NEQ_INT: return "NEQ_INT";
    case OUO_OP_NEQ_FLOAT: return "NEQ_FLOAT";
    case OUO_OP_NEQ_BOOL: return "NEQ_BOOL";

    case OUO_OP_LT_INT: return "LT_INT";
    case OUO_OP_LT_FLOAT: return "LT_FLOAT";
    case OUO_OP_LT_EQ_INT: return "LT_EQ_INT";
    case OUO_OP_LT_EQ_FLOAT: return "LT_EQ_FLOAT";
    case OUO_OP_GT_INT: return "GT_INT";
    case OUO_OP_GT_FLOAT: return "GT_FLOAT";
    case OUO_OP_GT_EQ_INT: return "GT_EQ_INT";
    case OUO_OP_GT_EQ_FLOAT: return "GT_EQ_FLOAT";

    // Logic
    case OUO_OP_NOT: return "NOT";

    // Control flow
    case OUO_OP_JUMP: return "JUMP";
    case OUO_OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
    case OUO_OP_LOOP: return "LOOP";
    case OUO_OP_RETURN: return "RETURN";

    // Input/output
    case OUO_OP_PRINT: return "PRINT";
  }
  return "";
}

static ptrdiff_t _ouo_chunk_op_dump(OuoChunk *chunk, uint8_t *ip) {
  uint8_t *ip_prev = ip;
  ptrdiff_t ip_idx = ip - chunk->bytecode.items;

  ouo_printdbg("%04zd ", ip_idx);
  size_t line_curr = _ouo_chunk_get_line(chunk, ip);
  if (ip_idx > 0 && line_curr == _ouo_chunk_get_line(chunk, ip - 1))
    ouo_printdbg("   | ");
  else ouo_printdbg("%4zu ", line_curr);

  OuoOpCode op_code = (OuoOpCode)*ip;
  ouo_printdbg("%-16s", _ouo_op_code_str(op_code));

  switch (op_code) {
    // Objects
    case OUO_OP_GET:
    case OUO_OP_SET: {
      uint8_t sym_idx = _ouo_chunk_read_byte(ip);
      ouo_printdbg("%4d ", sym_idx);
      break;
    }
    case OUO_OP_GET_GLOBAL: {
      uint8_t global_idx = _ouo_chunk_read_byte(ip);
      ouo_printdbg("%4d '", global_idx);
      _ouo_obj_dump(&chunk->globals.items[global_idx]);
      ouo_printdbg("'");
      break;
    }
    case OUO_OP_LIT: {
      uint8_t lit_idx = _ouo_chunk_read_byte(ip);
      ouo_printdbg("%4d '", lit_idx);
      _ouo_obj_dump(&chunk->literals.items[lit_idx]);
      ouo_printdbg("'");
      break;
    }
    // Control flow
    case OUO_OP_JUMP:
    case OUO_OP_JUMP_IF_FALSE:
    case OUO_OP_LOOP: {
      uint16_t jump = _ouo_chunk_read_bytes2(ip);
      ouo_printdbg("%4d: %td -> %td", jump, ip_idx,
          ip_idx + 3 + jump * (op_code == OUO_OP_LOOP ? -1 : 1));
      break;
    }
    default: break;
  }

  return ip - ip_prev;
}

static void _ouo_chunk_dump(OuoChunk *chunk) {
  ouo_printdbg("CHUNK ");
  if (chunk->name.start != NULL)
    ouo_printdbg("%.*s:", OUO_STRSL_FMT(chunk->name));

  ouo_printdbg("\nliterals: ");
  for (size_t i = 0; i < chunk->literals.count; i++) {
    ouo_printdbg("[%zu '", i);
    _ouo_obj_dump(&chunk->literals.items[i]);
    ouo_printdbg("'] ");
  }
  ouo_printdbg("\nglobals: ");
  for (size_t i = 0; i < chunk->globals.count; i++) {
    ouo_printdbg("[%zu '", i);
    _ouo_obj_dump(&chunk->globals.items[i]);
    ouo_printdbg("'] ");
  }
  ouo_printdbg("\n");

  ouo_printdbg("lines: ");
  for (size_t i = 0; i < chunk->lines.count; i += 2)
    ouo_printdbg("%zu-%zu ", chunk->lines.items[i], chunk->lines.items[i + 1]);
  ouo_printdbg("\n");

  OUO_DA_FOREACH(uint8_t, ip, &chunk->bytecode) {
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
  uint8_t *ip;
  OuoObject *stack;
} _OuoCallFrame;

typedef struct {
  _OuoCallFrame frames[OUO_FRAMES_SIZE];
  size_t frame_count;
  OuoInterpretResult *res;
} _OuoVm;

#define _ouo_vm_err(vm, fr, err_code, ...) \
  do { \
    (vm)->res->failed = true; \
    OuoError error = { \
        .code = (err_code), \
        .pos = {.line = _ouo_chunk_get_line((fr)->chunk, (fr)->ip), \
            .line_start = NULL}, \
        .msg = {0}, \
        .fn_name = fr->chunk->name, \
    }; \
    _ouo_err_sprintf(error, __VA_ARGS__); \
    (vm)->res->error = error; \
  } while (0)

static inline void _ouo_vm_init(
    _OuoVm *vm, OuoInterpretResult *res, OuoChunk *chunk) {
  res->failed = false;
  vm->res = res;
  _OuoCallFrame *fr = &vm->frames[vm->frame_count++];
  fr->chunk = chunk;
  fr->ip = chunk->bytecode.items;
  fr->stack = vm->res->stack.items;
}

static inline bool _ouo_obj_is_rc(OuoObject *obj) {
  return obj->kind == OUO_OBJ_STR || obj->kind == OUO_OBJ_FN;
}

static inline void _ouo_obj_rc_ref(OuoObject *obj) {
#ifdef OUO_DEBUG
  ouo_printdbg("ref %zu -> %zu: ", obj->as.ref->count, obj->as.ref->count + 1);
  _ouo_obj_dump(obj);
  ouo_printdbg("\n");
#endif
  obj->as.ref->count++;
}

static inline bool _ouo_obj_rc_deref(OuoObject *obj) {
  OuoRc *rc = obj->as.ref;
  if (rc == NULL || rc->count == 0) return false;

#ifdef OUO_DEBUG
  ouo_printdbg("deref %zu -> %zu: ", rc->count, rc->count - 1);
  _ouo_obj_dump(obj);
#endif

  rc->count--;
  if (rc->count > 0) {
#ifdef OUO_DEBUG
    ouo_printdbg("\n");
#endif
    return true;
  }

  switch (obj->kind) {
    // Copy-on-write
    case OUO_OBJ_INT:
    case OUO_OBJ_FLOAT:
    case OUO_OBJ_BOOL: break;
    // Reference-counted
    case OUO_OBJ_STR: ouo_da_free(((OuoRcStr *)rc)->str); break;
    case OUO_OBJ_FN: _ouo_chunk_free(&((OuoRcFn *)rc)->chunk); break;
  }
  ouo_free(rc);
  obj->as.ref = NULL;

#ifdef OUO_DEBUG
  ouo_printdbg("    FREED!\n");
#endif
  return true;
}

static inline void _ouo_vm_stack_push(
    _OuoVm *vm, _OuoCallFrame *fr, OuoObject *obj) {
  if (vm->res->stack.count + 1 >= OUO_VM_STACK_SIZE) {
    _ouo_vm_err(vm, fr, OUO_ERR_RUNTIME,
        "Maximum stack size exceeded (max %d).", OUO_VM_STACK_SIZE);
    return;
  }

  if (_ouo_obj_is_rc(obj)) _ouo_obj_rc_ref(obj);
  vm->res->stack.items[vm->res->stack.count] = *obj;
  vm->res->stack.count++;
}

static inline OuoObject *_ouo_vm_stack_pop(_OuoVm *vm, _OuoCallFrame *fr) {
  if (vm->res->stack.count == 0) {
    _ouo_vm_err(vm, fr, OUO_ERR_RUNTIME, "Trying to pop empty stack.");
    return &vm->res->stack.items[OUO_VM_STACK_SIZE];
  }

  vm->res->stack.count--;
  OuoObject *obj = &vm->res->stack.items[vm->res->stack.count];
  if (_ouo_obj_is_rc(obj) && !_ouo_obj_rc_deref(obj))
    return &vm->res->stack.items[OUO_VM_STACK_SIZE];
  return obj;
}

static inline OuoObject *_ouo_vm_stack_peek(
    _OuoVm *vm, _OuoCallFrame *fr, size_t offset) {
  if (offset + 1 > vm->res->stack.count) {
    _ouo_vm_err(vm, fr, OUO_ERR_RUNTIME,
        "Trying peek beyond the stack (offset %td, stack size %zu).",
        offset + 1, vm->res->stack.count);
    return &vm->res->stack.items[0];
  }

  return &vm->res->stack.items[vm->res->stack.count - offset - 1];
}

#define _OUO_VM_BIN_TO(vm, fr, T, OP, TO) \
  do { \
    ouo_##T##_t b = _ouo_vm_stack_pop((vm), (fr))->as.v_##T; \
    ouo_##T##_t a = _ouo_vm_stack_pop((vm), (fr))->as.v_##T; \
    _ouo_vm_stack_push((vm), (fr), &_ouo_obj_new_##TO(a OP b)); \
  } while (0)

#define _OUO_VM_BINARY(vm, fr, T, OP) _OUO_VM_BIN_TO(vm, fr, T, OP, T)

#define _OUO_VM_UNARY(vm, fr, T, OP) \
  do { \
    ouo_##T##_t a = _ouo_vm_stack_pop((vm), (fr))->as.v_##T; \
    _ouo_vm_stack_push((vm), (fr), &_ouo_obj_new_##T(OP a)); \
  } while (0)

static void _ouo_vm_run(_OuoVm *vm) {
  _OuoCallFrame *fr = &vm->frames[vm->frame_count - 1];

#ifdef OUO_DEBUG
  if (fr->chunk->name.start != NULL)
    ouo_printdbg("%.*s:\n", OUO_STRSL_FMT(fr->chunk->name));
#endif

  for (; fr->ip < fr->chunk->bytecode.items + fr->chunk->bytecode.count;
      ++(fr->ip)) {
    if (vm->res->failed) return;

#ifdef OUO_DEBUG
    _ouo_chunk_op_dump(fr->chunk, fr->ip);
    ouo_printdbg("\n");
#endif

    switch ((OuoOpCode)(*fr->ip)) {
      // Objects
      case OUO_OP_POP: _ouo_vm_stack_pop(vm, fr); break;
      case OUO_OP_GET: {
        uint8_t idx = _ouo_chunk_read_byte(fr->ip);
        _ouo_vm_stack_push(vm, fr, &fr->stack[idx]);
        break;
      }
      case OUO_OP_SET: {
        uint8_t idx = _ouo_chunk_read_byte(fr->ip);
        fr->stack[idx] = *_ouo_vm_stack_pop(vm, fr);
        break;
      }
      case OUO_OP_GET_GLOBAL: {
        OuoObject global =
            fr->chunk->globals.items[_ouo_chunk_read_byte(fr->ip)];
        _ouo_vm_stack_push(vm, fr, &global);
        break;
      }
      case OUO_OP_LIT: {
        OuoObject lit = fr->chunk->literals.items[_ouo_chunk_read_byte(fr->ip)];
        _ouo_vm_stack_push(vm, fr, &lit);
        break;
      }

      // Arithmetic
      case OUO_OP_ADD_INT: _OUO_VM_BINARY(vm, fr, int, +); break;
      case OUO_OP_ADD_FLOAT: _OUO_VM_BINARY(vm, fr, float, +); break;
      case OUO_OP_ADD_STR: {
        OuoRcStr *b = (OuoRcStr *)_ouo_vm_stack_pop(vm, fr)->as.ref;
        OuoRcStr *a = (OuoRcStr *)_ouo_vm_stack_pop(vm, fr)->as.ref;
        OuoRcStr *rc = _ouo_rc_new_str();
        ouo_da_append_many(&rc->str, a->str.items, a->str.count);
        ouo_da_append_many(&rc->str, b->str.items, b->str.count);
        _ouo_vm_stack_push(vm, fr, &_ouo_obj_new_str(rc));
        break;
      }
      case OUO_OP_SUB_INT: _OUO_VM_BINARY(vm, fr, int, -); break;
      case OUO_OP_SUB_FLOAT: _OUO_VM_BINARY(vm, fr, float, -); break;
      case OUO_OP_MULT_INT: _OUO_VM_BINARY(vm, fr, int, *); break;
      case OUO_OP_MULT_FLOAT: _OUO_VM_BINARY(vm, fr, float, *); break;
      case OUO_OP_DIV_INT: _OUO_VM_BINARY(vm, fr, int, /); break;
      case OUO_OP_DIV_FLOAT: _OUO_VM_BINARY(vm, fr, float, /); break;

      case OUO_OP_NEG_INT: _OUO_VM_UNARY(vm, fr, int, -); break;
      case OUO_OP_NEG_FLOAT: _OUO_VM_UNARY(vm, fr, float, -); break;

      // Comparison
      case OUO_OP_EQ_INT: _OUO_VM_BIN_TO(vm, fr, int, ==, bool); break;
      case OUO_OP_EQ_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, ==, bool); break;
      case OUO_OP_EQ_BOOL: _OUO_VM_BIN_TO(vm, fr, bool, ==, bool); break;
      case OUO_OP_NEQ_INT: _OUO_VM_BIN_TO(vm, fr, int, !=, bool); break;
      case OUO_OP_NEQ_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, !=, bool); break;
      case OUO_OP_NEQ_BOOL: _OUO_VM_BIN_TO(vm, fr, bool, !=, bool); break;

      case OUO_OP_LT_INT: _OUO_VM_BIN_TO(vm, fr, int, <, bool); break;
      case OUO_OP_LT_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, <, bool); break;
      case OUO_OP_LT_EQ_INT: _OUO_VM_BIN_TO(vm, fr, int, <=, bool); break;
      case OUO_OP_LT_EQ_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, <=, bool); break;
      case OUO_OP_GT_INT: _OUO_VM_BIN_TO(vm, fr, int, >, bool); break;
      case OUO_OP_GT_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, >, bool); break;
      case OUO_OP_GT_EQ_INT: _OUO_VM_BIN_TO(vm, fr, int, >=, bool); break;
      case OUO_OP_GT_EQ_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, >=, bool); break;

      // Logic
      case OUO_OP_NOT: _OUO_VM_UNARY(vm, fr, bool, !); break;

      // Control flow
      case OUO_OP_JUMP: {
        uint16_t jump = _ouo_chunk_read_bytes2(fr->ip);
        fr->ip += jump;
        break;
      }
      case OUO_OP_JUMP_IF_FALSE: {
        uint16_t jump = _ouo_chunk_read_bytes2(fr->ip);
        if (!_ouo_vm_stack_peek(vm, fr, 0)->as.v_bool) fr->ip += jump;
        break;
      }
      case OUO_OP_LOOP: {
        uint16_t jump = _ouo_chunk_read_bytes2(fr->ip);
        fr->ip -= jump;
        break;
      }
      case OUO_OP_RETURN: _ouo_vm_stack_pop(vm, fr); return;

      // Input/output
      case OUO_OP_PRINT:
        _ouo_obj_print(_ouo_vm_stack_peek(vm, fr, 0));
        ouo_print("\n");
        break;
    }

#ifdef OUO_DEBUG
    if (vm->res->stack.count != 0) {
      OUO_DA_FOREACH(OuoObject, obj, &vm->res->stack) {
        ouo_printdbg("[");
        _ouo_obj_dump(obj);
        ouo_printdbg("] ");
      }
      ouo_printdbg("\n");
    }
#endif
  }

#ifdef OUO_DEBUG
  ouo_printdbg("\n");
#endif
}

void ouo_interpret(OuoChunk *chunk, OuoInterpretResult *res) {
  _OuoVm vm = {0};
  _ouo_vm_init(&vm, res, chunk);

  _ouo_vm_run(&vm);
}

void ouo_i_res_cleanup(OuoInterpretResult *res) {
  OUO_DA_FOREACH(OuoObject, obj, &res->stack) {
    if (_ouo_obj_is_rc(obj)) _ouo_obj_rc_deref(obj);
  }
}

#endif // OUO_NOEMIT

#endif // OUO_IMPLEMENTATION
