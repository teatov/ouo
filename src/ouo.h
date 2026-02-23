/// The ouo language

#ifndef OUO_H_
#define OUO_H_

enum OuoError {
  OUO_OK,
  OUO_ERR_OUT_OF_MEMORY,
  OUO_ERR_FILE_NOT_READ,
  OUO_ERR_INCORRECT_USAGE,
};

void ouo_hi(char *src);

#endif // OUO_H_
