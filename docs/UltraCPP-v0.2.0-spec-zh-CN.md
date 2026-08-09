# UltraCPP 0.2.0 语言规范

> **版本**：0.2.0
>
> **上一版本**：0.1.0 — [`UltraCPP-v0.1.0-spec-zh-CN.md`](./UltraCPP-v0.1.0-spec-zh-CN.md)
>
> **状态**：草稿 (draft) — 等待用户接受
>
> **日期**：2026-08-07
>
> **语言权威性**：本中文版为**权威版本**；英文版 [`UltraCPP-v0.2.0-spec-en.md`](./UltraCPP-v0.2.0-spec-en.md) 为对照译本，如有歧义以本文为准。
>
> **实施决策**：本规范修订基于 `src-c/` C 主机作为生产编译器的决策 (2026-08)。

---

## 修订摘要

本版本在 0.1.0 的基础上，落实 8 条语言设计决策 (D-1 .. D-8)。所有未列出的章节**与 0.1.0 逐字一致**。

| 决策 | 一句话摘要 |
|------|-----------|
| **D-1** | `unique` 是泛型类型构造器，可作用于任意类型 `unique T`，不再是硬编码的 `unique int`。 |
| **D-2** | 一元前缀 `&` 的含义确定为**不可变借用 (immutable borrow)**，不再是 "取地址"。 |
| **D-3** | `&mut` 正式进入语言：新增记号、新增类型 `T&mut`、新增表达式产生式。 |
| **D-4** | `move(p)` 是**内建 primitive**（不是语法糖）：运行时将源指针置为 null。 |
| **D-5** | 对 `unique T` 的赋值 `p1 = p2` 是 move 语义：**源** (`p2`) 置 null，目标 (`p1`) 接管所有权。 |
| **D-6** | `alloc` / `free` 由程序员负责配对，编译器**不强制**检查；`MissingFree` / `DoubleFree` 保留为未来扩展。 |
| **D-7** | 定义 `DanglingReference` 的触发条件，并给出正反例与诊断格式。 |
| **D-8** | 本规范修订基于 `src-c/` C 主机作为生产编译器的决策 (2026-08)。 |

---

## 变更日志 (Changelog)

格式：`D-N | §章节 | 一句话描述`

| 决策 | 章节 | 描述 |
|------|------|------|
| D-1 | §3.7 | `unique` 条目改写为泛型类型构造器，明确 `unique T` 对任意 `T` 合法。 |
| D-1 | §12.1 | EBNF 新增 `unique_type ::= 'unique' type` 产生式，并接入 `type`；不再暗示 `unique int`。 |
| D-2 | §1.3 | 符号约定表 `T&` 行改为「T 的不可变借用」，新增 `&expr` 行。 |
| D-2 | §2.6 | `&` 行由「按位与/取地址」改为「一元前缀：不可变借用；二元中缀：按位与」，删除 "取地址" 表述。 |
| D-2 | §3.3 | 引用类型章节改写：`T&` 明确为**不可变借用**的类型。 |
| D-2 | §4.5 | 位运算符表为 `&` 增加说明：该表仅描述**二元中缀**形式。 |
| D-2 | §7.8 | **新增**「不可变借用 `&T`」，规定 `&T` 的创建规则与并发借用规则。 |
| D-2 | §12.3 | 优先级表第 8 级 `&` 描述限定为「按位与（二元中缀）」。 |
| D-3 | §1.3 | 符号约定表新增 `T&mut` 与 `&mut expr` 行。 |
| D-3 | §2.2 | 记号类型表的「运算符」行新增 `&mut`；「关键字」行新增 `mut`。 |
| D-3 | §2.3 | 保留字列表新增 `mut`。 |
| D-3 | §2.6 | 运算符表新增 `&mut` 记号行。 |
| D-3 | §3.3 | 新增类型 `T&mut`（可变借用）。 |
| D-3 | §7.9 | **新增**「可变借用 `&mut T`」，规定排他性规则。 |
| D-3 | §12.1 | EBNF：`unary_expression` 新增 `'&' 'mut' unary_expression` 产生式；`type` 新增 `mutable_reference_type ::= type '&' 'mut'`。 |
| D-3 | §12.2 | 保留关键字列表新增 `mut`。 |
| D-4 | §7.4 | `move(p)` 由「函数 / 语法糖」改写为「内建 primitive」；明确运行时将源置 null；引用 devhandbook §4.8 `record_move` 作为实现参考。 |
| D-4 | §1.3 | `move(ptr)` 行补充「内建 primitive」措辞。 |
| D-5 | §7.1 | 新增显式规则行：`p1 = p2` 中**源** `p2` 置 null，目标 `p1` 接管所有权；并补充函数实参与返回值的 move 时点。 |
| D-5 | §7.1 / §3.2 | **勘误核查结论**：0.1.0 原文写作 `int* p2 = p1; // p1 变为 null`，其中 `p1` 是**源**，与 Rust move 语义**一致**，因此无需修正，仅补充显式规则。审计 §7-5 提到的 "p1 becomes null" 疑虑在此澄清。详见下方「关于 D-5 的勘误说明」。 |
| D-6 | §11.5 | **新增**「`alloc` / `free` 配对约定」：由程序员负责配对，编译器不强制检查；`MissingFree` / `DoubleFree` 记为未来扩展方向。 |
| D-7 | §7.10 | **新增**「悬垂引用 (Dangling Reference)」：定义两条触发条件 (a)(b)，给出反例 + 正例与诊断格式。 |
| D-8 | 文档头部 | 新增一行：本规范修订基于 `src-c/` C 主机作为生产编译器的决策 (2026-08)。 |
| D-8 | 附录 12.4 | **新增**「对未来工作的影响」。 |

### 关于 D-5 的勘误说明

决策 D-5 的原始表述使用 `p1 = p2`（`p1` 为目标、`p2` 为源），而 0.1.0 规范的示例使用 `int* p2 = p1;`（`p2` 为目标、`p1` 为源）。两处的 `p1` / `p2` 角色**恰好相反**。

逐字核对 0.1.0 §3.2、§7.1、§7.4 后确认：**这三处原文均将「源」置 null**，与 Rust move 语义一致，**不存在语义错误**，因此本版本**不改写**这些示例，只在 §7.1 增补一条与变量命名无关的规范性规则，消除歧义。

### 章节编号说明

- 决策 D-7 原文要求「§7 加 7.7 悬垂引用」，但 0.1.0 的 §7.7 已被「指针运算」占用。为**保留未变章节原样**，本版本将新增章节顺延为 §7.8 / §7.9 / §7.10，§7.7 指针运算保持不动。
- 决策 D-6 原文要求「§9 (标准库) 加一节」，但 0.1.0 的 §9 是「模块和预处理器」，标准库是 §11。因此该节写入 §11.5。

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

