/*
 * The ouo language
 */

#ifndef OUO_H_
#define OUO_H_

#include <stddef.h>
#include <stdint.h>

/* Helper macros */

#define ouo_arr_len(arr) (sizeof(arr) / sizeof((arr)[0]))

#define OUO_STRINGIZE(x) #x
#define OUO_XSTRINGIZE(x) OUO_STRINGIZE(x)

#define ouo_abort(expr, err_code, err_msg, ...) \
  do { \
    fprintf(stderr, err_msg "\n", ##__VA_ARGS__); \
    exit(err_code); \
  } while (0)

#define ouo_assertf(expr, err_code, err_msg, ...) \
  if (!(expr)) ouo_abort(expr, err_code, err_msg, ##__VA_ARGS__)

#define ouo_assert(expr, err_code, err_msg, ...) \
  ouo_assertf(expr, err_code, \
              __FILE__ ":" OUO_XSTRINGIZE(__LINE__) ": " err_msg, \
              ##__VA_ARGS__)

/* Error handling */

typedef enum {
  OUO_OK,
  OUO_ERR_OUT_OF_MEMORY,
  OUO_ERR_INCORRECT_USAGE,
  OUO_ERR_FILE_NOT_READ,
  OUO_ERR_SYNTAX_NO_PREFIX_FN,
  OUO_ERR_SYNTAX_NO_INFIX_FN,
} OuoErrorCode;

/* Data types */

typedef int64_t ouo_int_t;
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

void ouo_parse(const char *src);
void ouo_ast_print(OuoAst *ast);

#endif // OUO_H_
