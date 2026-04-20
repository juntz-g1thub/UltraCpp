# UltraCPP 0.1 语言规范

> 版本 0.1 - 初始发布

---

## 1. 概述

### 1.1 语言目标

UltraCPP 是一种系统级编程语言，将 **C++ 语法的熟悉度** 与 **Rust 风格的内存安全保证** 相结合，无需垃圾回收器。

**核心目标：**
- 零成本抽象
- 编译时内存安全
- C++ 兼容性，便于迁移
- 无垃圾回收器

### 1.2 设计原则

1. **默认安全**：在可能的情况下，编译时强制内存安全
2. **显式优于隐式**：所有权、可变性和安全语义在语法中可见
3. **务实兼容**：利用 C++ 开发者的熟悉度
4. **最小运行时**：无重运行时，适用于系统编程

### 1.3 符号约定

| 符号 | 含义 |
|------|------|
| `T*` | 指向 T 的指针 |
| `T&` | T 的引用 |
| `fn(T) -> U` | 函数类型 |
| `alloc(T)` | 为类型 T 分配内存 |
| `free(ptr)` | 释放内存 |
| `unique T*` | 唯一指针（不可复制，可移动） |
| `move(x)` | 转移所有权 |

---

## 2. 词法结构

### 2.1 源文件约定

```
*.uc   — UltraCPP 源文件（推荐）
*.upp  — UltraCPP 源文件（兼容）
```

### 2.2 记号类型

| 类别 | 示例 |
|------|------|
| **关键字** | `if`, `else`, `while`, `for`, `return`, `struct`, `export`, `import`, `const`, `unique`, `move`, `free`, `alloc`, `null`, `true`, `false`, `void`, `extern`, `unsafe`, `typedef` |
| **标识符** | `foo`, `myVariable`, `_private`, `CamelCase` |
| **字面量** | `42`, `3.14`, `'x'`, `"hello"`, `true`, `false` |
| **运算符** | `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`, `&`, `|`, `^`, `~`, `<<`, `>>`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=` |
| **分隔符** | `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `::` |
| **预处理器** | `#import`, `#include`, `#ifdef`, `#ifndef`, `#endif`, `#define` |

### 2.3 关键字（保留字）

```
if          else        while       for         return
struct      export      import      const       typedef
unique      move        free        alloc       null
true        false       void        extern      unsafe
as          static
```

### 2.4 标识符规则

```
identifier ::= letter (letter | digit)*
letter     ::= 'a'..'z' | 'A'..'Z' | '_'
digit      ::= '0'..'9'
```

- 标识符大小写敏感
- 无长度限制（由实现定义）
- 不得与关键字冲突

### 2.5 字面量

#### 整数字面量
```
int-literal ::= decimal | hex | octal | binary
decimal     ::= [1-9][0-9]*
hex         ::= '0x'[0-9a-fA-F]+
octal       ::= '0'[0-7]+
binary      ::= '0b'[01]+
```

#### 浮点字面量
```
float-literal ::= [0-9]+'.'[0-9]+ | '.'[0-9]+ | [0-9]+'.'
```

#### 字符字面量
```
char-literal ::= "'" character "'"
```

#### 字符串字面量
```
string-literal ::= '"' (character | escape)* '"'
escape         ::= '\n' | '\t' | '\r' | '\\' | '\'' | '\"' | '\x'[0-9a-fA-F]{2}
```

#### 布尔字面量
```
true  // 布尔真
false // 布尔假
```

### 2.6 运算符和分隔符

| 运算符 | 描述 |
|--------|------|
| `+` | 加法 |
| `-` | 减法/取负 |
| `*` | 乘法/解引用 |
| `/` | 除法 |
| `%` | 取模 |
| `=` | 赋值 |
| `==` | 相等 |
| `!=` | 不等 |
| `<` | 小于 |
| `>` | 大于 |
| `<=` | 小于等于 |
| `>=` | 大于等于 |
| `&&` | 逻辑与 |
| `\|\|` | 逻辑或 |
| `!` | 逻辑非 |
| `&` | 按位与/取地址 |
| `\|` | 按位或 |
| `^` | 按位异或 |
| `~` | 按位非 |
| `<<` | 左移 |
| `>>` | 右移 |
| `++` | 递增 |
| `--` | 递减 |
| `->` | 箭头（指针成员访问） |
| `.` | 点号（成员访问） |
| `::` | 作用域解析 |

