#include "test.h"

// Wrappers

static char *_src_wrap(const char *setup, const char *type, const char *expr) {
  char *buf = NULL;
  int n;
  if (setup != NULL)
    n = snprintf(NULL, 0, "%s\nvar __r: %s = %s", setup, type, expr);
  else n = snprintf(NULL, 0, "var __r: %s = %s", type, expr);
  buf = ouo_malloc((size_t)n + 1);
  ouo_assert_nomem(buf);
  if (setup != NULL)
    snprintf(buf, (size_t)n + 1, "%s\nvar __r: %s = %s", setup, type, expr);
  else snprintf(buf, (size_t)n + 1, "var __r: %s = %s", type, expr);
  return buf;
}

// static inline void test_exec_ok(const char *name, const char *src) {
//   TestOptions opt = {.stage = TEST_INTERPRET};
//   test(name, src, &opt);
// }

static inline void test_exec_stack_empty(const char *name, const char *src) {
  TestOptions opt = {.stage = TEST_INTERPRET};
  test(name, src, &opt);
}

static inline void test_exec_err(
    const char *name, const char *src, OuoErrorCode code) {
  TestOptions opt = {
      .stage = TEST_INTERPRET,
      .fail = true,
      .fail_code = code,
  };
  test(name, src, &opt);
}

// Object helpers

#define OBJ_INT(v) (&(OuoObject){.kind = OUO_OBJ_INT, .as.v_int = (v)})
#define OBJ_FLOAT(v) (&(OuoObject){.kind = OUO_OBJ_FLOAT, .as.v_float = (v)})
#define OBJ_BOOL(v) (&(OuoObject){.kind = OUO_OBJ_BOOL, .as.v_bool = (v)})

#define _STR_POOL_SIZE 128
static OuoRcStr _str_pool[_STR_POOL_SIZE];
static OuoObject _str_obj_pool[_STR_POOL_SIZE];
static size_t _str_pool_idx = 0;

static OuoObject *OBJ_STR(const char *s) {
  ouo_assertf(_str_pool_idx < _STR_POOL_SIZE, OUO_ERR_OUT_OF_MEMORY,
      "string pool exhausted");
  OuoRcStr *rc = &_str_pool[_str_pool_idx];
  rc->ref.count = 1;
  rc->str.items = (char *)s;
  rc->str.count = strlen(s);
  rc->str.capacity = rc->str.count;
  OuoObject *obj = &_str_obj_pool[_str_pool_idx++];
  obj->kind = OUO_OBJ_STR;
  obj->as.ref = (OuoRc *)rc;
  return obj;
}

static inline void test_exec_int(
    const char *name, const char *setup, const char *expr, ouo_int_t exp) {
  char *src = _src_wrap(setup, "int", expr);
  TestOptions opt = {
      .stage = TEST_INTERPRET,
      .keep_module_scope = true,
      .exp_obj = OBJ_INT(exp),
  };
  test(name, src, &opt);
  ouo_free(src);
}

static inline void test_exec_float(
    const char *name, const char *setup, const char *expr, ouo_float_t exp) {
  char *src = _src_wrap(setup, "float", expr);
  TestOptions opt = {
      .stage = TEST_INTERPRET,
      .keep_module_scope = true,
      .exp_obj = OBJ_FLOAT(exp),
  };
  test(name, src, &opt);
  ouo_free(src);
}

static inline void test_exec_bool(
    const char *name, const char *setup, const char *expr, ouo_bool_t exp) {
  char *src = _src_wrap(setup, "bool", expr);
  TestOptions opt = {
      .stage = TEST_INTERPRET,
      .keep_module_scope = true,
      .exp_obj = OBJ_BOOL(exp),
  };
  test(name, src, &opt);
  ouo_free(src);
}

static inline void test_exec_str(
    const char *name, const char *setup, const char *expr, const char *exp) {
  char *src = _src_wrap(setup, "str", expr);
  TestOptions opt = {
      .stage = TEST_INTERPRET,
      .keep_module_scope = true,
      .exp_obj = OBJ_STR(exp),
  };
  test(name, src, &opt);
  ouo_free(src);
}

// Tests

