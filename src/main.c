#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TK_PUNCT,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token {
  TokenKind kind;
  int value;
  char *location;
  int length;
  struct Token *next;
} Token;

static void error(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static Token *new_token(TokenKind kind, char *start, char *end) {
  Token *token = calloc(1, sizeof(Token));
  token->kind = kind;
  token->location = start;
  token->length = end - start;
  return token;
}

static Token *tokenize(char *p) {
  Token head = {};
  Token *curr = &head;

  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }

    if (isdigit(*p)) {
      char *q = p;
      int value = strtoul(p, &p, 10);
      curr = curr->next = new_token(TK_NUM, q, p);
      curr->value = value;
      continue;
    }

    if (*p == '+' || *p == '-') {
      curr = curr->next = new_token(TK_PUNCT, p, p + 1);
      p++;
      continue;
    }

    error("invalid token");
  }

  curr->next = new_token(TK_EOF, p, p);
  return head.next;
}

static bool equal(Token *token, char *op) {
  return memcmp(token->location, op, token->length) == 0 &&
         op[token->length] == '\0';
}

static Token *skip(Token *token, char *s) {
  if (!equal(token, s)) {
    error("expected '%s'", s);
  }
  return token->next;
}

static int get_number(Token *token) {
  if (token->kind != TK_NUM) error("expected a number");
  return token->value;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of arguments", argv[0]);
  }

  Token *token = tokenize(argv[1]);

  printf("  .global main\n");
  printf("main:\n");

  printf("  mov $%d, %%rax\n", get_number(token));
  token = token->next;

  while (token->kind != TK_EOF) {
    if (equal(token, "+")) {
      printf("  add $%d, %%rax\n", get_number(token->next));
      token = token->next->next;
      continue;
    }

    token = skip(token, "-");
    printf("  sub $%d, %%rax\n", get_number(token));
    token = token->next;
  }

  printf("  ret\n");
  return 0;
}