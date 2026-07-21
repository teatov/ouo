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
  do { \
    if (!(expr)) ouo_abort(err_code, __VA_ARGS__); \
  } while (0)

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
  // Type
  OUO_TYPE_TYPE,
} OuoTypeKind;

struct OuoTypeRef;

typedef struct {
  OuoTypeKind kind;
  struct OuoTypeRef *ref;
} OuoType;

typedef struct {
  OuoStringSlice name;
  OuoType type;
} OuoNameType;

typedef struct {
  OuoNameType *items;
  size_t count;
  size_t capacity;
} OuoNameTypes;

typedef struct OuoTypeRef {
  OuoTypeKind kind;

  union {
    struct {
      OuoNameTypes params;
      OuoType return_type;
    } t_fn;

    OuoType t_type;
  } as;
} OuoTypeRef;

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
  // Static analysis
  OUO_ERR_ANALYZE_FAIL,
  OUO_ERR_TYPE,
  OUO_ERR_SEMANTIC,
  // Compilation
  OUO_ERR_COMPILE_FAIL,
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

typedef struct {
  bool failed;
  OuoErrors errors;
} OuoStageResult;

void ouo_res_free(OuoStageResult *res);

//
// Lexing
//

typedef enum {
  OUO_TOK_ILLEGAL,
  OUO_TOK_EOF,
  OUO_TOK_NEWLINE,
  OUO_TOK_IDENT,
  OUO_TOK_BUILTIN,
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
  OUO_TOK_KW_TYPE,
  OUO_TOK_KW_RETURN,
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
  OUO_AST_BUILTIN,
  // Literals
  OUO_AST_LIT_INT,
  OUO_AST_LIT_FLOAT,
  OUO_AST_LIT_BOOL,
  OUO_AST_LIT_STR,
  OUO_AST_LIT_TYPE,
  // Expressions
  OUO_AST_ASSIGN,
  OUO_AST_BINOP,
  OUO_AST_UNOP,
  OUO_AST_BLOCK,
  OUO_AST_IF,
  OUO_AST_WHILE,
  OUO_AST_CALL,
  // Statements
  OUO_AST_EXPR_STMT,
  OUO_AST_PRINT,
  OUO_AST_RETURN,
  OUO_AST_DECL_VAR,
  OUO_AST_DECL_FN,
  OUO_AST_DECL_TYPE,
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

typedef struct {
  bool visited;
  OuoType type;
  struct OuoSymbol *sym;
} AstAnalysisResult;

/// Owns memory for any child AST nodes.
typedef struct OuoAst {
  OuoAstKind kind;
  OuoToken tok;

  AstAnalysisResult a_res;

  // Common
  struct {
    struct OuoAst **items;
    size_t count;
    size_t capacity;
  } children;

  union {
    struct {
      OuoStringSlice name;
    } ident;

    // Literals
    ouo_int_t lit_int;
    ouo_float_t lit_float;
    ouo_bool_t lit_bool;
    OuoString lit_str;

    struct {
      OuoType t;
      struct OuoAst *ident;
    } lit_type;

    // Expressions
    struct {
      struct OuoAst *target;
      struct OuoAst *value;
    } assign;

    struct {
      struct OuoAst *left;
      OuoTokenKind op;
      struct OuoAst *right;
    } binop;

    struct {
      OuoTokenKind op;
      struct OuoAst *right;
    } unop;

    struct {
      struct OuoAst *condition;
      struct OuoAst *then_branch;
      struct OuoAst *else_branch;
    } if_expr;

    struct {
      struct OuoAst *condition;
      struct OuoAst *body;
    } while_expr;

    struct {
      struct OuoAst *target;
    } call;

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
      OuoAstNameTypes params;
      struct OuoAst *return_type_annot;
      struct OuoAst *body;
    } decl_fn;

    struct {
      OuoToken name;
      struct OuoAst *type_annot;
    } decl_type;
  } as;
} OuoAst;

/// Caller owns the result and `res_ast`.
OuoStageResult ouo_parse(const char *src, OuoAst **res_ast);

void ouo_ast_free(OuoAst *ast);

//
// Static analysis
//

/// Caller owns the result.
OuoStageResult ouo_analyze(OuoAst *ast);

//
// Compilation
//

// typedef enum {
//   OUO_SYM_LOCAL,
//   OUO_SYM_GLOBAL,
//   OUO_SYM_BUILTIN,
// } OuoSymbolKind;

// typedef struct OuoSymbol {
//   OuoSymbolKind kind;
//   size_t idx;
//   OuoStringSlice name;
//   OuoType type;
//   size_t scope_depth;
//   OuoToken tok;
// } OuoSymbol;

// #ifndef OUO_NOEMIT

// typedef enum {
//   // Objects
//   OUO_OP_POP,         // [... v] -> [...]
//   OUO_OP_POP_N,       // u8 pop_count: [... vN ... v1] -> [...]
//   OUO_OP_GET,         // u8 slot_idx: [... vI ...] -> [... vI ... vI]
//   OUO_OP_SET,         // u8 slot_idx: [... vA ... vB] -> [... vB ...]
//   OUO_OP_GET_GLOBAL,  // u8 global_idx: [...] -> [... vI]
//   OUO_OP_GET_BUILTIN, // u8 builtin_idx: [...] -> [... vI]
//   OUO_OP_LIT,         // u8 lit_idx: [...] -> [... vI]
//   OUO_OP_PUSH_0,      // [...] -> [... 0]
//   OUO_OP_PUSH_1,      // [...] -> [... 1]
//   OUO_OP_PUSH_INT8,   // u8 int: [...] -> [... int]
//   OUO_OP_PUSH_TRUE,   // [...] -> [... true]
//   OUO_OP_PUSH_FALSE,  // [...] -> [... false]
//   // Arithmetic
//   OUO_OP_ADD_INT, // [... a b] -> [... a+b]
//   OUO_OP_ADD_FLOAT,
//   OUO_OP_ADD_STR,
//   OUO_OP_SUB_INT, // [... a b] -> [... a-b]
//   OUO_OP_SUB_FLOAT,
//   OUO_OP_MULT_INT, // [... a b] -> [... a*b]
//   OUO_OP_MULT_FLOAT,
//   OUO_OP_DIV_INT, // [... a b] -> [... a/b]
//   OUO_OP_DIV_FLOAT,
//   OUO_OP_NEG_INT, // [... a] -> [... -a]
//   OUO_OP_NEG_FLOAT,
//   // Comparison
//   OUO_OP_EQ_INT, // [... a b] -> [... a==b]
//   OUO_OP_EQ_FLOAT,
//   OUO_OP_EQ_BOOL,
//   OUO_OP_NEQ_INT, // [... a b] -> [... a!=b]
//   OUO_OP_NEQ_FLOAT,
//   OUO_OP_NEQ_BOOL,
//   OUO_OP_LT_INT, // [... a b] -> [... a<b]
//   OUO_OP_LT_FLOAT,
//   OUO_OP_LT_EQ_INT, // [... a b] -> [... a<=b]
//   OUO_OP_LT_EQ_FLOAT,
//   OUO_OP_GT_INT, // [... a b] -> [... a>b]
//   OUO_OP_GT_FLOAT,
//   OUO_OP_GT_EQ_INT, // [... a b] -> [... a>=b]
//   OUO_OP_GT_EQ_FLOAT,
//   // Logic
//   OUO_OP_NOT, // [... a] -> [... !a]
//   // Control flow
//   OUO_OP_JUMP,          // u16 offset
//   OUO_OP_JUMP_IF_FALSE, // u16 offset: [... v] -> [...]
//   OUO_OP_LOOP,          // u16 offset
//   OUO_OP_CALL,        // u8 argc: [... arg1 ... argN callee] -> [arg1 ...
//   argN] OUO_OP_RETURN,      // u8 pop_count: [... vN ... v1] -> [...]
//   OUO_OP_RETURN_VOID, // u8 pop_count: [... vN ... v1] -> [...]
//   // Input/output
//   OUO_OP_PRINT, // [... v]
// } OuoOpCode;

// struct OuoObject;

// /// Owns memory for `items`.
// typedef struct {
//   struct OuoObject *items;
//   size_t count;
//   size_t capacity;
// } OuoObjects;

// /// Owns memory for `literals`, `globals`, `bytecode` and `lines`.
// typedef struct OuoChunk {
//   OuoStringSlice name;

//   OuoObjects literals;
//   OuoObjects globals;
//   OuoObjects builtins;

//   struct {
//     uint8_t *items;
//     size_t count;
//     size_t capacity;
//   } bytecode;

//   // RLE encoded, length first
//   struct {
//     size_t *items;
//     size_t count;
//     size_t capacity;
//   } lines;
// } OuoChunk;

// #endif // OUO_NOEMIT

// /// Owns memory for `items`.
// typedef struct {
//   OuoSymbol *items;
//   size_t count;
//   size_t capacity;
// } OuoChunkSymbols;

// /// Owns memory for `chunk`, `local_syms`, `global_syms`, `types` and
// `errors`. typedef struct {
//   bool failed;
//   bool keep_module_scope;
//   bool echo;

// #ifndef OUO_NOEMIT
//   OuoChunk chunk;
// #endif

//   OuoChunkSymbols local_syms;
//   OuoChunkSymbols global_syms;
//   OuoChunkSymbols builtin_syms;

//   struct {
//     OuoTypeRef *items;
//     size_t count;
//     size_t capacity;
//   } type_refs;

//   OuoErrors errors;
// } OuoCompileResult;

// /// Caller owns the compile result's `chunk`, `local_syms`,
// /// `global_syms` and `errors`.
// void ouo_compile(OuoAst *ast, OuoCompileResult *res);

// /// Frees the compile result's `chunk` (except for `chunk.globals`)
// /// and `errors`.
// void ouo_c_res_free(OuoCompileResult *res);

// /// Frees the compile result's `chunk.globals`, `local_syms`,
// /// `global_syms` and `types`.
// void ouo_c_res_cleanup(OuoCompileResult *res);

// #ifndef OUO_NOEMIT

// //
// // Virtual machine
// //

// typedef enum {
//   // Copy-on-write
//   OUO_OBJ_INT,
//   OUO_OBJ_FLOAT,
//   OUO_OBJ_BOOL,
//   // Reference-counted
//   OUO_OBJ_STR,
//   OUO_OBJ_FN,
//   // Builtin
//   OUO_OBJ_BUILTIN_FN,
// } OuoObjectKind;

// typedef struct {
//   size_t count;
// } OuoRc;

// typedef bool (*OuoBuiltinFn)(struct OuoObject *arg1, struct OuoObject *arg2,
//     struct OuoObject *arg3, struct OuoObject *ret);

// typedef struct OuoObject {
//   OuoObjectKind kind;

//   union {
//     // Copy-on-write
//     ouo_int_t v_int;
//     ouo_float_t v_float;
//     ouo_bool_t v_bool;
//     // Reference-counted
//     OuoRc *ref;
//     // Builtin
//     OuoBuiltinFn bifn;
//   } as;
// } OuoObject;

// /// Owns memory for `str`.
// typedef struct {
//   OuoRc ref;
//   OuoString str;
// } OuoRcStr;

// /// Owns memory for `chunk`.
// typedef struct {
//   OuoRc ref;
//   OuoChunk chunk;
// } OuoRcFn;

// #define OUO_FRAMES_SIZE 64
// #define OUO_VM_STACK_SIZE (OUO_FRAMES_SIZE * UINT8_MAX)

// /// Owns memory for any reference-counted objects on the `stack`.
// typedef struct {
//   bool failed;

//   struct {
//     OuoObject items[OUO_VM_STACK_SIZE];
//     OuoObject *top;
//   } stack;

//   OuoError error;
// } OuoInterpretResult;

// /// Caller owns the interpret result's `errors`.
// void ouo_interpret(OuoChunk *chunk, OuoInterpretResult *res);

// /// Frees the interpret result's `stack`.
// void ouo_i_res_cleanup(OuoInterpretResult *res);

// #endif // OUO_NOEMIT

#endif // OUO_H

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#ifdef OUO_IMPLEMENTATION
//
// Data types
//

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
      if (!_ouo_type_is(
              &a->ref->as.t_fn.return_type, &b->ref->as.t_fn.return_type))
        return false;
      if (a->ref->as.t_fn.params.count != b->ref->as.t_fn.params.count)
        return false;
      for (size_t i = 0; i < a->ref->as.t_fn.params.count; i++)
        if (!_ouo_type_is(&a->ref->as.t_fn.params.items[i].type,
                &b->ref->as.t_fn.params.items[i].type))
          return false;
      break;
    case OUO_TYPE_TYPE: return false;
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
    // Type
    case OUO_TYPE_TYPE: return "type";
  }
  return "";
}

static void _ouo_type_str_append(
    OuoString *str, const char *to_append, size_t to_append_len) {
  ouo_da_append_many(str, to_append, to_append_len);
}

static OuoString _ouo_type_str_rec(OuoType *type, size_t depth) {
  depth--;
  OuoString str = {0};
  if (depth == 0) {
    _ouo_type_str_append(&str, "...", 3);
    return str;
  }
  const char *kind_str = _ouo_type_kind_str(type->kind);
  size_t kind_strlen = strlen(kind_str);
  _ouo_type_str_append(&str, kind_str, kind_strlen);

  if (type->kind == OUO_TYPE_FN) {
    _ouo_type_str_append(&str, "(", 1);
    for (size_t i = 0; i < type->ref->as.t_fn.params.count; i++) {
      OuoNameType *param = &type->ref->as.t_fn.params.items[i];
      _ouo_type_str_append(&str, param->name.start, param->name.len);
      _ouo_type_str_append(&str, ": ", 2);

      OuoString param_str = _ouo_type_str_rec(&param->type, depth);
      _ouo_type_str_append(&str, param_str.items, param_str.count);
      ouo_da_free(param_str);

      if (i < type->ref->as.t_fn.params.count - 1)
        _ouo_type_str_append(&str, ", ", 2);
    }
    _ouo_type_str_append(&str, "): ", 3);

    OuoString return_type_str =
        _ouo_type_str_rec(&type->ref->as.t_fn.return_type, depth);
    _ouo_type_str_append(&str, return_type_str.items, return_type_str.count);
    ouo_da_free(return_type_str);
  } else if (type->kind == OUO_TYPE_TYPE) {
    _ouo_type_str_append(&str, "[", 1);
    OuoString type_str = _ouo_type_str_rec(&type->ref->as.t_type, depth);
    _ouo_type_str_append(&str, type_str.items, type_str.count);
    ouo_da_free(type_str);
    _ouo_type_str_append(&str, "]", 3);
  }

  return str;
}

static OuoString _ouo_type_str(OuoType *type) {
  return _ouo_type_str_rec(type, 10);
}