### 2.7 注释

```cpp
// 单行注释

/*
 * 多行注释
 */
```

### 2.8 空白符

空格、制表符和换行符在分隔记号时被忽略。行尾保留用于错误报告。

---

## 3. 类型系统

### 3.1 基本类型

| 类型 | 描述 | 大小 |
|------|------|------|
| `void` | 无值 | - |
| `bool` | 布尔 | 1 字节 |
| `char` | 字符 | 1 字节 |
| `int` | 有符号整数 | 4 字节 |
| `i8` | 8 位有符号 | 1 字节 |
| `i16` | 16 位有符号 | 2 字节 |
| `i32` | 32 位有符号 | 4 字节 |
| `i64` | 64 位有符号 | 8 字节 |
| `uint` | 无符号整数 | 4 字节 |
| `u8` | 8 位无符号 | 1 字节 |
| `u16` | 16 位无符号 | 2 字节 |
| `u32` | 32 位无符号 | 4 字节 |
| `u64` | 64 位无符号 | 8 字节 |
| `f32` | 32 位浮点 | 4 字节 |
| `f64` | 64 位双精度 | 8 字节 |
| `usize` | 无符号大小 | 平台相关 |
| `isize` | 有符号大小 | 平台相关 |

### 3.2 指针类型

```cpp
T*              // 指向 T 的可变指针
const T*        // 指向 const T 的指针（T 通过此指针不可变）
T* const        // 常量指针（指针本身不可改变）
const T* const  // 指向常量 T 的常量指针
void*           // 通用指针（不安全）
```

**示例：**
```cpp
int* p1;              // 指向 int 的可变指针
const int* p2;        // 指向不可变 int 的指针
int* const p3;        // 指向可变 int 的常量指针
const int* const p4;  // 指向不可变 int 的常量指针
void* vptr;           // 通用指针
```

### 3.3 引用类型

```cpp
T&  // T 的引用（必须初始化，不能为空）
```

**示例：**
```cpp
int x = 42;
int& r = x;  // 引用必须初始化
r = 100;     // 修改 x
```

### 3.4 函数指针类型

```cpp
int (*)(int, int)    // 函数类型：接受两个 int，返回 int
void (*)(const char*) // 返回 void 的函数类型
int (*)(int, int)   // 二元函数，返回 int
```

**示例：**
```cpp
typedef int (*Comparator)(int, int);
typedef void (*Callback)(const char*);

Comparator cmp;
Callback cb;
```

### 3.5 数组类型

```cpp
T[n]  // 包含 n 个 T 类型元素的定长数组
```

**示例：**
```cpp
int[10] arr;           // 10 个 int 的数组
char[256] buffer;      // 256 字节的缓冲区
int[3] nums = [1, 2, 3];  // 初始化数组
```

### 3.6 结构体类型

```cpp
struct Point {
    double x;
    double y;
}

struct Rectangle {
    Point origin;
    double width;
    double height;
}
```

### 3.7 类型修饰符

| 修饰符 | 含义 |
|--------|------|
| `const` | 值不可修改 |
| `unique` | 具有排他所有权的指针（不可复制，可移动） |
| `static` | 内部链接 |

---

## 4. 表达式

### 4.1 运算符优先级和结合性

| 优先级 | 运算符 | 结合性 |
|--------|--------|--------|
| 1 | `::` | 左到右 |
| 2 | `()` `[]` `.` `->` `++` `--` | 左到右 |
| 3 | `*` `/` `%` | 左到右 |
| 4 | `+` `-` | 左到右 |
| 5 | `<<` `>>` | 左到右 |
| 6 | `<` `>` `<=` `>=` | 左到右 |
| 7 | `==` `!=` | 左到右 |
| 8 | `&` | 左到右 |
| 9 | `^` | 左到右 |
| 10 | `\|` | 左到右 |
| 11 | `&&` | 左到右 |
| 12 | `\|\|` | 左到右 |
| 13 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | 右到左 |
| 14 | `?:` （三元条件） | 右到左 |
| 15 | `move` | - |

