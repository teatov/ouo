#include "test.h"

#include <stddef.h>

static inline void test_parse_exp_ast(
    const char *name, const char *src, OuoAst *exp) {
  TestOptions opt = {.exp_ast = exp};
  test(name, src, &opt);
}

static inline void test_parse_exp_ast_expr(
    const char *name, const char *src, OuoAst *exp) {
  TestOptions opt = {.exp_ast_expr = exp};
  test(name, src, &opt);
}

static inline void test_parse_exp_ast_stmt(
    const char *name, const char *src, OuoAst *exp) {
  TestOptions opt = {.exp_ast_stmt = exp};
  test(name, src, &opt);
}

static inline void test_parse(const char *name, const char *src) {
  TestOptions opt = {0};
  test(name, src, &opt);
}

static inline void test_parse_fail(const char *msg, const char *src) {
  TestOptions opt = {.fail = true};
  test(msg, src, &opt);
}

int main(void) {
  test_parse_exp_ast(TN("empty string"), "",
      &(OuoAst){.kind = OUO_AST_MODULE, .stmts = {.count = 0}});

  test_parse_exp_ast_expr(TN("identifier"), "ass",
      &(OuoAst){.kind = OUO_AST_IDENT,
          .ident = {
              .name = {.start = "ass", .len = 3},
          }});

  test_parse_exp_ast_expr(
      TN("single int"), "2", &(OuoAst){.kind = OUO_AST_LIT_INT, .lit_int = 2});

  test_parse_exp_ast_expr(TN("single float"), "2.5",
      &(OuoAst){.kind = OUO_AST_LIT_FLOAT, .lit_float = 2.5});

  test_parse_exp_ast_expr(TN("assign"), "a = 5",
      &(OuoAst){.kind = OUO_AST_ASSIGN,
          .assign = {.target = &(OuoAst){.kind = OUO_AST_IDENT,
                         .ident =
                             {
                                 .name = {.start = "a", .len = 1},
                             }},
              .value = &(OuoAst){.kind = OUO_AST_LIT_INT, .lit_int = 5}}});

  test_parse_exp_ast_expr(TN("bin op"), "2 + 2",
      &(OuoAst){.kind = OUO_AST_BIN_OP,
          .bin_op = {.left = &(OuoAst){.kind = OUO_AST_LIT_INT, .lit_int = 2},
              .op = OUO_TOK_PLUS,
              .right = &(OuoAst){.kind = OUO_AST_LIT_INT, .lit_int = 2}}});

  test_parse_fail(TN("unknown symbol fails"), "%");
  test_parse_fail(TN("single operator fails"), "+");
  test_parse_fail(TN("two numbers fails"), "2 2");
  test_parse_fail(TN("two numbers fails newline"), "2 2\n");
  test_parse_fail(TN("unfinished bin op fails"), "2 +");
  test_parse_fail(TN("unfinished bin op fails newline"), "\n2 +\n\n");
  test_parse_fail(TN("two operators fails"), "2 + +");

  test_parse_fail(
      TN("huge integer fails"), "999999999999999999999999999999999999");
  test_parse_fail(TN("huge float fails"),
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999.0");

  test_parse(TN("empty block"), "{}");
  test_parse(TN("empty blocks"), "{{{}}}");
  test_parse(TN("block + int"), "{}+1");
  test_parse(TN("int + block"), "1+{}");
  test_parse(TN("block with expr"), "{2+2}");
  test_parse(TN("block with expr newlines"), "{\n\t2+2\n}");
  test_parse(TN("blocks with expr newlines"), "{\n\t2+{\n\t2+2\n}\n}");
  test_parse_fail(TN("unclosed block fails"), "{");
  test_parse_fail(TN("unclosed blocks fails"), "{{{}");
  test_parse_fail(TN("block extra closing brace fails"), "{}}");

  test_parse_exp_ast_stmt(TN("variable declaration"), "var a = 5",
      &(OuoAst){.kind = OUO_AST_DECL_VAR,
          .decl_var = {.name = {.start = "a", .len = 1},
              .value = &(OuoAst){.kind = OUO_AST_LIT_INT, .lit_int = 5}}});

  test_parse(TN("var decl with type"), "var a: int = 5");
  test_parse_fail(TN("var decl without identifier fails"), "var");
  test_parse_fail(TN("var decl without '=' fails"), "var a");
  test_parse_fail(TN("var decl without type"), "var a:");
  test_parse_fail(TN("var decl without type with '=' fails"), "var a:=");
  test_parse_fail(
      TN("var decl without with type without '=' fails"), "var a: float");

  test_print_total();
  return 0;
}
