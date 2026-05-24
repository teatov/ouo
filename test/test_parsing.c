#include "test.h"

#include <stddef.h>

// Wrappers

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

static inline void test_err_syntax(const char *msg, const char *src) {
  TestOptions opt = {.fail = true, .fail_code = OUO_ERR_SYNTAX};
  test(msg, src, &opt);
}

static inline void test_err_parse_fail(const char *msg, const char *src) {
  TestOptions opt = {.fail = true, .fail_code = OUO_ERR_PARSE_FAIL};
  test(msg, src, &opt);
}

// AST helpers

#define MK_INT(v) &(OuoAst){.kind = OUO_AST_LIT_INT, .as.lit_int = v}
#define MK_FLOAT(v) &(OuoAst){.kind = OUO_AST_LIT_FLOAT, .as.lit_float = v}
#define MK_BOOL(v) &(OuoAst){.kind = OUO_AST_LIT_BOOL, .as.lit_bool = v}
#define MK_IDENT(s) \
  &(OuoAst){.kind = OUO_AST_IDENT, \
      .as.ident = {.name = {.start = s, .len = strlen(s)}}}

#define MK_BINOP(l, tokop, r) \
  &(OuoAst){.kind = OUO_AST_BINARY, \
      .as.binary = {.left = l, .op = tokop, .right = r}}

#define MK_UNOP(tokop, r) \
  &(OuoAst){.kind = OUO_AST_UNARY, .as.unary = {.op = tokop, .right = r}}

#define MK_ASSIGN(tgt, val) \
  &(OuoAst){.kind = OUO_AST_ASSIGN, .as.assign = {.target = tgt, .value = val}}

// Tests

static void test_parse_errors(void) {
  ouo_printerr("general errors\n");

  test_err_syntax(TN("unknown symbol %"), "%");
  test_err_syntax(TN("unknown symbol @"), "@");
}

static void test_module(void) {
  ouo_printerr("module\n");

  test_parse_exp_ast(TN("empty file"), "",
      &(OuoAst){.kind = OUO_AST_MODULE, .children = {.count = 0}});
  test_parse_exp_ast(TN("only newlines"), "\n\n\n",
      &(OuoAst){.kind = OUO_AST_MODULE, .children = {.count = 0}});

  test_parse(TN("stmt followed by newline"), "1\n");
  test_parse(TN("two stmts on separate lines"), "1\n2");
  test_parse(TN("multiple stmts by newlines"), "1\n2\n3");
  test_parse(TN("empty lines between stmts"), "1\n\n\n2");
}

static void test_int_literals(void) {
  ouo_printerr("int literals\n");

  test_parse_exp_ast_expr(TN("int 0"), "0", MK_INT(0));
  test_parse_exp_ast_expr(TN("int 1"), "1", MK_INT(1));
  test_parse_exp_ast_expr(TN("int 42"), "42", MK_INT(42));
  test_parse_exp_ast_expr(
      TN("int 9999999999"), "9999999999", MK_INT(9999999999LL));

  test_err_parse_fail(
      TN("huge integer fails"), "999999999999999999999999999999999999");

  test_parse_exp_ast_expr(
      TN("unary minus on 1"), "-1", MK_UNOP(OUO_TOK_MINUS, MK_INT(1)));
  test_parse_exp_ast_expr(
      TN("unary minus on 0"), "-0", MK_UNOP(OUO_TOK_MINUS, MK_INT(0)));
}

static void test_float_literals(void) {
  ouo_printerr("float literals\n");

  test_parse_exp_ast_expr(TN("float 0.0"), "0.0", MK_FLOAT(0.0));
  test_parse_exp_ast_expr(TN("float 1.0"), "1.0", MK_FLOAT(1.0));
  test_parse_exp_ast_expr(TN("float 2.5"), "2.5", MK_FLOAT(2.5));
  test_parse_exp_ast_expr(TN("float 3.14"), "3.14", MK_FLOAT(3.14));
  test_parse_exp_ast_expr(TN("float 0.001"), "0.001", MK_FLOAT(0.001));

  test_err_parse_fail(TN("huge float fails"),
      "9999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999.0");
  test_err_syntax(TN("no decimal part fails"), "1.");
  test_err_syntax(TN("no whole part fails"), ".1");
}

