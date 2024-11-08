#include <assert.h>
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

static char *code;

static void error(char *format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static void verror_at(char *location, char *format, va_list ap) {
  int pos = location - code;
  fprintf(stderr, "%s\n", code);
  fprintf(stderr, "%*s", pos, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, format, ap);
  fprintf(stderr, "\n");
  exit(1);
}

static void error_at(char *location, char *format, ...) {
  va_list ap;
  va_start(ap, format);
  verror_at(location, format, ap);
}

static void error_token(Token *token, char *format, ...) {
  va_list ap;
  va_start(ap, format);
  verror_at(token->location, format, ap);
}

/**
 * Tokenize
 */

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

    if (ispunct(*p)) {
      curr = curr->next = new_token(TK_PUNCT, p, p + 1);
      p++;
      continue;
    }

    error_at(p, "invalid token");
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
  if (token->kind != TK_NUM) {
    error_token(token, "expected a number");
  }
  return token->value;
}

/**
 * Parser
 */

typedef enum { ND_ADD, ND_SUB, ND_MUL, ND_DIV, ND_NEG, ND_NUM } NodeKind;

typedef struct Node {
  NodeKind kind;
  int value;
  struct Node *lhs;
  struct Node *rhs;
} Node;

static Node *new_node(NodeKind kind) {
  Node *node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node *new_binary(NodeKind kind, Node *lhs, Node *rhs) {
  Node *node = new_node(kind);
  node->lhs = lhs;
  node->rhs = rhs;
  return node;
}

static Node *new_unary(NodeKind kind, Node *expr) {
  Node *node = new_node(kind);
  node->lhs = expr;
  return node;
}

static Node *new_num(int value) {
  Node *node = new_node(ND_NUM);
  node->value = value;
  return node;
}

static Node *expr(Token **rest, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);

static Node *expr(Token **rest, Token *token) {
  Node *node = mul(&token, token);
  while (true) {
    if (equal(token, "+")) {
      node = new_binary(ND_ADD, node, mul(&token, token->next));
      continue;
    }

    if (equal(token, "-")) {
      node = new_binary(ND_SUB, node, mul(&token, token->next));
      continue;
    }

    *rest = token;

    return node;
  }
}

static Node *mul(Token **rest, Token *token) {
  Node *node = unary(&token, token);
  while (true) {
    if (equal(token, "*")) {
      node = new_binary(ND_MUL, node, unary(&token, token->next));
      continue;
    }

    if (equal(token, "/")) {
      node = new_binary(ND_DIV, node, unary(&token, token->next));
      continue;
    }

    *rest = token;

    return node;
  }
}

static Node *unary(Token **rest, Token *token) {
  if (equal(token, "+")) {
    return unary(rest, token->next);
  }

  if (equal(token, "-")) {
    return new_unary(ND_NEG, unary(rest, token->next));
  }

  return primary(rest, token);
}

static Node *primary(Token **rest, Token *token) {
  Node *node;
  if (equal(token, "(")) {
    node = expr(&token, token->next);
    *rest = skip(token, ")");
    return node;
  }

  if (token->kind == TK_NUM) {
    node = new_num(token->value);
    *rest = token->next;
    return node;
  }

  error_token(token, "error");

  return NULL;
}

/*
 * Codegen
 */

static int depth = 0;

static void push(char *arg) {
  printf("  push %s\n", arg);
  depth++;
}

static void pop(char *arg) {
  printf("  pop %s\n", arg);
  depth--;
}

static void gen_expr(Node *node) {
  if (node->kind == ND_NUM) {
    printf("  mov $%d, %%rax\n", node->value);
    return;
  }

  if (node->kind == ND_NEG) {
    gen_expr(node->lhs);
    printf("  neg %%rax\n");
    return;
  }

  gen_expr(node->rhs);
  push("%rax");
  gen_expr(node->lhs);
  pop("%rdi");

  switch (node->kind) {
    case ND_ADD:
      printf("  add %%rdi, %%rax\n");
      return;
    case ND_SUB:
      printf("  sub %%rdi, %%rax\n");
      return;
    case ND_MUL:
      printf("  imul %%rdi, %%rax\n");
      return;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv %%rdi\n");
      return;
    default:
      error("invalid expression");
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: invalid number of arguments", argv[0]);
  }

  code = argv[1];
  Token *token = tokenize(code);
  Node *node = expr(&token, token);

  if (token->kind != TK_EOF) {
    error_token(token, "extra token");
  }

  printf("  .global main\n");
  printf("main:\n");
  gen_expr(node);
  printf("  ret\n");

  assert(depth == 0);
  return 0;
}