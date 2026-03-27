#ifndef TEST_H
#define TEST_H

// #undef OUO_DEBUG
#define OUO_IMPLEMENTATION
#include "../src/ouo.h"

static int total = 0;
static int passes = 0;

#define _TEST_PASS OUO_EBGRN "PASS" OUO_ER
#define _TEST_FAIL OUO_EBRED "FAIL" OUO_ER

#define _ast_eq_assert(ast, expr, fmt, exp, got) \
  do { \
    if (!(expr)) { \
      ouo_printerr( \
          _TEST_FAIL " " #expr ": expected " fmt ", got " fmt "\n", exp, got); \
      return false; \
    } \
  } while (0)

static bool _ast_eq(OuoAst *exp, OuoAst *got) {
  if (exp == NULL && got == NULL) return true;
  if (exp == NULL) {
    ouo_printerr("exp is NULL\n");
    return false;
  }
  if (got == NULL) {
    ouo_printerr("got is NULL\n");
    return false;
  }

  _ast_eq_assert(got, exp->kind == got->kind, "%d", exp->kind, got->kind);

  switch (exp->kind) {
    case OUO_AST_MODULE:
    case OUO_AST_BLOCK:
      _ast_eq_assert(got, exp->children.count == got->children.count, "%zu",
          exp->children.count, got->children.count);
      for (size_t i = 0; i < exp->children.count; i++)
        if (!_ast_eq(exp->children.items[i], got->children.items[i]))
          return false;
      break;
    case OUO_AST_IDENT:
      _ast_eq_assert(got, _ouo_tok_eq(&exp->k.ident.name, &got->k.ident.name),
          "%.*s", _OUO_TOK_FMT_ARGS(exp->k.ident.name),
          _OUO_TOK_FMT_ARGS(got->k.ident.name));
      break;

    // Literals
    case OUO_AST_LIT_INT:
      _ast_eq_assert(got, exp->k.lit_int == got->k.lit_int, "%" OUO_PRId,
          exp->k.lit_int, got->k.lit_int);
      break;
    case OUO_AST_LIT_FLOAT:
      _ast_eq_assert(got, exp->k.lit_float == got->k.lit_float, "%" OUO_PRIf,
          exp->k.lit_float, got->k.lit_float);
      break;
    case OUO_AST_LIT_BOOL:
      _ast_eq_assert(got, exp->k.lit_bool == got->k.lit_bool, "%d",
          exp->k.lit_bool, got->k.lit_bool);
      break;

    // Expressions
    case OUO_AST_ASSIGN:
      if (!_ast_eq(exp->k.assign.target, got->k.assign.target)) return false;
      if (!_ast_eq(exp->k.assign.value, got->k.assign.value)) return false;
      break;
    case OUO_AST_BINARY:
      _ast_eq_assert(got, exp->k.binary.op == got->k.binary.op, "%s",
          _ouo_tok_kind_str(exp->k.binary.op),
          _ouo_tok_kind_str(got->k.binary.op));
      if (!_ast_eq(exp->k.binary.left, got->k.binary.left)) return false;
      if (!_ast_eq(exp->k.binary.right, got->k.binary.right)) return false;
      break;
    case OUO_AST_UNARY:
      _ast_eq_assert(got, exp->k.unary.op == got->k.unary.op, "%s",
          _ouo_tok_kind_str(exp->k.unary.op),
          _ouo_tok_kind_str(got->k.unary.op));
      if (!_ast_eq(exp->k.unary.right, got->k.unary.right)) return false;
      break;
    case OUO_AST_IF:
      if (!_ast_eq(exp->k.if_expr.condition, got->k.if_expr.condition))
        return false;
      if (!_ast_eq(exp->k.if_expr.then_branch, got->k.if_expr.then_branch))
        return false;
      if (!_ast_eq(exp->k.if_expr.else_branch, got->k.if_expr.else_branch))
        return false;
      break;

    // Statements
    case OUO_AST_EXPR_STMT:
    case OUO_AST_PRINT:
      if (!_ast_eq(exp->k.child, got->k.child)) return false;
      break;
    case OUO_AST_DECL_VAR:
      _ast_eq_assert(got,
          _ouo_tok_eq(&exp->k.decl_var.name, &got->k.decl_var.name), "%.*s",
          _OUO_TOK_FMT_ARGS(exp->k.decl_var.name),
          _OUO_TOK_FMT_ARGS(got->k.decl_var.name));
      if (!_ast_eq(exp->k.decl_var.value, got->k.decl_var.value)) return false;
      break;
  }

  return true;
}

#define TN(name) OUO_ED OUO_CODEPOS OUO_ER name

typedef struct {
  bool fail;
  OuoAst *exp_ast;
  OuoAst *exp_ast_expr;
  OuoAst *exp_ast_stmt;
} TestOptions;

static inline void test(const char *name, const char *src, TestOptions *opt) {
  ouo_printerr("%s: ", name);
  OuoParseResult p_res = {0};
  ouo_parse(src, &p_res);
  bool pass = false;

  if (p_res.failed) {
    pass = opt->fail;
    ouo_printerr(pass ? _TEST_PASS : _TEST_FAIL);
    ouo_printerr("\n");
    if (!opt->fail) {
      OUO_DA_FOREACH(OuoError, err, &p_res.errors) {
        ouo_err_msg_print(err, src, NULL);
      }
    }
    goto parse_defer;
  } else if (opt->exp_ast != NULL || opt->exp_ast_stmt != NULL ||
      opt->exp_ast_expr != NULL) {
    OuoAst *exp_ast = opt->exp_ast != NULL
        ? opt->exp_ast
        : &(OuoAst){.kind = OUO_AST_MODULE,
              .children = {
                  .count = 1,
                  .items = &(OuoAst *){opt->exp_ast_stmt
                          ? opt->exp_ast_stmt
                          : &(OuoAst){.kind = OUO_AST_EXPR_STMT,
                                .k.child = opt->exp_ast_expr}},
              }};

    if (_ast_eq(exp_ast, p_res.ast)) {
      pass = true;
      ouo_printerr(_TEST_PASS);
      ouo_printerr("\n");
    }
  } else {
    pass = !opt->fail;
    ouo_printerr(pass ? _TEST_PASS : _TEST_FAIL);
    ouo_printerr("\n");
  }

parse_defer:
  ouo_ast_free(p_res.ast);
  ouo_da_free(p_res.errors);

  total++;
  if (pass) passes++;
}

static inline void test_print_total(void) {
  ouo_printerr("total: %d/%d ", passes, total);
  ouo_printerr(passes == total ? _TEST_PASS : _TEST_FAIL);
  ouo_printerr("\n");
}

#endif // TEST_H