### 1.3 符号约定 *(0.2.0 修订：D-2, D-3, D-4)*

| 符号 | 含义 |
|------|------|
| `T*` | 指向 T 的指针（默认不可复制，赋值即转移所有权） |
| `unique T` | T 的排他所有权指针（泛型形式，见 §3.7） |
| `T&` | T 的**不可变借用**（immutable borrow） |
| `T&mut` | T 的**可变借用**（mutable borrow） |
| `&expr` | 创建对 `expr` 的不可变借用（见 §7.8） |
| `&mut expr` | 创建对 `expr` 的可变借用（见 §7.9） |
| `T(*)(U)` | 函数指针类型（C++ 风格） |
| `alloc(T)` | 为类型 T 分配内存 |
| `free(ptr)` | 释放内存 |
| `move(ptr)` | **内建 primitive**：转移所有权（源指针在运行时变为 null，见 §7.4） |
| `clone(ptr)` | 克隆指针（复制一份，独立所有权） |

---

## 2. 词法结构

### 2.1 源文件约定

```
*.uc   — UltraCPP 源文件（推荐）
*.upp  — UltraCPP 源文件（兼容）
```

### 2.2 记号类型 *(0.2.0 修订：D-3)*

| 类别 | 示例 |
|------|------|
| **关键字** | `if`, `else`, `while`, `for`, `return`, `struct`, `export`, `import`, `const`, `unique`, `mut`, `move`, `free`, `alloc`, `null`, `true`, `false`, `void`, `extern`, `unsafe`, `typedef`, `clone` |
| **标识符** | `foo`, `myVariable`, `_private`, `CamelCase` |
| **字面量** | `42`, `3.14`, `'x'`, `"hello"`, `true`, `false` |
| **运算符** | `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`, `&`, `&mut`, `|`, `^`, `~`, `<<`, `>>`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=` |
| **分隔符** | `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `::` |
| **预处理器** | `#import`, `#include`, `#ifdef`, `#ifndef`, `#endif`, `#define` |

> **[0.2.0 · D-3]** `&mut` 是**单一记号**（token），词法分析器在读到 `&` 后须向前查看紧邻的 `mut` 关键字并合并；`&` 与 `mut` 之间允许空白符。`mut` 同时成为保留字，不得用作标识符。

### 2.3 关键字（保留字） *(0.2.0 修订：D-3)*

```
if          else        while       for         return
struct      export      import      const       typedef
unique      move        free        alloc       null
true        false       void        extern      unsafe
as          static      clone       mut
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

### 2.6 运算符和分隔符 *(0.2.0 修订：D-2, D-3)*

| 运算符 | 描述 |
|--------|------|
| `+` | 加法 |
| `-` | 减法/取负 |
| `*` | 乘法/解引用 |
| `/` | 除法 |
| `%` | 取模 |
| `=` | 赋值（指针类型时触发所有权转移，见 §7.1） |
| `==` | 相等 |
| `!=` | 不等 |
| `<` | 小于 |
| `>` | 大于 |
| `<=` | 小于等于 |
| `>=` | 大于等于 |
| `&&` | 逻辑与 |
| `\|\|` | 逻辑或 |
| `!` | 逻辑非 |
| `&` | **一元前缀：引用 / borrow（不可变借用，见 §7.8）**；二元中缀：按位与 |
| `&mut` | **一元前缀：引用 / borrow（可变借用，见 §7.9）** |
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

> **[0.2.0 · D-2]** 0.1.0 中 `&` 的描述为「按位与/取地址」。本版本**删除 "取地址" 语义**：UltraCPP 没有独立的取地址运算符，一元前缀 `&` 一律解释为**不可变借用**。二元中缀 `&`（如 `a & b`）仍是按位与，两种形式由语法位置区分（见 §12.1 的 `bitwise_and_expression` 与 `unary_expression`）。

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

**所有指针默认不可复制，赋值即转移所有权。**

```cpp
T*              // 指向 T 的指针（不可复制，赋值转移所有权）
const T*        // 指向 const T 的只读指针
T* const        // 常量指针（指针本身不可改变）
const T* const  // 指向 const T 的常量指针
void*           // 通用指针（不安全）
```

**示例：**
```cpp
int* p1;              // 指向 int 的指针，不可复制
int* p2 = p1;         // p1 变为 null，p2 获得所有权
int* p3 = clone(p1);  // 克隆一份，两个指针独立

const int* cp;        // 指向不可变 int 的只读指针
int* const cp2;        // 指向可变 int 的常量指针
const int* const cp3; // 指向不可变 int 的常量指针
void* vptr;            // 通用指针
```

> **[0.2.0 · D-5]** 上例中 `int* p2 = p1;` 的**源**是 `p1`、**目标**是 `p2`，注释「p1 变为 null」指的是**源被置 null**，与 §7.1 的规范性规则一致。

### 3.3 引用类型（借用类型） *(0.2.0 修订：D-2, D-3)*

UltraCPP 有两种借用类型。二者都**必须初始化**、**不能为 null**，且**不拥有**被指向的值——所有权始终留在 owner 处。

```cpp
T&     // T 的不可变借用（immutable borrow）：只读，可同时存在多个
T&mut  // T 的可变借用（mutable borrow）：可写，同一时刻至多一个
```

**`T&` —— 不可变借用**

`T&` 是**不可变借用的类型**。通过 `T&` 只能读取被借用的值，任何写入都是编译期错误。同一 owner 在同一时刻可以有任意多个 `T&`。

**两种形式对比**（0.1.0 与 0.2.0 并存展示，0.2.0 为推荐形式）：

**0.1.0 形式（保留对照）**

```cpp
int x = 42;
int& r = x;      // 0.1.0 写法：省略取址符，等价于隐式 &x
r = 100;         // 通过引用写入（C++ 语义）
```

**0.2.0 形式（推荐）**

```cpp
int x = 42;
int& r1 = &x;    // 不可变借用，显式取址
int& r2 = &x;    // ✅ 多个不可变借用可以并存
int y = r1;      // ✅ 读取
// r1 = 100;     // ❌ 错误：不能通过不可变借用写入
```

**对比说明**：
- (i) 0.2.0 要求借用类型的初始化式必须是一个**借用表达式** `&x`，使借用点在语法上可见（设计原则 §1.2「显式优于隐式」）。
- (ii) 0.1.0 的无 `&` 写法 `int& r = x;` 作为**兼容形式**保留，编译器可将其视为隐式的 `&x` 并给出提示；该写法在 0.2.0 下不会改变不可变借用的语义（仍不能写入）。

**`T&mut` —— 可变借用**

`T&mut` 允许通过借用写入。同一 owner 在同一时刻**至多只能有一个** `T&mut`，且它不能与任何 `T&` 并存。

**0.2.0 形式（推荐，0.1.0 无对应形式）**

```cpp
int x = 42;
int&mut m = &mut x;  // 可变借用
m = 100;             // ✅ 修改 x，此后 x == 100
// int& r = &x;      // ❌ 错误：m 仍然活跃，不可变借用与可变借用不能并存
```

借用的创建规则见 §7.8（不可变）与 §7.9（可变）；生命周期约束见 §7.10。

> **[0.2.0 · D-2]** 0.1.0 的示例写作 `int& r = x;`（无 `&`）。本版本统一为 `int& r = &x;`：借用类型的初始化式必须是一个**借用表达式**，使借用点在语法上可见（设计原则 §1.2「显式优于隐式」）。0.1.0 的无 `&` 写法作为兼容形式保留，编译器可将其视为隐式的 `&x` 并给出提示。

### 3.4 函数指针类型

```cpp
int (*)(int, int)       // 函数类型：接受两个 int，返回 int（C++ 风格）
void (*)(const char*)   // 返回 void 的函数指针
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

