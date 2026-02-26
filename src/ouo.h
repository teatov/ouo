/*
 * The ouo language
 */

#ifndef OUO_H_
#define OUO_H_

#include <stdbool.h>
#include <stddef.h>

/* Helpers */

#define OUO_STRINGIZE(x) #x
#define OUO_XSTRINGIZE(x) OUO_STRINGIZE(x)

// Arrays

#define ouo_arr_len(arr) (sizeof(arr) / sizeof((arr)[0]))

#define OUO_DA_INIT_CAPACITY 8

#define ouo_da_reserve(da, expected_capacity) \
  do { \
    if ((expected_capacity) > (da)->capacity) { \
      if ((da)->capacity == 0) (da)->capacity = OUO_DA_INIT_CAPACITY; \
      while ((expected_capacity) > (da)->capacity) (da)->capacity *= 2; \
      (da)->items = \
          realloc((da)->items, (da)->capacity * sizeof(*(da)->items)); \
      ouo_assert((da)->items != NULL, OUO_ERR_OUT_OF_MEMORY, \
                 "Cannot allocate memory for array '" #da "'."); \
    } \
  } while (0)

#define ouo_da_append(da, item) \
  do { \
    ouo_da_reserve((da), (da)->count + 1); \
    (da)->items[(da)->count] = (item); \
    (da)->count++; \
  } while (0)

#define ouo_da_free(da) free((da).items)

#define ouo_da_foreach(Type, it, da) \
  for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

// Asserting

#define ouo_abort(expr, err_code, fmt, ...) \
  do { \
    fprintf(stderr, fmt "\n", ##__VA_ARGS__); \
    exit(err_code); \
  } while (0)

#define ouo_assertf(expr, err_code, fmt, ...) \
  if (!(expr)) ouo_abort(expr, err_code, fmt, ##__VA_ARGS__)

#define ouo_assert(expr, err_code, fmt, ...) \
  ouo_assertf(expr, err_code, __FILE__ ":" OUO_XSTRINGIZE(__LINE__) ": " fmt, \
              ##__VA_ARGS__)

/* Error handling */

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

typedef struct {
  struct OuoError *items;
  size_t count;
  size_t capacity;
} OuoErrors;

void ouo_err_print(OuoError *err, const char *src, const char *path);

/* Data types */

#define OUO_INT_MIN LONG_MIN
#define OUO_INT_MAX LONG_MAX
#define ouo_strtoi strtol
#define OUO_PRId "ld"

typedef long ouo_int_t;
typedef double ouo_float_t;

/* Lexing */

typedef enum {
  OUO_TOK_EOF,
  OUO_TOK_ILLEGAL,
  // Literals
  OUO_TOK_LITERAL_INT,
  OUO_TOK_LITERAL_FLOAT,
  // Operators
  OUO_TOK_PLUS,
  OUO_TOK_ASTERISK,
} OuoTokenKind;

typedef struct {
  OuoTokenKind kind;
  const char *start;
  size_t len;
} OuoToken;

/* Parsing */

typedef enum {
  // Literals
  OUO_AST_LITERAL_INT,
  OUO_AST_LITERAL_FLOAT,
  // Expressions
  OUO_AST_BIN_OP,
} OuoAstKind;

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

typedef struct {
  bool failed;
  OuoAst *ast;
  OuoErrors errors;
} OuoParseResult;

OuoParseResult ouo_parse(const char *src);

void ouo_ast_free(OuoAst *ast);
void ouo_ast_print(OuoAst *ast);

#endif // OUO_H_