static void test_bool_literals(void) {
  ouo_printerr("bool literals\n");

  test_parse_exp_ast_expr(TN("true"), "true", MK_BOOL(true));
  test_parse_exp_ast_expr(TN("false"), "false", MK_BOOL(false));
}

static void test_string_literals(void) {
  ouo_printerr("string literals\n");

  test_parse(TN("empty string literal"), "\"\"");
  test_parse(TN("simple string"), "\"hello\"");
  test_parse(TN("string with spaces"), "\"hello world\"");
  test_parse(TN("string with escape newline"), "\"hello\\nworld\"");
  test_parse(TN("string with escape tab"), "\"\\t\"");
  test_parse(TN("string with escaped quote"), "\"say \\\"hi\\\"\"");
  test_parse(TN("string with backslash"), "\"a\\\\b\"");

  test_err_syntax(TN("unclosed string fails"), "\"hello");
  test_err_syntax(TN("unclosed string mid-newline fails"), "\"hello\n\"");
}

static void test_identifiers(void) {
  ouo_printerr("identifiers\n");

  test_parse_exp_ast_expr(TN("single letter"), "a", MK_IDENT("a"));
  test_parse_exp_ast_expr(TN("multi-letter"), "ass", MK_IDENT("ass"));
  test_parse_exp_ast_expr(TN("with underscore"), "my_var", MK_IDENT("my_var"));
  test_parse_exp_ast_expr(TN("starts with underscore"), "_x", MK_IDENT("_x"));
  test_parse_exp_ast_expr(TN("long ident"), "abcdefghijklmnopqrstuvwxyz",
      MK_IDENT("abcdefghijklmnopqrstuvwxyz"));
}

static void test_binary_ops(void) {
  ouo_printerr("binary operators\n");

  test_parse_exp_ast_expr(
      TN("2 + 2"), "2 + 2", MK_BINOP(MK_INT(2), OUO_TOK_PLUS, MK_INT(2)));
  test_parse_exp_ast_expr(
      TN("3 - 1"), "3 - 1", MK_BINOP(MK_INT(3), OUO_TOK_MINUS, MK_INT(1)));
  test_parse_exp_ast_expr(
      TN("4 * 5"), "4 * 5", MK_BINOP(MK_INT(4), OUO_TOK_ASTERISK, MK_INT(5)));
  test_parse_exp_ast_expr(
      TN("10 / 2"), "10 / 2", MK_BINOP(MK_INT(10), OUO_TOK_SLASH, MK_INT(2)));
  test_parse_exp_ast_expr(TN("a == b"), "a == b",
      MK_BINOP(MK_IDENT("a"), OUO_TOK_EQ, MK_IDENT("b")));
  test_parse_exp_ast_expr(TN("a != b"), "a != b",
      MK_BINOP(MK_IDENT("a"), OUO_TOK_NEQ, MK_IDENT("b")));
  test_parse_exp_ast_expr(
      TN("a < b"), "a < b", MK_BINOP(MK_IDENT("a"), OUO_TOK_LT, MK_IDENT("b")));
  test_parse_exp_ast_expr(TN("a <= b"), "a <= b",
      MK_BINOP(MK_IDENT("a"), OUO_TOK_LT_EQ, MK_IDENT("b")));
  test_parse_exp_ast_expr(
      TN("a > b"), "a > b", MK_BINOP(MK_IDENT("a"), OUO_TOK_GT, MK_IDENT("b")));
  test_parse_exp_ast_expr(TN("a >= b"), "a >= b",
      MK_BINOP(MK_IDENT("a"), OUO_TOK_GT_EQ, MK_IDENT("b")));
  test_parse_exp_ast_expr(TN("true or false"), "true or false",
      MK_BINOP(MK_BOOL(true), OUO_TOK_KW_OR, MK_BOOL(false)));
  test_parse_exp_ast_expr(TN("true and false"), "true and false",
      MK_BINOP(MK_BOOL(true), OUO_TOK_KW_AND, MK_BOOL(false)));

  test_err_syntax(TN("bare +"), "+");
  test_err_syntax(TN("bare *"), "*");
  test_err_syntax(TN("bare /"), "/");
  test_err_syntax(TN("two numbers no op"), "2 2");
  test_err_syntax(TN("two numbers no op newline"), "2 2\n");
  test_err_syntax(TN("unfinished binop"), "2 +");
  test_err_syntax(TN("unfinished binop with newline"), "\n2 +\n\n");
  test_err_syntax(TN("two operators"), "2 + +");
  test_err_syntax(TN("two operators mixed"), "2 + * 3");
  test_err_syntax(TN("operator on own line fails"), "1\n+\n2");
}