// static void _ouo_type_ref_free(OuoTypeRef *type_ref) {
//   if (type_ref->kind == OUO_TYPE_FN) ouo_da_free(type_ref->as.t_fn.params);
// }

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
    // Static analysis
    case OUO_ERR_ANALYZE_FAIL: return "ANALYSIS FAIL";
    case OUO_ERR_TYPE: return "TYPE ERROR";
    case OUO_ERR_SEMANTIC: return "SEMANTIC ERROR";
    // Compilation
    case OUO_ERR_COMPILE_FAIL: return "COMPILATION FAIL";
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
  if (err->pos.line != 0) {
    ouo_printerr("%zu:", err->pos.line);
    if (err->pos.col != 0) ouo_printerr("%zu:", err->pos.col);
  }
  if (path != NULL || err->pos.line != 0) ouo_printerr(" ");
  if (err->fn_name.start != NULL && err->fn_name.len > 0)
    ouo_printerr("%.*s: ", OUO_STRSL_FMT(err->fn_name));

  ouo_printerr(OUO_ER "%s%s: " OUO_EBR "%s" OUO_ER,
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

void ouo_res_free(OuoStageResult *res) { ouo_da_free(res->errors); }

//
// Lexing
//

typedef struct {
  const char *tok_start;
  const char *curr;
  OuoCodePosition pos;
} _OuoLexer;

static inline void _ouo_l_init(_OuoLexer *l, size_t lines, const char *src) {
  l->tok_start = src;
  l->curr = src;

  l->pos.line = lines + 1;
  l->pos.col = 1;
  l->pos.line_start = src;
}

static inline bool _ouo_l_is_eof(_OuoLexer *l) { return *l->curr == '\0'; }

static inline bool _ouo_l_isdigit(char c) { return c >= '0' && c <= '9'; }

static inline bool _ouo_l_ishex(char c) {
  return _ouo_l_isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int _ouo_l_hexval(char c) {
  return (c <= '9') ? (c - '0') : (c <= 'F') ? (c - 'A' + 10) : (c - 'a' + 10);
}

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

static OuoToken _ouo_l_read_word(_OuoLexer *l, OuoTokenKind kind) {
  while (_ouo_l_isalpha(_ouo_l_peek(l))) _ouo_l_advance(l);
  if (kind != OUO_TOK_IDENT) return _ouo_l_tok_new(l, kind);

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
    case 't':
      if (is_long) switch (l->tok_start[1]) {
          case 'y': return _ouo_l_check_kw(l, 2, 2, "pe", OUO_TOK_KW_TYPE);
          case 'r': return _ouo_l_check_kw(l, 2, 2, "ue", OUO_TOK_LIT_TRUE);
        }
      break;
    case 'r': return _ouo_l_check_kw(l, 1, 5, "eturn", OUO_TOK_KW_RETURN);
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
  while (_ouo_l_peek(l) != '"' && _ouo_l_peek(l) != '\n' && !_ouo_l_is_eof(l)) {
    if (_ouo_l_peek(l) == '\\') _ouo_l_advance(l);
    _ouo_l_advance(l);
  }
  if (_ouo_l_peek(l) != '\n' && !_ouo_l_is_eof(l)) _ouo_l_advance(l);
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

  if (_ouo_l_isalpha(c)) return _ouo_l_read_word(l, OUO_TOK_IDENT);

  // Literals
  if (_ouo_l_isdigit(c)) return _ouo_l_read_number(l);

  switch (c) {
    case '@':
      if (_ouo_l_isalpha(_ouo_l_peek(l)))
        return _ouo_l_read_word(l, OUO_TOK_BUILTIN);
      break;
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
    case OUO_TOK_BUILTIN: return "BUILTIN";
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
    case OUO_TOK_KW_TYPE: return "type";
    case OUO_TOK_KW_RETURN: return "return";
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
  OuoStageResult *res;

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
  _OUO_PREC_COMPARISON,
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

static void _ouo_p_err_append(_OuoParser *p, OuoError *err) {
  ouo_da_append(&p->res->errors, *err);
}

#define _ouo_p_err_add(p, tok, err_code, ...) \
  do { \
    OuoError err = { \
        .code = (err_code), \
        .len = (tok).str.len == 0 ? 1 : (tok).str.len, \
        .pos = (tok).pos, \
        .msg = {0}, \
    }; \
    _ouo_err_sprintf(err, __VA_ARGS__); \
    _ouo_p_err_append(p, &err); \
  } while (0)

#define _ouo_p_err(p, tok, err_code, ...) \
  do { \
    if (!(p)->panic_mode) { \
      (p)->res->failed = true; \
      (p)->panic_mode = true; \
      _ouo_p_err_add(p, tok, err_code, __VA_ARGS__); \
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

static inline void _ouo_p_init(_OuoParser *p, _OuoLexer *l, OuoStageResult *res,
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

  ast->a_res.visited = false;
  ast->a_res.type.kind = OUO_TYPE_UNKNOWN;
  ast->a_res.sym = NULL;

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
  ast->as.ident.name = p->curr.str;
  return ast;
}

static OuoAst *_ouo_p_builtin(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_BUILTIN);
  ast->as.ident.name.start = p->curr.str.start + 1;
  ast->as.ident.name.len = p->curr.str.len - 1;
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
  p->panic_mode = false;
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_LIT_STR);
  ast->as.lit_str.items = NULL;
  ast->as.lit_str.count = 0;
  ast->as.lit_str.capacity = 0;

  const char *end = p->curr.str.start + p->curr.str.len - 1;
  for (const char *c = p->curr.str.start + 1; c < end; c++) {
    if (p->panic_mode) break;
    if (*c != '\\') {
      ouo_da_append(&ast->as.lit_str, *c);
      continue;
    }
    c++;
    if (c >= end) {
      _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX, "Unterminated string.");
      break;
    }
    char esc = '\\';
    switch (*c) {
      case 'n': esc = '\n'; break;
      case 'r': esc = '\r'; break;
      case 't': esc = '\t'; break;
      case '\\': esc = '\\'; break;
      case '\'': esc = '\''; break;
      case '"': esc = '\"'; break;
      case 'x': {
        if (c + 2 >= end) {
          _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX, "Incomplete hex escape code.");
          break;
        }
        char h1 = *(c + 1);
        char h2 = *(c + 2);

        if (!_ouo_l_ishex(h1) || !_ouo_l_ishex(h2)) {
          _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX,
              "Invalid hex escape '\\x%c%c'.", h1, h2);
          break;
        }

        int v1 = _ouo_l_hexval(h1);
        int v2 = _ouo_l_hexval(h2);
        int v = (v1 << 4) | v2;
        esc = (char)v;
        c += 2;
        break;
      }
      default:
        esc = *c;
        _ouo_p_err(
            p, p->curr, OUO_ERR_SYNTAX, "Unknown escape sequence '\\%c'.", *c);
        break;
    }
    ouo_da_append(&ast->as.lit_str, esc);
  }

  if (*(p->curr.str.start + p->curr.str.len - 1) != '"')
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX, "Unterminated string.");
  return ast;
}

static inline OuoAst *_ouo_p_lit_type(_OuoParser *p) {
  OuoTypeKind type_kind = OUO_TYPE_UNKNOWN;
  switch (p->curr.kind) {
    case OUO_TOK_IDENT: break;
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
  ast->as.lit_type.t.kind = type_kind;
  ast->as.lit_type.ident =
      p->curr.kind == OUO_TOK_IDENT ? _ouo_p_ident(p) : NULL;
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

static OuoAst *_ouo_p_binop(_OuoParser *p, OuoAst *left) {
  OuoToken op = p->curr;
  _OuoPrecedence prec = _ouo_p_get_rule(p, op.kind)->prec;
  _ouo_p_advance(p);
  OuoAst *right = _ouo_p_expr(p, prec);

  OuoAst *ast = _ouo_ast_new(&op, OUO_AST_BINOP);
  ast->as.binop.left = left;
  ast->as.binop.op = op.kind;
  ast->as.binop.right = right;
  return ast;
}

static OuoAst *_ouo_p_unary(_OuoParser *p) {
  OuoToken op = p->curr;
  _ouo_p_advance(p);
  OuoAst *right = _ouo_p_expr(p, _OUO_PREC_UNARY);

  OuoAst *ast = _ouo_ast_new(&op, OUO_AST_UNOP);
  ast->as.unop.op = op.kind;
  ast->as.unop.right = right;
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
      _ouo_p_err_add(p, tok, OUO_ERR_NOTE, "Grouping starts here.");
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
      _ouo_p_err_add(p, ast->tok, OUO_ERR_NOTE, "Block starts here.");
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

    OuoAstNameType nt = {.name = ident, .type_annot = type};
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
  ast->as.decl_fn.params.items = NULL;
  ast->as.decl_fn.params.count = 0;
  ast->as.decl_fn.params.capacity = 0;

  ast->as.decl_fn.return_type_annot = NULL;
  ast->as.decl_fn.body = NULL;

  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_IDENT) {
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX,
        "Expected an identifier, got '%.*s'.", OUO_TOK_FMT(p->curr));
    return ast;
  }

  ast->as.decl_fn.name = p->curr;
  _ouo_p_advance(p);

  if (p->curr.kind != OUO_TOK_PAREN_OPN) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_PAREN_OPN);
    return ast;
  }
  bool ignore_newline_prev = p->ignore_newline;
  p->ignore_newline = true;

  _ouo_p_name_types(p, &ast->as.decl_fn.params, OUO_TOK_PAREN_CLS);

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
  ast->as.decl_fn.body = _ouo_p_expr(p, _OUO_PREC_LOWEST);
  return ast;
}

static OuoAst *_ouo_p_decl_type(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_DECL_TYPE);
  ast->as.decl_type.type_annot = NULL;

  _ouo_p_advance(p);
  if (p->curr.kind != OUO_TOK_IDENT) {
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX,
        "Expected an identifier, got '%.*s'.", OUO_TOK_FMT(p->curr));
    return ast;
  }

  ast->as.decl_type.name = p->curr;
  _ouo_p_advance(p);

  if (p->curr.kind != OUO_TOK_COLON) {
    _ouo_p_err_unexpected(p, p->curr, OUO_TOK_COLON);
    return ast;
  }
  _ouo_p_advance(p);

  ast->as.decl_type.type_annot = _ouo_p_lit_type(p);
  if (ast->as.decl_type.type_annot == NULL) {
    _ouo_p_err(p, p->curr, OUO_ERR_SYNTAX, "Expected a type, got '%.*s'.",
        OUO_TOK_FMT(p->curr));
    return ast;
  }

  return ast;
}

static OuoAst *_ouo_p_return(_OuoParser *p) {
  OuoAst *ast = _ouo_ast_new(&p->curr, OUO_AST_RETURN);
  if (p->peek.kind != OUO_TOK_EOF && p->peek.kind != OUO_TOK_NEWLINE &&
      p->peek.kind != OUO_TOK_BRACE_CLS) {
    _ouo_p_advance(p);
    ast->as.expr_stmt.expr = _ouo_p_expr(p, _OUO_PREC_LOWEST);
  } else ast->as.expr_stmt.expr = NULL;
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
    case OUO_TOK_KW_TYPE: ast = _ouo_p_decl_type(p); break;
    case OUO_TOK_KW_RETURN: ast = _ouo_p_return(p); break;
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
    [OUO_TOK_BUILTIN] = {_ouo_p_builtin, NULL, _OUO_PREC_LOWEST},
    // Keywords
    [OUO_TOK_KW_OR] = {NULL, _ouo_p_binop, _OUO_PREC_OR},
    [OUO_TOK_KW_AND] = {NULL, _ouo_p_binop, _OUO_PREC_AND},
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
    [OUO_TOK_PLUS] = {NULL, _ouo_p_binop, _OUO_PREC_SUM},
    [OUO_TOK_MINUS] = {_ouo_p_unary, _ouo_p_binop, _OUO_PREC_SUM},
    [OUO_TOK_ASTERISK] = {NULL, _ouo_p_binop, _OUO_PREC_PRODUCT},
    [OUO_TOK_SLASH] = {NULL, _ouo_p_binop, _OUO_PREC_PRODUCT},
    [OUO_TOK_EQ] = {NULL, _ouo_p_binop, _OUO_PREC_COMPARISON},
    [OUO_TOK_NEQ] = {NULL, _ouo_p_binop, _OUO_PREC_COMPARISON},
    [OUO_TOK_LT] = {NULL, _ouo_p_binop, _OUO_PREC_COMPARISON},
    [OUO_TOK_LT_EQ] = {NULL, _ouo_p_binop, _OUO_PREC_COMPARISON},
    [OUO_TOK_GT] = {NULL, _ouo_p_binop, _OUO_PREC_COMPARISON},
    [OUO_TOK_GT_EQ] = {NULL, _ouo_p_binop, _OUO_PREC_COMPARISON},
    [OUO_TOK_BANG] = {_ouo_p_unary, NULL, _OUO_PREC_LOWEST},
    // Punctuation
    [OUO_TOK_PAREN_OPN] = {_ouo_p_grouping, _ouo_p_call, _OUO_PREC_ACCESS},
    [OUO_TOK_BRACE_OPN] = {_ouo_p_block, NULL, _OUO_PREC_LOWEST},
};

OuoStageResult ouo_parse(const char *src, OuoAst **res_ast) {
  _OuoLexer l = {0};
  _ouo_l_init(&l, 0, src);
  // _ouo_l_init(&l, res, src);

#ifdef OUO_DEBUG
  for (;;) {
    OuoToken tok = _ouo_l_next_token(&l);
    _ouo_tok_dump(&tok);
    if (tok.kind == OUO_TOK_EOF) break;
  }
  ouo_printdbg("\n");
  _ouo_l_init(&l, 0, src);
  // _ouo_l_init(&l, res, src);
#endif

  OuoStageResult res = {0};
  _OuoParser p = {0};
  _ouo_p_init(&p, &l, &res, _ouo_p_rules, ouo_arr_len(_ouo_p_rules));

  *res_ast = _ouo_p_module(&p);
  // res->line = l.pos.line;

#ifdef OUO_DEBUG
  _ouo_ast_dump(*res_ast);
  ouo_printdbg("\n");
#endif

  return res;
}

void ouo_ast_free(OuoAst *ast) {
  if (ast == NULL) return;

  switch (ast->kind) {
    case OUO_AST_MODULE:
    case OUO_AST_BLOCK:
      OUO_DA_FOREACH(OuoAst *, child, &ast->children) { ouo_ast_free(*child); }
      ouo_da_free(ast->children);
      break;
    case OUO_AST_IDENT:
    case OUO_AST_BUILTIN: break;
    // Literals
    case OUO_AST_LIT_INT:
    case OUO_AST_LIT_FLOAT:
    case OUO_AST_LIT_BOOL: break;
    case OUO_AST_LIT_STR: ouo_da_free(ast->as.lit_str); break;
    case OUO_AST_LIT_TYPE:
      if (ast->as.lit_type.ident != NULL) ouo_ast_free(ast->as.lit_type.ident);
      break;
    // Expressions
    case OUO_AST_ASSIGN:
      ouo_ast_free(ast->as.assign.target);
      ouo_ast_free(ast->as.assign.value);
      break;
    case OUO_AST_BINOP:
      ouo_ast_free(ast->as.binop.left);
      ouo_ast_free(ast->as.binop.right);
      break;
    case OUO_AST_UNOP: ouo_ast_free(ast->as.unop.right); break;
    case OUO_AST_IF:
      ouo_ast_free(ast->as.if_expr.condition);
      ouo_ast_free(ast->as.if_expr.then_branch);
      ouo_ast_free(ast->as.if_expr.else_branch);
      break;
    case OUO_AST_WHILE:
      ouo_ast_free(ast->as.while_expr.condition);
      ouo_ast_free(ast->as.while_expr.body);
      break;
    case OUO_AST_CALL:
      ouo_ast_free(ast->as.call.target);
      OUO_DA_FOREACH(OuoAst *, child, &ast->children) { ouo_ast_free(*child); }
      ouo_da_free(ast->children);
      break;
    // Statements
    case OUO_AST_EXPR_STMT:
    case OUO_AST_PRINT:
    case OUO_AST_RETURN: ouo_ast_free(ast->as.expr_stmt.expr); break;
    case OUO_AST_DECL_VAR:
      if (ast->as.decl_var.type_annot != NULL)
        ouo_ast_free(ast->as.decl_var.type_annot);
      ouo_ast_free(ast->as.decl_var.value);
      break;
    case OUO_AST_DECL_FN:
      OUO_DA_FOREACH(OuoAstNameType, param, &ast->as.decl_fn.params) {
        ouo_ast_free(param->type_annot);
      }
      ouo_da_free(ast->as.decl_fn.params);
      if (ast->as.decl_fn.return_type_annot != NULL)
        ouo_ast_free(ast->as.decl_fn.return_type_annot);
      ouo_ast_free(ast->as.decl_fn.body);
      break;
    case OUO_AST_DECL_TYPE: ouo_ast_free(ast->as.decl_type.type_annot); break;
  }

  ouo_free(ast);
}

#ifdef OUO_DEBUG

static const char *_ouo_ast_kind_str(OuoAstKind kind) {
  switch (kind) {
    case OUO_AST_MODULE: return "MODULE";
    case OUO_AST_IDENT: return "IDENT";
    case OUO_AST_BUILTIN: return "BUILTIN";
    // Literals
    case OUO_AST_LIT_INT: return "LIT_INT";
    case OUO_AST_LIT_FLOAT: return "LIT_FLOAT";
    case OUO_AST_LIT_BOOL: return "LIT_BOOL";
    case OUO_AST_LIT_STR: return "LIT_STR";
    case OUO_AST_LIT_TYPE: return "LIT_TYPE";
    // Expressions
    case OUO_AST_ASSIGN: return "ASSIGN";
    case OUO_AST_BINOP: return "BINOP";
    case OUO_AST_UNOP: return "UNOP";
    case OUO_AST_BLOCK: return "BLOCK";
    case OUO_AST_IF: return "IF";
    case OUO_AST_WHILE: return "WHILE";
    case OUO_AST_CALL: return "CALL";
    // Statements
    case OUO_AST_EXPR_STMT: return "EXPR_STMT";
    case OUO_AST_PRINT: return "PRINT";
    case OUO_AST_RETURN: return "RETURN";
    case OUO_AST_DECL_VAR: return "DECL_VAR";
    case OUO_AST_DECL_FN: return "DECL_FN";
    case OUO_AST_DECL_TYPE: return "DECL_TYPE";
  }
  return "";
}

// static void _ouo_ast_eval_ctx(OuoAstEvalContext *ctx) {
//   if (ctx->exp != NULL)
//     ouo_printdbg("exp '%s' ", _ouo_type_kind_str(ctx->exp->kind));
//   if (ctx->got != NULL)
//     ouo_printdbg("got '%s' ", _ouo_type_kind_str(ctx->got->kind));
// }

