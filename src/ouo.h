///
/// The ouo language
///

#ifndef OUO_H
#define OUO_H

#include <stdbool.h>
#include <stddef.h>

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

#define ouo_alloc(...) malloc(__VA_ARGS__)
#define ouo_realloc(...) realloc(__VA_ARGS__)
#define ouo_free(...) free(__VA_ARGS__)

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

#define OUO_INT_MIN LONG_MIN
#define OUO_INT_MAX LONG_MAX
#define ouo_strtoi(...) strtol(__VA_ARGS__)
#define OUO_PRId "ld"

typedef long ouo_int_t;
typedef double ouo_float_t;

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

/// Caller must free the result's `ast` and `errors.items`.
OuoParseResult ouo_parse(const char *src);

/// Recursively frees the entire AST tree.
void ouo_ast_free(OuoAst *ast);

/// Prints the AST tree for debugging.
void ouo_ast_dump(OuoAst *ast);

#endif // OUO_H