### 3.7 类型修饰符 *(0.2.0 修订：D-1, D-3)*

| 修饰符 | 含义 |
|--------|------|
| `const` | 值不可修改 |
| `unique` | **泛型类型构造器**：`unique T` 表示对任意类型 `T` 的排他所有权指针（与 `T*` 的默认行为一致，可省略） |
| `mut` | 仅作为 `&mut` 记号的组成部分出现（见 §2.6），不单独用作类型修饰符 |
| `static` | 内部链接 |

#### 3.7.1 `unique` 是泛型类型构造器 *(0.2.0 新增：D-1)*

> **[0.2.0 · D-1]** `unique` 是**泛型类型构造器**，可用于任意类型 `unique T`。它**不是**硬编码的 `unique int`，也不是只能修饰单一内建类型的关键字。

形式上：对任意类型 `T`，`unique T` 是一个合法类型，其运行时表示与 `T*` 相同，其静态语义为「对 `T` 的排他所有权」。

```cpp
unique int      x1;   // 等价于 int*
unique char     x2;   // 等价于 char*
unique Point    x3;   // 用户定义 struct 同样合法
unique int*     x4;   // T 本身是指针类型时也合法（对 int* 的排他所有权）
unique int[10]  x5;   // T 为数组类型
```

**规则：**

1. `unique T` 中的 `T` 可以是 §3.1–§3.6 的**任意类型**，包括用户定义的 `struct`、数组类型和指针类型。
2. `unique T` 与 `T*` 在类型系统中**等价**；`unique` 只是把「排他所有权」这一默认属性写成显式形式。
3. `unique` 不能作用于借用类型：`unique T&` 与 `unique T&mut` 是**非法**的——借用不拥有值，谈不上排他所有权。
4. 语法上的位置见 §12.1 的 `unique_type ::= 'unique' type`。

**编译器实现注记**：0.1.0 时期的两套解析器都把 `unique` 无条件降级为 `Pointer(Int)`（`src-c/src/parser.c` 与 `src/frontend/parser.rs`），这与本条规则冲突，属于实现缺陷，须按本节修正。

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
| 15 | `move` `clone` | - |

> **[0.2.0 · D-2, D-3]** 表中第 8 级的 `&` 指**二元中缀**按位与。一元前缀的 `&`（不可变借用）与 `&mut`（可变借用）属于一元运算符，与 `*`、`!`、`~` 同级，优先级高于所有二元运算符。

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

### 4.5 位运算符 *(0.2.0 修订：D-2)*

> **[0.2.0 · D-2]** 本表仅描述**二元中缀**形式。一元前缀 `&` 不是位运算符，而是不可变借用（见 §7.8）。

| 运算符 | 描述 | 示例 |
|--------|------|------|
| `&` | 按位与（二元中缀） | `a & b` |
| `\|` | 按位或 | `a \| b` |
| `^` | 按位异或 | `a ^ b` |
| `~` | 按位非 | `~a` |
| `<<` | 左移 | `a << 2` |
| `>>` | 右移 | `a >> 2` |

### 4.6 赋值运算符

| 运算符 | 描述 |
|--------|------|
| `=` | 简单赋值（指针类型时触发所有权转移） |
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

**引用传递用于修改（两种形式对比）：**

**0.1.0 形式（保留对照）**

```cpp
void inc(int& x) {     // 0.1.0：用 T& 兼作可写形参
    x = x + 1;         // 通过引用写入
}

int n = 10;
inc(n);                // 0.1.0：调用点直接传变量名
// n 现在是 11
```

**0.2.0 形式（推荐）**

```cpp
void inc(int&mut x) {  // 0.2.0：可变借用形参
    x = x + 1;         // 影响调用者
}

int n = 10;
inc(&mut n);           // 0.2.0：调用点写明 &mut n
// n 现在是 11
```

**对比说明**：
- (i) 因为 0.2.0 的 `T&` 是**不可变**借用（§3.3），通过它写入是编译期错误，所以「用于修改」的形参必须改为 `T&mut`（决策 D-2 与 D-3 的结合）。
- (ii) 调用点 `&mut n` 显式表达「可变借用」的意图。
- (iii) 只读形参继续使用 `T&`：
  ```cpp
  int read(int& x) { return x; }   // 只读，使用不可变借用
  int n = 10;
  int v = read(&n);
  ```

> **[0.2.0 · D-2, D-3]** 0.1.0 此处写作 `void inc(int& x)` 并调用 `inc(n)`。因为 `T&` 现在明确是**不可变**借用（§3.3），通过它写入是编译期错误，所以「用于修改」的形参类型必须改为 `int&mut`，调用点必须写出 `&mut n`。只读形参继续使用 `T&`。

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

> **[0.2.0 · D-7]** 按**值**返回局部变量（如上例的 `return p;`）始终合法。返回指向局部变量的**借用**则是悬垂引用，见 §7.10。

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

### 7.1 所有权语义 *(0.2.0 修订：D-5)*

**所有指针默认不可复制，赋值操作触发所有权转移（move 语义）。**

```cpp
int* p1 = alloc(int);
int* p2 = p1;    // p1 变为 null，所有权转移给 p2
int* p3 = p2;    // p2 变为 null，所有权转移给 p3
```

#### 7.1.1 赋值的 move 规则 *(0.2.0 新增：D-5)*

> **[0.2.0 · D-5]** 对 `unique T`（等价地，`T*`）类型的赋值 `p1 = p2` 是 **move 语义**：`p2`（**源**）在赋值后变为 null，`p1` 接管所有权。

规范性表述，与变量命名无关：

> 设赋值语句为 `dst = src`，其中 `dst` 与 `src` 的类型均为 `unique T`。求值后：
> 1. `dst` 成为该分配的**唯一所有者**；
> 2. `src` 在运行时的值变为 `null`；
> 3. `src` 在静态检查中被标记为 **moved-out**，此后对 `src` 的任何**使用**（读取、解引用、再次 move、传参、`free`）都是编译期错误 `UseAfterMove`；
> 4. 对 `src` 的**重新赋值**（`src = <新值>`）合法，并使其重新变为已初始化状态。