static void _ouo_ast_dump(OuoAst *ast) {
  if (ast == NULL) {
    ouo_printdbg("(NULL)");
    return;
  }

  ouo_printdbg("(%s ", _ouo_ast_kind_str(ast->kind));
  // if (ast->returns.exp != NULL || ast->returns.got != NULL) {
  //   ouo_printdbg("ret ");
  //   _ouo_ast_eval_ctx(&ast->returns);
  // }

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
      ouo_printdbg("%.*s", OUO_STRSL_FMT(ast->as.ident.name));
      break;
    case OUO_AST_BUILTIN:
      ouo_printdbg("%.*s", OUO_STRSL_FMT(ast->as.ident.name));
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
      OuoString type_str = _ouo_type_str(&ast->as.lit_type.t);
      ouo_printdbg("%.*s", OUO_STR_FMT(type_str));
      ouo_da_free(type_str);
      if (ast->as.lit_type.ident != NULL) _ouo_ast_dump(ast->as.lit_type.ident);
      break;
    }
    // Expressions
    case OUO_AST_ASSIGN:
      _ouo_ast_dump(ast->as.assign.target);
      ouo_printdbg(" %s ", _ouo_tok_kind_str(OUO_TOK_ASSIGN));
      _ouo_ast_dump(ast->as.assign.value);
      break;
    case OUO_AST_BINOP:
      _ouo_ast_dump(ast->as.binop.left);
      ouo_printdbg(" %s ", _ouo_tok_kind_str(ast->as.binop.op));
      _ouo_ast_dump(ast->as.binop.right);
      break;
    case OUO_AST_UNOP:
      ouo_printdbg("%s ", _ouo_tok_kind_str(ast->as.unop.op));
      _ouo_ast_dump(ast->as.unop.right);
      break;
    case OUO_AST_IF:
      _ouo_ast_dump(ast->as.if_expr.condition);
      ouo_printdbg(" then ");
      _ouo_ast_dump(ast->as.if_expr.then_branch);
      ouo_printdbg(" else ");
      _ouo_ast_dump(ast->as.if_expr.else_branch);
      break;
    case OUO_AST_WHILE:
      _ouo_ast_dump(ast->as.while_expr.condition);
      _ouo_ast_dump(ast->as.while_expr.body);
      break;
    case OUO_AST_CALL:
      _ouo_ast_dump(ast->as.call.target);
      ouo_printdbg(" (");
      OUO_DA_FOREACH(OuoAst *, expr, &ast->children) { _ouo_ast_dump(*expr); }
      ouo_printdbg(")");
      break;
    // Statements
    case OUO_AST_EXPR_STMT:
      ouo_printdbg("%d ", ast->as.expr_stmt.pop);
      _ouo_ast_dump(ast->as.expr_stmt.expr);
      break;
    case OUO_AST_PRINT:
    case OUO_AST_RETURN: _ouo_ast_dump(ast->as.expr_stmt.expr); break;
    case OUO_AST_DECL_VAR:
      ouo_printdbg("%.*s ", OUO_TOK_FMT(ast->as.decl_var.name));
      _ouo_ast_dump(ast->as.decl_var.type_annot);
      ouo_printdbg(" ");
      _ouo_ast_dump(ast->as.decl_var.value);
      break;
    case OUO_AST_DECL_FN:
      ouo_printdbg("%.*s (", OUO_TOK_FMT(ast->as.decl_fn.name));
      OUO_DA_FOREACH(OuoAstNameType, param, &ast->as.decl_fn.params) {
        ouo_printdbg("(%.*s ", OUO_TOK_FMT(param->name));
        _ouo_ast_dump(param->type_annot);
        ouo_printdbg(")");
      }
      ouo_printdbg(") ");
      _ouo_ast_dump(ast->as.decl_fn.return_type_annot);
      ouo_printdbg(" ");
      _ouo_ast_dump(ast->as.decl_fn.body);
      break;
    case OUO_AST_DECL_TYPE:
      ouo_printdbg("%.*s ", OUO_TOK_FMT(ast->as.decl_type.name));
      _ouo_ast_dump(ast->as.decl_type.type_annot);
      break;
  }

  ouo_printdbg(")");
}

#endif // OUO_DEBUG

//
// Static analysis
//

typedef struct {
  bool panic_mode;
  OuoStageResult *res;
} _OuoAnalyzer;

static void _ouo_a_err_append(_OuoAnalyzer *c, OuoError *err) {
  ouo_da_append(&c->res->errors, *err);
}

#define _ouo_a_err_add(a, tok, err_code, ...) \
  do { \
    OuoError err = { \
        .code = (err_code), \
        .len = (tok).str.len, \
        .pos = (tok).pos, \
        .msg = {0}, \
    }; \
    _ouo_err_sprintf(err, __VA_ARGS__); \
    _ouo_a_err_append(a, &err); \
  } while (0)

#define _ouo_a_err(a, tok, err_code, ...) \
  do { \
    if (!(a)->panic_mode) { \
      (a)->res->failed = true; \
      (a)->panic_mode = true; \
      _ouo_a_err_add(a, tok, err_code, __VA_ARGS__); \
    } \
  } while (0)

static inline void _ouo_a_init(_OuoAnalyzer *a, OuoStageResult *res) {
  a->res = res;
}

static void _ouo_a_err_todo(_OuoAnalyzer *a, OuoAst *ast) {
  _ouo_a_err(a, ast->tok, OUO_ERR_ANALYZE_FAIL, "TODO %s",
      _ouo_ast_kind_str(ast->kind));
}

static void _ouo_a_err_binop_type(_OuoAnalyzer *a, OuoAst *ast) {
  OuoString left_type_str = _ouo_type_str(&ast->as.binop.left->a_res.type);
  OuoString right_type_str = _ouo_type_str(&ast->as.binop.right->a_res.type);

  _ouo_a_err(a, ast->tok, OUO_ERR_TYPE,
      "Operation '%s' does not support '%.*s' and '%.*s'.",
      _ouo_tok_kind_str(ast->as.binop.op), OUO_STR_FMT(left_type_str),
      OUO_STR_FMT(right_type_str));

  ouo_da_free(left_type_str);
  ouo_da_free(right_type_str);
}

static void _ouo_a_err_unary_type(_OuoAnalyzer *a, OuoAst *ast) {
  OuoString type_str = _ouo_type_str(&ast->as.unop.right->a_res.type);

  _ouo_a_err(a, ast->tok, OUO_ERR_TYPE,
      "Operation '%s' does not support '%.*s'.",
      _ouo_tok_kind_str(ast->as.unop.op), OUO_STR_FMT(type_str));

  ouo_da_free(type_str);
}

static inline bool _ouo_ast_binop_is(OuoAst *ast, OuoTypeKind type_kind) {
  return ast->as.binop.left->a_res.type.kind == type_kind &&
      ast->as.binop.right->a_res.type.kind == type_kind;
}

static inline bool _ouo_ast_unop_is(OuoAst *ast, OuoTypeKind type_kind) {
  return ast->as.unop.right->a_res.type.kind == type_kind;
}

static inline void _ouo_a_ast_visit(_OuoAnalyzer *a, OuoAst *ast) {
  ast->a_res.visited = true;

  switch (ast->kind) {
    case OUO_AST_MODULE:
    case OUO_AST_BLOCK:
      ast->a_res.type.kind = OUO_TYPE_VOID;

      OUO_DA_FOREACH(OuoAst *, stmt_p, &ast->children) {
        OuoAst *stmt = *stmt_p;
        _ouo_a_ast_visit(a, stmt);
      }

      break;
    case OUO_AST_IDENT: {
      _ouo_a_err_todo(a, ast);
      break;
    }
    case OUO_AST_BUILTIN: {
      _ouo_a_err_todo(a, ast);
      break;
    }

    // Literals
    case OUO_AST_LIT_INT: ast->a_res.type.kind = OUO_TYPE_INT; break;
    case OUO_AST_LIT_FLOAT: ast->a_res.type.kind = OUO_TYPE_FLOAT; break;
    case OUO_AST_LIT_BOOL: ast->a_res.type.kind = OUO_TYPE_BOOL; break;
    case OUO_AST_LIT_STR: ast->a_res.type.kind = OUO_TYPE_STR; break;
    case OUO_AST_LIT_TYPE: {
      if (ast->as.lit_type.ident != NULL)
        _ouo_a_ast_visit(a, ast->as.lit_type.ident);

      _ouo_a_err_todo(a, ast);
      break;
    }

    // Expressions
    case OUO_AST_ASSIGN: {
      _ouo_a_ast_visit(a, ast->as.assign.target);
      _ouo_a_ast_visit(a, ast->as.assign.value);

      _ouo_a_err_todo(a, ast);
      break;
    }
    case OUO_AST_BINOP:
      _ouo_a_ast_visit(a, ast->as.binop.left);
      _ouo_a_ast_visit(a, ast->as.binop.right);

      switch (ast->as.binop.op) {
        // Arithmetic
        case OUO_TOK_PLUS:
          if (_ouo_ast_binop_is(ast, OUO_TYPE_INT))
            ast->a_res.type.kind = OUO_TYPE_INT;
          else if (_ouo_ast_binop_is(ast, OUO_TYPE_FLOAT))
            ast->a_res.type.kind = OUO_TYPE_FLOAT;
          else if (_ouo_ast_binop_is(ast, OUO_TYPE_STR))
            ast->a_res.type.kind = OUO_TYPE_STR;
          else _ouo_a_err_binop_type(a, ast);
          break;
        case OUO_TOK_MINUS:
        case OUO_TOK_ASTERISK:
        case OUO_TOK_SLASH:
          if (_ouo_ast_binop_is(ast, OUO_TYPE_INT))
            ast->a_res.type.kind = OUO_TYPE_INT;
          else if (_ouo_ast_binop_is(ast, OUO_TYPE_FLOAT))
            ast->a_res.type.kind = OUO_TYPE_FLOAT;
          else _ouo_a_err_binop_type(a, ast);
          break;

        // Comparison
        case OUO_TOK_EQ:
        case OUO_TOK_NEQ:
          if (_ouo_ast_binop_is(ast, OUO_TYPE_INT) ||
              _ouo_ast_binop_is(ast, OUO_TYPE_FLOAT) ||
              _ouo_ast_binop_is(ast, OUO_TYPE_BOOL))
            ast->a_res.type.kind = OUO_TYPE_BOOL;
          else _ouo_a_err_binop_type(a, ast);
          break;
        case OUO_TOK_LT:
        case OUO_TOK_LT_EQ:
        case OUO_TOK_GT:
        case OUO_TOK_GT_EQ:
          if (_ouo_ast_binop_is(ast, OUO_TYPE_INT) ||
              _ouo_ast_binop_is(ast, OUO_TYPE_FLOAT))
            ast->a_res.type.kind = OUO_TYPE_BOOL;
          else _ouo_a_err_binop_type(a, ast);
          break;

        // Logic
        case OUO_TOK_KW_OR:
        case OUO_TOK_KW_AND:
          if (_ouo_ast_binop_is(ast, OUO_TYPE_BOOL))
            ast->a_res.type.kind = OUO_TYPE_BOOL;
          else _ouo_a_err_binop_type(a, ast);
          break;

        default:
          _ouo_a_err(a, ast->tok, OUO_ERR_ANALYZE_FAIL,
              "Unknown binary operator '%s'.",
              _ouo_tok_kind_str(ast->as.binop.op));
          break;
      }
      break;
    case OUO_AST_UNOP:
      _ouo_a_ast_visit(a, ast->as.unop.right);

      switch (ast->as.unop.op) {
        // Arithmetic
        case OUO_TOK_MINUS:
          if (_ouo_ast_unop_is(ast, OUO_TYPE_INT))
            ast->a_res.type.kind = OUO_TYPE_INT;
          else if (_ouo_ast_unop_is(ast, OUO_TYPE_FLOAT))
            ast->a_res.type.kind = OUO_TYPE_FLOAT;
          else _ouo_a_err_unary_type(a, ast);
          break;

        // Logic
        case OUO_TOK_BANG:
          if (_ouo_ast_unop_is(ast, OUO_TYPE_BOOL))
            ast->a_res.type.kind = OUO_TYPE_BOOL;
          else _ouo_a_err_unary_type(a, ast);
          break;

        default:
          _ouo_a_err(a, ast->tok, OUO_ERR_COMPILE_FAIL,
              "Unknown unary operator '%s'.",
              _ouo_tok_kind_str(ast->as.unop.op));
          break;
      }
      break;
    case OUO_AST_IF: {
      _ouo_a_ast_visit(a, ast->as.if_expr.condition);
      _ouo_a_ast_visit(a, ast->as.if_expr.then_branch);
      if (ast->as.if_expr.else_branch != NULL)
        _ouo_a_ast_visit(a, ast->as.if_expr.else_branch);

      _ouo_a_err_todo(a, ast);
      break;
    }
    case OUO_AST_WHILE: {
      _ouo_a_ast_visit(a, ast->as.while_expr.condition);
      _ouo_a_ast_visit(a, ast->as.while_expr.body);

      _ouo_a_err_todo(a, ast);
      break;
    }
    case OUO_AST_CALL: {
      _ouo_a_ast_visit(a, ast->as.call.target);
      OUO_DA_FOREACH(OuoAst *, expr_p, &ast->children) {
        _ouo_a_ast_visit(a, *expr_p);
      }

      _ouo_a_err_todo(a, ast);
      break;
    }

    // Statements
    case OUO_AST_EXPR_STMT: {
      _ouo_a_ast_visit(a, ast->as.expr_stmt.expr);

      OuoAst *expr = ast->as.expr_stmt.expr;
      ast->a_res.type = expr->a_res.type;
      break;
    }
    case OUO_AST_PRINT: {
      _ouo_a_ast_visit(a, ast->as.expr_stmt.expr);

      _ouo_a_err_todo(a, ast);
      break;
    }
    case OUO_AST_RETURN: {
      OuoAst *expr = ast->as.expr_stmt.expr;
      if (expr != NULL) _ouo_a_ast_visit(a, expr);

      _ouo_a_err_todo(a, ast);
      break;
    }
    case OUO_AST_DECL_VAR: {
      if (ast->as.decl_var.type_annot != NULL)
        _ouo_a_ast_visit(a, ast->as.decl_var.type_annot);
      _ouo_a_ast_visit(a, ast->as.decl_var.value);

      _ouo_a_err_todo(a, ast);
      break;
    }
    case OUO_AST_DECL_FN: {
      _ouo_a_err_todo(a, ast);
      break;
    }
    case OUO_AST_DECL_TYPE: {
      _ouo_a_err_todo(a, ast);
      break;
    }
  }

  _ouo_ast_dump(ast);
  OuoString type_str = _ouo_type_str(&ast->a_res.type);
  ouo_printdbg(": %.*s\n", (int)type_str.count, type_str.items);
  ouo_da_free(type_str);
}

OuoStageResult ouo_analyze(OuoAst *ast) {
  OuoStageResult res = {0};
  _OuoAnalyzer a = {0};
  _ouo_a_init(&a, &res);

  _ouo_a_ast_visit(&a, ast);

  return res;
}

//
// Compilation
//

// typedef struct {
//   bool panic_mode;
//   size_t scope_depth;
//   bool noemit;
//   OuoCompileResult *res;
// } _OuoCompiler;

// typedef struct {
//   const char *name;
//   OuoType type;
// #ifndef OUO_NOEMIT
//   OuoObject obj;
// #endif
// } _OuoBuiltin;

// static void _ouo_c_err_append(_OuoCompiler *c, OuoError *err) {
//   ouo_da_append(&c->res->errors, *err);
// }

// #define _ouo_c_err_add(c, tok, err_code, ...) \
//   do { \
//     OuoError err = { \
//         .code = (err_code), \
//         .len = (tok).str.len, \
//         .pos = (tok).pos, \
//         .msg = {0}, \
//     }; \
//     _ouo_err_sprintf(err, __VA_ARGS__); \
//     _ouo_c_err_append(c, &err); \
//   } while (0)

// #define _ouo_c_err(c, tok, err_code, ...) \
//   do { \
//     if (!(c)->panic_mode) { \
//       (c)->res->failed = true; \
//       (c)->panic_mode = true; \
//       _ouo_c_err_add(c, tok, err_code, __VA_ARGS__); \
//     } \
//   } while (0)

// static inline size_t _ouo_c_chunk_scope_pop(
//     _OuoCompiler *c, OuoChunkSymbols *syms);
// static void _ouo_c_ast_visit(_OuoCompiler *c, OuoAst *ast, OuoAst *parent);
// static void _ouo_c_res_cleanup(OuoCompileResult *res);

// #ifdef OUO_DEBUG
// static void _ouo_c_dump(_OuoCompiler *c, OuoAst *ast);
// #endif

// #ifndef OUO_NOEMIT
// static inline void _ouo_obj_ref(OuoObject *obj);
// static inline void _ouo_obj_deref(OuoObject *obj);

// static void _ouo_chunk_free(OuoChunk *chunk);
// static void _ouo_chunk_cleanup(OuoChunk *chunk);
// #endif // OUO_NOEMIT

// static inline void _ouo_c_init(
//     _OuoCompiler *c, OuoCompileResult *res, size_t scope_depth) {
//   res->failed = false;
//   c->res = res;
//   c->scope_depth = scope_depth;
// }

// static inline bool _ouo_chunk_find_sym(
//     OuoChunkSymbols *syms, OuoStringSlice *name, OuoSymbol **res_sym) {
//   if (syms->count == 0) return false;

//   for (size_t i = syms->count - 1; i >= 0; i--) {
//     if (ouo_str_slice_eq(name, &syms->items[i].name)) {
//       if (res_sym != NULL) *res_sym = &syms->items[i];
//       return true;
//     }
//     if (i == 0) break;
//   }

//   return false;
// }

// static inline bool _ouo_c_find_local_or_global_sym(
//     _OuoCompiler *c, OuoStringSlice *name, OuoSymbol **res_sym) {
//   if (_ouo_chunk_find_sym(&c->res->local_syms, name, res_sym)) return true;
//   else if (_ouo_chunk_find_sym(&c->res->global_syms, name, res_sym))
//     return true;
//   return false;
// }

// static inline OuoTypeRef *_ouo_c_add_type_ref(
//     _OuoCompiler *c, OuoTypeKind kind, OuoTypeRef *ref) {
//   ref->kind = kind;
//   ouo_da_append(&c->res->type_refs, *ref);
//   return &c->res->type_refs.items[c->res->type_refs.count - 1];
// }

// static inline OuoSymbol *_ouo_c_chunk_add_sym(_OuoCompiler *c,
//     OuoChunkSymbols *syms, OuoSymbolKind kind, OuoStringSlice *name,
//     OuoType *type, OuoToken *tok) {
//   if (c->res->failed && c->res->keep_module_scope) return NULL;