static void test_int_arith(void) {
  test_exec_int(TN("int literal 0"), NULL, "0", 0);
  test_exec_int(TN("int literal 1"), NULL, "1", 1);
  test_exec_int(TN("int add"), NULL, "1 + 2", 3);
  test_exec_int(TN("int sub"), NULL, "5 - 3", 2);
  test_exec_int(TN("int mul"), NULL, "3 * 4", 12);
  test_exec_int(TN("int div"), NULL, "10 / 2", 5);
  test_exec_int(TN("int div truncates"), NULL, "7 / 2", 3);
  test_exec_int(TN("int negate"), NULL, "-7", -7);
  test_exec_int(TN("int negate zero"), NULL, "-0", 0);
  test_exec_int(TN("int double negate"), NULL, "--3", 3);
  test_exec_int(TN("int add chain"), NULL, "1 + 2 + 3", 6);
  test_exec_int(TN("int sub left-assoc"), NULL, "10 - 3 - 2", 5);
  test_exec_int(TN("int mul then add"), NULL, "2 * 3 + 1", 7);
  test_exec_int(TN("int add then mul prec"), NULL, "1 + 2 * 3", 7);
  test_exec_int(TN("int sub then mul prec"), NULL, "10 - 2 * 3", 4);
  test_exec_int(TN("int div then add"), NULL, "6 / 2 + 1", 4);
  test_exec_int(TN("negate in add lhs"), NULL, "-1 + 2", 1);
  test_exec_int(TN("negate in add rhs"), NULL, "2 + -1", 1);
  test_exec_int(TN("negate then mul"), NULL, "-2 * 3", -6);
  test_exec_int(TN("int large literal"), NULL, "9999999999", 9999999999LL);
}

static void test_float_arith(void) {
  test_exec_float(TN("float literal 0.0"), NULL, "0.0", 0.0);
  test_exec_float(TN("float literal 1.0"), NULL, "1.0", 1.0);
  test_exec_float(TN("float add"), NULL, "1.5 + 2.5", 4.0);
  test_exec_float(TN("float sub"), NULL, "5.0 - 2.5", 2.5);
  test_exec_float(TN("float mul"), NULL, "2.0 * 3.0", 6.0);
  test_exec_float(TN("float div"), NULL, "9.0 / 4.0", 2.25);
  test_exec_float(TN("float negate"), NULL, "-3.0", -3.0);
  test_exec_float(TN("float double negate"), NULL, "--1.5", 1.5);
  test_exec_float(TN("float sub left-assoc"), NULL, "5.0 - 2.0 - 1.0", 2.0);
  test_exec_float(TN("float mul then add"), NULL, "2.0 * 3.0 + 1.0", 7.0);
  test_exec_float(TN("float negate in add"), NULL, "-1.0 + 2.0", 1.0);
}

static void test_bool_logic(void) {
  test_exec_bool(TN("true literal"), NULL, "true", true);
  test_exec_bool(TN("false literal"), NULL, "false", false);
  test_exec_bool(TN("not true"), NULL, "!true", false);
  test_exec_bool(TN("not false"), NULL, "!false", true);
  test_exec_bool(TN("double not true"), NULL, "!!true", true);
  test_exec_bool(TN("double not false"), NULL, "!!false", false);
  test_exec_bool(TN("true and true"), NULL, "true and true", true);
  test_exec_bool(TN("true and false"), NULL, "true and false", false);
  test_exec_bool(TN("false and true"), NULL, "false and true", false);
  test_exec_bool(TN("false and false"), NULL, "false and false", false);
  test_exec_bool(TN("true or true"), NULL, "true or true", true);
  test_exec_bool(TN("true or false"), NULL, "true or false", true);
  test_exec_bool(TN("false or true"), NULL, "false or true", true);
  test_exec_bool(TN("false or false"), NULL, "false or false", false);
  test_exec_bool(TN("not then and"), NULL, "!false and true", true);
  test_exec_bool(TN("and or precedence"), NULL, "false and true or true", true);
  test_exec_bool(
      TN("or and precedence"), NULL, "true or false and false", true);
}

