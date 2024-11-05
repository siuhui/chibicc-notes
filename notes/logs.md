# logs

看到有人推荐学习 [chibicc](https://github.com/rui314/chibicc) 这个项目，这是一个 small C compiler，实现了大部分 C11 特性。

记录一下从第一个 commit 开始看代码和学习的过程

## Compile an integer to an exectuable that exits with the given number

commit `0522e2d77e3ab82d3b80a5be8dbbdc8d4180561c`

### `.globl main` 是什么意思

- `.globl`: 伪指令，表示接下来的符号是全局的
- `main`: 符号名称，在这里指的是 main 函数
  
通过使用 `.globl main` 告诉汇编器和链接器 main 函数是一个全局符号，可以在链接阶段被其他模块访问。

### `printf("  mov $%d, %%rax\n", atoi(argv[1]));` 什么意思

- `$` 表示立即数，`%d` 是占位符，会被替换
- `%` 表示寄存器，为什么要两个 %，因为在 printf 中，% 为特殊字符，需要 %% 来表示实际的 %
- rax 是 x86-64 架构中的一个 64 位通用寄存器。它是 eax 寄存器的扩展版本，eax 是 32 位寄存器，而 rax 是 64 位寄存器

这条指令将立即数移动到 rax 寄存器中

### x86-64 还有哪些寄存器？

### `ret` 这条指令是什么意思

ret 指令在 x86-64 汇编语言中用于实现子程序返回机制。这条指令会从硬件支持的内存栈中弹出一个代码位置，然后无条件跳转到该代码位置。

在函数调用过程中，ret 指令用于返回到调用函数的下一条指令位置。它通过弹出栈顶的返回地址来实现这一点，这个返回地址是在调用函数时由 call 指令压入栈中的。

### 怎么测试的

用的是 bash 脚本

流程是：

1. 把 chibicc 的输出重定向到 tmp.s 文件中
2. 用 gcc 把汇编文件 tmp.s 汇编和链接，生成可执行文件 tmp
3. 运行 tmp，把实际结果和预期结果对比

这样做有个问题，退出状态码的取值是 0 到 255，所以 tmp 只有返回之间的数，才能正常测试。

### gcc 的 -static 是什么意思

`-static`选项表示生成静态链接的可执行文件。静态链接会将所有需要的库直接包含在可执行文件中，这样生成的可执行文件在运行时不需要依赖外部库。

### bash脚本里的 `actual="$?"` 什么意思

`$?` 是一个特殊变量，用于存储上一个命令的退出状态码

## Add + and - operators

commit `bf7081fba7d8c6b1cd8a12eb329697a5481c604e`

### strtol 函数是什么

`strtol` 是 C 标准库中的一个函数，用于将字符串转换为长整型数（`long int`）。

函数原型：

```c
long int strtol(const char *str, char **endptr, int base);
```

参数：

- str: 要转换的字符串。
- endptr: 指向字符串中第一个不能转换的字符的指针。如果不需要，可以设置为 `NULL`。
- base: 基数，必须介于 2 和 36（包含）之间，或者是特殊值 0。特殊值 0 表示根据字符串前缀判断进制（如 `0x` 表示十六进制，`0` 表示八进制，否则为十进制）。

返回值：

- 成功时，返回转换后的长整型数。
- 如果不能转换或字符串为空，返回 0。
- 如果转换结果超出 `long int` 的表示范围，返回 `LONG_MAX` 或 `LONG_MIN`，并设置 `errno` 为 `ERANGE`.

### add 和 sub 指令是什么样的

```assembly
add source, destination
sub source, destination
```

- destination: 可以是寄存器或内存位置。
- source: 可以是寄存器、内存位置或立即数。

destination 加/减 source，结果储存在 destination

## Add a tokenizer to allow space characters between tokens

commit `a1ab0ff26f23c82f15180051204eeb6279747c9a`

把输入的字符串通过 tokenize 函数转换为 tokens 链表，再处理 tokens 链表，生成汇编代码

### 支持了哪些类型的 token

```c
typedef enum {
  TK_PUNCT, // Punctuators，标点
  TK_NUM,   // Numeric literals
  TK_EOF,   // End-of-file markers
} TokenKind;
```

### token 怎么组成

```c
struct Token {
  TokenKind kind; // Token kind
  Token *next;    // Next token
  int val;        // If kind is TK_NUM, its value
  char *loc;      // Token location
  int len;        // Token length
};
```

### 有哪些和 token 相关的操作

- `static bool equal(Token *tok, char *op)`: 判断 tok 内容和字符串 op 是不是相同
- `static Token *skip(Token *tok, char *s)`: 跳过当前的内容是字符串 s 的 token，到下一个节点；如果内容不是 s 就报错
- `static int get_number(Token *tok)`: 得到数字类型 token 的值
- `static Token *new_token(TokenKind kind, char *start, char *end)`: 生成 token 节点

### 回顾一下 C 中 static 的作用

static 关键字的作用具体取决于它的使用位置：

**在函数内部**：

当 `static` 变量声明在函数内部时，它是一个局部变量，但它的生命周期是整个程序运行期间。也就是说，即使函数结束后，`static` 变量的值也会被保留，并在下次调用该函数时继续使用

**在函数外部或全局变量**：

当 `static` 变量声明在函数外部或作为全局变量时，它的作用域仅限于声明它的文件。这意味着其他文件中的代码无法访问或修改这个 `static` 变量。这种用法通常用于实现模块化编程，防止命名冲突。

**在函数声明前**：

当 `static` 用于函数声明前时，这个函数的作用域也仅限于声明它的文件。其他文件无法调用这个函数。

### C 的可变参数

```c
type va_arg(va_list arg_ptr, type);
void va_end(va_list arg_ptr);
void va_start(va_list arg_ptr, prev_param);
```

### fprintf 和 vfprintf

**`fprintf`**:

- 用于将格式化的输出写入指定的文件流。
- 函数原型：`int fprintf(FILE *stream, const char *format, ...);`
- 例如：`fprintf(file, "Hello, %s!", name);`

**`vfprintf`**:

- 是 `fprintf` 的变体，用于处理可变参数列表。
- 函数原型：`int vfprintf(FILE *stream, const char *format, va_list arg);`
- 适用于需要传递 `va_list` 类型参数的情况。
- 例如：在实现一个日志函数时，可以使用 `vfprintf` 来处理可变参数列表：

```c
void log(FILE *file, const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(file, "%s: ", getTimestamp());
    vfprintf(file, format, args);
    va_end(args);
}
```

## Improve error message

commit `cc5a6d978144bda90220bd10866c4fd908d07546`

感觉只是粗略的处理了一下，代码在终端显示有多行的时候 ^ 不能指到出错的位置。

### `fprintf(stderr, "%*s", pos, "")` 为什么会打印空格

## Add *, / and ()

commit `84cfcaf98f3d19c8f0f316e22a61725ad201f0f6`

做表达式的 parser 和 codegen

### NodeKind

```c
typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // Integer
} NodeKind;
```

前四个是二元操作符，所以又抽象出构造二元节点的函数，ND_NUM 对应数字节点

### 文法

```
expr:
  expr -> expr "+" mul
        | expr "-" mul
        | mul

mul:
  mul -> mul "*" primary
       | mul "/" primary
       | primary

primary:
  primary -> "(" expr ")"
           | NUM
```

### push 和 pop 指令

```assembly
push <src>
```

`<src>`：可以是立即数、寄存器或内存地址

```assembly
pop <dst>
```

`<dst>`：可以是寄存器或内存地址

### 怎么从 AST 生成汇编

### 先处理 rhs 或 lhs 有什么区别吗

这里先处理 rhs, 在处理减和除这种有顺序的运算好像要方便一点，因为最后结果放在 rax 上。

### `cqo` 和 `idiv`