//   OuoSymbol sym = {
//       .kind = kind,
//       .idx = syms->count,
//       .name = *name,
//       .type = *type,
//       .scope_depth = c->scope_depth,
//   };
//   if (tok != NULL) sym.tok = *tok;
//   ouo_da_append(syms, sym);
//   return &syms->items[syms->count - 1];
// }

// static inline OuoSymbol *_ouo_c_add_local_sym(
//     _OuoCompiler *c, OuoToken *name, OuoType *type) {
//   if (c->res->local_syms.count > UINT8_MAX) {
//     _ouo_c_err(c, *name, OUO_ERR_COMPILE_FAIL,
//         "Maximum amount of local symbols exceeded (max %d).", UINT8_MAX + 1);
//     return NULL;
//   }

//   OuoSymbol *found_sym = NULL;
//   if (_ouo_c_find_local_or_global_sym(c, &name->str, &found_sym)) {
//     _ouo_c_err(c, *name, OUO_ERR_SEMANTIC, "Symbol '%.*s' is already
//     defined.",
//         OUO_STRSL_FMT(found_sym->name));
//     _ouo_c_err_add(
//         c, found_sym->tok, OUO_ERR_NOTE, "Previous definition here.");
//     return NULL;
//   }

//   return _ouo_c_chunk_add_sym(
//       c, &c->res->local_syms, OUO_SYM_LOCAL, &name->str, type, name);
// }

// static inline OuoSymbol *_ouo_c_add_global_sym(
//     _OuoCompiler *c, OuoToken *name, OuoType *type) {
//   if (c->res->global_syms.count > UINT8_MAX) {
//     _ouo_c_err(c, *name, OUO_ERR_COMPILE_FAIL,
//         "Maximum amount of global symbols exceeded (max %d).", UINT8_MAX +
//         1);
//     return NULL;
//   }

//   OuoSymbol *found_sym = NULL;
//   if (_ouo_c_find_local_or_global_sym(c, &name->str, &found_sym)) {
//     _ouo_c_err(c, *name, OUO_ERR_SEMANTIC, "Symbol '%.*s' is already
//     defined.",
//         OUO_STRSL_FMT(found_sym->name));
//     _ouo_c_err_add(
//         c, found_sym->tok, OUO_ERR_NOTE, "Previous definition here.");
//     return NULL;
//   }

//   return _ouo_c_chunk_add_sym(
//       c, &c->res->global_syms, OUO_SYM_GLOBAL, &name->str, type, name);
// }

// static inline OuoSymbol *_ouo_c_add_builtin_sym(
//     _OuoCompiler *c, OuoStringSlice *name, OuoType *type) {
//   if (c->res->builtin_syms.count > UINT8_MAX) {
//     _ouo_c_err(c, (OuoToken){0}, OUO_ERR_COMPILE_FAIL,
//         "Maximum amount of builtin symbols exceeded (max %d).", UINT8_MAX +
//         1);
//     return NULL;
//   }

//   OuoSymbol *found_sym = NULL;
//   if (_ouo_chunk_find_sym(&c->res->builtin_syms, name, &found_sym)) {
//     _ouo_c_err(c, (OuoToken){0}, OUO_ERR_SEMANTIC,
//         "Builtin '@%.*s' is already defined.",
//         OUO_STRSL_FMT(found_sym->name));
//     return NULL;
//   }

//   if (type->kind == OUO_TYPE_FN && type->ref->as.t_fn.params.count > 3) {
//     _ouo_c_err(c, (OuoToken){0}, OUO_ERR_COMPILE_FAIL,
//         "Builtin functions cannot have more than 3 parameters (got %zu).",
//         type->ref->as.t_fn.params.count);
//     return NULL;
//   }

//   return _ouo_c_chunk_add_sym(
//       c, &c->res->builtin_syms, OUO_SYM_BUILTIN, name, type, NULL);
// }

// // Static analysis

// // static void _ouo_c_err_todo(_OuoCompiler *c, OuoAst *ast, const char *msg)
// {
// //   _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL, "TODO: %s", msg);
// // }

// static void _ouo_c_err_ident_no_sym(_OuoCompiler *c, OuoAst *ast) {
//   _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
//       "Identifier '%.*s' has no symbol.", OUO_TOK_FMT(ast->tok));
// }

// static void _ouo_c_err_ident_undefined(_OuoCompiler *c, OuoAst *ast) {
//   _ouo_c_err(c, ast->tok, OUO_ERR_SEMANTIC, "Undefined symbol '%.*s'.",
//       OUO_TOK_FMT(ast->tok));
// }

// static void _ouo_c_err_builtin_undefined(_OuoCompiler *c, OuoAst *ast) {
//   _ouo_c_err(c, ast->tok, OUO_ERR_SEMANTIC, "Undefined builtin '%.*s'.",
//       OUO_TOK_FMT(ast->tok));
// }

// static void _ouo_c_err_ident_not_type(_OuoCompiler *c, OuoAst *ast) {
//   _ouo_c_err(c, ast->tok, OUO_ERR_SEMANTIC, "'%.*s' is not a type.",
//       OUO_TOK_FMT(ast->tok));
// }

// static void _ouo_c_err_assign_type(
//     _OuoCompiler *c, OuoToken *tok, OuoType *target_type, OuoType
//     *value_type) {
//   OuoString value_type_str = _ouo_type_str(value_type);
//   OuoString target_type_str = _ouo_type_str(target_type);

//   _ouo_c_err(c, *tok, OUO_ERR_TYPE, "Cannot assign '%.*s' to '%.*s'.",
//       OUO_STR_FMT(value_type_str), OUO_STR_FMT(target_type_str));

//   ouo_da_free(value_type_str);
//   ouo_da_free(target_type_str);
// }

// static void _ouo_c_err_assign_invalid(_OuoCompiler *c, OuoAst *ast) {
//   _ouo_c_err(c, ast->tok, OUO_ERR_SEMANTIC,
//       "Assignment target can only be a variable.");
// }

// static void _ouo_c_err_if_condition_type(_OuoCompiler *c, OuoAst *ast) {
//   OuoString type_str = _ouo_type_str(&ast->as.if_expr.condition->type);

//   _ouo_c_err(c, ast->as.if_expr.condition->tok, OUO_ERR_TYPE,
//       "Condition can only be '%s', got '%.*s'.",
//       _ouo_type_kind_str(OUO_TYPE_BOOL), OUO_STR_FMT(type_str));

//   ouo_da_free(type_str);
// }

// static void _ouo_c_err_call_type(_OuoCompiler *c, OuoAst *ast) {
//   OuoString type_str = _ouo_type_str(&ast->as.call.target->type);

//   _ouo_c_err(c, ast->tok, OUO_ERR_TYPE, "'%.*s' is not callable.",
//       OUO_STR_FMT(type_str));

//   ouo_da_free(type_str);
// }

// static void _ouo_c_err_call_arg_num(
//     _OuoCompiler *c, OuoAst *ast, OuoType *fn_type, size_t got) {
//   OuoString type_str = _ouo_type_str(fn_type);

//   _ouo_c_err(c, ast->tok, OUO_ERR_TYPE,
//       "'%.*s' expects %zu arguments, got %zu.", OUO_STR_FMT(type_str),
//       fn_type->ref->as.t_fn.params.count, got);

//   ouo_da_free(type_str);
// }

// static void _ouo_c_err_call_arg_type(_OuoCompiler *c, OuoAst *ast,
//     OuoType *exp_type, OuoType *got_type, size_t arg_idx) {
//   OuoString exp_type_str = _ouo_type_str(exp_type);
//   OuoString got_type_str = _ouo_type_str(got_type);

//   _ouo_c_err(c, ast->tok, OUO_ERR_TYPE,
//       "Argument %zu must be '%.*s', got '%.*s'.", arg_idx + 1,
//       OUO_STR_FMT(exp_type_str), OUO_STR_FMT(got_type_str));

//   ouo_da_free(exp_type_str);
//   ouo_da_free(got_type_str);
// }

// static void _ouo_c_err_if_branch_type(
//     _OuoCompiler *c, OuoAst *ast, OuoType *then_type, OuoType *else_type) {
//   OuoString then_type_str = _ouo_type_str(then_type);
//   OuoString else_type_str = _ouo_type_str(else_type);

//   _ouo_c_err(c, ast->tok, OUO_ERR_TYPE,
//       "All branches must evaluate to the same type (then is '%.*s', else is "
//       "'%.*s').",
//       OUO_STR_FMT(then_type_str), OUO_STR_FMT(else_type_str));

//   ouo_da_free(then_type_str);
//   ouo_da_free(else_type_str);
// }

// static void _ouo_c_err_stmt_type(
//     _OuoCompiler *c, OuoToken *tok, OuoType *type) {
//   OuoString type_str = _ouo_type_str(type);

//   _ouo_c_err(c, *tok, OUO_ERR_TYPE, "Type of '%.*s' cannot be '%.*s'.",
//       OUO_TOK_FMT(*tok), OUO_STR_FMT(type_str));

//   ouo_da_free(type_str);
// }

// static void _ouo_c_err_var_type(_OuoCompiler *c, OuoAst *ast, OuoType *type)
// {
//   OuoString type_str = _ouo_type_str(type);

//   _ouo_c_err(c, ast->tok, OUO_ERR_TYPE, "A variabe cannot be '%.*s'.",
//       OUO_STR_FMT(type_str));

//   ouo_da_free(type_str);
// }

// static void _ouo_c_err_fn_type(
//     _OuoCompiler *c, OuoAst *ast, OuoType *exp, OuoType *got) {
//   OuoString exp_type_str = _ouo_type_str(exp);
//   OuoString got_type_str = _ouo_type_str(got);

//   _ouo_c_err(c, ast->tok, OUO_ERR_TYPE,
//       "Function returns '%.*s', but got '%.*s'.", OUO_STR_FMT(exp_type_str),
//       OUO_STR_FMT(got_type_str));

//   ouo_da_free(exp_type_str);
//   ouo_da_free(got_type_str);
// }

// static inline OuoSymbol *_ouo_ast_get_sym(_OuoCompiler *c, OuoAst *ast) {
//   OuoSymbol *sym = NULL;
//   switch (ast->kind) {
//     case OUO_AST_IDENT:
//     case OUO_AST_BUILTIN: sym = ast->as.ident.sym; break;
//     default: return NULL;
//   }

//   if (sym == NULL) _ouo_c_err_ident_no_sym(c, ast);
//   return sym;
// }

// static void _ouo_c_ast_analyze(_OuoCompiler *c, OuoAst *ast) {
//   switch (ast->kind) {
//     case OUO_AST_MODULE: ast->type.kind = OUO_TYPE_VOID; break;
//     case OUO_AST_IDENT: {
//       OuoSymbol *sym = NULL;
//       if (_ouo_c_find_local_or_global_sym(c, &ast->as.ident.name, &sym)) {
//         ast->type = sym->type;
//         ast->as.ident.sym = sym;
//       } else _ouo_c_err_ident_undefined(c, ast);
//       break;
//     }
//     case OUO_AST_BUILTIN: {
//       OuoSymbol *sym = NULL;
//       if (_ouo_chunk_find_sym(
//               &c->res->builtin_syms, &ast->as.ident.name, &sym)) {
//         ast->type = sym->type;
//         ast->as.ident.sym = sym;
//       } else _ouo_c_err_builtin_undefined(c, ast);
//       break;
//     }

//     // Literals
//     case OUO_AST_LIT_INT: ast->type.kind = OUO_TYPE_INT; break;
//     case OUO_AST_LIT_FLOAT: ast->type.kind = OUO_TYPE_FLOAT; break;
//     case OUO_AST_LIT_BOOL: ast->type.kind = OUO_TYPE_BOOL; break;
//     case OUO_AST_LIT_STR: ast->type.kind = OUO_TYPE_STR; break;
//     case OUO_AST_LIT_TYPE: {
//       ast->type.kind = OUO_TYPE_VOID;
//       if (ast->as.lit_type.ident != NULL) {
//         OuoSymbol *sym = ast->as.lit_type.ident->as.ident.sym;
//         if (sym != NULL && sym->type.kind == OUO_TYPE_TYPE)
//           ast->as.lit_type.t = sym->type.ref->as.t_type;
//         else if (sym != NULL)
//           _ouo_c_err_ident_not_type(c, ast->as.lit_type.ident);
//       }
//       break;
//     }

//     // Expressions
//     case OUO_AST_ASSIGN: {
//       ast->type.kind = OUO_TYPE_VOID;
//       OuoSymbol *sym = _ouo_ast_get_sym(c, ast->as.assign.target);
//       if (sym == NULL || sym->kind != OUO_SYM_LOCAL) {
//         _ouo_c_err_assign_invalid(c, ast);
//         break;
//       }

//       if (!_ouo_type_is(
//               &ast->as.assign.value->type, &ast->as.assign.target->type))
//         _ouo_c_err_assign_type(c, &ast->tok, &ast->as.assign.target->type,
//             &ast->as.assign.value->type);
//       break;
//     }
//     case OUO_AST_BINARY:
//       switch (ast->as.binary.op) {
//         // Arithmetic
//         case OUO_TOK_PLUS:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             ast->type.kind = OUO_TYPE_INT;
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             ast->type.kind = OUO_TYPE_FLOAT;
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_STR))
//             ast->type.kind = OUO_TYPE_STR;
//           else _ouo_c_err_binary_type(c, ast);
//           break;
//         case OUO_TOK_MINUS:
//         case OUO_TOK_ASTERISK:
//         case OUO_TOK_SLASH:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             ast->type.kind = OUO_TYPE_INT;
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             ast->type.kind = OUO_TYPE_FLOAT;
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         // Comparison
//         case OUO_TOK_EQ:
//         case OUO_TOK_NEQ:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT) ||
//               _ouo_ast_binary_is(ast, OUO_TYPE_FLOAT) ||
//               _ouo_ast_binary_is(ast, OUO_TYPE_BOOL))
//             ast->type.kind = OUO_TYPE_BOOL;
//           else _ouo_c_err_binary_type(c, ast);
//           break;
//         case OUO_TOK_LT:
//         case OUO_TOK_LT_EQ:
//         case OUO_TOK_GT:
//         case OUO_TOK_GT_EQ:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT) ||
//               _ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             ast->type.kind = OUO_TYPE_BOOL;
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         // Logic
//         case OUO_TOK_KW_OR:
//         case OUO_TOK_KW_AND:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_BOOL))
//             ast->type.kind = OUO_TYPE_BOOL;
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         default: _ouo_c_err_binary_unknown(c, ast); break;
//       }
//       break;
//     case OUO_AST_UNARY:
//       switch (ast->as.unary.op) {
//         // Arithmetic
//         case OUO_TOK_MINUS:
//           if (_ouo_ast_unary_is(ast, OUO_TYPE_INT))
//             ast->type.kind = OUO_TYPE_INT;
//           else if (_ouo_ast_unary_is(ast, OUO_TYPE_FLOAT))
//             ast->type.kind = OUO_TYPE_FLOAT;
//           else _ouo_c_err_unary_type(c, ast);
//           break;

//         // Logic
//         case OUO_TOK_BANG:
//           if (_ouo_ast_unary_is(ast, OUO_TYPE_BOOL))
//             ast->type.kind = OUO_TYPE_BOOL;
//           else _ouo_c_err_unary_type(c, ast);
//           break;

//         default: _ouo_c_err_unary_unknown(c, ast); break;
//       }
//       break;
//     case OUO_AST_BLOCK: ast->type.kind = OUO_TYPE_VOID; break;
//     case OUO_AST_IF: {
//       if (ast->as.if_expr.condition->type.kind != OUO_TYPE_BOOL) {
//         bool panic_prev = c->panic_mode;
//         c->panic_mode = false;
//         _ouo_c_err_if_condition_type(c, ast);
//         c->panic_mode = panic_prev;
//       }

//       OuoAst *then_branch = ast->as.if_expr.then_branch;
//       OuoAst *else_branch = ast->as.if_expr.else_branch;

//       OuoType *else_type = else_branch != NULL
//           ? &else_branch->type
//           : &(OuoType){.kind = OUO_TYPE_VOID};

//       if (_ouo_type_is(&then_branch->type, else_type))
//         ast->type = then_branch->type;
//       else _ouo_c_err_if_branch_type(c, ast, &then_branch->type, else_type);
//       break;
//     }
//     case OUO_AST_WHILE: {
//       ast->type.kind = OUO_TYPE_VOID;
//       if (ast->as.while_expr.condition->type.kind != OUO_TYPE_BOOL) {
//         bool panic_prev = c->panic_mode;
//         c->panic_mode = false;
//         _ouo_c_err_if_condition_type(c, ast);
//         c->panic_mode = panic_prev;
//       }
//       break;
//     }
//     case OUO_AST_CALL: {
//       OuoSymbol *sym = _ouo_ast_get_sym(c, ast->as.call.target);
//       if (sym == NULL) {
//         _ouo_c_err_call_type(c, ast);
//         break;
//       }
//       bool panic_prev = c->panic_mode;
//       c->panic_mode = false;

//       if (sym->type.kind == OUO_TYPE_FN)
//         ast->type = sym->type.ref->as.t_fn.return_type;
//       else {
//         _ouo_c_err_call_type(c, ast);
//         break;
//       }

