///
/// The ouo language
///

#include "ouo.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

//
// Error handling
//

static const char *err_code_str(OuoErrorCode err_code) {
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
  ouo_printerr("%zu:%zu: %s: %s\n", line + 1, col + 1, err_code_str(err->code),
      err->msg);

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
} OuoLexer;

static inline void l_init(OuoLexer *l, const char *src) {
  l->start = src;
  l->current = src;
}

static inline bool l_is_eof(OuoLexer *l) { return *l->current == '\0'; }

static inline bool l_is_digit(char c) { return c >= '0' && c <= '9'; }

static inline char l_advance(OuoLexer *l) {
  l->current++;
  return l->current[-1];
}

static inline char l_peek(OuoLexer *l) { return *l->current; }

static inline char l_peek_next(OuoLexer *l) {
  if (l_is_eof(l)) return '\0';
  return l->current[1];
}

static inline void l_skip_whitespace(OuoLexer *l) {
  for (;;) {
    char c = l_peek(l);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') l_advance(l);
    else return;
  }
}

static inline OuoToken l_tok_new(OuoLexer *l, OuoTokenKind kind) {
  OuoToken tok = {
      .kind = kind,
      .start = l->start,
      .len = (size_t)(l->current - l->start),
  };
  return tok;
}

static OuoToken l_read_number(OuoLexer *l) {
  OuoTokenKind kind = OUO_TOK_LIT_INT;

  while (l_is_digit(l_peek(l))) l_advance(l);

  if (l_peek(l) == '.' && l_is_digit(l_peek_next(l))) {
    kind = OUO_TOK_LIT_FLOAT;
    l_advance(l);

    while (l_is_digit(l_peek(l))) l_advance(l);
  }

  return l_tok_new(l, kind);
}

static OuoToken l_next_token(OuoLexer *l) {
  l_skip_whitespace(l);
  l->start = (l->current);

  if (l_is_eof(l)) return l_tok_new(l, OUO_TOK_EOF);

  char c = l_advance(l);

  // Literals
  if (l_is_digit(c)) return l_read_number(l);

  switch (c) {
    // Operators
    case '+': return l_tok_new(l, OUO_TOK_PLUS);
    case '*': return l_tok_new(l, OUO_TOK_ASTERISK);
  }

  return l_tok_new(l, OUO_TOK_ILLEGAL);
}

static const char *tok_kind_str(OuoTokenKind kind) {
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
}

static void l_tok_dump(OuoLexer *l) {
  for (;;) {
    OuoToken tok = l_next_token(l);
    ouo_printdbg(
        "[ %s '%.*s'] ", tok_kind_str(tok.kind), (int)tok.len, tok.start);
    if (tok.kind == OUO_TOK_EOF) break;
  }
}

//
// Parsing
//

struct OuoParseRule;

typedef struct {
  OuoLexer *l;
  OuoToken curr;
  OuoToken peek;

  bool failed;
  OuoErrors errors;

  struct {
    const struct OuoParseRule *items;
    size_t count;
  } rules;
} OuoParser;

typedef OuoAst *(*OuoParsePrefixFn)(OuoParser *p);
typedef OuoAst *(*OuoParseInfixFn)(OuoParser *p, OuoAst *left);

typedef enum {
  OUO_PREC_LOWEST,
  OUO_PREC_SUM,     // +
  OUO_PREC_PRODUCT, // *
} OuoPrecedence;

typedef struct OuoParseRule {
  OuoParsePrefixFn prefix_fn;
  OuoParseInfixFn infix_fn;
  OuoPrecedence prec;
} OuoParseRule;

static inline void p_advance(OuoParser *p) {
  p->curr = p->peek;
  p->peek = l_next_token(p->l);
}

static inline void p_init(
    OuoParser *p, OuoLexer *l, const OuoParseRule *rules, size_t rules_count) {
  p->l = l;
  p->rules.items = rules;
  p->rules.count = rules_count;
  p_advance(p);
  p_advance(p);
}

#define p_err(p, tok, err_code, fmt, ...) \
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