例（`p1` 为目标、`p2` 为源，与决策 D-1..D-8 的命名一致）：

```cpp
unique int p2 = alloc(int);
*p2 = 7;

unique int p1 = p2;   // move：p2 变为 null，p1 接管所有权

// int v = *p2;       // ❌ 编译期错误 UseAfterMove：p2 已被移出
int v = *p1;          // ✅ v == 7
free(p1);             // ✅ 由新所有者释放
```

**发生 move 的其他位置**（均适用上述四条）：

| 位置 | 源 | 目标 |
|------|----|------|
| 变量初始化 `unique T d = s;` | `s` | `d` |
| 赋值语句 `d = s;` | `s` | `d` |
| 函数实参 `f(s)`（形参为 `unique T`） | `s` | 形参 |
| 函数返回 `return s;`（返回类型为 `unique T`） | `s` | 调用方接收处 |
| 显式 `move(s)`（见 §7.4） | `s` | `move` 表达式的结果 |

**不发生 move 的位置**：`clone(s)`（见 §7.5）、`&s` 与 `&mut s`（借用，见 §7.8 / §7.9）、`s == null` 等仅读取指针值的比较。

**勘误核查**：0.1.0 §7.1 与 §3.2 的示例写作 `int* p2 = p1; // p1 变为 null`。其中 `p1` 是**源**，因此「源被置 null」的结论与 Rust move 语义一致，**原文无误**，本版本予以保留。

### 7.2 alloc 函数

```cpp
int* alloc(int)                  // 分配单个元素
int* alloc(int, int count)      // 分配 count 个元素的数组
```

**示例：**
```cpp
int* p = alloc(int);         // 单个 int
int* arr = alloc(int, 10);    // 10 个 int 的数组
char* buf = alloc(char, 256); // 256 字节的缓冲区
```

### 7.3 free 函数

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

> **[0.2.0 · D-6]** `alloc` 与 `free` 的配对由程序员负责，编译器**不强制**检查，见 §11.5。

### 7.4 move(p) 表达式 *(0.2.0 修订：D-4)*

> **[0.2.0 · D-4]** `move(p)` 是语言的**内建 primitive**，**不是**语法糖，也**不是**普通函数。0.1.0 将本节命名为「move 函数」并描述为对赋值行为的简写；本版本将其提升为一等语义构造。

**语法**（见 §12.1 `unary_expression`）：

```
move '(' expression ')'
```

**语义：**

1. `move(p)` 的**操作数必须是一个可寻址的左值**，其类型为 `unique T`。`move(alloc(int))`、`move(f())` 这类以右值为操作数的写法是编译期错误——右值本就没有需要失效的所有者。
2. `move(p)` 的**结果**是 `p` 原先持有的指针值，类型为 `unique T`。
3. **运行时效果**：求值 `move(p)` 后，编译器**必须**发出把 `p` 的存储单元写为 `null` 的代码。这是本条决策的关键——source 变 null 是**可观测的运行时行为**，而不仅是编译期的静态标记。因此下面的程序有确定行为：

   ```cpp
   unique int p1 = alloc(int);
   unique int p2 = move(p1);
   // 运行时 p1 的存储单元此刻确实是 null
   ```

4. **静态效果**：`p` 被标记为 moved-out，此后对 `p` 的任何使用都是编译期错误 `UseAfterMove`（规则同 §7.1.1）。
5. `move` **不产生新的分配**，也不复制被指向的对象；它只转移所有权。
6. `move(p)` 与隐式 move（`q = p`）的**语义完全相同**；`move` 的价值在于让转移点在代码中显式可见，以及在实参位置等隐式 move 可能被误读的场合消除歧义。

```cpp
unique int p1 = alloc(int);
*p1 = 42;

unique int p2 = move(p1);   // p1 运行时变为 null，静态标记为 moved-out

if (p1 == null) {           // ❌ 编译期错误 UseAfterMove：p1 已被移出
    // 即使运行时 p1 确实是 null，静态检查仍然拒绝对 moved-out 变量的读取
}

*p2 = 43;                   // ✅
free(p2);                   // ✅
```

**实现参考**：C 主机的所有权检查器应按 `.dev/plans/0.1.0-devhandbook.md` §4.8「移动语义」的 `record_move(OwnershipChecker* checker, const char* var, size_t location)` 实现静态标记部分；运行时置 null 的那条存储指令由代码生成阶段发出。

### 7.5 clone 函数

`clone()` 复制指针，两份独立所有权。

```cpp
int* p1 = alloc(int);
int* p2 = clone(p1);  // p1 和 p2 独立，各自释放
```

### 7.6 null 常量

```cpp
int* p = null;      // 空指针
if (p == null) {    // 比较
    // p 为 null
}
```

### 7.7 指针运算

```cpp
int* ptr = alloc(int, 10);
ptr[0] = 1;          // 索引访问
ptr[5] = ptr[0];    // 复制值

int* p1 = alloc(int, 10);
int* p2 = p1 + 5;   // 偏移指针
```

### 7.8 不可变借用 `&T` *(0.2.0 新增：D-2)*

> **[0.2.0 · D-2]** 本节是新增章节，规定一元前缀 `&` 的语义。0.1.0 没有借用规则，`&` 被描述为「取地址」。

**语法**（见 §12.1 `unary_expression`）：

```
'&' unary_expression
```

**`&expr` 创建一个对 `expr` 所指对象的不可变借用**，结果类型为 `T&`，其中 `T` 是 `expr` 的类型。借用**不转移所有权**：`expr` 的 owner 保持不变，也不会被置 null。

#### 7.8.1 创建规则

1. **操作数必须是左值。** `&x`、`&s.field`、`&arr[i]`、`&*p` 合法；`&42`、`&(a + b)`、`&f()` 是编译期错误。
2. **owner 必须处于活跃且已初始化的状态。** 对 moved-out 的变量取借用是 `UseAfterMove`；对已 `free` 的分配取借用是 `UseAfterDrop`。
3. **借用是只读的。** 通过 `T&` 写入（`r = v;`、`*r = v;`、`r.field = v;`）是编译期错误。
4. **借用不能超出 owner 的生命周期。** 违反时报 `DanglingReference`，见 §7.10。
5. **`&` 不改变 owner 的所有权状态**，因此 `&p` 之后 `p` 仍然可用、仍然需要被 `free`。

#### 7.8.2 并存规则

- 同一 owner 在同一时刻可以有**任意多个** `T&`。
- `T&` **不能**与该 owner 的任何活跃 `T&mut` 并存（见 §7.9）。
- 借用的活跃区间从创建点开始，到该借用的**最后一次使用**为止（非词法作用域，与 devhandbook §4.7 的借用检查算法一致）。