### 4.2 算术运算符

| 运算符 | 描述 | 示例 |
|--------|------|------|
| `+` | 加法 | `a + b` |
| `-` | 减法 | `a - b` |
| `*` | 乘法 | `a * b` |
| `/` | 除法 | `a / b` |
| `%` | 取模 | `a % b` |

### 4.3 比较运算符

| 运算符 | 描述 | 示例 |
|--------|------|------|
| `==` | 相等 | `a == b` |
| `!=` | 不等 | `a != b` |
| `<` | 小于 | `a < b` |
| `>` | 大于 | `a > b` |
| `<=` | 小于等于 | `a <= b` |
| `>=` | 大于等于 | `a >= b` |

### 4.4 逻辑运算符

| 运算符 | 描述 | 示例 |
|--------|------|------|
| `&&` | 逻辑与 | `a && b` |
| `\|\|` | 逻辑或 | `a \|\| b` |
| `!` | 逻辑非 | `!a` |

### 4.5 位运算符

| 运算符 | 描述 | 示例 |
|--------|------|------|
| `&` | 按位与 | `a & b` |
| `\|` | 按位或 | `a \| b` |
| `^` | 按位异或 | `a ^ b` |
| `~` | 按位非 | `~a` |
| `<<` | 左移 | `a << 2` |
| `>>` | 右移 | `a >> 2` |

### 4.6 赋值运算符

| 运算符 | 描述 |
|--------|------|
| `=` | 简单赋值 |
| `+=` | 加法赋值 |
| `-=` | 减法赋值 |
| `*=` | 乘法赋值 |
| `/=` | 除法赋值 |
| `%=` | 取模赋值 |
| `&=` | 按位与赋值 |
| `\|=` | 按位或赋值 |
| `^=` | 按位异或赋值 |
| `<<=` | 左移赋值 |
| `>>=` | 右移赋值 |

### 4.7 条件运算符

```cpp
condition ? expr1 : expr2
```

### 4.8 带括号表达式

```cpp
(expr)  // 括号中的任意表达式
```

---

## 5. 语句

### 5.1 表达式语句

```cpp
x + y;        // 求值并丢弃
func(10);     // 函数调用
```

### 5.2 复合语句（块）

```cpp
{
    int x = 10;
    int y = 20;
    x = x + y;
}
```

### 5.3 if 语句

```cpp
if (condition) {
    // code
}

if (condition) {
    // code
} else {
    // code
}

if (a > b) {
    // code
} else if (a == b) {
    // code
} else {
    // code
}
```

### 5.4 while 语句

```cpp
while (condition) {
    // code
}
```

### 5.5 for 语句

```cpp
for (int i = 0; i < 10; i++) {
    // code
}

for (int x : array) {
    // code
}
```

### 5.6 return 语句

```cpp
return;              // 返回 void
return 42;           // 返回值
return x + y;        // 返回表达式结果
```

### 5.7 break 语句

```cpp
while (true) {
    if (condition) {
        break;       // 退出循环
    }
}
```

### 5.8 continue 语句

```cpp
for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;    // 跳过迭代
    }
}
```

### 5.9 free 语句

```cpp
int* p = alloc(int);
*p = 42;
free(p);  // 释放内存
```

### 5.10 声明语句

```cpp
int x;                // 变量声明
int x = 42;           // 带初始化器的声明
const int y = 100;    // 常量
int* p = alloc(int);  // 指针分配
```

---

## 6. 函数

### 6.1 函数定义

```cpp
int add(int a, int b) {
    return a + b;
}

void greet(const char* name) {
    print(name);
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

### 6.2 函数调用

```cpp
int result = add(10, 20);
greet("Hello");
int fact = factorial(5);
```

### 6.3 参数传递

**传值语义：**
- 参数被复制到函数形参
- 修改形参不影响实参

```cpp
void inc(int x) {
    x = x + 1;  // 不影响调用者
}

int n = 10;
inc(n);
// n 仍然是 10
```

**引用传递用于修改：**
```cpp
void inc(int& x) {
    x = x + 1;  // 影响调用者
}

