#ifndef TEST_H
#define TEST_H

#define OUO_IMPLEMENTATION
#include "../src/ouo.h"

#define TEST_PASS OUO_EBGRN "PASS\n" OUO_ER
#define TEST_FAIL OUO_EBRED "FAIL\n" OUO_ER

typedef struct {
  bool fail;
} TestOptions;

static inline void test(const char *msg, const char *src, TestOptions *opt) {
  ouo_printdbg("%s ", msg);
  OuoParseResult parse_res = ouo_parse(src);

  if (parse_res.failed) {
    ouo_printdbg(opt->fail ? TEST_PASS : TEST_FAIL);
    if (!opt->fail) {
      OUO_DA_FOREACH(OuoError, err, &parse_res.errors) {
        ouo_err_msg_print(err, src, NULL);
      }
    }
    goto parse_defer;
  } else {
    ouo_printdbg(opt->fail ? TEST_FAIL : TEST_PASS);
  }

parse_defer:
  ouo_ast_free(parse_res.ast);
  ouo_da_free(parse_res.errors);
}

#endif // TEST_H