```cpp
int x = 42;
int& r1 = &x;
int& r2 = &x;        // ✅ 多个不可变借用并存
int sum = r1 + r2;   // ✅ 读取
// r1 = 100;         // ❌ 错误：不可变借用不可写
x = 7;               // ✅ r1 / r2 在此之后不再使用，借用已结束
```

对被拥有的分配取借用：

```cpp
unique int p = alloc(int);
*p = 10;

int& r = &*p;    // 借用 p 指向的 int，p 仍是所有者
int v = r;       // ✅ v == 10

free(p);         // ✅ r 在此之前已结束使用
```

#### 7.8.3 诊断

```
error[E0502]: cannot assign through an immutable borrow
  --> src/main.uc:4:5
   |
 3 |     int& r1 = &x;
   |               --- immutable borrow of `x` created here
 4 |     r1 = 100;
   |     ^^^^^^^^ cannot write through `int&`
   |
   = help: use `int&mut r1 = &mut x;` if you need to modify `x`
```

### 7.9 可变借用 `&mut T` *(0.2.0 新增：D-3)*

> **[0.2.0 · D-3]** 本节是新增章节。0.1.0 的语法中根本没有 `&mut`，devhandbook §4.5–§4.7 描述的借用冲突规则因此无法从规范的文法到达。本节修复该缺口。

**语法**（见 §12.1 `unary_expression`）：

```
'&' 'mut' unary_expression
```

**`&mut expr` 创建一个对 `expr` 所指对象的可变借用**，结果类型为 `T&mut`。与 `&` 相同，可变借用**不转移所有权**。

#### 7.9.1 创建规则

1. **操作数必须是左值**（同 §7.8.1 第 1 条）。
2. **owner 必须是可变的。** 对 `const` 变量或 `const T*` 所指对象取 `&mut` 是编译期错误。
3. **owner 必须处于活跃且已初始化的状态**（同 §7.8.1 第 2 条）。
4. **可变借用可写。** 通过 `T&mut` 读写均合法。
5. **借用不能超出 owner 的生命周期**（见 §7.10）。

#### 7.9.2 排他性规则

这是借用检查的核心约束，与 devhandbook §4.5 的三条规则一致：

1. 允许**多个**不可变借用 `T&` 并存；
2. 允许**至多一个**可变借用 `T&mut`；
3. 不可变借用与可变借用**不能同时存在**。

违反任意一条报 `BorrowConflict`。

```cpp
int x = 42;

int&mut m = &mut x;   // ✅ 唯一的可变借用
m = 100;              // ✅ 写入，此后 x == 100
int v = m;            // ✅ 读取

// 以下三种写法在 m 仍活跃时都是错误：
// int&mut m2 = &mut x;  // ❌ BorrowConflict：第二个可变借用
// int&  r   = &x;       // ❌ BorrowConflict：可变借用活跃期间不可再取不可变借用
// int   w   = x;        // ❌ BorrowConflict：可变借用活跃期间不可通过 owner 读取
```

借用结束后 owner 恢复完全可用：

```cpp
int x = 42;
{
    int&mut m = &mut x;
    m = 100;              // m 的最后一次使用
}
int v = x;                // ✅ 借用已结束，v == 100
int& r = &x;              // ✅ 现在可以取不可变借用
```

#### 7.9.3 诊断

```
error[E0499]: cannot borrow `x` as mutable more than once at a time
  --> src/main.uc:4:18
   |
 3 |     int&mut m  = &mut x;
   |                  ------ first mutable borrow of `x` occurs here
 4 |     int&mut m2 = &mut x;
   |                  ^^^^^^ second mutable borrow of `x` occurs here
 5 |     m = 100;
   |     ------- first borrow is still in use here
   |
   = note: `&mut T` grants exclusive access; only one may be live at a time
```

### 7.10 悬垂引用 (Dangling Reference) *(0.2.0 新增：D-7)*

> **[0.2.0 · D-7]** 本节定义 devhandbook §4.6 的 `DanglingReference` 错误在用户语言层面的**触发条件**。0.1.0 只在 §3.3 定义了 `T&` 类型，从未说明它何时失效。

**定义**：一个 `T&` 或 `T&mut` 借用是**悬垂 (dangling)** 的，当它在其 owner 的存储被销毁之后仍然可达。UltraCPP 在编译期拒绝所有可静态判定的悬垂引用，错误种类为 `DanglingReference`。

#### 7.10.1 触发条件

编译器在下列两种情况下报 `DanglingReference`：

**(a) `&T` / `&mut T` 的 owner 已离开作用域。**
借用的活跃区间延伸到了 owner 的作用域结束之后。owner 可以是块内的局部变量，也可以是被 `free` 释放的分配（后者按同一条规则处理：`free(p)` 结束 `p` 所指对象的存储生命期）。

**(b) 函数返回引用，但 owner 是该函数的局部变量。**
函数返回类型为 `T&` 或 `T&mut`，而返回表达式借用的是形参之外的函数局部变量、局部数组或函数内 `alloc` 且未转移出去的分配。这是 (a) 的一个特例，因为函数体结束时所有局部变量都离开作用域；单独列出是因为它是最常见的形态，且诊断信息不同。

**不触发**的情况：

- 借用的 owner 是**函数形参**中的借用（`T&` / `T&mut`），且返回类型的生命周期由该形参提供 —— 借用只是被转发，owner 在调用方仍然活跃。
- 借用在 owner 离开作用域**之前**就完成了最后一次使用。
- 按**值**返回（`T`、`unique T`）—— 值被移动或复制出去，不产生借用。

#### 7.10.2 示例 (a)：owner 离开作用域

**反例（应被拒绝）：**

```cpp
int main() {
    int& r;               // 尚未初始化的借用
    {
        int x = 42;
        r = &x;           // 借用 x
    }                     // ← x 在此离开作用域
    return r;             // ❌ DanglingReference：r 指向已销毁的 x
}
```

诊断：

```
error[E0597]: `x` does not live long enough
  --> src/main.uc:5:5
   |
 4 |         int x = 42;
   |             - binding `x` declared here
 5 |         r = &x;
   |             ^^ borrowed value does not live long enough
 6 |     }
   |     - `x` dropped here while still borrowed
 7 |     return r;
   |            - borrow later used here
   |
   = note: the borrow must not outlive the owner's scope
```

**正例（应被接受）：**

```cpp
int main() {
    int result;
    {
        int x = 42;
        int& r = &x;      // 借用 x
        result = r;       // r 的最后一次使用，借用在此结束
    }                     // ← x 离开作用域时已无活跃借用
    return result;        // ✅
}
```

#### 7.10.3 示例 (b)：函数返回指向局部变量的引用

**反例（应被拒绝）：**

```cpp
int& make_answer() {
    int x = 42;
    return &x;            // ❌ DanglingReference：x 是局部变量
}
```

诊断：

