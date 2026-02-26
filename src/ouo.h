/*
 * The ouo language
 */

#ifndef OUO_H_
#define OUO_H_

#include <stddef.h>

/* Helper macros */

#define STRINGIZE(x) #x
#define XSTRINGIZE(x) STRINGIZE(x)

#define ouo_abort(expr, err_code, err_msg, ...) \
  do { \
    fprintf(stderr, err_msg "\n", ##__VA_ARGS__); \
    exit(err_code); \
  } while (0)

#define ouo_assertf(expr, err_code, err_msg, ...) \
  if (!(expr)) ouo_abort(expr, err_code, err_msg, ##__VA_ARGS__)

#define ouo_assert(expr, err_code, err_msg, ...) \
  ouo_assertf(expr, err_code, __FILE__ ":" XSTRINGIZE(__LINE__) ": " err_msg, \
              ##__VA_ARGS__)

/* Error handling */

typedef enum {
  OUO_OK,
  OUO_ERR_OUT_OF_MEMORY,
  OUO_ERR_FILE_NOT_READ,
  OUO_ERR_INCORRECT_USAGE,
} OuoErrorCode;

/* Lexing */

typedef enum {
  OUO_TOK_EOF,
  OUO_TOK_ILLEGAL,
  // Literals
  OUO_TOK_LITERAL_INT,
  OUO_TOK_LITERAL_FLOAT,
} OuoTokenKind;

typedef struct {
  OuoTokenKind kind;
  const char *start;
  int len;
} OuoToken;

/* Parsing */

void ouo_parse(const char *src);

#endif // OUO_H_