//       if (sym->type.ref->as.t_fn.params.count != ast->children.count) {
//         _ouo_c_err_call_arg_num(c, ast, &sym->type, ast->children.count);
//         break;
//       }

//       for (size_t i = 0; i < ast->children.count; i++) {
//         OuoAst *arg = ast->children.items[i];
//         OuoType *param = &sym->type.ref->as.t_fn.params.items[i].type;
//         if (arg->type.kind != OUO_TYPE_UNKNOWN &&
//             !_ouo_type_is(&arg->type, param)) {
//           _ouo_c_err_call_arg_type(c, arg, param, &arg->type, i);
//           c->panic_mode = false;
//         }
//       }

//       c->panic_mode = panic_prev;
//       break;
//     }

//     // Statements
//     case OUO_AST_EXPR_STMT: {
//       OuoAst *expr = ast->as.expr_stmt.expr;
//       ast->type = expr->type;
//       break;
//     }
//     case OUO_AST_PRINT: {
//       ast->type.kind = OUO_TYPE_VOID;
//       if (ast->as.expr_stmt.expr->type.kind == OUO_TYPE_VOID)
//         _ouo_c_err_stmt_type(c, &ast->tok, &ast->as.expr_stmt.expr->type);
//       break;
//     }
//     case OUO_AST_RETURN: {
//       ast->type.kind = OUO_TYPE_VOID;
//       OuoType *return_type = ast->as.expr_stmt.expr != NULL
//           ? &ast->as.expr_stmt.expr->type
//           : &ast->type;

//       ast->returns.got = return_type;
//       if (ast->returns.exp != NULL &&
//           !_ouo_type_is(return_type, ast->returns.exp))
//         _ouo_c_err_fn_type(c, ast, ast->returns.exp, return_type);
//       break;
//     }
//     case OUO_AST_DECL_VAR: {
//       ast->type.kind = OUO_TYPE_VOID;
//       bool panic_prev = c->panic_mode;
//       c->panic_mode = false;

//       OuoType *value_type = &ast->as.decl_var.value->type;
//       OuoType *type = ast->as.decl_var.type_annot != NULL
//           ? &ast->as.decl_var.type_annot->as.lit_type.t
//           : value_type;

//       if (type->kind == OUO_TYPE_VOID) {
//         _ouo_c_err_var_type(c, ast, type);
//       } else if (value_type->kind != OUO_TYPE_UNKNOWN &&
//           type->kind != OUO_TYPE_UNKNOWN && !_ouo_type_is(value_type, type))
//           {
//         _ouo_c_err_assign_type(c, &ast->tok, type, value_type);
//       }

//       _ouo_c_add_local_sym(c, &ast->as.decl_var.name, type);
//       c->panic_mode = panic_prev;
//       break;
//     }
//     case OUO_AST_DECL_FN: {
//       ast->type.kind = OUO_TYPE_VOID;
//       bool panic_prev = c->panic_mode;
//       c->panic_mode = false;

//       OuoAst *body = ast->as.decl_fn.body;
//       OuoAst *return_type_annot = ast->as.decl_fn.return_type_annot;

//       OuoType *body_type =
//           body->returns.got != NULL ? body->returns.got : &body->type;
//       OuoType *return_type = return_type_annot != NULL
//           ? &return_type_annot->as.lit_type.t
//           : body_type;

//       if (body_type->kind != OUO_TYPE_UNKNOWN && body->returns.got == NULL &&
//           !_ouo_type_is(body_type, return_type))
//         _ouo_c_err_fn_type(c, ast, return_type, body_type);
//       else if (return_type_annot == NULL)
//         ast->as.decl_fn.sym->type.ref->as.t_fn.return_type = *body_type;

//       c->panic_mode = panic_prev;
//       break;
//     }
//     case OUO_AST_DECL_TYPE: break;
//   }
// }

// #ifndef OUO_NOEMIT

// // Codegen

// static inline size_t _ouo_chunk_get_line(OuoChunk *chunk, const uint8_t *ip)
// {
//   size_t ip_idx = (size_t)(ip - chunk->bytecode.items);
//   size_t ip_idx_curr = 0;
//   for (size_t i = 0; i < chunk->lines.count; i += 2) {
//     ip_idx_curr += chunk->lines.items[i];
//     if (ip_idx_curr > ip_idx) return chunk->lines.items[i + 1];
//   }
//   return 0;
// }

// static inline void _ouo_c_chunk_write(
//     _OuoCompiler *c, uint8_t byte, size_t line) {
//   ouo_da_append(&c->res->chunk.bytecode, byte);

//   size_t lines_count = c->res->chunk.lines.count;
//   if (lines_count == 0 || c->res->chunk.lines.items[lines_count - 1] != line)
//   {
//     ouo_da_append(&c->res->chunk.lines, 1);
//     ouo_da_append(&c->res->chunk.lines, line);
//   } else {
//     c->res->chunk.lines.items[lines_count - 2]++;
//   }
// }

// static inline void _ouo_c_write_u8(_OuoCompiler *c, OuoAst *ast, uint8_t u8)
// {
//   _ouo_c_chunk_write(c, u8, ast->tok.pos.line);
// }

// static inline void _ouo_c_write_u16(
//     _OuoCompiler *c, OuoAst *ast, uint16_t u16) {
//   _ouo_c_write_u8(c, ast, (u16 >> 8) & 0xFF);
//   _ouo_c_write_u8(c, ast, u16 & 0xFF);
// }

// static inline size_t _ouo_chunk_add_obj(OuoObjects *objs, OuoObject *obj) {
//   _ouo_obj_ref(obj);
//   ouo_da_append(objs, *obj);
//   return objs->count - 1;
// }

// static inline void _ouo_c_emit_lit(
//     _OuoCompiler *c, OuoAst *ast, OuoObject *obj) {
//   if (c->res->chunk.literals.count > UINT8_MAX) {
//     _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
//         "Maximum amount of literals exceeded (max %d).", UINT8_MAX + 1);
//     return;
//   }

//   if (obj->kind == OUO_OBJ_INT) {
//     if (obj->as.v_int == 0) {
//       _ouo_c_write_u8(c, ast, OUO_OP_PUSH_0);
//       return;
//     } else if (obj->as.v_int == 1) {
//       _ouo_c_write_u8(c, ast, OUO_OP_PUSH_1);
//       return;
//     } else if (obj->as.v_int >= INT8_MIN && obj->as.v_int <= INT8_MAX) {
//       _ouo_c_write_u8(c, ast, OUO_OP_PUSH_INT8);
//       _ouo_c_write_u8(c, ast, (uint8_t)obj->as.v_int);
//       return;
//     }
//   } else if (obj->kind == OUO_OBJ_BOOL) {
//     _ouo_c_write_u8(
//         c, ast, obj->as.v_bool ? OUO_OP_PUSH_TRUE : OUO_OP_PUSH_FALSE);
//     return;
//   }

//   size_t lit_idx = _ouo_chunk_add_obj(&c->res->chunk.literals, obj);
//   _ouo_c_write_u8(c, ast, OUO_OP_LIT);
//   _ouo_c_write_u8(c, ast, (uint8_t)lit_idx);
// }

// static inline size_t _ouo_c_add_global_obj(
//     _OuoCompiler *c, OuoAst *ast, OuoObject *obj) {
//   if (c->res->chunk.globals.count > UINT8_MAX) {
//     _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
//         "Maximum amount of globals exceeded (max %d).", UINT8_MAX + 1);
//     return SIZE_MAX;
//   }

//   return _ouo_chunk_add_obj(&c->res->chunk.globals, obj);
// }

// static inline size_t _ouo_c_add_builtin_obj(_OuoCompiler *c, OuoObject *obj)
// {
//   if (c->res->chunk.builtins.count > UINT8_MAX) {
//     _ouo_c_err(c, (OuoToken){0}, OUO_ERR_COMPILE_FAIL,
//         "Maximum amount of builtins exceeded (max %d).", UINT8_MAX + 1);
//     return SIZE_MAX;
//   }

//   return _ouo_chunk_add_obj(&c->res->chunk.builtins, obj);
// }

// static inline size_t _ouo_c_emit_jump(
//     _OuoCompiler *c, OuoAst *ast, uint8_t op) {
//   _ouo_c_write_u8(c, ast, op);
//   _ouo_c_write_u8(c, ast, UINT8_MAX);
//   _ouo_c_write_u8(c, ast, UINT8_MAX);
//   return c->res->chunk.bytecode.count - 2;
// }

// static inline void _ouo_c_patch_jump(
//     _OuoCompiler *c, OuoAst *ast, size_t op_idx) {
//   size_t jump = c->res->chunk.bytecode.count - op_idx - 2;
//   if (jump > UINT16_MAX) {
//     _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
//         "Maximum jump offset exceeded (max %d, got %zu).", UINT16_MAX, jump);
//     return;
//   }

//   c->res->chunk.bytecode.items[op_idx] = (jump >> 8) & 0xFF;
//   c->res->chunk.bytecode.items[op_idx + 1] = jump & 0xFF;
// }

// static inline void _ouo_c_emit_loop(
//     _OuoCompiler *c, OuoAst *ast, size_t op_idx) {
//   size_t jump = c->res->chunk.bytecode.count - op_idx + 3;
//   if (jump > UINT16_MAX) {
//     _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
//         "Maximum loop offset exceeded (max %d, got %zu).", UINT16_MAX, jump);
//     return;
//   }

//   _ouo_c_write_u8(c, ast, OUO_OP_LOOP);
//   _ouo_c_write_u16(c, ast, (uint16_t)jump);
// }

// static inline void _ouo_c_emit_return(
//     _OuoCompiler *c, OuoAst *ast, bool is_void) {
//   _ouo_c_write_u8(c, ast, is_void ? OUO_OP_RETURN_VOID : OUO_OP_RETURN);

//   size_t pop_count = c->res->local_syms.count;
//   if (pop_count > UINT8_MAX) {
//     _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
//         "Maximum POP_N count exceeded (max %d).", UINT8_MAX);
//     return;
//   }

//   _ouo_c_write_u8(c, ast, (uint8_t)pop_count);
// }

// // Copy-on-write
// #define _ouo_obj_new_int(v) ((OuoObject){.kind = OUO_OBJ_INT, .as.v_int =
// (v)})

// #define _ouo_obj_new_float(v) \
//   ((OuoObject){.kind = OUO_OBJ_FLOAT, .as.v_float = (v)})

// #define _ouo_obj_new_bool(v) \
//   ((OuoObject){.kind = OUO_OBJ_BOOL, .as.v_bool = (v)})

// // Reference-counted
// static inline OuoRc *_ouo_rc_new(size_t size) {
//   OuoRc *rc = ouo_malloc(size);
//   ouo_assert_nomem(rc);
//   rc->count = 0;
//   return rc;
// }

// static inline OuoRcStr *_ouo_rc_new_str(void) {
//   OuoRcStr *rc = (OuoRcStr *)_ouo_rc_new(sizeof(OuoRcStr));
//   rc->str.items = NULL;
//   rc->str.count = 0;
//   rc->str.capacity = 0;
//   return rc;
// }

// #define _ouo_obj_new_str(rc) \
//   ((OuoObject){.kind = OUO_OBJ_STR, .as.ref = (OuoRc *)(rc)})

// static inline OuoRcFn *_ouo_rc_new_fn(void) {
//   OuoRcFn *rc = (OuoRcFn *)_ouo_rc_new(sizeof(OuoRcFn));
//   rc->chunk = (OuoChunk){0};
//   return rc;
// }

// #define _ouo_obj_new_fn(rc) \
//   ((OuoObject){.kind = OUO_OBJ_FN, .as.ref = (OuoRc *)(rc)})

// static void _ouo_c_ast_emit(_OuoCompiler *c, OuoAst *ast) {
//   switch (ast->kind) {
//     case OUO_AST_MODULE: break;
//     case OUO_AST_IDENT:
//     case OUO_AST_BUILTIN: {
//       OuoSymbol *sym = _ouo_ast_get_sym(c, ast);
//       if (sym == NULL) break;

//       if (sym->kind == OUO_SYM_GLOBAL)
//         _ouo_c_write_u8(c, ast, OUO_OP_GET_GLOBAL);
//       else if (sym->kind == OUO_SYM_BUILTIN)
//         _ouo_c_write_u8(c, ast, OUO_OP_GET_BUILTIN);
//       else _ouo_c_write_u8(c, ast, OUO_OP_GET);

//       _ouo_c_write_u8(c, ast, (uint8_t)sym->idx);
//       break;
//     }

//     // Literals
//     case OUO_AST_LIT_INT:
//       _ouo_c_emit_lit(c, ast, &_ouo_obj_new_int(ast->as.lit_int));
//       break;
//     case OUO_AST_LIT_FLOAT:
//       _ouo_c_emit_lit(c, ast, &_ouo_obj_new_float(ast->as.lit_float));
//       break;
//     case OUO_AST_LIT_BOOL:
//       _ouo_c_emit_lit(c, ast, &_ouo_obj_new_bool(ast->as.lit_bool));
//       break;
//     case OUO_AST_LIT_STR: {
//       OuoRcStr *rc = _ouo_rc_new_str();
//       ouo_da_append_many(
//           &rc->str, ast->as.lit_str.items, ast->as.lit_str.count);
//       _ouo_c_emit_lit(c, ast, &_ouo_obj_new_str(rc));
//       break;
//     }
//     case OUO_AST_LIT_TYPE: break;

//     // Expressions
//     case OUO_AST_ASSIGN: {
//       OuoSymbol *sym = _ouo_ast_get_sym(c, ast->as.assign.target);
//       if (sym == NULL) break;
//       _ouo_c_write_u8(c, ast, OUO_OP_SET);
//       _ouo_c_write_u8(c, ast, (uint8_t)sym->idx);
//       break;
//     }
//     case OUO_AST_BINARY:
//       switch (ast->as.binary.op) {
//         // Arithmetic
//         case OUO_TOK_PLUS:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_ADD_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_ADD_FLOAT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_STR))
//             _ouo_c_write_u8(c, ast, OUO_OP_ADD_STR);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         case OUO_TOK_MINUS:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_SUB_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_SUB_FLOAT);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         case OUO_TOK_ASTERISK:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_MULT_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_MULT_FLOAT);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         case OUO_TOK_SLASH:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_DIV_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_DIV_FLOAT);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         // Comparison
//         case OUO_TOK_EQ:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_EQ_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_EQ_FLOAT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_BOOL))
//             _ouo_c_write_u8(c, ast, OUO_OP_EQ_BOOL);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         case OUO_TOK_NEQ:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_NEQ_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_NEQ_FLOAT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_BOOL))
//             _ouo_c_write_u8(c, ast, OUO_OP_NEQ_BOOL);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         case OUO_TOK_LT:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_LT_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_LT_FLOAT);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         case OUO_TOK_LT_EQ:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_LT_EQ_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_LT_EQ_FLOAT);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         case OUO_TOK_GT:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_GT_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_GT_FLOAT);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         case OUO_TOK_GT_EQ:
//           if (_ouo_ast_binary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_GT_EQ_INT);
//           else if (_ouo_ast_binary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_GT_EQ_FLOAT);
//           else _ouo_c_err_binary_type(c, ast);
//           break;

//         // Logic
//         case OUO_TOK_KW_OR: break;
//         case OUO_TOK_KW_AND: break;

//         default: _ouo_c_err_binary_unknown(c, ast); break;
//       }
//       break;
//     case OUO_AST_UNARY:
//       switch (ast->as.unary.op) {
//         // Arithmetic
//         case OUO_TOK_MINUS:
//           if (_ouo_ast_unary_is(ast, OUO_TYPE_INT))
//             _ouo_c_write_u8(c, ast, OUO_OP_NEG_INT);
//           else if (_ouo_ast_unary_is(ast, OUO_TYPE_FLOAT))
//             _ouo_c_write_u8(c, ast, OUO_OP_NEG_FLOAT);
//           else _ouo_c_err_unary_type(c, ast);
//           break;

//         // Logic
//         case OUO_TOK_BANG:
//           if (_ouo_ast_unary_is(ast, OUO_TYPE_BOOL))
//             _ouo_c_write_u8(c, ast, OUO_OP_NOT);
//           else _ouo_c_err_unary_type(c, ast);
//           break;

//         default: _ouo_c_err_unary_unknown(c, ast); break;
//       }
//       break;
//     case OUO_AST_BLOCK: break;
//     case OUO_AST_IF: break;
//     case OUO_AST_WHILE: break;
//     case OUO_AST_CALL: {
//       OuoSymbol *sym = _ouo_ast_get_sym(c, ast->as.call.target);

//       if (sym->type.kind == OUO_TYPE_FN) {
//         _ouo_c_write_u8(c, ast, OUO_OP_CALL);
//         _ouo_c_write_u8(c, ast,
//         (uint8_t)sym->type.ref->as.t_fn.params.count);
//       } else {
//         _ouo_c_err_call_type(c, ast);
//       }
//       break;
//     }