int n = 10;
inc(n);
// n 现在是 11
```

### 6.4 返回值

```cpp
int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

Point get_point() {
    Point p = {1.0, 2.0};
    return p;  // 按值返回（复制）
}
```

### 6.5 main 函数约定

```cpp
int main() {
    // program code
    return 0;
}
```

`main` 函数：
- 返回 `int`
- 可以不接受参数，或：
  - `int main(int argc, char** argv)`

---

## 7. 内存管理

### 7.1 alloc 函数

```cpp
int* alloc(int)                  // 分配单个元素
int* alloc(int, int count)       // 分配 count 个元素的数组
```

**示例：**
```cpp
int* p = alloc(int);         // 单个 int
int* arr = alloc(int, 10);    // 10 个 int 的数组
char* buf = alloc(char, 256); // 256 字节的缓冲区
```

### 7.2 free 函数

```cpp
free(ptr)    // 释放内存
```

**示例：**
```cpp
int* p = alloc(int);
*p = 42;
free(p);

int* arr = alloc(int, 10);
free(arr);
```

### 7.3 unique 指针语义

`unique` 指针强制**排他所有权**：
- 不可复制
- 可移动
- 离开作用域时自动释放

```cpp
unique int* p1 = alloc(int);
*p1 = 42;

// unique int* p2 = p1;  // 错误：unique 不可复制
unique int* p2 = move(p1);  // 正确：所有权转移
// p1 现在为 null

free(p2);  // 用完后释放
```

### 7.4 move 函数

```cpp
move(ptr)   // 转移所有权
```

**示例：**
```cpp
unique int* p1 = alloc(int);
unique int* p2 = move(p1);  // p1 变为 null
```

### 7.5 null 常量

```cpp
int* p = null;      // 空指针
if (p == null) {    // 比较
    // p 为 null
}
```

### 7.6 指针运算

```cpp
int* ptr = alloc(int, 10);
ptr[0] = 1;          // 索引访问
ptr[5] = ptr[0];    // 复制值

int* p1 = alloc(int, 10);
int* p2 = p1 + 5;   // 偏移指针
```

---

## 8. struct 类型

### 8.1 struct 定义

```cpp
struct Point {
    double x;
    double y;
}

struct Circle {
    Point center;
    double radius;
}
```

### 8.2 字段访问

```cpp
Point p;
p.x = 1.0;
p.y = 2.0;

Circle c;
c.center.x = 0.0;
c.center.y = 0.0;
c.radius = 5.0;
```

### 8.3 内存布局

结构体使用**顺序布局**，可能为对齐添加填充：

```cpp
struct Example {
    char  c;    // 1 字节
    int   i;    // 4 字节（可能添加 3 字节填充）
    short s;    // 2 字节
}
```

### 8.4 作为字段的函数指针

```cpp
struct Comparator {
    int (*compare)(int, int);
    const char* name;
}

int add(int a, int b) { return a + b; }

Comparator cmp = { add, "add" };
int result = cmp.compare(10, 20);  // result = 30
```

---

## 9. 模块和预处理器

### 9.1 #import 语法和语义

导入**模块**（单独编译）：
```cpp
#import "module_name";
```

**示例：**
```cpp
#import "math";
#import "io" as stdio;
```

**行为：**
1. 模块单独编译
2. 通过 `module.symbol` 访问导出的符号
3. 检测并拒绝循环导入

### 9.2 #include 语法和语义

包含**代码片段**（源码复制）：
```cpp
#include "file_path"
```

**示例：**
```cpp
#include "_macros.uc"
#include "_common.uc"
```

**行为：**
- 源代码被逐字复制到 `#include` 位置
- 不单独编译
- 所有符号直接可用

### 9.3 export 声明

从模块导出符号：
```cpp
export int add(int a, int b) {
    return a + b;
}

export const double PI = 3.14159;

export struct Point {
    double x;
    double y;
}
```

没有 `export` 的符号是模块私有的。

### 9.4 模块搜索路径

- 相对于当前文件
- 相对于编译时指定的包含路径
- 标准库路径（由实现定义）

### 9.5 循环依赖检测

