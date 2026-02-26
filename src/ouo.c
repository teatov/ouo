/*
 * The ouo language
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "ouo.h"

/* Lexing */

typedef struct {
  const char *start;
  const char *current;
} OuoLexer;

static bool l_is_eof(OuoLexer *l) { return *l->current == '\0'; }

static bool l_is_digit(char c) { return c >= '0' && c <= '9'; }

static char l_advance(OuoLexer *l) {
  l->current++;
  return l->current[-1];
}

static char l_peek(OuoLexer *l) { return *l->current; }

static char l_peek_next(OuoLexer *l) {
  if (l_is_eof(l)) return '\0';
  return l->current[1];
}

static void l_skip_whitespace(OuoLexer *l) {
  for (;;) {
    char c = l_peek(l);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') l_advance(l);
    else return;
  }
}

static OuoToken l_new_token(OuoLexer *l, OuoTokenKind kind) {
  OuoToken tok = {
      .kind = kind,
      .start = l->start,
      .len = l->current - l->start,
  };
  return tok;
}

static OuoToken l_read_number(OuoLexer *l) {
  OuoTokenKind kind = OUO_TOK_LITERAL_INT;

  while (l_is_digit(l_peek(l))) l_advance(l);

  if (l_peek(l) == '.' && l_is_digit(l_peek_next(l))) {
    kind = OUO_TOK_LITERAL_FLOAT;
    l_advance(l);

    while (l_is_digit(l_peek(l))) l_advance(l);
  }

  return l_new_token(l, kind);
}

static OuoToken l_next_token(OuoLexer *l) {
  l_skip_whitespace(l);
  l->start = l->current;

  if (l_is_eof(l)) return l_new_token(l, OUO_TOK_EOF);

  char c = l_advance(l);

  if (l_is_digit(c)) return l_read_number(l);

  return l_new_token(l, OUO_TOK_ILLEGAL);
}

/* Parsing */

void ouo_parse(const char *src) {
  OuoLexer l = {.start = src, .current = src};
  for (;;) {
    OuoToken tok = l_next_token(&l);
    printf("[%d %.*s] ", tok.kind, tok.len, tok.start);
    if (tok.kind == OUO_TOK_EOF) break;
  }
  printf("\n");
}
