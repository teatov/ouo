#ifndef TEST_H
#define TEST_H

#undef OUO_DEBUG
#define OUO_IMPLEMENTATION
#include "../src/ouo.h"

static int total = 0;
static int passes = 0;

#define _TEST_PASS OUO_EBGRN "PASS" OUO_ER
#define _TEST_FAIL OUO_EBRED "FAIL" OUO_ER

#define _test_eq_fail(expr, msg) \
  do { \
    if (!(expr)) { \
      ouo_printerr(_TEST_FAIL " %s\n", msg); \
      return false; \
    } \
  } while (0)

#define _test_eq_assert(expr, fmt, exp, got) \
  do { \
    if (!(expr)) { \
      ouo_printerr( \
          _TEST_FAIL " " #expr ": exp " fmt ", got " fmt "\n", exp, got); \
      return false; \
    } \
  } while (0)

static bool _ast_eq(OuoAst *exp, OuoAst *got) {
  if (exp == NULL && got == NULL) return true;
  _test_eq_fail(exp != NULL, "exp NULL, got non-NULL");
  _test_eq_fail(got != NULL, "exp non-NULL, got NULL");

  _test_eq_assert(exp->kind == got->kind, "%d", exp->kind, got->kind);

  switch (exp->kind) {
    case OUO_AST_MODULE:
    case OUO_AST_BLOCK:
      _test_eq_assert(exp->children.count == got->children.count, "%zu",
          exp->children.count, got->children.count);
      for (size_t i = 0; i < exp->children.count; i++)
        if (!_ast_eq(exp->children.items[i], got->children.items[i]))
          return false;
      break;

    case OUO_AST_IDENT:
    case OUO_AST_BUILTIN:
      _test_eq_fail(ouo_str_slice_eq(&exp->as.ident.name, &got->as.ident.name),
          "ident name mismatch");
      break;

    // Literals
    case OUO_AST_LIT_INT:
      _test_eq_assert(exp->as.lit_int == got->as.lit_int, "%" OUO_PRId,
          exp->as.lit_int, got->as.lit_int);
      break;
    case OUO_AST_LIT_FLOAT:
      _test_eq_assert(exp->as.lit_float == got->as.lit_float, "%" OUO_PRIf,
          exp->as.lit_float, got->as.lit_float);
      break;
    case OUO_AST_LIT_BOOL:
      _test_eq_assert(exp->as.lit_bool == got->as.lit_bool, "%d",
          (int)exp->as.lit_bool, (int)got->as.lit_bool);
      break;
    case OUO_AST_LIT_STR:
      _test_eq_assert(exp->as.lit_str.count == got->as.lit_str.count, "%zu",
          exp->as.lit_str.count, got->as.lit_str.count);
      _test_eq_fail(memcmp(exp->as.lit_str.items, got->as.lit_str.items,
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
      _test_eq_assert(exp->as.binary.op == got->as.binary.op, "%d",
          exp->as.binary.op, got->as.binary.op);
      if (!_ast_eq(exp->as.binary.left, got->as.binary.left)) return false;
      if (!_ast_eq(exp->as.binary.right, got->as.binary.right)) return false;
      break;
    case OUO_AST_UNARY:
      _test_eq_assert(exp->as.unary.op == got->as.unary.op, "%d",
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
      _test_eq_assert(ouo_str_slice_eq(&exp->as.decl_var.name.str,
                          &got->as.decl_var.name.str),
          "%.*s", OUO_TOK_FMT(exp->as.decl_var.name),
          OUO_TOK_FMT(got->as.decl_var.name));
      if (!_ast_eq(exp->as.decl_var.type_annot, got->as.decl_var.type_annot))
        return false;
      if (!_ast_eq(exp->as.decl_var.value, got->as.decl_var.value))
        return false;
      break;
    case OUO_AST_DECL_FN:
      _test_eq_assert(ouo_str_slice_eq(
                          &exp->as.decl_fn.name.str, &got->as.decl_fn.name.str),
          "%.*s", OUO_TOK_FMT(exp->as.decl_fn.name),
          OUO_TOK_FMT(got->as.decl_fn.name));
      if (!_ast_eq(exp->as.decl_fn.body, got->as.decl_fn.body)) return false;
      break;
    case OUO_AST_DECL_TYPE:
      _test_eq_assert(ouo_str_slice_eq(&exp->as.decl_type.name.str,
                          &got->as.decl_type.name.str),
          "%.*s", OUO_TOK_FMT(exp->as.decl_type.name),
          OUO_TOK_FMT(got->as.decl_type.name));
      if (!_ast_eq(exp->as.decl_type.type_annot, got->as.decl_type.type_annot))
        return false;
      break;
  }

  return true;
}

static bool _obj_eq(OuoObject *exp, OuoObject *got) {
  _test_eq_assert(exp->kind == got->kind, "%d", exp->kind, got->kind);

  switch (exp->kind) {
    case OUO_OBJ_INT:
      _test_eq_assert(exp->as.v_int == got->as.v_int, "%" OUO_PRId,
          exp->as.v_int, got->as.v_int);
      break;
    case OUO_OBJ_FLOAT:
      _test_eq_assert(exp->as.v_float == got->as.v_float, "%" OUO_PRIf,
          exp->as.v_float, got->as.v_float);
      break;
    case OUO_OBJ_BOOL:
      _test_eq_assert(exp->as.v_bool == got->as.v_bool, "%d",
          (int)exp->as.v_bool, (int)got->as.v_bool);
      break;
    case OUO_OBJ_STR: {
      OuoRcStr *exp_s = (OuoRcStr *)exp->as.ref;
      OuoRcStr *got_s = (OuoRcStr *)got->as.ref;
      _test_eq_assert(exp_s->str.count == got_s->str.count, "%zu",
          exp_s->str.count, got_s->str.count);
      _test_eq_assert(
          memcmp(exp_s->str.items, got_s->str.items, exp_s->str.count) == 0,
          "%zu", exp_s->str.count, got_s->str.count);
      break;
    }
    case OUO_OBJ_FN:
    case OUO_OBJ_BUILTIN_FN:
      _test_eq_assert(exp->as.ref == got->as.ref, "%p", (void *)exp->as.ref,
          (void *)got->as.ref);
      break;
  }

  return true;
}

#define TN(name) OUO_ED OUO_CODEPOS OUO_ER name

typedef enum {
  TEST_PARSE,
  TEST_COMPILE,
  TEST_INTERPRET,
} TestStage;

typedef struct {
  TestStage stage;
  bool fail;
  OuoErrorCode fail_code;
  OuoAst *exp_ast;
  OuoAst *exp_ast_expr;
  OuoAst *exp_ast_stmt;
  OuoObject *exp_obj;
  bool keep_module_scope;
} TestOptions;

static bool _errors_match(OuoErrors *errors, OuoErrorCode err_code) {
  if (err_code == OUO_OK) return errors->count > 0;
  OUO_DA_FOREACH(OuoError, err, errors) {
    if (err->code == err_code) return true;
  }
  return false;
}

static inline void test(const char *name, const char *src, TestOptions *opt) {
  ouo_printerr("%s: ", name);
  bool pass = false;

  OuoParseResult p_res = {0};
  OuoCompileResult c_res = {.keep_module_scope = opt->keep_module_scope};
  OuoInterpretResult i_res = {0};

  ouo_parse(src, &p_res);

  if (p_res.failed) {
    if (opt->fail) {
      pass = _errors_match(&p_res.errors, opt->fail_code);
      ouo_printerr(pass ? _TEST_PASS "\n" : _TEST_FAIL " wrong error code\n");
    } else {
      ouo_printerr(_TEST_FAIL "\n");
      OUO_DA_FOREACH(OuoError, err, &p_res.errors)
      ouo_err_msg_print(err, src, NULL);
    }
    goto parse_defer;
  } else if (opt->stage == TEST_PARSE) {
    if (opt->exp_ast != NULL || opt->exp_ast_stmt != NULL ||
        opt->exp_ast_expr != NULL) {
      OuoAst *exp = opt->exp_ast != NULL
          ? opt->exp_ast
          : &(OuoAst){.kind = OUO_AST_MODULE,
                .children = {
                    .count = 1,
                    .items = &(OuoAst *){opt->exp_ast_stmt
                            ? opt->exp_ast_stmt
                            : &(OuoAst){.kind = OUO_AST_EXPR_STMT,
                                  .as.expr_stmt.expr = opt->exp_ast_expr}},
                }};
      pass = _ast_eq(exp, p_res.ast);
      if (pass) ouo_printerr(_TEST_PASS "\n");
    } else {
      pass = !opt->fail;
      ouo_printerr(pass ? _TEST_PASS "\n" : _TEST_FAIL "\n");
    }
    goto parse_defer;
  }

  ouo_compile(p_res.ast, &c_res);

parse_defer:
  ouo_p_res_free(&p_res);
  if (p_res.failed || opt->stage == TEST_PARSE) goto final_defer;

  if (c_res.failed) {
    if (opt->fail) {
      pass = _errors_match(&c_res.errors, opt->fail_code);
      ouo_printerr(pass ? _TEST_PASS "\n" : _TEST_FAIL " wrong error code\n");
    } else {
      ouo_printerr(_TEST_FAIL "\n");
      OUO_DA_FOREACH(OuoError, err, &c_res.errors)
      ouo_err_msg_print(err, src, NULL);
    }
    goto compile_defer;
  } else if (opt->stage == TEST_COMPILE) {
    pass = !opt->fail;
    ouo_printerr(pass ? _TEST_PASS "\n" : _TEST_FAIL "\n");
    goto compile_defer;
  }

  ouo_interpret(&c_res.chunk, &i_res);

compile_defer:
  ouo_c_res_free(&c_res);
  if (c_res.failed || opt->stage == TEST_COMPILE) goto final_defer;

  if (i_res.failed) {
    if (opt->fail) {
      pass = (opt->fail_code == OUO_OK || i_res.error.code == opt->fail_code);
      ouo_printerr(pass ? _TEST_PASS "\n" : _TEST_FAIL " wrong error code\n");
    } else {
      ouo_printerr(_TEST_FAIL "\n");
      ouo_err_msg_print(&i_res.error, src, NULL);
    }
    goto final_defer;
  } else if (opt->fail) {
    ouo_printerr(_TEST_FAIL " expected error, succeeded\n");
    goto final_defer;
  } else {
    size_t stack_size = (size_t)(i_res.stack.top - i_res.stack.items);
    if (opt->exp_obj != NULL) {
      if (stack_size == 0) {
        ouo_printerr(_TEST_FAIL " exp value on stack, got empty stack\n");
      } else {
        OuoObject *top = &i_res.stack.items[stack_size - 1];
        pass = _obj_eq(opt->exp_obj, top);
        if (pass) ouo_printerr(_TEST_PASS "\n");
      }
    } else {
      pass = stack_size == 0;
      ouo_printerr(pass ? _TEST_PASS "\n"
                        : _TEST_FAIL " exp empty stack, got %zu val(s)\n",
          stack_size);
    }
  }

final_defer:
  ouo_c_res_cleanup(&c_res);
  ouo_i_res_cleanup(&i_res);

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
