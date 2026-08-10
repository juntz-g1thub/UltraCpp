# UltraCPP 0.1 快速指南

**UltraCPP** — 融合 Rust 内存安全的 C++ 语法语言

## 目录

1. [简介](#1-简介)
2. [快速开始](#2-快速开始)
3. [基础语法](#3-基础语法)
4. [指针和内存](#4-指针和内存)
5. [struct 和数据](#5-struct-和数据)
6. [模块系统](#6-模块系统)
7. [FFI](#7-ffi)
8. [示例项目](#8-示例项目)

---

## 1. 简介

### 什么是 UltraCPP

UltraCPP 是一个技术可行性研究项目，探索将 Rust 的内存安全特性与 C++ 语法相结合的可能性。

**核心问题**：C++ 开发者能否接受一门保留熟悉语法、同时提供编译时内存安全保证的语言？

### 核心特点

| 特点 | 描述 |
|------|------|
| **简洁** | 去除 C++ 中过于复杂的部分（模板、异常、运算符重载） |
| **安全** | 采用 Rust 的所有权和借用检查，在编译期防止内存错误 |
| **C++ 风格** | 语法风格与 C++ 高度一致，降低学习成本 |

### 与 C++ 的关系

UltraCPP 不是要替代 C++，而是在 C++ 语法基础上增加内存安全保证：

- **保留**：函数定义、struct、控制流、指针操作等熟悉的语法
- **新增**：所有权类型（`Owned<T>`）、借用规则（`&T`、`&mut T`）
- **移除**：异常（改用 `Result<T, E>`）、模板、多继承

---

## 2. 快速开始

### 安装编译工具

```bash
git clone https://github.com/YOUR_USERNAME/ultracpp.git
cd ultracpp
./build.sh   # 或 make
```

### 第一个程序

创建 `hello.uc`：

```cpp
int main() {
    print("Hello, UltraCPP!");
    return 0;
}
```

### 编译运行

```bash
ucac hello.uc -o hello    # 编译
./hello                     # 运行
```

输出：

```
Hello, UltraCPP!
```

---

## 3. 基础语法

### 变量声明

```cpp
int x = 42;                  // 变量
const int MAX = 100;          // 常量（const 修饰）
int y = x + MAX;              // 使用常量
```

### 基本类型

| 类型 | 描述 |
|------|------|
| `int`, `long` | 有符号整数 |
| `unsigned int`, `unsigned long` | 无符号整数 |
| `float`, `double` | 浮点数 |
| `bool` | 布尔值 (`true` / `false`) |
| `char` | 字符 |
| `string` | 字符串 |

### 算术运算

```cpp
int main() {
    int a = 10;
    int b = 3;
    
    int sum = a + b;         // 13
    int diff = a - b;        // 7
    int product = a * b;     // 30
    int quotient = a / b;    // 3
    int remainder = a % b;    // 1
    
    print_num(sum);
    return 0;
}
```

### 控制流

**if/else 表达式**：

```cpp
int main() {
    int x = 10;
    
    if (x > 0) {
        print("positive");
    } else if (x < 0) {
        print("negative");
    } else {
        print("zero");
    }
    
    return 0;
}
```

**while 循环**：

```cpp
int main() {
    int mut count = 5;
    
    while (count > 0) {
        print_num(count);
        count = count - 1;
    }
    
    return 0;
}
```

**for 循环**：

```cpp
int main() {
    int mut sum = 0;
    
    for (int i = 1; i < 5; i++) {
        sum = sum + i;
    }
    
    print_num(sum);  // 1 + 2 + 3 + 4 = 10
    return 0;
}
```

### 函数定义和调用

```cpp
int add(int a, int b) {
    return a + b;
}

bool is_even(int n) {
    return n % 2 == 0;
}

int main() {
    int result = add(10, 20);
    bool check = is_even(result);
    
    if (check) {
        print("result is even");
    }
    
    return 0;
}
```

---

## 4. 指针和内存

### 指针声明和使用

```cpp
int main() {
    int mut value = 42;
    int* ptr = &mut value;
    
    // 通过指针读取和写入
    int dereferenced = *ptr;
    *ptr = 100;
    
    print_num(dereferenced);  // 42
    print_num(value);         // 100
    
    return 0;
}
```

### malloc 和 free

手动内存管理（需谨慎使用）：

```cpp
extern "C" {
    void* malloc(size_t size);
    void free(void* ptr);
}

int main() {
    // 分配 128 字节
    void* ptr = malloc(128);
    
    if (ptr == nullptr) {
        return 1;  // 分配失败
    }
    
    // 使用内存...
    
    // 释放内存
    free(ptr);
    
    return 0;
}
```

### unique 指针语义

UltraCPP 使用 `Owned<T>` 表示独占所有权：

```cpp
int main() {
    // 创建拥有所有权的字符串
    Owned<String> s = Owned::new("hello");
    
    // s 拥有字符串的所有权
    print(s);
    
    return 0;
}  // s 超出作用域，字符串自动释放
```

### move 操作

当赋值给另一个变量时，所有权会发生转移（move）：

```cpp
int main() {
    Owned<String> s1 = Owned::new("hello");
    Owned<String> s2 = s1;  // move: s1 的所有权转移到 s2
    
    // print(s1);  // 编译错误！s1 已经无效
    print(s2);  // OK: s2 拥有所有权
    
    return 0;
}
```

### null 检查

```cpp
int main() {
    extern "C" {
        void* malloc(size_t size);
    }
    
    void* ptr = malloc(100);
    
    if (ptr == nullptr) {
        print("allocation failed");
        return 1;
    }
    
    print("allocation successful");
    return 0;
}
```

### 借用规则

| 借用类型 | 规则 | 示例 |
|----------|------|------|
| `&T` | 不可变借用，可同时存在多个 | `int* r = &x;` |
| `&mut T` | 可变借用，同时只能存在一个 | `int* mut w = &mut x;` |
| 不能同时存在 `&T` 和 `&mut T` |

```cpp
int main() {
    Owned<Vec<int>> mut v = Owned::new(vec![1, 2, 3]);
    
    // 不可变借用 - 可以有多个
    Vec<int>* r1 = &v;
    Vec<int>* r2 = &v;
    print(*r1);  // OK: 读取
    
    // 可变借用 - 只能有一个
    Vec<int>* mut w = &mut v;
    w.push(4);  // OK: 独占访问
    
    return 0;
}
```

---

## 5. struct 和数据

### 定义 struct

```cpp
struct Point {
    double x;
    double y;
};

struct Person {
    string name;
    int age;
};
```

### 创建和使用 struct

```cpp
struct Point {
    double x;
    double y;
};

impl Point {
    Point new(double x, double y) {
        return Point { x, y };
    }
    
    double distance(Point self, Point other) {
        double dx = self.x - other.x;
        double dy = self.y - other.y;
        return sqrt(dx * dx + dy * dy);
    }
}

int main() {
    Point p1 = Point::new(0.0, 0.0);
    Point p2 = Point::new(3.0, 4.0);
    double d = p1.distance(p2);
    
    print_num(d);  // 5.0
    return 0;
}
```

### 函数指针字段

```cpp
struct Callback {
    int (*fn_ptr)(int);
};

int double(int x) {
    return x * 2;
}

int main() {
    Callback cb = Callback { fn_ptr: double };
    int result = cb.fn_ptr(21);
    
    print_num(result);  // 42
    return 0;
}
```

---

## 6. 模块系统

### #import 导入模块

`#import` 用于导入已编译的模块：

```cpp
// math.upp
export int add(int a, int b) {
    return a + b;
}

export int multiply(int a, int b) {
    return a * b;
}
```

### #include 包含代码

`#include` 用于嵌入其他文件的代码：

```cpp
// utils.upp
void print_hello() {
    print("Hello!");
}

void print_world() {
    print("World!");
}
```

### 模块调用语法

```cpp
// main.upp
#include "utils.upp"
#import "math"

int main() {
    utils.print_hello();
    utils.print_world();
    
    int sum = math.add(10, 20);
    int product = math.multiply(sum, 2);
    
    print_num(sum);     // 30
    print_num(product);  // 60
    
    return 0;
}
```

### 示例项目结构

```
my_project/
├── main.upp          # 主程序入口
├── math.upp          # 数学模块（可编译为 .umc）
├── utils.upp         # 工具代码（直接包含）
└── lib/
    ├── string.upp     # 字符串模块
    └── array.upp      # 数组模块
```

---

## 7. FFI

### 调用 C 函数

使用 `extern "C"` 块声明 C 函数：

```cpp
extern "C" {
    size_t strlen(const char* s);
    void* malloc(size_t size);
    void free(void* ptr);
}
```

### extern "C" 块

```cpp
extern "C" {
    int printf(const char* format, ...);
    int scanf(const char* format, ...);
}
```

### 常用 C 库调用示例

**strlen - 获取字符串长度**：

```cpp
extern "C" {
    size_t strlen(const char* s);
}

int main() {
    size_t len = strlen("Hello");
    print_num(len);  // 5
    return 0;
}
```

**malloc/free - 手动内存管理**：

```cpp
extern "C" {
    void* malloc(size_t size);
    void free(void* ptr);
}

int main() {
    void* ptr = malloc(256);
    
    if (ptr == nullptr) {
        return 1;
    }
    
    // 使用 ptr...
    
    free(ptr);
    return 0;
}
```

### unsafe 块

所有 FFI 调用必须包裹在 `unsafe { }` 块中：

```cpp
int main() {
    // 错误：必须使用 unsafe
    // size_t len = strlen("hello");
    
    // 正确
    size_t len = unsafe { strlen("hello") };
    print_num(len);
    
    return 0;
}
```

---

## 8. 示例项目

### 项目结构

```
calculator/
├── main.upp
├── operations.upp
└── README.md
```

### operations.upp

```cpp
export int add(int a, int b) {
    return a + b;
}

export int subtract(int a, int b) {
    return a - b;
}

export int multiply(int a, int b) {
    return a * b;
}

export int divide(int a, int b) {
    if (b == 0) {
        return 0;
    }
    return a / b;
}
```

### main.upp

```cpp
#include "operations.upp"

int main() {
    int x = operations.add(10, 20);
    int y = operations.subtract(100, 50);
    int z = operations.multiply(x, y);
    int w = operations.divide(z, 2);
    
    print_num(x);  // 30
    print_num(y);  // 50
    print_num(z);  // 1500
    print_num(w);  // 750
    
    return 0;
}
```

---

## 附录

### 快速参考

| 概念 | 语法 |
|------|------|
| 函数定义 | `ret_type name(params) { body }` |
| 变量声明 | `Type x = value;` / `Type mut x = value;` |
| 所有权类型 | `Owned<T>` |
| 不可变借用 | `&value` |
| 可变借用 | `&mut value` |
| 原始指针 | `T*` |
| FFI 声明 | `extern "C" { ret_type func(); }` |
| FFI 调用 | `unsafe { func(); }` |

### 参考链接

- [语言规范](./language-spec.md)
- [设计原则](./design-principles.md)
- [所有权规则](./ownership-rules.md)
- [FFI 设计](./ffi-design.md)
