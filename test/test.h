#ifndef TEST_H
#define TEST_H

#undef OUO_DEBUG
#define OUO_IMPLEMENTATION
#include "../src/ouo.h"

static int total = 0;
static int passes = 0;

#define _TEST_PASS OUO_EBGRN "PASS" OUO_ER
#define _TEST_FAIL OUO_EBRED "FAIL" OUO_ER

#define _ast_eq_fail(expr, msg) \
  do { \
    if (!(expr)) { \
      ouo_printerr(_TEST_FAIL " %s\n", msg); \
      return false; \
    } \
  } while (0)

#define _ast_eq_assert(ast, expr, fmt, exp, got) \
  do { \
    if (!(expr)) { \
      ouo_printerr( \
          _TEST_FAIL " " #expr ": exp " fmt ", got " fmt "\n", exp, got); \
      return false; \
    } \
  } while (0)

static bool _ast_eq(OuoAst *exp, OuoAst *got) {
  if (exp == NULL && got == NULL) return true;
  _ast_eq_fail(exp != NULL, "exp NULL, got non-NULL");
  _ast_eq_fail(got != NULL, "exp non-NULL, got NULL");

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
    case OUO_AST_BUILTIN:
      _ast_eq_fail(ouo_str_slice_eq(&exp->as.ident.name, &got->as.ident.name),
          "ident name mismatch");
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
          (int)exp->as.lit_bool, (int)got->as.lit_bool);
      break;
    case OUO_AST_LIT_STR:
      _ast_eq_assert(got, exp->as.lit_str.count == got->as.lit_str.count, "%zu",
          exp->as.lit_str.count, got->as.lit_str.count);
      _ast_eq_fail(memcmp(exp->as.lit_str.items, got->as.lit_str.items,
                       exp->as.lit_str.count) == 0,
          "lit_str content mismatch");
      break;
    case OUO_AST_LIT_TYPE:
      if (exp->as.lit_type.ident != NULL || got->as.lit_type.ident != NULL)
        if (!_ast_eq(exp->as.lit_type.ident, got->as.lit_type.ident))
          return false;
      break;

    // Expressions
    case OUO_AST_ASSIGN:
      if (!_ast_eq(exp->as.assign.target, got->as.assign.target)) return false;
      if (!_ast_eq(exp->as.assign.value, got->as.assign.value)) return false;
      break;
    case OUO_AST_BINARY:
      _ast_eq_assert(got, exp->as.binary.op == got->as.binary.op, "%d",
          exp->as.binary.op, got->as.binary.op);
      if (!_ast_eq(exp->as.binary.left, got->as.binary.left)) return false;
      if (!_ast_eq(exp->as.binary.right, got->as.binary.right)) return false;
      break;
    case OUO_AST_UNARY:
      _ast_eq_assert(got, exp->as.unary.op == got->as.unary.op, "%d",
          exp->as.unary.op, got->as.unary.op);
      if (!_ast_eq(exp->as.unary.right, got->as.unary.right)) return false;
      break;
    case OUO_AST_CALL:
      if (!_ast_eq(exp->as.call.target, got->as.call.target)) return false;
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
    case OUO_AST_RETURN:
      if (!_ast_eq(exp->as.expr_stmt.expr, got->as.expr_stmt.expr))
        return false;
      break;
    case OUO_AST_DECL_VAR:
      _ast_eq_assert(got,
          ouo_str_slice_eq(
              &exp->as.decl_var.name.str, &got->as.decl_var.name.str),
          "%.*s", OUO_TOK_FMT(exp->as.decl_var.name),
          OUO_TOK_FMT(got->as.decl_var.name));
      if (!_ast_eq(exp->as.decl_var.type_annot, got->as.decl_var.type_annot))
        return false;
      if (!_ast_eq(exp->as.decl_var.value, got->as.decl_var.value))
        return false;
      break;
    case OUO_AST_DECL_FN:
      _ast_eq_assert(got,
          ouo_str_slice_eq(
              &exp->as.decl_fn.name.str, &got->as.decl_fn.name.str),
          "%.*s", OUO_TOK_FMT(exp->as.decl_fn.name),
          OUO_TOK_FMT(got->as.decl_fn.name));
      if (!_ast_eq(exp->as.decl_fn.body, got->as.decl_fn.body)) return false;
      break;
    case OUO_AST_DECL_TYPE:
      _ast_eq_assert(got,
          ouo_str_slice_eq(
              &exp->as.decl_type.name.str, &got->as.decl_type.name.str),
          "%.*s", OUO_TOK_FMT(exp->as.decl_type.name),
          OUO_TOK_FMT(got->as.decl_type.name));
      if (!_ast_eq(exp->as.decl_type.type_annot, got->as.decl_type.type_annot))
        return false;
      break;
  }

  return true;
}

#define TN(name) OUO_ED OUO_CODEPOS OUO_ER name

typedef struct {
  bool fail;
  OuoErrorCode fail_code;
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
    if (opt->fail) {
      bool err_code_matches = false;
      OUO_DA_FOREACH(OuoError, err, &p_res.errors) {
        if (err->code == opt->fail_code) {
          err_code_matches = true;
          break;
        }
      }
      pass = err_code_matches;
      ouo_printerr(pass ? _TEST_PASS "\n" : _TEST_FAIL " wrong error code\n");
    } else {
      ouo_printerr(_TEST_FAIL "\n");
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
      ouo_printerr(_TEST_PASS "\n");
    }
  } else {
    pass = !opt->fail;
    ouo_printerr(pass ? _TEST_PASS : _TEST_FAIL);
    ouo_printerr("\n");
  }

parse_defer:
  ouo_p_res_free(&p_res);

  total++;
  if (pass) passes++;
}

static inline int test_summary(void) {
  ouo_printerr("total: %d/%d ", passes, total);
  ouo_printerr(passes == total ? _TEST_PASS : _TEST_FAIL);
  ouo_printerr("\n");
  return passes < total ? 1 : 0;
}

#endif // TEST_H