#define p_assert(p, expr, tok, err_code, fmt, ...) \
  do { \
    if (!(expr)) { \
      p_err(p, tok, err_code, fmt, ##__VA_ARGS__); \
      return NULL; \
    } \
  } while (0)

#define p_assert_defer(p, expr, tok, err_code, fmt, ...) \
  do { \
    if (!(expr)) { \
      p_err(p, tok, err_code, fmt, ##__VA_ARGS__); \
      goto errdefer; \
    } \
  } while (0)

static inline OuoAst *p_ast_new(OuoParser *p, OuoAstKind kind) {
  OuoAst *ast = ouo_alloc(sizeof(OuoAst));
  ouo_assert_nomem(ast);
  ast->kind = kind;
  ast->start = p->curr.start;
  ast->len = p->curr.len;
  return ast;
}

static inline const OuoParseRule *p_get_rule(OuoParser *p, OuoTokenKind tok) {
  if (tok >= p->rules.count) return &p->rules.items[OUO_TOK_EOF];
  return &p->rules.items[tok];
}

static OuoAst *p_expression(OuoParser *p, OuoPrecedence prec) {
  OuoParsePrefixFn prefix_fn = p_get_rule(p, p->curr.kind)->prefix_fn;
  p_assert(&p, prefix_fn != NULL, p->curr, OUO_ERR_SYNTAX,
      "Expected an expression, got '%.*s'.", (int)p->curr.len, p->curr.start);

  OuoAst *left = prefix_fn(p);

  while (p->peek.kind != OUO_TOK_EOF &&
         prec <= p_get_rule(p, p->peek.kind)->prec) {
    OuoParseInfixFn infix_fn = p_get_rule(p, p->peek.kind)->infix_fn;
    p_assert_defer(&p, infix_fn != NULL, p->peek, OUO_ERR_SYNTAX,
        "Expected a binary operator, got '%.*s'.", (int)p->peek.len,
        p->peek.start);

    p_advance(p);
    left = infix_fn(p, left);
  }

  return left;

errdefer:
  ouo_ast_free(left);
  return NULL;
}

static OuoAst *p_lit_int(OuoParser *p) {
  errno = 0;
  char *end = NULL;
  ouo_int_t lit = ouo_strtoi(p->curr.start, &end, 10);

  p_assert(&p, errno == 0, p->curr, OUO_ERR_PARSING_FAILED,
      "Integer literal value out of range (min %" OUO_PRId ", max %" OUO_PRId
      ").",
      OUO_INT_MIN, OUO_INT_MAX, LONG_MAX);
  p_assert(&p, end == p->curr.start + p->curr.len, p->curr,
      OUO_ERR_PARSING_FAILED,
      "Integer literal length mismatch. Expected %zu, read %zu.", p->curr.len,
      end - p->curr.start);

  OuoAst *ast = p_ast_new(p, OUO_AST_LIT_INT);
  ast->lit_int = lit;
  return ast;
}

static OuoAst *p_lit_float(OuoParser *p) {
  errno = 0;
  char *end = NULL;
  ouo_float_t lit = strtod(p->curr.start, &end);

  p_assert(&p, errno == 0, p->curr, OUO_ERR_PARSING_FAILED, "%s.",
      "Float literal value out of range.");
  p_assert(&p, end == p->curr.start + p->curr.len, p->curr,
      OUO_ERR_PARSING_FAILED,
      "Float literal length mismatch. Expected %zu, read %zu.", p->curr.len,
      end - p->curr.start);

  OuoAst *ast = p_ast_new(p, OUO_AST_LIT_FLOAT);
  ast->lit_float = lit;
  return ast;
}

static OuoAst *p_bin_op(OuoParser *p, OuoAst *left) {
  OuoTokenKind op = p->curr.kind;
  OuoPrecedence prec = p_get_rule(p, op)->prec;
  p_advance(p);
  OuoAst *right = p_expression(p, prec);

  OuoAst *ast = p_ast_new(p, OUO_AST_BIN_OP);
  ast->bin_op.left = left;
  ast->bin_op.op = op;
  ast->bin_op.right = right;
  return ast;
}

static const OuoParseRule p_rules[] = {
    [OUO_TOK_EOF] = {NULL, NULL, OUO_PREC_LOWEST},
    // Literals
    [OUO_TOK_LIT_INT] = {p_lit_int, NULL, OUO_PREC_LOWEST},
    [OUO_TOK_LIT_FLOAT] = {p_lit_float, NULL, OUO_PREC_LOWEST},
    // Operators
    [OUO_TOK_PLUS] = {NULL, p_bin_op, OUO_PREC_SUM},
    [OUO_TOK_ASTERISK] = {NULL, p_bin_op, OUO_PREC_PRODUCT},
};

OuoParseResult ouo_parse(const char *src) {
  OuoLexer l = {0};
  l_init(&l, src);

  l_tok_dump(&l);
  ouo_printdbg("\n");
  l_init(&l, src);

  l_init(&l, src);
  OuoParser p = {0};
  p_init(&p, &l, p_rules, ouo_arr_len(p_rules));

  OuoAst *expr = p_expression(&p, OUO_PREC_LOWEST);

  OuoParseResult res = {
      .failed = p.failed,
      .ast = expr,
      .errors = p.errors,
  };

  return res;
}

static const char *ast_kind_str(OuoAstKind kind) {
  switch (kind) {
    // Literals
    case OUO_AST_LIT_INT: return "LIT_INT";
    case OUO_AST_LIT_FLOAT: return "LIT_FLOAT";
    // Expressions
    case OUO_AST_BIN_OP: return "BIN_OP";
  }
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

  ouo_printdbg("( %s ", ast_kind_str(ast->kind));

  switch (ast->kind) {
    // Literals
    case OUO_AST_LIT_INT: ouo_printdbg("%ld", ast->lit_int); break;
    case OUO_AST_LIT_FLOAT: ouo_printdbg("%f", ast->lit_float); break;
    // Expressions
    case OUO_AST_BIN_OP:
      ouo_ast_dump(ast->bin_op.left);
      ouo_printdbg(" %s ", tok_kind_str(ast->bin_op.op));
      ouo_ast_dump(ast->bin_op.right);
      break;
  }

  ouo_printdbg(")");
}