```
error[E0106]: cannot return reference to local variable `x`
  --> src/main.uc:3:12
   |
 2 |     int x = 42;
   |         - `x` is a local variable of `make_answer`
 3 |     return &x;
   |            ^^ returns a reference to data owned by the current function
   |
   = help: return `int` by value, or return `unique int` to transfer ownership
```

**正例（应被接受）——转发形参借用：**

```cpp
int& first(int& a, int& b) {
    return &a;            // ✅ owner 在调用方，借用只是被转发
}

int main() {
    int x = 1;
    int y = 2;
    int& r = first(&x, &y);
    return r;             // ✅ x 在 main 中仍然活跃
}
```

**正例（应被接受）——按值或转移所有权返回：**

```cpp
int make_answer_by_value() {
    int x = 42;
    return x;             // ✅ 按值返回（复制）
}

unique int make_answer_owned() {
    unique int p = alloc(int);
    *p = 42;
    return p;             // ✅ move：所有权转移给调用方（见 §7.1.1）
}
```

#### 7.10.4 与 `free` 的交互

`free(p)` 结束 `p` 所指对象的存储生命期。此后仍活跃的借用同样触发 `DanglingReference`：

```cpp
int main() {
    unique int p = alloc(int);
    *p = 10;

    int& r = &*p;
    free(p);              // ← 存储在此销毁
    return r;             // ❌ DanglingReference：r 指向已释放的分配
}
```

**实现参考**：生命周期推断按 `.dev/plans/0.1.0-devhandbook.md` §12.4「借用检查算法」实现；错误种类沿用 §4.6 的 `DanglingReference { size_t location, size_t owner_location }`，两个位置分别对应上述诊断中的「borrow later used here」与「binding declared here」。

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

### 9.1 预处理阶段

UltraCPP 在词法分析之前运行**预处理阶段**，处理以下指令：

| 指令 | 行为 |
|------|------|
| `#include "path"` | 展开文件内容到当前位置 |
| `#import "module"` | 记录模块依赖（不展开内容） |
| `export <declaration>` | 标记导出的函数/变量 |

**预处理输出**：
1. 展开 `#include` 后的纯代码
2. 模块依赖列表
3. 导出符号列表

### 9.2 #include 语法和语义

**源码复制**：`#include` 将指定文件的内容**逐字复制**到 `#include` 指令位置。

```cpp
#include "file_path"
```

**示例：**
```cpp
// main.upp
#include "../../lib/math.uc"

int main() {
    int result = add(5, 3);  // add() 定义在 math.uc 中
    return result;
}
```

**行为**：
- `#include` 后的代码直接获得被包含文件的所有定义
- 不需要额外的声明即可调用被包含文件中的函数
- 文件路径相对于**当前源文件目录**解析

**搜索路径顺序**：
1. 相对于当前源文件目录
2. 相对于项目根目录
3. 相对于指定的 include 路径

### 9.3 #import 语法和语义

**模块导入**：`#import` 记录对某个模块的依赖，**不展开其内容**。

```cpp
#import "module_path"
#import "module_path" as alias
```

**示例：**
```cpp
#import "math";           // 导入 math 模块
#import "io" as stdio;    // 导入 io 模块并起别名
```

**行为**：
1. 记录模块依赖，但不展开源代码
2. 被导入模块需要单独编译
3. 调用模块函数需要 `extern` 声明或 `#include` 其接口

**生成的依赖文件**：
```json
{
  "module": "math",
  "file": "lib/math.uc",
  "exports": ["add", "multiply"]
}
```

### 9.4 export 声明

标记函数或变量为**模块导出**，外部可以通过 `#import` 访问。

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

**规则**：
- 没有 `export` 的符号是**模块私有**的
- `export` 只能在模块顶层使用
- 导出的函数/变量可以被 `#import` 该模块的代码访问

### 9.5 模块设计规则

UltraCPP 的模块系统遵循以下规则：

1. **定义函数不需要声明** - 同文件内函数调用无需前置声明
2. **外部模块函数需要 import** - 调用其他模块的函数必须 `#import` 该模块
3. **内部函数不可外部访问** - 未 export 的函数只能模块内部使用

**示例**：
```cpp
// lib/math.uc
export int add(int a, int b);  // 导出接口

int _internal_helper(int x) {  // 内部函数，外部不可见
    return x + 1;
}

int add(int a, int b) {
    return _internal_helper(a) + _internal_helper(b);
}
```

```cpp
// test/main.upp
#include "../../lib/math.uc"

int main() {
    add(5, 3);           // ✅ 可以调用
    _internal_helper(1); // ❌ 不可调用（未导出）
    return 0;
}
```

### 9.6 模块搜索路径

| 路径类型 | 解析方式 |
|----------|----------|
| 相对路径 | 相对于当前源文件目录 |
| 绝对路径 | 相对于项目根目录 |
| 指定路径 | 通过 `-I` 参数添加的搜索路径 |

### 9.7 循环依赖检测

```cpp
// a.uc
#include "b.uc"

int func_a() {
    return func_b() + 1;
}
```

```cpp
// b.uc
#include "a.uc"  // 错误：检测到循环依赖

int func_b() {
    return 42;
}
```

**检测规则**：如果两个模块相互 `#include`，编译器应报告错误。

### 9.8 编译流程

```
源代码 (.uc/.upp)
    │
    ▼
┌─────────────────┐
│   预处理阶段     │
│  - 展开 #include │
│  - 记录 #import  │
│  - 解析 export   │
└─────────────────┘
    │
    ▼
┌─────────────────┐
│   词法分析       │
│   语法分析       │
│   代码生成       │
└─────────────────┘
    │
    ▼
  LLVM IR (.ll)
    │
    ▼
  llc (汇编)
    │
    ▼
  目标文件 (.o)
    │
    ▼
  链接器 (gcc/ld)
    │
    ▼
  可执行文件
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

> **[0.2.0 · D-2, D-3, D-7]** `unsafe` 块内**抑制**所有权与借用检查：§7.1.1 的 `UseAfterMove`、§7.9.2 的 `BorrowConflict`、§7.10 的 `DanglingReference` 在 `unsafe` 块内不报告。`unsafe` **不改变**语义——`move` 仍然把源置 null（§7.4），`&` 仍然是借用（§7.8）——它只是把安全责任转移给程序员。

### 10.4 内联汇编

嵌入汇编必须放在 `unsafe` 块中。

**语法：**
```cpp
unsafe {
    asm {
        "assembly instruction"
        : [name] "constraint"(output_var)
        : [name] "constraint"(input_var)
        : "clobbered_register"
    }
}
```

**示例：**
```cpp
int result;
int input = 10;