static void test_precedence(void) {
  ouo_printerr("precedence and associativity\n");

  test_parse_exp_ast_expr(TN("mul before add"), "2 + 3 * 4",
      MK_BINOP(MK_INT(2), OUO_TOK_PLUS,
          MK_BINOP(MK_INT(3), OUO_TOK_ASTERISK, MK_INT(4))));

  test_parse_exp_ast_expr(TN("add after mul"), "2 * 3 + 4",
      MK_BINOP(MK_BINOP(MK_INT(2), OUO_TOK_ASTERISK, MK_INT(3)), OUO_TOK_PLUS,
          MK_INT(4)));

  test_parse_exp_ast_expr(TN("add left-assoc"), "1 + 2 + 3",
      MK_BINOP(MK_BINOP(MK_INT(1), OUO_TOK_PLUS, MK_INT(2)), OUO_TOK_PLUS,
          MK_INT(3)));

  test_parse_exp_ast_expr(TN("sub left-assoc"), "1 - 2 - 3",
      MK_BINOP(MK_BINOP(MK_INT(1), OUO_TOK_MINUS, MK_INT(2)), OUO_TOK_MINUS,
          MK_INT(3)));

  test_parse_exp_ast_expr(TN("mul left-assoc"), "1 * 2 * 3",
      MK_BINOP(MK_BINOP(MK_INT(1), OUO_TOK_ASTERISK, MK_INT(2)),
          OUO_TOK_ASTERISK, MK_INT(3)));

  test_parse_exp_ast_expr(TN("comparison lower than add"), "1 + 2 == 3",
      MK_BINOP(
          MK_BINOP(MK_INT(1), OUO_TOK_PLUS, MK_INT(2)), OUO_TOK_EQ, MK_INT(3)));

  test_parse_exp_ast_expr(TN("or lower than and"), "true or false and false",
      MK_BINOP(MK_BOOL(true), OUO_TOK_KW_OR,
          MK_BINOP(MK_BOOL(false), OUO_TOK_KW_AND, MK_BOOL(false))));

  test_parse_exp_ast_expr(TN("and lower than compare"), "1 < 2 and 3 > 0",
      MK_BINOP(MK_BINOP(MK_INT(1), OUO_TOK_LT, MK_INT(2)), OUO_TOK_KW_AND,
          MK_BINOP(MK_INT(3), OUO_TOK_GT, MK_INT(0))));
}

static void test_unary_ops(void) {
  ouo_printerr("unary operators\n");

  test_parse_exp_ast_expr(
      TN("negate int"), "-5", MK_UNOP(OUO_TOK_MINUS, MK_INT(5)));
  test_parse_exp_ast_expr(
      TN("negate float"), "-3.14", MK_UNOP(OUO_TOK_MINUS, MK_FLOAT(3.14)));
  test_parse_exp_ast_expr(
      TN("not true"), "!true", MK_UNOP(OUO_TOK_BANG, MK_BOOL(true)));
  test_parse_exp_ast_expr(
      TN("not false"), "!false", MK_UNOP(OUO_TOK_BANG, MK_BOOL(false)));
  test_parse_exp_ast_expr(TN("double negate"), "--1",
      MK_UNOP(OUO_TOK_MINUS, MK_UNOP(OUO_TOK_MINUS, MK_INT(1))));
  test_parse_exp_ast_expr(TN("double not"), "!!true",
      MK_UNOP(OUO_TOK_BANG, MK_UNOP(OUO_TOK_BANG, MK_BOOL(true))));
  test_parse_exp_ast_expr(TN("unary in binary lhs"), "-1 + 2",
      MK_BINOP(MK_UNOP(OUO_TOK_MINUS, MK_INT(1)), OUO_TOK_PLUS, MK_INT(2)));
}