//     // Statements
//     case OUO_AST_EXPR_STMT:
//       if (ast->as.expr_stmt.pop && ast->type.kind != OUO_TYPE_VOID) {
//         if (c->res->echo && c->scope_depth == 0)
//           _ouo_c_write_u8(c, ast, OUO_OP_PRINT);
//         _ouo_c_write_u8(c, ast, OUO_OP_POP);
//       }
//       break;
//     case OUO_AST_PRINT:
//       _ouo_c_write_u8(c, ast, OUO_OP_PRINT);
//       _ouo_c_write_u8(c, ast, OUO_OP_POP);
//       break;
//     case OUO_AST_RETURN:
//       _ouo_c_emit_return(c, ast, ast->as.expr_stmt.expr == NULL);
//       break;
//     case OUO_AST_DECL_VAR: break;
//     case OUO_AST_DECL_FN:
//       if (ast->as.decl_fn.params.count > UINT8_MAX) {
//         _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
//             "Maximum amount of function parameters exceeded (max %d).",
//             UINT8_MAX);
//       }
//       break;
//     case OUO_AST_DECL_TYPE: break;
//   }
// }

// static void _ouo_c_ast_transform(OuoAst *ast) {
//   switch (ast->kind) {
//     case OUO_AST_UNARY:
//       switch (ast->as.unary.op) {
//         // Arithmetic
//         case OUO_TOK_MINUS:
//           if (ast->as.unary.right->kind == OUO_AST_LIT_INT) {
//             ouo_ast_free(ast->as.unary.right);
//             ast->kind = OUO_AST_LIT_INT;
//             ast->as.lit_int = -ast->as.unary.right->as.lit_int;
//           } else if (ast->as.unary.right->kind == OUO_AST_LIT_FLOAT) {
//             ouo_ast_free(ast->as.unary.right);
//             ast->kind = OUO_AST_LIT_FLOAT;
//             ast->as.lit_float = -ast->as.unary.right->as.lit_float;
//           }
//           break;

//         default: break;
//       }
//       break;

//     default: break;
//   }
// }

// #endif // OUO_NOEMIT

// static inline void _ouo_c_res_transfer(
//     OuoCompileResult *from, OuoCompileResult *to) {
//   to->failed = from->failed || to->failed;
//   to->builtin_syms = from->builtin_syms;
//   to->type_refs = from->type_refs;
//   to->errors = from->errors;
// #ifndef OUO_NOEMIT
//   to->chunk.globals = from->chunk.globals;
//   to->chunk.builtins = from->chunk.builtins;
// #endif
// }

// static inline void _ouo_c_scope_begin(_OuoCompiler *c) { c->scope_depth++; }

// static inline bool _ouo_c_chunk_scope_has_syms(
//     _OuoCompiler *c, OuoChunkSymbols *syms) {
//   return syms->count > 0 &&
//       syms->items[syms->count - 1].scope_depth > c->scope_depth;
// }

// static inline size_t _ouo_c_chunk_scope_pop(
//     _OuoCompiler *c, OuoChunkSymbols *syms) {
//   size_t pop_count = 0;
//   while (_ouo_c_chunk_scope_has_syms(c, syms)) {
//     syms->count--;
//     pop_count++;
//   }
//   return pop_count;
// }

// static inline void _ouo_c_scope_end(_OuoCompiler *c, OuoAst *ast) {
//   c->scope_depth--;
//   _ouo_c_chunk_scope_pop(c, &c->res->global_syms);
//   size_t pop_count = _ouo_c_chunk_scope_pop(c, &c->res->local_syms);

// #ifndef OUO_NOEMIT
//   if (pop_count > UINT8_MAX) {
//     _ouo_c_err(c, ast->tok, OUO_ERR_COMPILE_FAIL,
//         "Maximum POP_N count exceeded (max %d).", UINT8_MAX);
//     return;
//   }

//   if (pop_count > 0) {
//     _ouo_c_write_u8(c, ast, OUO_OP_POP_N);
//     _ouo_c_write_u8(c, ast, (uint8_t)pop_count);
//   }
// #else
//   (void)ast, (void)pop_count;
// #endif
// }

// static inline bool _ouo_ast_is_global(OuoAst *ast) {
//   return ast->kind == OUO_AST_DECL_FN || ast->kind == OUO_AST_DECL_TYPE;
// }

// static void _ouo_c_ast_visit_global(_OuoCompiler *c, OuoAst *ast) {
//   if (ast->kind == OUO_AST_DECL_FN) {
//     OUO_DA_FOREACH(OuoAstNameType, param, &ast->as.decl_fn.params) {
//       _ouo_c_ast_visit(c, param->type_annot, ast);
//     }
//     if (ast->as.decl_fn.return_type_annot != NULL) {
//       _ouo_c_ast_visit(c, ast->as.decl_fn.return_type_annot, ast);
//       ast->as.decl_fn.body->returns.exp =
//           &ast->as.decl_fn.return_type_annot->as.lit_type.t;
//     }

//     OuoAst *type_annot = ast->as.decl_fn.return_type_annot;
//     OuoType fn_type = {
//         .kind = OUO_TYPE_FN,
//         .ref = _ouo_c_add_type_ref(c, OUO_TYPE_FN,
//             &(OuoTypeRef){
//                 .as.t_fn = {.return_type = type_annot != NULL
//                         ? type_annot->as.lit_type.t
//                         : (OuoType){.kind = OUO_TYPE_UNKNOWN},
//                     .params = {.items = NULL, .count = 0}},
//             }),
//     };

//     OUO_DA_FOREACH(OuoAstNameType, param, &ast->as.decl_fn.params) {
//       OuoNameType param_type = {
//           .name = param->name.str,
//           .type = param->type_annot->as.lit_type.t,
//       };
//       ouo_da_append(&fn_type.ref->as.t_fn.params, param_type);
//     }

//     ast->as.decl_fn.sym =
//         _ouo_c_add_global_sym(c, &ast->as.decl_fn.name, &fn_type);

// #ifndef OUO_NOEMIT
//     OuoRcFn *rc = _ouo_rc_new_fn();
//     _ouo_c_add_global_obj(c, ast, &_ouo_obj_new_fn(rc));
// #endif
//   } else if (ast->kind == OUO_AST_DECL_TYPE) {
//     _ouo_c_ast_visit(c, ast->as.decl_type.type_annot, ast);

//     OuoType type = {
//         .kind = OUO_TYPE_TYPE,
//         .ref = _ouo_c_add_type_ref(c, OUO_TYPE_TYPE,
//             &(OuoTypeRef){
//                 .as.t_type = ast->as.decl_type.type_annot->as.lit_type.t,
//             }),
//     };

//     ast->as.decl_type.sym =
//         _ouo_c_add_global_sym(c, &ast->as.decl_type.name, &type);
//   }
// }

// static inline void _ouo_c_ast_eval_ctx_down(
//     OuoAstEvalContext *child, OuoAstEvalContext *parent) {
//   child->exp = parent->exp;
// }

// static inline void _ouo_c_ast_eval_ctx_up(_OuoCompiler *c, OuoAst *ast,
//     OuoAstEvalContext *child, OuoAstEvalContext *parent) {
//   if (parent->got == NULL) parent->got = child->got;
//   else if (child->got != NULL && child->exp == NULL &&
//       !_ouo_type_is(child->got, parent->got)) {
//     _ouo_c_err_fn_type(c, ast, parent->got, child->got);
//   }
// }

// static void _ouo_c_ast_visit(_OuoCompiler *c, OuoAst *ast, OuoAst *parent) {
//   if (ast == NULL) return;
//   if (parent != NULL && !c->noemit)
//     _ouo_c_ast_eval_ctx_down(&ast->returns, &parent->returns);
//   bool new_scope = false;

// #ifndef OUO_NOEMIT
//   _ouo_c_ast_transform(ast);
// #endif

//   switch (ast->kind) {
//     case OUO_AST_MODULE:
//     case OUO_AST_BLOCK: {
//       bool panic_prev = c->panic_mode;
//       c->panic_mode = false;
//       if (ast->kind == OUO_AST_BLOCK || !c->res->keep_module_scope) {
//         new_scope = true;
//         _ouo_c_scope_begin(c);
//       }

//       OUO_DA_FOREACH(OuoAst *, stmt_p, &ast->children) {
//         OuoAst *stmt = *stmt_p;
//         if (!_ouo_ast_is_global(stmt)) continue;
//         c->panic_mode = false;
//         _ouo_c_ast_visit_global(c, stmt);
//       }

//       OUO_DA_FOREACH(OuoAst *, stmt_p, &ast->children) {
//         OuoAst *stmt = *stmt_p;
//         if (!_ouo_ast_is_global(stmt)) continue;
//         c->panic_mode = false;
//         _ouo_c_ast_visit(c, stmt, ast);
//       }

//       OUO_DA_FOREACH(OuoAst *, stmt_p, &ast->children) {
//         OuoAst *stmt = *stmt_p;
//         if (_ouo_ast_is_global(stmt)) continue;
//         c->panic_mode = false;
//         if (stmt->kind == OUO_AST_EXPR_STMT) stmt->as.expr_stmt.pop = true;
//         _ouo_c_ast_visit(c, stmt, ast);
//       }

//       c->panic_mode = panic_prev;
//       break;
//     }
//     case OUO_AST_IDENT: break;
//     case OUO_AST_BUILTIN: break;

//     // Literals
//     case OUO_AST_LIT_INT:
//     case OUO_AST_LIT_FLOAT:
//     case OUO_AST_LIT_BOOL:
//     case OUO_AST_LIT_STR: break;
//     case OUO_AST_LIT_TYPE:
//       if (ast->as.lit_type.ident != NULL) {
//         bool noemit_prev = c->noemit;
//         c->noemit = true;
//         _ouo_c_ast_visit(c, ast->as.lit_type.ident, ast);
//         c->noemit = noemit_prev;
//       }
//       break;

//     // Expressions
//     case OUO_AST_ASSIGN: {
//       bool noemit_prev = c->noemit;
//       c->noemit = true;
//       _ouo_c_ast_visit(c, ast->as.assign.target, ast);
//       c->noemit = noemit_prev;
//       _ouo_c_ast_visit(c, ast->as.assign.value, ast);
//       break;
//     }
//     case OUO_AST_BINARY:
//       switch (ast->as.binary.op) {
//         case OUO_TOK_KW_OR: {
//           _ouo_c_ast_visit(c, ast->as.binary.left, ast);

// #ifndef OUO_NOEMIT
//           size_t else_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP_IF_FALSE);
//           size_t end_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP);
//           _ouo_c_patch_jump(c, ast, else_jump);
//           _ouo_c_write_u8(c, ast, OUO_OP_POP);
// #endif

//           _ouo_c_ast_visit(c, ast->as.binary.right, ast);

// #ifndef OUO_NOEMIT
//           _ouo_c_patch_jump(c, ast, end_jump);
// #endif
//           break;
//         }
//         case OUO_TOK_KW_AND: {
//           _ouo_c_ast_visit(c, ast->as.binary.left, ast);

// #ifndef OUO_NOEMIT
//           size_t end_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP_IF_FALSE);
//           _ouo_c_write_u8(c, ast, OUO_OP_POP);
// #endif

//           _ouo_c_ast_visit(c, ast->as.binary.right, ast);

// #ifndef OUO_NOEMIT
//           _ouo_c_patch_jump(c, ast, end_jump);
// #endif
//           break;
//         }
//         default:
//           _ouo_c_ast_visit(c, ast->as.binary.left, ast);
//           _ouo_c_ast_visit(c, ast->as.binary.right, ast);
//           break;
//       }
//       break;
//     case OUO_AST_UNARY: _ouo_c_ast_visit(c, ast->as.unary.right, ast); break;
//     case OUO_AST_IF: {
//       c->panic_mode = false;
//       _ouo_c_ast_visit(c, ast->as.if_expr.condition, ast);
//       bool panic_prev = c->panic_mode;

// #ifndef OUO_NOEMIT
//       size_t then_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP_IF_FALSE);
//       _ouo_c_write_u8(c, ast, OUO_OP_POP);
// #endif

//       c->panic_mode = false;
//       _ouo_c_scope_begin(c);
//       _ouo_c_ast_visit(c, ast->as.if_expr.then_branch, ast);
//       _ouo_c_scope_end(c, ast);

// #ifndef OUO_NOEMIT
//       size_t else_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP);
//       _ouo_c_patch_jump(c, ast, then_jump);
//       _ouo_c_write_u8(c, ast, OUO_OP_POP);
// #endif

//       if (ast->as.if_expr.else_branch != NULL) {
//         c->panic_mode = false;
//         _ouo_c_scope_begin(c);
//         _ouo_c_ast_visit(c, ast->as.if_expr.else_branch, ast);
//         _ouo_c_scope_end(c, ast);
//       }

// #ifndef OUO_NOEMIT
//       _ouo_c_patch_jump(c, ast, else_jump);
// #endif
//       c->panic_mode = panic_prev;
//       break;
//     }
//     case OUO_AST_WHILE: {
// #ifndef OUO_NOEMIT
//       size_t loop_start = c->res->chunk.bytecode.count;
// #endif

//       c->panic_mode = false;
//       _ouo_c_ast_visit(c, ast->as.while_expr.condition, ast);
//       bool panic_prev = c->panic_mode;

// #ifndef OUO_NOEMIT
//       size_t exit_jump = _ouo_c_emit_jump(c, ast, OUO_OP_JUMP_IF_FALSE);
//       _ouo_c_write_u8(c, ast, OUO_OP_POP);
// #endif

//       c->panic_mode = false;
//       _ouo_c_scope_begin(c);
//       _ouo_c_ast_visit(c, ast->as.while_expr.body, ast);
//       _ouo_c_scope_end(c, ast);

// #ifndef OUO_NOEMIT
//       _ouo_c_emit_loop(c, ast, loop_start);
//       _ouo_c_patch_jump(c, ast, exit_jump);
//       _ouo_c_write_u8(c, ast, OUO_OP_POP);
// #endif
//       c->panic_mode = panic_prev;
//       break;
//     }
//     case OUO_AST_CALL: {
//       c->panic_mode = false;
//       OUO_DA_FOREACH(OuoAst *, expr_p, &ast->children) {
//         _ouo_c_ast_visit(c, *expr_p, ast);
//         c->panic_mode = false;
//       }

//       _ouo_c_ast_visit(c, ast->as.call.target, ast);
//       break;
//     }

//     // Statements
//     case OUO_AST_EXPR_STMT:
//     case OUO_AST_PRINT:
//     case OUO_AST_RETURN: {
//       OuoAst *expr = ast->as.expr_stmt.expr;
//       if (expr != NULL) _ouo_c_ast_visit(c, expr, ast);
//       break;
//     }
//     case OUO_AST_DECL_VAR:
//       if (ast->as.decl_var.type_annot != NULL)
//         _ouo_c_ast_visit(c, ast->as.decl_var.type_annot, ast);
//       _ouo_c_ast_visit(c, ast->as.decl_var.value, ast);
//       break;
//     case OUO_AST_DECL_FN: {
//       OuoCompileResult fn_res = {0};
//       _ouo_c_res_transfer(c->res, &fn_res);
//       OUO_DA_FOREACH(OuoSymbol, sym, &c->res->global_syms) {
//         ouo_da_append(&fn_res.global_syms, *sym);
//       }

// #ifndef OUO_NOEMIT
//       fn_res.chunk.name = ast->as.decl_fn.name.str;
// #endif

//       _OuoCompiler fn_c = {0};
//       _ouo_c_init(&fn_c, &fn_res, c->scope_depth);

//       _ouo_c_scope_begin(&fn_c);
//       OUO_DA_FOREACH(OuoAstNameType, param, &ast->as.decl_fn.params) {
//         _ouo_c_add_local_sym(
//             &fn_c, &param->name, &param->type_annot->as.lit_type.t);
//       }

//       OuoAst *fn_body = ast->as.decl_fn.body;
//       _ouo_c_ast_visit(&fn_c, fn_body, NULL);
//       _ouo_c_res_transfer(&fn_res, c->res);

// #ifndef OUO_NOEMIT
//       if (!fn_res.failed) {
//         _ouo_c_emit_return(&fn_c, fn_body, fn_body->type.kind ==
//         OUO_TYPE_VOID);

//         OuoRcFn *rc =
//             (OuoRcFn *)c->res->chunk.globals.items[ast->as.decl_fn.sym->idx]
//                 .as.ref;
//         rc->chunk = fn_res.chunk;

// #ifdef OUO_DEBUG
//         _ouo_c_dump(&fn_c, fn_body);
// #endif
//       } else _ouo_chunk_free(&fn_res.chunk);
// #endif // OUO_NOEMIT

//       _ouo_c_res_cleanup(&fn_res);
//       break;
//     }
//     case OUO_AST_DECL_TYPE: break;
//   }

//   _ouo_c_ast_analyze(c, ast);

//   if (parent != NULL && !c->noemit)
//     _ouo_c_ast_eval_ctx_up(c, ast, &ast->returns, &parent->returns);

// #ifndef OUO_NOEMIT
//   if (!c->res->failed && !c->noemit) _ouo_c_ast_emit(c, ast);
// #endif

//   if (new_scope) _ouo_c_scope_end(c, ast);
// }

// //
// // Builtins
// //

// #ifndef OUO_NOEMIT
// #define _ouo_obj_new_bifn(fn) \
//   ((OuoObject){.kind = OUO_OBJ_BUILTIN_FN, .as.bifn = fn})
// #define _OUO_BI_OBJ(T, v) _ouo_obj_new_##T(v)
// #else
// #define _OUO_BI_OBJ(T, v)
// #endif // OUO_NOEMIT

// static OuoNameType _ouo_bi_nice_args[] = {
//     {.name = {.start = "s", .len = 1}, .type = {.kind = OUO_TYPE_INT}}};
// static OuoTypeRef _ouo_bi_nice_type = {
//     .as.t_fn = {.return_type = {.kind = OUO_TYPE_INT},
//         .params = {.items = _ouo_bi_nice_args, .count = 1}}};