```cpp
// a.uc
#import "b";

int func_a() {
    return b.func_b() + 1;
}
```

```cpp
// b.uc
#import "a";  // 错误：检测到循环依赖

int func_b() {
    return 42;
}
```

---

## 10. FFI（外部函数接口）

### 10.1 extern "C" 块

```cpp
extern "C" {
    // C 函数声明
}
```

### 10.2 C 类型映射

| C 类型 | UltraCPP 类型 |
|--------|---------------|
| `T*` | `T*` |
| `const T*` | `const T*` |
| `void*` | `void*` |
| `int (*)(T)` | `int (*)(T)` |
| `int` | `int` |
| `double` | `double` |
| `char` | `char` |
| `char*` | `char*` |

### 10.3 unsafe 块

```cpp
unsafe {
    // FFI 调用和原始指针操作
}
```

**示例：**
```cpp
extern "C" {
    void* malloc(int size);
    void free(void* ptr);
}

char* allocate_buffer(int size) {
    unsafe {
        return malloc(size);
    }
}
```

### 10.4 内联汇编（可选）

```cpp
asm {
    "mov eax, 42";
    "ret";
}
```

---

## 11. 标准库约定

### 11.1 基本 I/O 函数

```cpp
void print_num(int n);        // 打印整数
void print(const char* s);    // 打印字符串
void print_float(double f);   // 打印浮点数
```

### 11.2 字符串函数

```cpp
int strlen(const char* s);  // 字符串长度
char* strcpy(char* dest, const char* src);
int strcmp(const char* a, const char* b);
```

### 11.3 内存函数

```cpp
void* memcpy(void* dest, const void* src, int n);
void* memmove(void* dest, const void* src, int n);
void* memset(void* s, int c, int n);
```

### 11.4 工具函数

```cpp
int sizeof_impl();       // 类型大小
int alignof_impl();       // 类型对齐
bool is_null(int* ptr);  // 空检查
int* move_impl(unique int* ptr);  // 移动所有权
```

---

## 12. 附录

### 12.1 完整 EBNF 语法