static void test_assignment(void) {
  ouo_printerr("assignment\n");

  test_parse_exp_ast_expr(
      TN("simple assign"), "a = 5", MK_ASSIGN(MK_IDENT("a"), MK_INT(5)));
  test_parse_exp_ast_expr(
      TN("assign float"), "x = 3.14", MK_ASSIGN(MK_IDENT("x"), MK_FLOAT(3.14)));
  test_parse_exp_ast_expr(
      TN("assign bool"), "b = true", MK_ASSIGN(MK_IDENT("b"), MK_BOOL(true)));
  test_parse_exp_ast_expr(TN("assign expression"), "a = 1 + 2",
      MK_ASSIGN(MK_IDENT("a"), MK_BINOP(MK_INT(1), OUO_TOK_PLUS, MK_INT(2))));
  test_parse(TN("chained assign parses"), "a = b = 5");
}

static void test_blocks(void) {
  ouo_printerr("blocks\n");

  test_parse_exp_ast_expr(TN("empty block"), "{}",
      &(OuoAst){.kind = OUO_AST_BLOCK, .children = {.count = 0}});
  test_parse(TN("nested empty blocks"), "{{{}}}");
  test_parse(TN("block with int expression"), "{2+2}");
  test_parse(TN("block with newlines"), "{\n\t2+2\n}");
  test_parse(TN("nested block with expr"), "{\n\t2+{\n\t2+2\n}\n}");
  test_parse(TN("block + expr"), "{}+1");
  test_parse(TN("expr + block"), "1+{}");
  test_parse(TN("block with stmt"), "{var a = 5}");
  test_parse(TN("block with multiple stmts"), "{\nvar a = 5\nvar b = 6\n}");

  test_err_syntax(TN("unclosed block"), "{");
  test_err_syntax(TN("unclosed nested block"), "{{{}");
  test_err_syntax(TN("extra closing brace"), "{}}");
}

static void test_var_decl(void) {
  ouo_printerr("var declarations\n");

  test_parse_exp_ast_stmt(TN("var decl int"), "var a = 5",
      &(OuoAst){.kind = OUO_AST_DECL_VAR,
          .as.decl_var = {
              .name.str = {.start = "a", .len = 1}, .value = MK_INT(5)}});
  test_parse_exp_ast_stmt(TN("var decl float"), "var f = 3.14",
      &(OuoAst){.kind = OUO_AST_DECL_VAR,
          .as.decl_var = {
              .name.str = {.start = "f", .len = 1}, .value = MK_FLOAT(3.14)}});
  test_parse_exp_ast_stmt(TN("var decl bool"), "var b = true",
      &(OuoAst){.kind = OUO_AST_DECL_VAR,
          .as.decl_var = {
              .name.str = {.start = "b", .len = 1}, .value = MK_BOOL(true)}});

  test_parse(TN("var decl with int type"), "var a: int = 5");
  test_parse(TN("var decl with float type"), "var f: float = 3.14");
  test_parse(TN("var decl with bool type"), "var b: bool = true");
  test_parse(TN("var decl with str type"), "var s: str = \"hi\"");

  test_parse(TN("var decl underscore name"), "var my_var = 42");
  test_parse(TN("var decl expr value"), "var c = 1 + 2");
  test_parse(TN("var decl block value"), "var c = {1 + 2}");

  test_err_syntax(TN("var without ident"), "var");
  test_err_syntax(TN("var without ="), "var a");
  test_err_syntax(TN("var with : no type"), "var a:");
  test_err_syntax(TN("var := no type"), "var a:=");
  test_err_syntax(TN("var type no ="), "var a: int");
  test_err_syntax(TN("var = no value"), "var a =");
}

