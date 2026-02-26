/*
 * The ouo language
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "ouo.h"

/* Lexing */

typedef struct {
  const char *start;
  const char *current;
} OuoLexer;

static void l_init(OuoLexer *l, const char *src) {
  l->start = src;
  l->current = src;
}

static bool l_is_eof(OuoLexer *l) { return *l->current == '\0'; }

static bool l_is_digit(char c) { return c >= '0' && c <= '9'; }

static char l_advance(OuoLexer *l) {
  l->current++;
  return l->current[-1];
}

static char l_peek(OuoLexer *l) { return *l->current; }

static char l_peek_next(OuoLexer *l) {
  if (l_is_eof(l)) return '\0';
  return l->current[1];
}

static void l_skip_whitespace(OuoLexer *l) {
  for (;;) {
    char c = l_peek(l);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') l_advance(l);
    else return;
  }
}

static OuoToken l_new_token(OuoLexer *l, OuoTokenKind kind) {
  OuoToken tok = {
      .kind = kind,
      .start = l->start,
      .len = (size_t)(l->current - l->start),
  };
  return tok;
}

static OuoToken l_read_number(OuoLexer *l) {
  OuoTokenKind kind = OUO_TOK_LITERAL_INT;

  while (l_is_digit(l_peek(l))) l_advance(l);

  if (l_peek(l) == '.' && l_is_digit(l_peek_next(l))) {
    kind = OUO_TOK_LITERAL_FLOAT;
    l_advance(l);

    while (l_is_digit(l_peek(l))) l_advance(l);
  }

  return l_new_token(l, kind);
}

static OuoToken l_next_token(OuoLexer *l) {
  l_skip_whitespace(l);
  l->start = l->current;

  if (l_is_eof(l)) return l_new_token(l, OUO_TOK_EOF);

  char c = l_advance(l);

  // Literals
  if (l_is_digit(c)) return l_read_number(l);

  switch (c) {
  // Operators
  case '+': return l_new_token(l, OUO_TOK_PLUS);
  case '*': return l_new_token(l, OUO_TOK_ASTERISK);
  }

  return l_new_token(l, OUO_TOK_ILLEGAL);
}

static void tok_kind_print(OuoTokenKind kind) {
  switch (kind) {
  case OUO_TOK_EOF: printf("EOF"); break;
  case OUO_TOK_ILLEGAL: printf("ILL"); break;
  // Literals
  case OUO_TOK_LITERAL_INT: printf("INT"); break;
  case OUO_TOK_LITERAL_FLOAT: printf("FLOAT"); break;
  // Operators
  case OUO_TOK_PLUS: printf("+"); break;
  case OUO_TOK_ASTERISK: printf("*"); break;
  }
}

/* Parsing */

struct OuoParseRule;

typedef struct {
  OuoLexer *l;
  OuoToken curr;
  OuoToken peek;

  struct {
    struct OuoParseRule *items;
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

static void p_advance(OuoParser *p) {
  p->curr = p->peek;
  p->peek = l_next_token(p->l);
}

static void p_init(OuoParser *p, OuoLexer *l, OuoParseRule *rules,
                   size_t rules_count) {
  p->l = l;
  p->rules.items = rules;
  p->rules.count = rules_count;
  p_advance(p);
  p_advance(p);
}

static OuoAst *p_ast_new(OuoParser *p, OuoAstKind kind) {
  OuoAst *ast = malloc(sizeof(OuoAst));
  ast->kind = kind;
  ast->start = p->curr.start;
  ast->len = p->curr.len;
  return ast;
}

static OuoParseRule *p_get_rule(OuoParser *p, OuoTokenKind tok) {
  if (tok >= p->rules.count) return &p->rules.items[OUO_TOK_EOF];
  return &p->rules.items[tok];
}

static OuoAst *p_expression(OuoParser *p, OuoPrecedence prec) {
  OuoParsePrefixFn prefix_fn = p_get_rule(p, p->curr.kind)->prefix_fn;
  ouo_assert(prefix_fn != NULL, OUO_ERR_SYNTAX_NO_PREFIX_FN,
             "no prefix fn for '%.*s'", (int)p->curr.len, p->curr.start);

  OuoAst *left = prefix_fn(p);

  while (prec < p_get_rule(p, p->peek.kind)->prec) {
    OuoParseInfixFn infix_fn = p_get_rule(p, p->peek.kind)->infix_fn;
    ouo_assert(infix_fn != NULL, OUO_ERR_SYNTAX_NO_INFIX_FN,
               "no infix fn for '%.*s'", (int)p->peek.len, p->peek.start);

    p_advance(p);
    left = infix_fn(p, left);
  }

  return left;
}

static OuoAst *p_lit_int(OuoParser *p) {
  OuoAst *ast = p_ast_new(p, OUO_AST_LITERAL_INT);
  ast->lit_int = strtol(p->curr.start, NULL, 10);
  return ast;
}

static OuoAst *p_lit_float(OuoParser *p) {
  OuoAst *ast = p_ast_new(p, OUO_AST_LITERAL_FLOAT);
  ast->lit_float = strtod(p->curr.start, NULL);
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

static OuoParseRule p_rules[] = {
    [OUO_TOK_EOF] = {NULL, NULL, OUO_PREC_LOWEST},
    // Literals
    [OUO_TOK_LITERAL_INT] = {p_lit_int, NULL, OUO_PREC_LOWEST},
    [OUO_TOK_LITERAL_FLOAT] = {p_lit_float, NULL, OUO_PREC_LOWEST},
    // Operators
    [OUO_TOK_PLUS] = {NULL, p_bin_op, OUO_PREC_SUM},
    [OUO_TOK_ASTERISK] = {NULL, p_bin_op, OUO_PREC_PRODUCT},
};

void ouo_parse(const char *src) {
  OuoLexer l = {0};
  l_init(&l, src);

  for (;;) {
    OuoToken tok = l_next_token(&l);
    printf("[");
    tok_kind_print(tok.kind);
    printf(" '%.*s'] ", (int)tok.len, tok.start);
    if (tok.kind == OUO_TOK_EOF) break;
  }

  printf("\n");

  l_init(&l, src);
  OuoParser p = {0};
  p_init(&p, &l, p_rules, ouo_arr_len(p_rules));
  OuoAst *expr = p_expression(&p, OUO_PREC_LOWEST);

  ouo_ast_print(expr);
  printf("\n");
}

static void ast_kind_print(OuoAstKind kind) {
  switch (kind) {
  // Literals
  case OUO_AST_LITERAL_INT: printf("int"); break;
  case OUO_AST_LITERAL_FLOAT: printf("float"); break;
  // Expressions
  case OUO_AST_BIN_OP: printf("bin_op"); break;
  }
}

void ouo_ast_print(OuoAst *ast) {
  printf("(");
  ast_kind_print(ast->kind);
  printf(" ");

  switch (ast->kind) {
  // Literals
  case OUO_AST_LITERAL_INT:
  case OUO_AST_LITERAL_FLOAT:
    printf("'%.*s'", (int)ast->len, ast->start);
    break;
  // Expressions
  case OUO_AST_BIN_OP:
    ouo_ast_print(ast->bin_op.left);
    printf(" ");
    tok_kind_print(ast->bin_op.op);
    printf(" ");
    ouo_ast_print(ast->bin_op.right);
    break;
  }

  printf(")");
}
