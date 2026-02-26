/// The ouo language

#ifndef OUO_H_
#define OUO_H_

#define STRINGIZE(x) #x
#define XSTRINGIZE(x) STRINGIZE(x)

#define ouo_abort(expr, err_code, err_msg, ...) \
  do { \
    fprintf(stderr, err_msg "\n", ##__VA_ARGS__); \
    exit(err_code); \
  } while (0)

#define ouo_assertf(expr, err_code, err_msg, ...) \
  if (!(expr)) ouo_abort(expr, err_code, err_msg, ##__VA_ARGS__)

#define ouo_assert(expr, err_code, err_msg, ...) \
  ouo_assertf(expr, err_code, __FILE__ ":" XSTRINGIZE(__LINE__) ": " err_msg, \
              ##__VA_ARGS__)

enum OuoErrCode {
  OUO_OK,
  OUO_ERR_OUT_OF_MEMORY,
  OUO_ERR_FILE_NOT_READ,
  OUO_ERR_INCORRECT_USAGE,
};

void ouo_hi(char *src);

#endif // OUO_H_