static void test_fn_decl(void) {
  ouo_printerr("fn declarations\n");

  test_parse(TN("fn no params void body"), "fn f() => {}");
  test_parse(TN("fn no params expr body"), "fn f() => 42");
  test_parse(TN("fn one param"), "fn f(x: int) => x");
  test_parse(TN("fn two params"), "fn f(x: int, y: int) => x + y");
  test_parse(TN("fn with return type"), "fn f(): int => 42");
  test_parse(TN("fn param and return type"), "fn f(x: int): int => x");
  test_parse(TN("fn block body"), "fn f(x: int): int => {\nreturn x\n}");
  test_parse(TN("fn multiple stmts"), "fn f() => {\nvar a = 1\nvar b = 2\n}");
  test_parse(
      TN("fn float params"), "fn add(a: float, b: float): float => a + b");

  test_err_syntax(TN("fn no name"), "fn () => {}");
  test_err_syntax(TN("fn no parens"), "fn f => {}");
  test_err_syntax(TN("fn unclosed parens"), "fn f( => {}");
  test_err_syntax(TN("fn no arrow"), "fn f() {}");
  test_err_syntax(TN("fn no body"), "fn f() =>");
  test_err_syntax(TN("fn param no type"), "fn f(x) => x");
  test_err_syntax(TN("fn param no colon"), "fn f(x int) => x");
}

static void test_if_expr(void) {
  ouo_printerr("if expressions\n");

  test_parse(TN("if then"), "if (true) {}");
  test_parse(TN("if then else"), "if (true) {} else {}");
  test_parse(TN("if with expr condition"), "if (1 < 2) {}");
  test_parse(TN("if-else if-else chain"), "if (a) {} else if (b) {} else {}");
  test_parse(TN("if with block body"), "if (true) {\nvar x = 1\n}");
  test_parse(TN("if else with stmts"), "if (true) {\n1\n} else {\n2\n}");
  test_parse(TN("nested if"), "if (true) { if (false) {} }");

  test_err_syntax(TN("if no condition"), "if {}");
  test_err_syntax(TN("if no parens"), "if true {}");
  test_err_syntax(TN("if unclosed paren"), "if (true {}");
  test_err_syntax(TN("if no body"), "if (true)");
  test_err_syntax(TN("else without if"), "else {}");
  test_err_syntax(TN("if else no body"), "if (true) {} else");
}

static void test_while_expr(void) {
  ouo_printerr("while expressions\n");

  test_parse(TN("while basic"), "while (true) {}");
  test_parse(TN("while with condition"), "while (a < 10) {}");
  test_parse(TN("while with body"), "while (true) {\nvar x = 1\n}");
  test_parse(TN("nested while"), "while (true) { while (false) {} }");

  test_err_syntax(TN("while no condition"), "while {}");
  test_err_syntax(TN("while no parens"), "while true {}");
  test_err_syntax(TN("while unclosed paren"), "while (true {}");
  test_err_syntax(TN("while no body"), "while (true)");
}

static void test_return(void) {
  ouo_printerr("return statement\n");

  test_parse(TN("return int"), "fn f(): int => {\nreturn 1\n}");
  test_parse(TN("return expr"), "fn f(): int => {\nreturn 1 + 2\n}");
  test_parse(TN("return void (no expr)"), "fn f() => {\nreturn\n}");
  test_parse(TN("return ident"), "fn f(x: int): int => {\nreturn x\n}");
}

static void test_print(void) {
  ouo_printerr("print statement\n");

  test_parse(TN("print int"), "print 1");
  test_parse(TN("print expr"), "print 1 + 2");
  test_parse(TN("print ident"), "print a");
  test_parse(TN("print bool"), "print true");
  test_parse(TN("print string"), "print \"hello\"");
}

static void test_fn_call(void) {
  ouo_printerr("function call\n");

  test_parse(TN("call no args"), "f()");
  test_parse(TN("call one arg"), "f(1)");
  test_parse(TN("call two args"), "f(1, 2)");
  test_parse(TN("call with expr args"), "f(1 + 2, x)");
  test_parse(TN("nested calls"), "f(g())");
  test_parse(TN("call result in expr"), "f() + 1");
  test_parse(TN("call in var decl"), "var a = f()");

  test_err_syntax(TN("call unclosed paren"), "f(");
  test_err_syntax(TN("call missing comma"), "f(1 2)");
}

int main(void) {
  test_parse_errors();
  test_module();
  test_int_literals();
  test_float_literals();
  test_bool_literals();
  test_string_literals();
  test_identifiers();
  test_binary_ops();
  test_precedence();
  test_unary_ops();
  test_assignment();
  test_blocks();
  test_var_decl();
  test_fn_decl();
  test_if_expr();
  test_while_expr();
  test_return();
  test_print();
  test_fn_call();

  return test_summary();
}