static void test_int_comparison(void) {
  test_exec_bool(TN("1 == 1"), NULL, "1 == 1", true);
  test_exec_bool(TN("1 == 2"), NULL, "1 == 2", false);
  test_exec_bool(TN("1 != 2"), NULL, "1 != 2", true);
  test_exec_bool(TN("1 != 1"), NULL, "1 != 1", false);
  test_exec_bool(TN("1 < 2"), NULL, "1 < 2", true);
  test_exec_bool(TN("2 < 1"), NULL, "2 < 1", false);
  test_exec_bool(TN("2 <= 2"), NULL, "2 <= 2", true);
  test_exec_bool(TN("1 <= 2"), NULL, "1 <= 2", true);
  test_exec_bool(TN("3 <= 2"), NULL, "3 <= 2", false);
  test_exec_bool(TN("2 > 1"), NULL, "2 > 1", true);
  test_exec_bool(TN("1 > 2"), NULL, "1 > 2", false);
  test_exec_bool(TN("2 >= 2"), NULL, "2 >= 2", true);
  test_exec_bool(TN("3 >= 2"), NULL, "3 >= 2", true);
  test_exec_bool(TN("1 >= 2"), NULL, "1 >= 2", false);
  test_exec_bool(TN("neg eq neg"), NULL, "-1 == -1", true);
  test_exec_bool(TN("neg lt zero"), NULL, "-1 < 0", true);
  test_exec_bool(TN("zero gt neg"), NULL, "0 > -5", true);
}

static void test_float_comparison(void) {
  test_exec_bool(TN("1.0 == 1.0"), NULL, "1.0 == 1.0", true);
  test_exec_bool(TN("1.0 == 2.0"), NULL, "1.0 == 2.0", false);
  test_exec_bool(TN("1.0 != 2.0"), NULL, "1.0 != 2.0", true);
  test_exec_bool(TN("1.0 < 2.0"), NULL, "1.0 < 2.0", true);
  test_exec_bool(TN("2.0 > 1.0"), NULL, "2.0 > 1.0", true);
  test_exec_bool(TN("1.5 <= 1.5"), NULL, "1.5 <= 1.5", true);
  test_exec_bool(TN("2.5 >= 2.5"), NULL, "2.5 >= 2.5", true);
  test_exec_bool(TN("1.5 <= 2.5"), NULL, "1.5 <= 2.5", true);
  test_exec_bool(TN("2.5 >= 1.5"), NULL, "2.5 >= 1.5", true);
}

static void test_bool_eq(void) {
  test_exec_bool(TN("true == true"), NULL, "true == true", true);
  test_exec_bool(TN("false == false"), NULL, "false == false", true);
  test_exec_bool(TN("true == false"), NULL, "true == false", false);
  test_exec_bool(TN("true != false"), NULL, "true != false", true);
  test_exec_bool(TN("false != false"), NULL, "false != false", false);
}

static void test_strings(void) {
  test_exec_str(TN("string literal"), NULL, "\"hello\"", "hello");
  test_exec_str(TN("empty string"), NULL, "\"\"", "");
  test_exec_str(
      TN("string concat"), NULL, "\"hello\" + \" world\"", "hello world");
  test_exec_str(TN("concat empty left"), NULL, "\"\" + \"hi\"", "hi");
  test_exec_str(TN("concat empty right"), NULL, "\"hi\" + \"\"", "hi");
  test_exec_str(TN("concat two empties"), NULL, "\"\" + \"\"", "");
  test_exec_str(TN("string escape newline"), NULL, "\"a\\nb\"", "a\nb");
  test_exec_str(TN("string escape tab"), NULL, "\"a\\tb\"", "a\tb");
  test_exec_str(
      TN("string escape quote"), NULL, "\"say \\\"hi\\\"\"", "say \"hi\"");
  test_exec_str(TN("string escape backslash"), NULL, "\"a\\\\b\"", "a\\b");
  test_exec_str(TN("triple concat"), NULL, "\"a\" + \"b\" + \"c\"", "abc");
}

