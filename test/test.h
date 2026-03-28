#ifndef TEST_H
#define TEST_H

#undef OUO_DEBUG
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
      _ast_eq_assert(got,
          _ouo_str_slice_eq(&exp->as.ident.name.str, &got->as.ident.name.str),
          "%.*s", _OUO_TOK_FMT(exp->as.ident.name),
          _OUO_TOK_FMT(got->as.ident.name));
      break;

    // Literals
    case OUO_AST_LIT_INT:
      _ast_eq_assert(got, exp->as.lit_int == got->as.lit_int, "%" OUO_PRId,
          exp->as.lit_int, got->as.lit_int);
      break;
    case OUO_AST_LIT_FLOAT:
      _ast_eq_assert(got, exp->as.lit_float == got->as.lit_float, "%" OUO_PRIf,
          exp->as.lit_float, got->as.lit_float);
      break;
    case OUO_AST_LIT_BOOL:
      _ast_eq_assert(got, exp->as.lit_bool == got->as.lit_bool, "%d",
          exp->as.lit_bool, got->as.lit_bool);
      break;
    case OUO_AST_LIT_STR:
      _ast_eq_assert(got, exp->as.lit_str.count == got->as.lit_str.count, "%zu",
          exp->as.lit_str.count, got->as.lit_str.count);
      _ast_eq_assert(got, false && "TODO", "%zu", exp->as.lit_str.count,
          got->as.lit_str.count);
      break;

    // Expressions
    case OUO_AST_ASSIGN:
      if (!_ast_eq(exp->as.assign.target, got->as.assign.target)) return false;
      if (!_ast_eq(exp->as.assign.value, got->as.assign.value)) return false;
      break;
    case OUO_AST_BINARY:
      _ast_eq_assert(got, exp->as.binary.op == got->as.binary.op, "%s",
          _ouo_tok_kind_str(exp->as.binary.op),
          _ouo_tok_kind_str(got->as.binary.op));
      if (!_ast_eq(exp->as.binary.left, got->as.binary.left)) return false;
      if (!_ast_eq(exp->as.binary.right, got->as.binary.right)) return false;
      break;
    case OUO_AST_UNARY:
      _ast_eq_assert(got, exp->as.unary.op == got->as.unary.op, "%s",
          _ouo_tok_kind_str(exp->as.unary.op),
          _ouo_tok_kind_str(got->as.unary.op));
      if (!_ast_eq(exp->as.unary.right, got->as.unary.right)) return false;
      break;
    case OUO_AST_IF:
      if (!_ast_eq(exp->as.if_expr.condition, got->as.if_expr.condition))
        return false;
      if (!_ast_eq(exp->as.if_expr.then_branch, got->as.if_expr.then_branch))
        return false;
      if (!_ast_eq(exp->as.if_expr.else_branch, got->as.if_expr.else_branch))
        return false;
      break;
    case OUO_AST_WHILE:
      if (!_ast_eq(exp->as.while_expr.condition, got->as.while_expr.condition))
        return false;
      if (!_ast_eq(exp->as.while_expr.body, got->as.while_expr.body))
        return false;
      break;

    // Statements
    case OUO_AST_EXPR_STMT:
    case OUO_AST_PRINT:
      if (!_ast_eq(exp->as.expr_stmt.expr, got->as.expr_stmt.expr))
        return false;
      break;
    case OUO_AST_DECL_VAR:
      _ast_eq_assert(got,
          _ouo_str_slice_eq(
              &exp->as.decl_var.name.str, &got->as.decl_var.name.str),
          "%.*s", _OUO_TOK_FMT(exp->as.decl_var.name),
          _OUO_TOK_FMT(got->as.decl_var.name));
      if (!_ast_eq(exp->as.decl_var.value, got->as.decl_var.value))
        return false;
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
                                .as.expr_stmt.expr = opt->exp_ast_expr}},
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