unsafe {
    asm {
        "mov eax, %[in_val]"
        "add eax, 5"
        "mov %[out_val], eax"
        : [out_val] "=r"(result)
        : [in_val] "r"(input)
        : "eax"
    }
}
```

**约束字符：**
| 约束 | 含义 |
|------|------|
| `r` | 通用寄存器 |
| `=` | 输出寄存器 |
| `&` | early clobber（不得重用） |
| `m` | 内存引用 |

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
int alignof_impl();      // 类型对齐
bool is_null(int* ptr);  // 空检查
int* clone_impl(int* ptr);  // 指针克隆
```

### 11.5 `alloc` / `free` 配对约定 *(0.2.0 新增：D-6)*

> **[0.2.0 · D-6]** `alloc(T)` 与 `free(p)` 的配对**由程序员负责**，编译器**不强制**检查。

**规范性规则：**

1. 每一次成功的 `alloc(T)` 都产生一个新的分配，其所有权归接收该结果的 `unique T` 变量。
2. 每一个分配**应当**恰好被 `free` 一次。这是一条**程序员责任**，不是编译器强制的静态约束。
3. 编译器**不**报告以下情况：
   - **未释放**（分配后没有对应的 `free`）——内存泄漏；
   - **重复释放**（同一分配被 `free` 两次）；
   - **释放非 `alloc` 产物**（对栈变量的借用调用 `free`）。
4. 编译器**仍然**报告的相关错误（它们由所有权跟踪而非配对分析得出）：
   - 对已被 move 出去的变量调用 `free` → `UseAfterMove`（§7.1.1）；
   - 对已 `free` 的变量再次**使用**（读取、解引用、借用）→ `UseAfterDrop`；
   - `free` 之后仍有活跃借用 → `DanglingReference`（§7.10.4）。

**理由：** 0.2.0 的检查器边界划在**所有权与借用**上，不划在**分配计数**上。判定 `MissingFree` 需要完整的过程间可达性分析，判定 `DoubleFree` 在存在条件分支与循环时需要路径敏感分析；两者都超出本版本的实现范围，且都会在缺乏完整分析时产生大量误报。

**约定用法：**

```cpp
unique int p = alloc(int);
*p = 42;
// ... 使用 p ...
free(p);                  // 程序员负责的配对
```

跨函数转移所有权时，`free` 的责任随所有权一起转移：

```cpp
unique int make() {
    unique int p = alloc(int);
    *p = 42;
    return p;             // 所有权转移给调用方，本函数不 free
}

int main() {
    unique int q = make();
    free(q);              // 调用方负责 free
    return 0;
}
```

#### 11.5.1 未来扩展方向

以下错误种类**在本版本中不实现**，记录在此作为后续版本的候选：

| 候选错误 | 含义 | 所需分析 |
|----------|------|----------|
| `MissingFree` | 分配在所有路径上都未被释放（内存泄漏） | 过程间可达性 + 逃逸分析 |
| `DoubleFree` | 同一分配在某条路径上被释放两次 | 路径敏感的所有权状态机 |

若将来实现，二者应作为**可选诊断**（如 `--warn-leaks` / `--strict-free`）引入，默认关闭，以免破坏既有代码。

#### 11.5.2 作为未来可选安全级别 *(0.2.0 决策细化：2026-08-07)*

> **本小节是 §11.5 关于 `alloc`/`free` 配对策略的正式承诺**：0.2.0 **不强制** `alloc`/`free` 配对；未来版本可能将其作为**可选安全编译级别**激活。

**0.2.0 的承诺（明确边界）**：

1. 0.2.0 把 `alloc`/`free` 的配对视为**程序员责任**。编译器不报告 `MissingFree`、`DoubleFree`、或「`free` 一个非 `alloc` 产物」。
2. 0.2.0 **不会**提供强制启用上述检查的命令行选项。本规范的这一边界不随前端 flag 变化。
3. 错误枚举的权威来源是 `devhandbook` §4.6（`OwnershipError`）；0.2.0 报告的与所有权/借用相关的错误种类即 §11.5 第 4 条所列三项（`UseAfterMove`、`UseAfterDrop`、`DanglingReference`）。

**未来版本（0.3.0+）的方向（仅作为候选，不构成 0.2.0 承诺）**：

1. 后续版本**可以**引入一个可选的安全编译级别，候选名称如 `--safe-mem`（或等价形式），用以激活 `MissingFree` / `DoubleFree` 检查。该级别将**默认关闭**，仅在用户显式打开时生效。
2. 该级别的语义将以 0.2.0 的 §11.5.1 表为起点：在 0.2.0 已经划出的「所有权/借用」检查之上，再叠加「分配计数」检查。
3. 本规范的**未来修订**将确定具体的 flag 名、激活条件、以及与现有诊断级别的关系。

**理由**：在 0.2.0 上线时强制 `alloc`/`free` 配对检查会产生大量误报（见 §11.5 第 1 段「理由」），与现有代码的兼容性代价过高，因此把该能力延后为可选项，而非默认行为。

---

## 12. 附录

### 12.1 完整 EBNF 语法 *(0.2.0 修订：D-1, D-3)*

> **[0.2.0 · D-1, D-3]** 相对 0.1.0 的改动共 4 处，全部为**新增**，未删除或修改任何既有产生式：
> 1. `type` 新增两个候选：`unique_type`、`mutable_reference_type`；
> 2. 新增 `unique_type ::= 'unique' type`（D-1）；
> 3. 新增 `mutable_reference_type ::= type '&' 'mut'`（D-3）；
> 4. `unary_expression` 新增 `'&' 'mut' unary_expression`（D-3）。
>
> 改动处在下方以 `// [0.2.0 D-N]` 行内注释标出。

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
                    | unique_type              // [0.2.0 D-1]
                    | reference_type
                    | mutable_reference_type   // [0.2.0 D-3]
                    | array_type
                    | function_type
                    | type_identifier

fundamental_type  ::= 'void' | 'bool' | 'char' | 'int' | 'i8' | 'i16' | 'i32' | 'i64'
                    | 'uint' | 'u8' | 'u16' | 'u32' | 'u64'
                    | 'f32' | 'f64' | 'usize' | 'isize'

pointer_type      ::= type '*'
                    | type '*' 'const'

// [0.2.0 D-1] `unique` is a generic type constructor: `unique T` for any T.
// It is NOT hard-coded to `unique int`.
unique_type       ::= 'unique' type

reference_type    ::= type '&'

// [0.2.0 D-3] mutable borrow type `T&mut`
mutable_reference_type
                   ::= type '&' 'mut'

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

// NOTE: binary infix '&' remains bitwise AND. Unary prefix '&' is a borrow.
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
                    | '&' 'mut' unary_expression        // [0.2.0 D-3] mutable borrow
                    | 'sizeof' '(' type ')'
                    | 'move' '(' expression ')'
                    | 'clone' '(' expression ')'

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

#### 12.1.1 关于 `&` / `&mut` 的解析注记 *(0.2.0 新增：D-2, D-3)*