// #ifndef OUO_NOEMIT
// static bool _ouo_bi_nice(
//     OuoObject *arg1, OuoObject *arg2, OuoObject *arg3, OuoObject *ret) {
//   (void)arg2, (void)arg3;
//   ouo_print("NICE %" OUO_PRId "!!!\n", arg1->as.v_int);
//   *ret = _ouo_obj_new_int(arg1->as.v_int * 2);
//   return true;
// }
// #endif // OUO_NOEMIT

// // Math

// static OuoNameType _ouo_bi_fcheck_args[] = {
//     {.name = {.start = "f", .len = 1}, .type = {.kind = OUO_TYPE_FLOAT}}};
// static OuoTypeRef _ouo_bi_fcheck_type = {
//     .as.t_fn = {.return_type = {.kind = OUO_TYPE_BOOL},
//         .params = {.items = _ouo_bi_fcheck_args, .count = 1}}};

// #ifndef OUO_NOEMIT
// static bool _ouo_bi_isinf(
//     OuoObject *arg1, OuoObject *arg2, OuoObject *arg3, OuoObject *ret) {
//   (void)arg2, (void)arg3;
//   *ret = _ouo_obj_new_bool(isinf(arg1->as.v_float));
//   return true;
// }

// static bool _ouo_bi_isnan(
//     OuoObject *arg1, OuoObject *arg2, OuoObject *arg3, OuoObject *ret) {
//   (void)arg2, (void)arg3;
//   *ret = _ouo_obj_new_bool(isnan(arg1->as.v_float));
//   return true;
// }

// static bool _ouo_bi_signbit(
//     OuoObject *arg1, OuoObject *arg2, OuoObject *arg3, OuoObject *ret) {
//   (void)arg2, (void)arg3;
//   *ret = _ouo_obj_new_bool(signbit(arg1->as.v_float));
//   return true;
// }
// #endif // OUO_NOEMIT

// static _OuoBuiltin _ouo_c_builtins[] = {
//     {"nice", {.kind = OUO_TYPE_FN, .ref = &_ouo_bi_nice_type},
//         _OUO_BI_OBJ(bifn, _ouo_bi_nice)},
//     // Math
//     {"INT_MIN", {.kind = OUO_TYPE_INT}, _OUO_BI_OBJ(int, OUO_INT_MIN)},
//     {"INT_MAX", {.kind = OUO_TYPE_INT}, _OUO_BI_OBJ(int, OUO_INT_MAX)},
//     {"INF", {.kind = OUO_TYPE_FLOAT}, _OUO_BI_OBJ(float, INFINITY)},
//     {"NAN", {.kind = OUO_TYPE_FLOAT}, _OUO_BI_OBJ(float, NAN)},
//     {"isinf", {.kind = OUO_TYPE_FN, .ref = &_ouo_bi_fcheck_type},
//         _OUO_BI_OBJ(bifn, _ouo_bi_isinf)},
//     {"isnan", {.kind = OUO_TYPE_FN, .ref = &_ouo_bi_fcheck_type},
//         _OUO_BI_OBJ(bifn, _ouo_bi_isnan)},
//     {"signbit", {.kind = OUO_TYPE_FN, .ref = &_ouo_bi_fcheck_type},
//         _OUO_BI_OBJ(bifn, _ouo_bi_signbit)},
// };

// static void _ouo_c_register_builtins(_OuoCompiler *c) {
//   size_t builtins_len = ouo_arr_len(_ouo_c_builtins);

//   for (size_t i = 0; i < builtins_len; i++) {
//     _OuoBuiltin *builtin = &_ouo_c_builtins[i];
//     OuoStringSlice name = {
//         .start = builtin->name, .len = strlen(builtin->name)};
//     OuoSymbol *added = _ouo_c_add_builtin_sym(c, &name, &builtin->type);
// #ifndef OUO_NOEMIT
//     if (added != NULL) _ouo_c_add_builtin_obj(c, &builtin->obj);
// #else
//     (void)added;
// #endif
//   }
// }

// void ouo_compile(OuoAst *ast, OuoCompileResult *res) {
//   _OuoCompiler c = {0};
//   _ouo_c_init(&c, res, 0);
//   if (c.res->builtin_syms.count == 0) _ouo_c_register_builtins(&c);

//   _ouo_c_ast_visit(&c, ast, NULL);

// #ifdef OUO_DEBUG
//   _ouo_c_dump(&c, ast);
// #endif
// }

// void ouo_c_res_free(OuoCompileResult *res) {
//   ouo_da_free(res->errors);
// #ifndef OUO_NOEMIT
//   _ouo_chunk_free(&res->chunk);
// #endif
// }

// static void _ouo_c_res_cleanup(OuoCompileResult *res) {
//   ouo_da_free(res->local_syms);
//   ouo_da_free(res->global_syms);
// }

// void ouo_c_res_cleanup(OuoCompileResult *res) {
//   _ouo_c_res_cleanup(res);
//   ouo_da_free(res->builtin_syms);

//   OUO_DA_FOREACH(OuoTypeRef, type_ref, &res->type_refs) {
//     _ouo_type_ref_free(type_ref);
//   }
//   ouo_da_free(res->type_refs);

// #ifndef OUO_NOEMIT
//   _ouo_chunk_cleanup(&res->chunk);
// #endif
// }

// #ifndef OUO_NOEMIT

// static void _ouo_chunk_free(OuoChunk *chunk) {
// #ifdef OUO_DEBUG
//   ouo_printdbg("freeing %.*s...\n", OUO_STRSL_FMT(chunk->name));
// #endif

//   OUO_DA_FOREACH(OuoObject, lit, &chunk->literals) { _ouo_obj_deref(lit); }
//   ouo_da_free(chunk->literals);
//   ouo_da_free(chunk->bytecode);
//   ouo_da_free(chunk->lines);
// }

// static void _ouo_chunk_cleanup(OuoChunk *chunk) {
//   OUO_DA_FOREACH(OuoObject, obj, &chunk->globals) { _ouo_obj_deref(obj); }
//   ouo_da_free(chunk->globals);

//   ouo_da_free(chunk->builtins);
// }

// #define _ouo_chunk_read_u8(ip) *(++(ip))
// #define _ouo_chunk_read_u16(ip) (ip += 2, (uint16_t)((ip[-1] << 8) | ip[0]))

// static inline void _ouo_obj_print(OuoObject *obj) {
//   switch (obj->kind) {
//     // Copy-on-write
//     case OUO_OBJ_INT: ouo_print("%" OUO_PRId, obj->as.v_int); break;
//     case OUO_OBJ_FLOAT: ouo_print("%" OUO_PRIf, obj->as.v_float); break;
//     case OUO_OBJ_BOOL: ouo_print(obj->as.v_bool ? "true" : "false"); break;
//     // Reference-counted
//     case OUO_OBJ_STR:
//       ouo_print("%.*s", OUO_STR_FMT(((OuoRcStr *)obj->as.ref)->str));
//       break;
//     case OUO_OBJ_FN: {
//       OuoRcFn *fn = (OuoRcFn *)obj->as.ref;
//       ouo_print("<fn %.*s>", OUO_STRSL_FMT(fn->chunk.name));
//       break;
//     }
//     // Builtin
//     case OUO_OBJ_BUILTIN_FN: ouo_print("<builtin>"); break;
//   }
// }

// #ifdef OUO_DEBUG

// static inline void _ouo_obj_dump(OuoObject *obj) {
//   if (obj == NULL) {
//     ouo_printdbg("(NULL)");
//     return;
//   }
//   if (obj->kind == OUO_OBJ_STR) ouo_printdbg("\"");
//   _ouo_obj_print(obj);
//   fflush(stdout);
//   if (obj->kind == OUO_OBJ_STR) ouo_printdbg("\"");
// }

// static const char *_ouo_op_code_str(OuoOpCode op_code) {
//   switch (op_code) {
//     // Objects
//     case OUO_OP_POP: return "POP";
//     case OUO_OP_POP_N: return "POP_N";
//     case OUO_OP_GET: return "GET";
//     case OUO_OP_SET: return "SET";
//     case OUO_OP_GET_GLOBAL: return "GET_GLOBAL";
//     case OUO_OP_GET_BUILTIN: return "GET_BUILTIN";
//     case OUO_OP_LIT: return "LIT";
//     case OUO_OP_PUSH_0: return "PUSH_0";
//     case OUO_OP_PUSH_1: return "PUSH_1";
//     case OUO_OP_PUSH_INT8: return "PUSH_INT8";
//     case OUO_OP_PUSH_TRUE: return "PUSH_TRUE";
//     case OUO_OP_PUSH_FALSE: return "PUSH_FALSE";

//     // Arithmetic
//     case OUO_OP_ADD_INT: return "ADD_INT";
//     case OUO_OP_ADD_FLOAT: return "ADD_FLOAT";
//     case OUO_OP_ADD_STR: return "ADD_STR";
//     case OUO_OP_SUB_INT: return "SUB_INT";
//     case OUO_OP_SUB_FLOAT: return "SUB_FLOAT";
//     case OUO_OP_MULT_INT: return "MULT_INT";
//     case OUO_OP_MULT_FLOAT: return "MULT_FLOAT";
//     case OUO_OP_DIV_INT: return "DIV_INT";
//     case OUO_OP_DIV_FLOAT: return "DIV_FLOAT";

//     case OUO_OP_NEG_INT: return "NEG_INT";
//     case OUO_OP_NEG_FLOAT: return "NEG_FLOAT";

//     // Comparison
//     case OUO_OP_EQ_INT: return "EQ_INT";
//     case OUO_OP_EQ_FLOAT: return "EQ_FLOAT";
//     case OUO_OP_EQ_BOOL: return "EQ_BOOL";
//     case OUO_OP_NEQ_INT: return "NEQ_INT";
//     case OUO_OP_NEQ_FLOAT: return "NEQ_FLOAT";
//     case OUO_OP_NEQ_BOOL: return "NEQ_BOOL";

//     case OUO_OP_LT_INT: return "LT_INT";
//     case OUO_OP_LT_FLOAT: return "LT_FLOAT";
//     case OUO_OP_LT_EQ_INT: return "LT_EQ_INT";
//     case OUO_OP_LT_EQ_FLOAT: return "LT_EQ_FLOAT";
//     case OUO_OP_GT_INT: return "GT_INT";
//     case OUO_OP_GT_FLOAT: return "GT_FLOAT";
//     case OUO_OP_GT_EQ_INT: return "GT_EQ_INT";
//     case OUO_OP_GT_EQ_FLOAT: return "GT_EQ_FLOAT";

//     // Logic
//     case OUO_OP_NOT: return "NOT";

//     // Control flow
//     case OUO_OP_JUMP: return "JUMP";
//     case OUO_OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
//     case OUO_OP_LOOP: return "LOOP";
//     case OUO_OP_CALL: return "CALL";
//     case OUO_OP_RETURN: return "RETURN";
//     case OUO_OP_RETURN_VOID: return "RETURN_VOID";

//     // Input/output
//     case OUO_OP_PRINT: return "PRINT";
//   }
//   return "";
// }

// static ptrdiff_t _ouo_chunk_op_dump(OuoChunk *chunk, uint8_t *ip) {
//   uint8_t *ip_prev = ip;
//   ptrdiff_t ip_idx = ip - chunk->bytecode.items;

//   ouo_printdbg("%04zd ", ip_idx);
//   size_t line_curr = _ouo_chunk_get_line(chunk, ip);
//   if (ip_idx > 0 && line_curr == _ouo_chunk_get_line(chunk, ip - 1))
//     ouo_printdbg("   | ");
//   else ouo_printdbg("%4zu ", line_curr);

//   OuoOpCode op_code = (OuoOpCode)*ip;
//   ouo_printdbg("%-16s", _ouo_op_code_str(op_code));

//   switch (op_code) {
//     // Objects
//     case OUO_OP_POP_N:
//     case OUO_OP_GET:
//     case OUO_OP_SET:
//     case OUO_OP_CALL:
//     case OUO_OP_RETURN:
//     case OUO_OP_RETURN_VOID: {
//       uint8_t u8 = _ouo_chunk_read_u8(ip);
//       ouo_printdbg("%4d ", u8);
//       break;
//     }
//     case OUO_OP_GET_GLOBAL: {
//       uint8_t global_idx = _ouo_chunk_read_u8(ip);
//       ouo_printdbg("%4d: ", global_idx);
//       _ouo_obj_dump(&chunk->globals.items[global_idx]);
//       break;
//     }
//     case OUO_OP_GET_BUILTIN: {
//       uint8_t builtin_idx = _ouo_chunk_read_u8(ip);
//       ouo_printdbg("%4d: ", builtin_idx);
//       _ouo_obj_dump(&chunk->builtins.items[builtin_idx]);
//       break;
//     }
//     case OUO_OP_LIT: {
//       uint8_t lit_idx = _ouo_chunk_read_u8(ip);
//       ouo_printdbg("%4d: ", lit_idx);
//       _ouo_obj_dump(&chunk->literals.items[lit_idx]);
//       break;
//     }
//     case OUO_OP_PUSH_INT8: {
//       int8_t int8 = (int8_t)_ouo_chunk_read_u8(ip);
//       ouo_printdbg("%4d ", int8);
//       break;
//     }
//     // Control flow
//     case OUO_OP_JUMP:
//     case OUO_OP_JUMP_IF_FALSE:
//     case OUO_OP_LOOP: {
//       uint16_t jump = _ouo_chunk_read_u16(ip);
//       ouo_printdbg("%4d: %td -> %td", jump, ip_idx,
//           ip_idx + 3 + jump * (op_code == OUO_OP_LOOP ? -1 : 1));
//       break;
//     }
//     default: break;
//   }

//   return ip - ip_prev;
// }

// static void _ouo_chunk_dump(OuoChunk *chunk) {
//   ouo_printdbg("CHUNK ");
//   if (chunk->name.start != NULL)
//     ouo_printdbg("%.*s:", OUO_STRSL_FMT(chunk->name));

//   ouo_printdbg("\nliterals: ");
//   for (size_t i = 0; i < chunk->literals.count; i++) {
//     ouo_printdbg("[%zu: ", i);
//     _ouo_obj_dump(&chunk->literals.items[i]);
//     ouo_printdbg("] ");
//   }
//   ouo_printdbg("\nglobals: ");
//   for (size_t i = 0; i < chunk->globals.count; i++) {
//     ouo_printdbg("[%zu: ", i);
//     _ouo_obj_dump(&chunk->globals.items[i]);
//     ouo_printdbg("] ");
//   }
//   ouo_printdbg("\n");

//   ouo_printdbg("lines: ");
//   for (size_t i = 0; i < chunk->lines.count; i += 2)
//     ouo_printdbg("%zu-%zu ", chunk->lines.items[i], chunk->lines.items[i +
//     1]);
//   ouo_printdbg("\n");

//   OUO_DA_FOREACH(uint8_t, ip, &chunk->bytecode) {
//     ip += _ouo_chunk_op_dump(chunk, ip);
//     ouo_printdbg("\n");
//   }

//   ouo_printdbg("---------------------------------------\n");
// }

// static void _ouo_c_dump(_OuoCompiler *c, OuoAst *ast) {
//   ouo_printdbg("local syms: ");
//   for (size_t i = 0; i < c->res->local_syms.count; i++) {
//     OuoSymbol *sym = &c->res->local_syms.items[i];
//     OuoString type_str = _ouo_type_str(&sym->type);
//     ouo_printdbg("[%zu '%.*s' %.*s (%zu)] ", i, OUO_STR_FMT(type_str),
//         OUO_STRSL_FMT(sym->name), sym->scope_depth);
//     ouo_da_free(type_str);
//   }
//   ouo_printdbg("\nglobal syms: ");
//   for (size_t i = 0; i < c->res->global_syms.count; i++) {
//     OuoSymbol *sym = &c->res->global_syms.items[i];
//     OuoString type_str = _ouo_type_str(&sym->type);
//     ouo_printdbg("[%zu '%.*s' %.*s (%zu)] ", i, OUO_STR_FMT(type_str),
//         OUO_STRSL_FMT(sym->name), sym->scope_depth);
//     ouo_da_free(type_str);
//   }
//   ouo_printdbg("\ntype refs: %zu\n", c->res->type_refs.count);
//   _ouo_ast_dump(ast);
//   ouo_printdbg("\n");

// #ifndef OUO_NOEMIT
//   _ouo_chunk_dump(&c->res->chunk);
//   ouo_printdbg("\n");
// #endif // OUO_NOEMIT
// }

// #endif // OUO_DEBUG

// //
// // Virtual machine
// //

// typedef struct {
//   OuoChunk *chunk;
//   uint8_t *ip;
//   uint8_t *end_ip;
//   OuoObject *stack;
// } _OuoCallFrame;

// typedef struct {
//   _OuoCallFrame frames[OUO_FRAMES_SIZE];
//   size_t frame_count;
//   OuoInterpretResult *res;
// } _OuoVm;

// #define _ouo_vm_err(vm, fr, err_code, ...) \
//   do { \
//     (vm)->res->failed = true; \
//     OuoError error = { \
//         .code = (err_code), \
//         .pos = {.line = _ouo_chunk_get_line((fr)->chunk, (fr)->ip), \
//             .line_start = NULL}, \
//         .msg = {0}, \
//     }; \
//     _ouo_err_sprintf(error, __VA_ARGS__); \
//     (vm)->res->error = error; \
//   } while (0)

