#include "test.h"

static inline void test_parse(const char *msg, const char *src) {
  TestOptions opt = {.fail = false};
  test(msg, src, &opt);
}

static inline void test_parse_fail(const char *msg, const char *src) {
  TestOptions opt = {.fail = true};
  test(msg, src, &opt);
}

int main(void) {
  test_parse("single int", "2");
  test_parse("single float", "2.5");
  test_parse("bin op", "2 + 2");
  test_parse("double bin op", "2 + 2 + 2");

  test_parse_fail("empty string fails", "");
  test_parse_fail("unknown symbol fails", "%");
  test_parse_fail("single operator fails", "+");
  test_parse_fail("two numbers fails", "2 2");
  test_parse_fail("unfinished bin op fails", "2 +");
  test_parse_fail("two operators fails", "2 + +");

  test_parse_fail("huge integer fails", "999999999999999999999999999999999999");
  test_parse_fail("huge float fails",
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "999999999999999999999999999999999999999999999999999999999999999999999999"
      "9999999999999999999999999999999999999999.0");

  return 0;
}