```
// === Program ===
program           ::= top_level_declaration*

top_level_declaration
                   ::= function_definition
                    | struct_definition
                    | export_declaration
                    | import_directive
                    | include_directive
                    | const_declaration
                    | ';'

// === Imports ===
import_directive  ::= '#import' string_literal ('as' identifier)? ';'
include_directive ::= '#include' string_literal ';'

// === Export ===
export_declaration ::= 'export' declaration

// === Functions ===
function_definition
                   ::= type identifier '(' parameter_list? ')' compound_statement

parameter_list     ::= parameter (',' parameter)*
parameter          ::= identifier ':' type

// === Struct ===
struct_definition  ::= 'struct' identifier '{' struct_field_list '}'
struct_field_list ::= struct_field (';' struct_field)*
struct_field      ::= identifier ':' type

// === Declarations ===
declaration       ::= variable_declaration
                    | const_declaration

variable_declaration
                   ::= type identifier ('=' expression)? ';'

const_declaration  ::= 'const' type identifier '=' expression ';'

// === Types ===
type              ::= fundamental_type
                    | pointer_type
                    | reference_type
                    | array_type
                    | function_type
                    | type_identifier

fundamental_type  ::= 'void' | 'bool' | 'char' | 'int' | 'i8' | 'i16' | 'i32' | 'i64'
                    | 'uint' | 'u8' | 'u16' | 'u32' | 'u64'
                    | 'f32' | 'f64' | 'usize' | 'isize'

pointer_type      ::= type '*'
                    | type '*' 'const'
reference_type    ::= type '&'
array_type        ::= type '[' integer_literal ']'
function_type     ::= type '(' type_list? ')'

// === Statements ===
statement         ::= expression_statement
                    | compound_statement
                    | selection_statement
                    | iteration_statement
                    | jump_statement
                    | free_statement
                    | declaration_statement

expression_statement
                   ::= expression? ';'

compound_statement ::= '{' statement* '}'

selection_statement
                   ::= 'if' '(' expression ')' statement ('else' statement)?

iteration_statement
                   ::= 'while' '(' expression ')' statement
                    | 'for' '(' for_init? expression? ';' expression? ')' statement
                    | 'for' '(' identifier ':' expression ')' statement

for_init          ::= declaration | expression

jump_statement    ::= 'return' expression? ';'
                    | 'break' ';'
                    | 'continue' ';'

free_statement    ::= 'free' '(' expression ')' ';'

declaration_statement
                   ::= declaration

// === Expressions ===
expression        ::= assignment_expression

assignment_expression
                   ::= conditional_expression
                    | unary_expression '=' assignment_expression
                    | unary_expression compound_assignment assignment_expression

conditional_expression
                   ::= logical_or_expression ('?' expression ':' conditional_expression)?

logical_or_expression
                   ::= logical_and_expression ('||' logical_and_expression)*

logical_and_expression
                   ::= bitwise_or_expression ('&&' bitwise_or_expression)*

bitwise_or_expression
                   ::= bitwise_xor_expression ('|' bitwise_xor_expression)*

bitwise_xor_expression
                   ::= bitwise_and_expression ('^' bitwise_and_expression)*

bitwise_and_expression
                   ::= equality_expression ('&' equality_expression)*

equality_expression
                   ::= relational_expression (('==' | '!=') relational_expression)*

relational_expression
                   ::= shift_expression (('<' | '>' | '<=' | '>=') shift_expression)*

shift_expression   ::= additive_expression (('<<' | '>>') additive_expression)*

additive_expression
                   ::= multiplicative_expression (('+' | '-') multiplicative_expression)*

multiplicative_expression
                   ::= unary_expression (('*' | '/' | '%') unary_expression)*

unary_expression   ::= postfix_expression
                    | ('+' | '-' | '!' | '~' | '*' | '&') unary_expression
                    | 'sizeof' '(' type ')'
                    | 'move' '(' expression ')'

postfix_expression ::= primary_expression
                    | postfix_expression '[' expression ']'
                    | postfix_expression '(' argument_list? ')'
                    | postfix_expression '.' identifier
                    | postfix_expression '->' identifier
                    | postfix_expression '++'
                    | postfix_expression '--'

primary_expression ::= identifier
                    | literal
                    | '(' expression ')'
                    | 'null'

argument_list      ::= expression (',' expression)*

literal            ::= integer_literal
                    | float_literal
                    | char_literal
                    | string_literal
                    | 'true'
                    | 'false'

// === Lexical ===
integer_literal    ::= decimal | hex | octal | binary
hex                ::= '0x' [0-9a-fA-F]+
octal              ::= '0' [0-7]+
binary             ::= '0b' [01]+
decimal            ::= [1-9] [0-9]* | '0'

float_literal      ::= [0-9]+ '.' [0-9]*
                    | '.' [0-9]+
                    | [0-9]+ '.'

char_literal       ::= "'" character "'"
string_literal     ::= '"' (character | escape)* '"'

identifier         ::= letter (letter | digit)*
letter             ::= 'a'..'z' | 'A'..'Z' | '_'
digit              ::= '0'..'9'
```

### 12.2 保留关键字

```
as         break      char       const      continue
else       export     extern     false      fn
for        free       if         import     include
move       null       return     static     struct
true       typedef    unique     unsafe     void
while
```

### 12.3 运算符优先级表

| 级别 | 运算符 | 描述 |
|------|--------|------|
| 1 | `::` | 作用域解析 |
| 2 | `()` `[]` `.` `->` `++` `--` | 后缀 |
| 3 | `*` `/` `%` | 乘法 |
| 4 | `+` `-` | 加法 |
| 5 | `<<` `>>` | 移位 |
| 6 | `<` `>` `<=` `>=` | 关系 |
| 7 | `==` `!=` | 相等 |
| 8 | `&` | 按位与 |
| 9 | `^` | 按位异或 |
| 10 | `\|` | 按位或 |
| 11 | `&&` | 逻辑与 |
| 12 | `\|\|` | 逻辑或 |
| 13 | `?:` | 三元条件 |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | 赋值 |
| 15 | `move` | 所有权转移 |

---

## 文档历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 0.1 | 2026-04-13 | 初始规范 |

---

*UltraCPP 0.1 语言规范*