static void test_variables(void) {
  test_exec_int(TN("var int read"), "var a: int = 5", "a", 5);
  test_exec_float(TN("var float read"), "var f: float = 3.14", "f", 3.14);
  test_exec_bool(TN("var bool read"), "var b: bool = true", "b", true);
  test_exec_str(TN("var str read"), "var s: str = \"hi\"", "s", "hi");

  test_exec_int(TN("var inferred int"), "var x = 42", "x", 42);
  test_exec_bool(TN("var inferred bool"), "var x = false", "x", false);

  test_exec_int(TN("var reassign"), "var a: int = 1\na = 2", "a", 2);
  test_exec_int(TN("var reassign self"), "var a: int = 1\na = a + 5", "a", 6);

  test_exec_int(
      TN("two vars sum"), "var a: int = 3\nvar b: int = 4", "a + b", 7);
  test_exec_int(TN("var used in later decl"),
      "var a: int = 5\nvar b: int = a + 1", "b", 6);
  test_exec_int(TN("var in arithmetic"), "var x: int = 10", "x * x - 1", 99);

  test_exec_err(
      TN("var out of scope"), "{\nvar a: int = 1\n}\na", OUO_ERR_SEMANTIC);

  test_exec_err(
      TN("var redeclare"), "var a: int = 1\nvar a: int = 2", OUO_ERR_SEMANTIC);
  test_exec_err(TN("undeclared var"), "x", OUO_ERR_SEMANTIC);
  test_exec_err(TN("assign undeclared"), "x = 5", OUO_ERR_SEMANTIC);
}

static void test_if_exec(void) {
  test_exec_int(TN("if true branch taken"),
      "var a: int = 0\nif (true) {\na = 1\n}", "a", 1);
  test_exec_int(TN("if false branch skipped"),
      "var a: int = 0\nif (false) {\na = 1\n}", "a", 0);
  test_exec_int(TN("if-else true"),
      "var a: int = 0\nif (true) {\na = 1\n} else {\na = 2\n}", "a", 1);
  test_exec_int(TN("if-else false"),
      "var a: int = 0\nif (false) {\na = 1\n} else {\na = 2\n}", "a", 2);
  test_exec_int(TN("if-elseif-else first"),
      "var a: int = 0\nif (true) {\na = 1\n} else if (true) {\na = 2\n} else "
      "{\na = 3\n}",
      "a", 1);
  test_exec_int(TN("if-elseif-else second"),
      "var a: int = 0\nif (false) {\na = 1\n} else if (true) {\na = 2\n} else "
      "{\na = 3\n}",
      "a", 2);
  test_exec_int(TN("if-elseif-else else"),
      "var a: int = 0\nif (false) {\na = 1\n} else if (false) {\na = 2\n} else "
      "{\na = 3\n}",
      "a", 3);
  test_exec_int(TN("if cond compare gt"),
      "var a: int = 5\nvar b: int = 0\nif (a > 3) {\nb = 1\n}", "b", 1);
  test_exec_int(TN("if cond compare false"),
      "var a: int = 2\nvar b: int = 0\nif (a > 3) {\nb = 1\n}", "b", 0);
  test_exec_int(TN("nested if both true"),
      "var a: int = 0\nif (true) {\nif (true) {\na = 7\n}\n}", "a", 7);
  test_exec_int(TN("nested if inner false"),
      "var a: int = 0\nif (true) {\nif (false) {\na = 7\n}\n}", "a", 0);

  test_exec_err(TN("if int condition"), "if (1) {}", OUO_ERR_TYPE);
  test_exec_err(TN("if str condition"), "if (\"hi\") {}", OUO_ERR_TYPE);
  test_exec_err(TN("if float condition"), "if (1.0) {}", OUO_ERR_TYPE);
}

static void test_while_exec(void) {
  test_exec_int(TN("while count to 5"),
      "var i: int = 0\nwhile (i < 5) {\ni = i + 1\n}", "i", 5);
  test_exec_int(TN("while not entered"),
      "var i: int = 10\nwhile (i < 5) {\ni = i + 1\n}", "i", 10);
  test_exec_int(TN("while count down"),
      "var n: int = 5\nwhile (n > 0) {\nn = n - 1\n}", "n", 0);
  test_exec_int(TN("while accumulate sum"),
      "var sum: int = 0\nvar i: int = 1\nwhile (i <= 4) {\nsum = sum + i\ni = "
      "i + 1\n}",
      "sum", 10);
  test_exec_int(TN("while nested count"),
      "var r: int = 0\nvar i: int = 0\nwhile (i < 3) {\nvar j: int = 0\nwhile "
      "(j < 3) {\nr = r + 1\nj = j + 1\n}\ni = i + 1\n}",
      "r", 9);

  test_exec_err(TN("while int condition"), "while (1) {}", OUO_ERR_TYPE);
  test_exec_err(TN("while str condition"), "while (\"x\") {}", OUO_ERR_TYPE);
}