`'&' 'mut' unary_expression` 与 `('&') unary_expression` 在 `&` 之后需要 **1 个记号的前瞻**：若紧邻的记号是关键字 `mut`，走可变借用产生式；否则走不可变借用产生式。因为 `mut` 是保留字（§2.3），`&x` 与 `&mut x` 之间不存在歧义。

`bitwise_and_expression` 中的 `&` 与 `unary_expression` 中的 `&` 由**语法位置**区分：前者出现在一个完整的操作数之后（中缀），后者出现在操作数之前（前缀）。这与 C/C++ 处理 `*`（乘法 / 解引用）的方式相同，递归下降解析器无需额外机制。

### 12.2 保留关键字 *(0.2.0 修订：D-3)*

```
as         break      char       const      continue
else       export     extern     false      for
free       if         import     include    move
mut        null       return     static     struct
true       typedef    unique     unsafe     void
while      clone
```

> **[0.2.0 · D-3]** 新增 `mut`。它只出现在 `&mut` 记号中（§2.6），但作为保留字，不得用作标识符。

### 12.3 运算符优先级表 *(0.2.0 修订：D-2)*

| 级别 | 运算符 | 描述 |
|------|--------|------|
| 1 | `::` | 作用域解析 |
| 2 | `()` `[]` `.` `->` `++` `--` | 后缀 |
| 3 | `*` `/` `%` | 乘法 |
| 4 | `+` `-` | 加法 |
| 5 | `<<` `>>` | 移位 |
| 6 | `<` `>` `<=` `>=` | 关系 |
| 7 | `==` `!=` | 相等 |
| 8 | `&` | 按位与（**二元中缀**） |
| 9 | `^` | 按位异或 |
| 10 | `\|` | 按位或 |
| 11 | `&&` | 逻辑与 |
| 12 | `\|\|` | 逻辑或 |
| 13 | `?:` | 三元条件 |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | 赋值 |
| 15 | `move` `clone` | 所有权操作 |

**一元运算符**（优先级高于上表所有二元运算符，右到左结合）：

| 运算符 | 描述 |
|--------|------|
| `+` `-` | 正号 / 取负 |
| `!` | 逻辑非 |
| `~` | 按位非 |
| `*` | 解引用 |
| `&` | **不可变借用**（§7.8） |
| `&mut` | **可变借用**（§7.9） |

### 12.4 附录 X：对未来工作的影响 *(0.2.0 新增：D-8)*

本节记录本次规范修订对后续实现工作的约束，不属于语言定义本身。

#### 12.4.1 C 主机是生产编译器

本规范修订基于 **`src-c/` C 主机作为生产编译器**的决策 (2026-08)。所有由 0.2.0 引入的语义（D-1 .. D-7）都以 C 主机为**首个且唯一的**落地目标。

C 主机的所有权检查器实现将基于本规范；具体算法参考 `.dev/plans/0.1.0-devhandbook.md` 的以下章节：

| 本规范章节 | devhandbook 参考 | 内容 |
|-----------|------------------|------|
| §7.1.1 赋值 move 规则 | §4.4 所有权跟踪 | 变量所有权状态表 |
| §7.1.1 / §7.4 moved-out 标记 | §4.8 移动语义 (`record_move`) | 移动点记录 |
| §7.8 / §7.9 借用规则 | §4.5 所有权规则、§4.7 借用检查算法 | 三条借用并存规则 |
| §7.9.2 `BorrowConflict` | §4.6 所有权错误、§4.7 `check_borrow_conflict` | 冲突判定 |
| §7.10 `DanglingReference` | §4.6 所有权错误、§12.4 借用检查算法 | 生命周期推断 |
| 全部诊断 | §12.2 所有权跟踪 (`ownership_checker_*`) | 检查器接口 |

实现须新增的产物（按依赖顺序）：

1. **词法**：`mut` 关键字与 `&mut` 记号（`src-c/include/uc_token.h`、`src-c/src/lexer.c`）。
2. **文法**：`unique_type`、`mutable_reference_type`、`&mut` 一元产生式（`src-c/src/parser.c`）；同时修正 `unique` 被硬编码为 `Pointer(Int)` 的缺陷（D-1）。
3. **错误种类**：`UC_ERR_OWNERSHIP` 及其子类 `UseAfterMove` / `UseAfterDrop` / `BorrowConflict` / `DanglingReference`（`src-c/include/uc_error.h`）。
4. **新的编译阶段**：在 `uc_parser_parse` 与 `uc_codegen_generate` 之间插入所有权检查（`src-c/src/main.c`）。
5. **代码生成**：`move(p)` 与隐式 move 的「源置 null」存储指令（`src-c/src/codegen.c`，见 §7.4 第 3 条）。
6. **测试语料**：`test/borrowck/pass/` 与 `test/borrowck/fail/`，逐条覆盖 §7.1.1、§7.8、§7.9、§7.10 的正反例。

#### 12.4.2 Rust 参考实现暂缓

**Rust 参考实现暂缓。** `src/semantic/{checker,ownership,resolver}.rs` 目前是未被 `compile()` 调用的死代码；本版本**不要求**将其唤醒，也**不要求**它跟进 D-1 .. D-7。

后果：

- `src/` 与本规范的偏离是**已知且被接受**的，不应记为缺陷。
- 若将来恢复 Rust 参考实现，须以本规范 0.2.0（或更新版本）为准重新对齐，而不是以 0.1.0 为准。
- 本规范的所有一致性声明（conformance claims）仅针对 `src-c/`。

#### 12.4.3 未落入本版本的事项

| 事项 | 状态 | 去向 |
|------|------|------|
| `MissingFree` / `DoubleFree` | 不实现 | §11.5.1，候选可选诊断 |
| `Owned<T>` / `Ref<T>` / `RefMut<T>` 内部类型表示 | 不进入用户语言 | devhandbook §4.1，属实现细节 |
| `null` 的多态类型 | 未定义 | 遗留自 0.1.0，待后续版本 |
| 非词法生命周期的精确定义 | 仅在 §7.8.2 描述为「到最后一次使用」 | 待 §12.4 算法实现后回填精确定义 |

---

## 文档历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 0.1 | 2026-04-13 | 初始规范 |
| 0.1 | 2026-04-17 | 更新所有权默认语义（所有指针不可复制）、添加 clone()、更新内联汇编语法 |
| 0.1.0 | 2026-04-18 | 0.1.0 定版 |
| 0.2.0 | 2026-08-07 | 落实 8 条设计决策 D-1 .. D-8：`unique` 泛型化、`&` 确定为不可变借用、`&mut` 进入语言、`move` 提升为内建 primitive、显式化赋值 move 规则、`alloc`/`free` 不强制配对、定义 `DanglingReference` 触发条件、记录 C 主机优先决策。状态：草稿。 |

---

*UltraCPP 0.2.0 语言规范（草稿）*