// static inline void _ouo_vm_frame_init(
//     _OuoVm *vm, _OuoCallFrame *fr, OuoChunk *chunk, size_t stack_offset) {
//   fr->chunk = chunk;
//   fr->ip = chunk->bytecode.items;
//   fr->end_ip = fr->chunk->bytecode.items + fr->chunk->bytecode.count;
//   fr->stack = vm->res->stack.top - stack_offset;
// }

// static inline void _ouo_vm_init(
//     _OuoVm *vm, OuoInterpretResult *res, OuoChunk *chunk) {
//   res->failed = false;
//   vm->res = res;
//   vm->res->stack.top = vm->res->stack.items;
//   _OuoCallFrame *fr = &vm->frames[vm->frame_count++];
//   _ouo_vm_frame_init(vm, fr, chunk, 0);
// }

// static inline bool _ouo_obj_is_rc(OuoObject *obj) {
//   return obj->kind == OUO_OBJ_STR || obj->kind == OUO_OBJ_FN;
// }

// static inline void _ouo_obj_ref(OuoObject *obj) {
//   if (!_ouo_obj_is_rc(obj)) return;
// #ifdef OUO_DEBUG
//   ouo_printdbg("ref %zu -> %zu: ", obj->as.ref->count, obj->as.ref->count +
//   1); _ouo_obj_dump(obj); ouo_printdbg("\n");
// #endif
//   obj->as.ref->count++;
// }

// static inline void _ouo_obj_deref(OuoObject *obj) {
//   if (!_ouo_obj_is_rc(obj)) return;
//   OuoRc *rc = obj->as.ref;

// #ifdef OUO_DEBUG
//   ouo_printdbg("deref %zu -> %zu: ", rc->count, rc->count - 1);
//   _ouo_obj_dump(obj);
//   ouo_assertf(rc != NULL && rc->count > 0, OUO_ERR_RUNTIME,
//       "Trying to deref a freed object.");
// #endif

//   rc->count--;
//   if (rc->count > 0) {
// #ifdef OUO_DEBUG
//     ouo_printdbg("\n");
// #endif
//     return;
//   }

//   switch (obj->kind) {
//     // Copy-on-write
//     case OUO_OBJ_INT:
//     case OUO_OBJ_FLOAT:
//     case OUO_OBJ_BOOL: break;
//     // Reference-counted
//     case OUO_OBJ_STR: ouo_da_free(((OuoRcStr *)rc)->str); break;
//     case OUO_OBJ_FN: {
//       _ouo_chunk_free(&((OuoRcFn *)rc)->chunk);
//       break;
//     }
//     // Builtin
//     case OUO_OBJ_BUILTIN_FN: break;
//   }
//   ouo_free(rc);
//   obj->as.ref = NULL;

// #ifdef OUO_DEBUG
//   ouo_printdbg("    FREED!\n");
// #endif
//   return;
// }

// static inline void _ouo_vm_stack_push_noref(
//     _OuoVm *vm, _OuoCallFrame *fr, OuoObject *obj) {
// #ifdef OUO_DEBUG
//   if (vm->res->stack.top == vm->res->stack.items + OUO_VM_STACK_SIZE) {
//     _ouo_vm_err(vm, fr, OUO_ERR_RUNTIME,
//         "Maximum stack size exceeded (max %d).", OUO_VM_STACK_SIZE);
//     return;
//   }
// #else
//   (void)fr;
// #endif

//   *vm->res->stack.top = *obj;
//   vm->res->stack.top++;
// }

// static inline void _ouo_vm_stack_push(
//     _OuoVm *vm, _OuoCallFrame *fr, OuoObject *obj) {
//   _ouo_vm_stack_push_noref(vm, fr, obj);
//   _ouo_obj_ref(obj);
// }

// static inline OuoObject *_ouo_vm_stack_pop_noderef(
//     _OuoVm *vm, _OuoCallFrame *fr) {
// #ifdef OUO_DEBUG
//   if (vm->res->stack.top == vm->res->stack.items) {
//     _ouo_vm_err(vm, fr, OUO_ERR_RUNTIME, "Trying to pop empty stack.");
//     return NULL;
//   }
// #else
//   (void)fr;
// #endif

//   vm->res->stack.top--;
//   OuoObject *obj = vm->res->stack.top;
//   return obj;
// }

// static inline OuoObject *_ouo_vm_stack_pop(_OuoVm *vm, _OuoCallFrame *fr) {
//   OuoObject *obj = _ouo_vm_stack_pop_noderef(vm, fr);
//   _ouo_obj_deref(obj);
//   return obj;
// }

// static inline void _ouo_vm_stack_pop_n(
//     _OuoVm *vm, _OuoCallFrame *fr, uint8_t n) {
//   for (size_t i = 0; i < n; i++) _ouo_vm_stack_pop(vm, fr);
// }

// static inline OuoObject *_ouo_vm_stack_peek(
//     _OuoVm *vm, _OuoCallFrame *fr, size_t offset) {
// #ifdef OUO_DEBUG
//   if ((ptrdiff_t)offset + 1 > vm->res->stack.top - vm->res->stack.items) {
//     _ouo_vm_err(vm, fr, OUO_ERR_RUNTIME,
//         "Trying to peek beyond the stack (offset %td, stack size %zu).",
//         offset + 1, vm->res->stack.top - vm->res->stack.items);
//     return &vm->res->stack.items[0];
//   }
// #else
//   (void)fr;
// #endif

//   return &vm->res->stack.top[-offset - 1];
// }

// #define _OUO_VM_BIN_TO(vm, fr, T, OP, TO) \
//   do { \
//     ouo_##T##_t b = _ouo_vm_stack_pop((vm), (fr))->as.v_##T; \
//     ouo_##T##_t a = _ouo_vm_stack_pop((vm), (fr))->as.v_##T; \
//     _ouo_vm_stack_push((vm), (fr), &_ouo_obj_new_##TO(a OP b)); \
//   } while (0)

// #define _OUO_VM_BINARY(vm, fr, T, OP) _OUO_VM_BIN_TO(vm, fr, T, OP, T)

// #define _OUO_VM_UNARY(vm, fr, T, OP) \
//   do { \
//     ouo_##T##_t a = _ouo_vm_stack_pop((vm), (fr))->as.v_##T; \
//     _ouo_vm_stack_push((vm), (fr), &_ouo_obj_new_##T(OP a)); \
//   } while (0)

// static void _ouo_vm_run(_OuoVm *vm) {
//   _OuoCallFrame *fr = &vm->frames[vm->frame_count - 1];

// #ifdef OUO_DEBUG
//   if (fr->chunk->name.start != NULL)
//     ouo_printdbg("%.*s:\n", OUO_STRSL_FMT(fr->chunk->name));
// #endif

//   for (; fr->ip < fr->end_ip; (fr->ip)++) {
//     if (vm->res->failed) return;

// #ifdef OUO_DEBUG
//     _ouo_chunk_op_dump(fr->chunk, fr->ip);
//     ouo_printdbg("\n");
// #endif

//     OuoOpCode op = (OuoOpCode)(*fr->ip);
//     switch (op) {
//       // Objects
//       case OUO_OP_POP: _ouo_vm_stack_pop(vm, fr); break;
//       case OUO_OP_POP_N: {
//         uint8_t pop_count = _ouo_chunk_read_u8(fr->ip);
//         _ouo_vm_stack_pop_n(vm, fr, pop_count);
//         break;
//       }
//       case OUO_OP_GET: {
//         uint8_t idx = _ouo_chunk_read_u8(fr->ip);
//         _ouo_vm_stack_push(vm, fr, &fr->stack[idx]);
//         break;
//       }
//       case OUO_OP_SET: {
//         uint8_t idx = _ouo_chunk_read_u8(fr->ip);
//         _ouo_obj_deref(&fr->stack[idx]);
//         fr->stack[idx] = *_ouo_vm_stack_pop_noderef(vm, fr);
//         break;
//       }
//       case OUO_OP_GET_GLOBAL: {
//         OuoObject global =
//         fr->chunk->globals.items[_ouo_chunk_read_u8(fr->ip)];
//         _ouo_vm_stack_push(vm, fr, &global);
//         break;
//       }
//       case OUO_OP_GET_BUILTIN: {
//         OuoObject builtin =
//             fr->chunk->builtins.items[_ouo_chunk_read_u8(fr->ip)];
//         _ouo_vm_stack_push(vm, fr, &builtin);
//         break;
//       }
//       case OUO_OP_LIT: {
//         OuoObject lit =
//         fr->chunk->literals.items[_ouo_chunk_read_u8(fr->ip)];
//         _ouo_vm_stack_push(vm, fr, &lit);
//         break;
//       }
//       case OUO_OP_PUSH_0:
//         _ouo_vm_stack_push(vm, fr, &_ouo_obj_new_int(0));
//         break;
//       case OUO_OP_PUSH_1:
//         _ouo_vm_stack_push(vm, fr, &_ouo_obj_new_int(1));
//         break;
//       case OUO_OP_PUSH_INT8: {
//         int8_t int8 = (int8_t)_ouo_chunk_read_u8(fr->ip);
//         _ouo_vm_stack_push(vm, fr, &_ouo_obj_new_int(int8));
//         break;
//       }
//       case OUO_OP_PUSH_TRUE:
//         _ouo_vm_stack_push(vm, fr, &_ouo_obj_new_bool(true));
//         break;
//       case OUO_OP_PUSH_FALSE:
//         _ouo_vm_stack_push(vm, fr, &_ouo_obj_new_bool(false));
//         break;

//       // Arithmetic
//       case OUO_OP_ADD_INT: _OUO_VM_BINARY(vm, fr, int, +); break;
//       case OUO_OP_ADD_FLOAT: _OUO_VM_BINARY(vm, fr, float, +); break;
//       case OUO_OP_ADD_STR: {
//         OuoObject *b_obj = _ouo_vm_stack_pop_noderef(vm, fr);
//         OuoObject *a_obj = _ouo_vm_stack_pop_noderef(vm, fr);
//         OuoRcStr *b = (OuoRcStr *)b_obj->as.ref;
//         OuoRcStr *a = (OuoRcStr *)a_obj->as.ref;
//         OuoRcStr *rc = _ouo_rc_new_str();
//         ouo_da_append_many(&rc->str, a->str.items, a->str.count);
//         ouo_da_append_many(&rc->str, b->str.items, b->str.count);
//         _ouo_obj_deref(a_obj);
//         _ouo_obj_deref(b_obj);
//         _ouo_vm_stack_push(vm, fr, &_ouo_obj_new_str(rc));
//         break;
//       }
//       case OUO_OP_SUB_INT: _OUO_VM_BINARY(vm, fr, int, -); break;
//       case OUO_OP_SUB_FLOAT: _OUO_VM_BINARY(vm, fr, float, -); break;
//       case OUO_OP_MULT_INT: _OUO_VM_BINARY(vm, fr, int, *); break;
//       case OUO_OP_MULT_FLOAT: _OUO_VM_BINARY(vm, fr, float, *); break;
//       case OUO_OP_DIV_INT: _OUO_VM_BINARY(vm, fr, int, /); break;
//       case OUO_OP_DIV_FLOAT: _OUO_VM_BINARY(vm, fr, float, /); break;

//       case OUO_OP_NEG_INT: _OUO_VM_UNARY(vm, fr, int, -); break;
//       case OUO_OP_NEG_FLOAT: _OUO_VM_UNARY(vm, fr, float, -); break;

//       // Comparison
//       case OUO_OP_EQ_INT: _OUO_VM_BIN_TO(vm, fr, int, ==, bool); break;
//       case OUO_OP_EQ_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, ==, bool); break;
//       case OUO_OP_EQ_BOOL: _OUO_VM_BIN_TO(vm, fr, bool, ==, bool); break;
//       case OUO_OP_NEQ_INT: _OUO_VM_BIN_TO(vm, fr, int, !=, bool); break;
//       case OUO_OP_NEQ_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, !=, bool); break;
//       case OUO_OP_NEQ_BOOL: _OUO_VM_BIN_TO(vm, fr, bool, !=, bool); break;

//       case OUO_OP_LT_INT: _OUO_VM_BIN_TO(vm, fr, int, <, bool); break;
//       case OUO_OP_LT_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, <, bool); break;
//       case OUO_OP_LT_EQ_INT: _OUO_VM_BIN_TO(vm, fr, int, <=, bool); break;
//       case OUO_OP_LT_EQ_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, <=, bool);
//       break; case OUO_OP_GT_INT: _OUO_VM_BIN_TO(vm, fr, int, >, bool); break;
//       case OUO_OP_GT_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, >, bool); break;
//       case OUO_OP_GT_EQ_INT: _OUO_VM_BIN_TO(vm, fr, int, >=, bool); break;
//       case OUO_OP_GT_EQ_FLOAT: _OUO_VM_BIN_TO(vm, fr, float, >=, bool);
//       break;

//       // Logic
//       case OUO_OP_NOT: _OUO_VM_UNARY(vm, fr, bool, !); break;

//       // Control flow
//       case OUO_OP_JUMP: {
//         uint16_t jump = _ouo_chunk_read_u16(fr->ip);
//         fr->ip += jump;
//         break;
//       }
//       case OUO_OP_JUMP_IF_FALSE: {
//         uint16_t jump = _ouo_chunk_read_u16(fr->ip);
//         if (!_ouo_vm_stack_peek(vm, fr, 0)->as.v_bool) fr->ip += jump;
//         break;
//       }
//       case OUO_OP_LOOP: {
//         uint16_t jump = _ouo_chunk_read_u16(fr->ip);
//         fr->ip -= jump;
//         break;
//       }
//       case OUO_OP_CALL: {
//         OuoObject *obj = _ouo_vm_stack_pop(vm, fr);
//         uint8_t argc = _ouo_chunk_read_u8(fr->ip);

//         if (obj->kind == OUO_OBJ_FN) {
//           if (vm->frame_count == OUO_FRAMES_SIZE) {
//             _ouo_vm_err(vm, fr, OUO_ERR_RUNTIME, "Stack overflow.");
//             return;
//           }
//           OuoRcFn *fn = (OuoRcFn *)obj->as.ref;
//           _OuoCallFrame *new_fr = &vm->frames[vm->frame_count++];
//           _ouo_vm_frame_init(vm, new_fr, &fn->chunk, argc);
//           fr = &vm->frames[vm->frame_count - 1];
//           fr->ip--;

// #ifdef OUO_DEBUG
//           if (fr->chunk->name.start != NULL)
//             ouo_printdbg("%.*s:\n", OUO_STRSL_FMT(fr->chunk->name));
// #endif
//         } else {
//           OuoBuiltinFn builtin_fn = obj->as.bifn;
//           OuoObject *arg3 =
//               argc >= 3 ? _ouo_vm_stack_pop_noderef(vm, fr) : NULL;
//           OuoObject *arg2 =
//               argc >= 2 ? _ouo_vm_stack_pop_noderef(vm, fr) : NULL;
//           OuoObject *arg1 =
//               argc >= 1 ? _ouo_vm_stack_pop_noderef(vm, fr) : NULL;
//           OuoObject ret = {0};
//           bool has_ret = builtin_fn(arg1, arg2, arg3, &ret);
//           if (has_ret) _ouo_vm_stack_push(vm, fr, &ret);
//         }

//         break;
//       }
//       case OUO_OP_RETURN:
//       case OUO_OP_RETURN_VOID: {
//         OuoObject *res =
//             op != OUO_OP_RETURN_VOID ? _ouo_vm_stack_pop_noderef(vm, fr) :
//             NULL;

//         uint8_t pop_count = _ouo_chunk_read_u8(fr->ip);
//         _ouo_vm_stack_pop_n(vm, fr, pop_count);

//         vm->frame_count--;
//         if (vm->frame_count == 0) return;
//         if (op != OUO_OP_RETURN_VOID) _ouo_vm_stack_push_noref(vm, fr, res);
//         fr = &vm->frames[vm->frame_count - 1];

// #ifdef OUO_DEBUG
//         if (fr->chunk->name.start != NULL)
//           ouo_printdbg("%.*s:\n", OUO_STRSL_FMT(fr->chunk->name));
// #endif
//         break;
//       }

//       // Input/output
//       case OUO_OP_PRINT:
//         _ouo_obj_print(_ouo_vm_stack_peek(vm, fr, 0));
//         ouo_print("\n");
//         break;
//     }

// #ifdef OUO_DEBUG
//     if (vm->res->stack.top != vm->res->stack.items) {
//       for (OuoObject *obj = vm->res->stack.items; obj < vm->res->stack.top;
//           obj++) {
//         ouo_printdbg("[");
//         _ouo_obj_dump(obj);
//         ouo_printdbg("] ");
//       }
//       ouo_printdbg("\n");
//     }
// #endif
//   }

// #ifdef OUO_DEBUG
//   ouo_printdbg("\n");
// #endif
// }

// void ouo_interpret(OuoChunk *chunk, OuoInterpretResult *res) {
//   _OuoVm vm = {0};
//   _ouo_vm_init(&vm, res, chunk);

//   _ouo_vm_run(&vm);
// }

// void ouo_i_res_cleanup(OuoInterpretResult *res) {
//   for (OuoObject *obj = res->stack.items; obj < res->stack.top; obj++)
//     _ouo_obj_deref(obj);
// }

// #endif // OUO_NOEMIT

#endif // OUO_IMPLEMENTATION