static void test_functions(void) {
  test_exec_int(
      TN("fn returns literal"), "fn answer(): int => 42", "answer()", 42);
  test_exec_int(TN("fn identity int"), "fn id(x: int): int => x", "id(7)", 7);
  test_exec_bool(
      TN("fn identity bool"), "fn id(b: bool): bool => b", "id(true)", true);
  test_exec_str(
      TN("fn identity str"), "fn id(s: str): str => s", "id(\"hi\")", "hi");
  test_exec_float(
      TN("fn identity float"), "fn id(x: float): float => x", "id(2.5)", 2.5);
  test_exec_int(TN("fn add one"), "fn inc(x: int): int => x + 1", "inc(4)", 5);
  test_exec_int(TN("fn two params add"), "fn add(a: int, b: int): int => a + b",
      "add(3, 4)", 7);
  test_exec_int(TN("fn two params sub"), "fn sub(a: int, b: int): int => a - b",
      "sub(10, 3)", 7);
  test_exec_float(TN("fn float div"), "fn half(x: float): float => x / 2.0",
      "half(5.0)", 2.5);
  test_exec_int(TN("fn block body return"),
      "fn f(x: int): int => {\nvar y: int = x + 1\nreturn y\n}", "f(9)", 10);
  test_exec_str(TN("fn string concat"),
      "fn greet(name: str): str => \"hello \" + name", "greet(\"world\")",
      "hello world");
  test_exec_int(TN("fn call in expr"), "fn double(x: int): int => x * 2",
      "double(3) + 1", 7);
  test_exec_int(TN("fn nested calls"), "fn double(x: int): int => x * 2",
      "double(double(3))", 12);
  test_exec_int(TN("fn calls other fn"),
      "fn square(x: int): int => x * x\nfn sum_sq(a: int, b: int): int => "
      "square(a) + square(b)",
      "sum_sq(3, 4)", 25);
  test_exec_int(TN("fn recursive factorial"),
      "fn fact(n: int): int => {\nif (n <= 1) {\nreturn 1\n}\nreturn n * "
      "fact(n - 1)\n}",
      "fact(5)", 120);
  test_exec_int(TN("fn factorial base case"),
      "fn fact(n: int): int => {\nif (n <= 1) {\nreturn 1\n}\nreturn n * "
      "fact(n - 1)\n}",
      "fact(0)", 1);
  test_exec_int(TN("fn recursive fibonacci"),
      "fn fib(n: int): int => {\nif (n <= 1) {\nreturn n\n}\nreturn fib(n - 1) "
      "+ fib(n - 2)\n}",
      "fib(7)", 13);

  test_exec_stack_empty(
      TN("void fn leaves empty stack"), "fn noop() => {}\nnoop()");

  test_exec_err(
      TN("call non-function var"), "var f: int = 5\nf()", OUO_ERR_TYPE);
  test_exec_err(
      TN("fn too few args"), "fn f(x: int): int => x\nf()", OUO_ERR_TYPE);
  test_exec_err(
      TN("fn too many args"), "fn f(x: int): int => x\nf(1, 2)", OUO_ERR_TYPE);
  test_exec_err(
      TN("fn wrong return type"), "fn f(): int => true", OUO_ERR_TYPE);
  test_exec_err(TN("fn wrong return type block"),
      "fn f(): bool => {\nreturn 1\n}", OUO_ERR_TYPE);
  test_exec_err(
      TN("fn wrong arg type"), "fn f(x: int): int => x\nf(1.0)", OUO_ERR_TYPE);
  test_exec_err(TN("fn str arg to int param"),
      "fn f(x: int): int => x\nf(\"hi\")", OUO_ERR_TYPE);
}

static void test_type_errors(void) {
  test_exec_err(TN("int + float"), "var r: int = 1 + 1.0", OUO_ERR_TYPE);
  test_exec_err(TN("float + int"), "var r: float = 1.0 + 1", OUO_ERR_TYPE);
  test_exec_err(TN("int + bool"), "var r: int = 1 + true", OUO_ERR_TYPE);
  test_exec_err(TN("int + str"), "var r: int = 1 + \"hi\"", OUO_ERR_TYPE);
  test_exec_err(TN("bool + bool"), "var r: bool = true + false", OUO_ERR_TYPE);
  test_exec_err(TN("str - str"), "var r: str = \"a\" - \"b\"", OUO_ERR_TYPE);

  test_exec_err(TN("int == float"), "var r: bool = 1 == 1.0", OUO_ERR_TYPE);
  test_exec_err(TN("int == bool"), "var r: bool = 1 == true", OUO_ERR_TYPE);
  test_exec_err(TN("str == int"), "var r: bool = \"a\" == 1", OUO_ERR_TYPE);

  test_exec_err(TN("negate bool"), "var r: bool = -true", OUO_ERR_TYPE);
  test_exec_err(TN("negate str"), "var r: str = -\"hi\"", OUO_ERR_TYPE);
  test_exec_err(TN("not int"), "var r: bool = !1", OUO_ERR_TYPE);
  test_exec_err(TN("not float"), "var r: bool = !1.0", OUO_ERR_TYPE);

  test_exec_err(TN("int and bool"), "var r: bool = 1 and true", OUO_ERR_TYPE);
  test_exec_err(TN("int or bool"), "var r: bool = 1 or true", OUO_ERR_TYPE);

  test_exec_err(TN("if int condition"), "if (1) {}", OUO_ERR_TYPE);
  test_exec_err(TN("while int condition"), "while (1) {}", OUO_ERR_TYPE);

  test_exec_err(TN("var int = float"), "var a: int = 1.0", OUO_ERR_TYPE);
  test_exec_err(TN("var bool = int"), "var a: bool = 1", OUO_ERR_TYPE);
  test_exec_err(TN("var str = int"), "var a: str = 5", OUO_ERR_TYPE);
  test_exec_err(TN("var float = bool"), "var a: float = true", OUO_ERR_TYPE);
}

static void test_semantic_errors(void) {
  test_exec_err(TN("undefined variable"), "x", OUO_ERR_SEMANTIC);
  test_exec_err(TN("undefined in expr"), "x + 1", OUO_ERR_SEMANTIC);
  test_exec_err(TN("undefined fn call"), "f()", OUO_ERR_SEMANTIC);
  test_exec_err(TN("var redeclaration"), "var a: int = 1\nvar a: int = 2",
      OUO_ERR_SEMANTIC);
  test_exec_err(
      TN("var out of scope"), "{\nvar a: int = 1\n}\na", OUO_ERR_SEMANTIC);
}

static void test_stack_discipline(void) {
  test_exec_stack_empty(TN("print int"), "print 1");
  test_exec_stack_empty(TN("print bool"), "print true");
  test_exec_stack_empty(TN("print str"), "print \"x\"");
  test_exec_stack_empty(TN("print expr"), "print 1 + 2");
  test_exec_stack_empty(TN("fn decl only"), "fn f(): int => 42");
  test_exec_stack_empty(
      TN("var + if"), "var a: int = 0\nif (true) {\na = 1\n}");
  test_exec_stack_empty(
      TN("var + while"), "var i: int = 0\nwhile (i < 3) {\ni = i + 1\n}");
  test_exec_stack_empty(TN("void fn call"), "fn noop() => {}\nnoop()");
}

int main(void) {
  test_int_arith();
  test_float_arith();
  test_bool_logic();
  test_int_comparison();
  test_float_comparison();
  test_bool_eq();
  test_strings();
  test_variables();
  test_if_exec();
  test_while_exec();
  test_functions();
  test_type_errors();
  test_semantic_errors();
  test_stack_discipline();

  return test_summary();
}
