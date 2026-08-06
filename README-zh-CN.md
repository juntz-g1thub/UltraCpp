# UltraCPP

**融合 C++ 语法与 Rust 内存安全特性的编程语言**

[![版本](https://img.shields.io/badge/version-0.1.0--alpha-orange.svg)](Cargo.toml)
|[[许可证](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

> **⚠️ 项目状态（2026-08-06）**
>
> 编译器正处于多阶段重写过程中。C 端口（`src-c/`）是当前主线实现；
> Rust 端口（`src/`）作为参考保留。asm 端口已删除；自举是下一方向。
>
> **先看这些**：
> - [`HANDOFF.md`](HANDOFF.md) — 30 秒恢复 + 完整项目状态
> - [`bootstrap/PLAN.md`](bootstrap/PLAN.md) — 自举路线图
> - [`src-c/README.md`](src-c/README.md) — 当前实现
> - [`AGENTS.md`](AGENTS.md) — Agent 工作约定
> - [`.dev/README.md`](.dev/README.md) — 开发过程文档（plans, drafts）
>
> 下文 "当前状态 (v0.1.0)" / "下一步" 是 v0.1.0 原版状态，已**不**是当前路线图。请看上面链接。

## 项目目标

UltraCPP 是一个研究项目，探索如何让 C++ 开发者使用具有以下特性的语言：

- **熟悉的 C++ 语法** - 学习曲线低
- **编译时内存安全** - 无需垃圾回收
- **默认移动语义** - 无需 `std::move` 即可转移所有权
- **现代模块系统** - 支持 `#import` 和 `#include` 指令

**核心问题**：我们能否构建一种让 C++ 开发者自然接受的语言，同时提供 Rust 般的安全保障？

## 愿景

```
C++ 语法 × Rust 安全 = UltraCPP
```

我们相信任何语言的成功取决于：
1. **语法熟悉度** - 开发者不应为尝试安全性而学习新语法
2. **工具链兼容性** - 可与现有调试器、链接器、构建系统配合
3. **渐进式采用** - 可以混合安全和不安全代码

## 当前状态 (v0.1.0)

编译器现已能够：

- ✅ 编译包含函数、变量、控制流的基本程序
- ✅ 生成 LLVM IR 并汇编为可执行文件
- ✅ 支持字符串字面量和全局常量
- ✅ 提供 I/O 库（`io.print_str()`, `io.getValue()`）
- ✅ 处理模块导入（`#import`）和包含（`#include`）
- ✅ 管理局部变量的加载/存储和类型转换

### 已实现功能

| 功能 | 状态 | 示例 |
|------|------|------|
| 基础类型 (int, char, float) | ✅ | `int x = 42;` |
| 函数 | ✅ | `int add(int a, int b) { return a + b; }` |
| 控制流 | ✅ | `if/else`, `while` |
| 字符串字面量 | ✅ | `"Hello, World!\n"` |
| 模块导入 | ✅ | `#import "lib/io"` |
| 模块包含 | ✅ | `#include "math.uc"` |
| sys$* 内置函数 | ✅ | `sys$strlen()`, `sys$write()` |

### 待实现功能

- [ ] 完整标准库实现
- [ ] 内存所有权和借用系统
- [ ] 错误处理（`Result<T, E>`）
- [ ] Cargo 风格的构建工具集成

## 快速开始

### 环境要求

- Rust（最新稳定版）
- LLVM（通过 `llvm-tools` 或系统 LLVM）
- GCC 或 Clang（用于链接）

### 构建

```bash
git clone https://github.com/your_username/ultracpp.git
cd ultracpp
cargo build --release
./target/release/ultracpp --help
```

### 第一个程序

创建 `hello.upp`：

```c
#import "lib/io"

int main() {
    io.print_str("Hello, World!\n");
    return 0;
}
```

### 编译运行

```bash
# 先构建 I/O 库
./scripts/uc-build lib lib/io.uc

# 编译程序
./scripts/uc-build compile test/test_hello_world/main.upp

# 手动链接运行
llc build/uc/*/test_*.ll -o /tmp/hello.s
gcc -c /tmp/hello.s -o /tmp/hello.o
gcc build/uc/lib_io/lib_io.o /tmp/hello.o -no-pie -o /tmp/hello
/tmp/hello
# 输出: Hello, World!
```

## 架构

```
源代码 (.uc/.upp)
       │
       ▼
┌──────────────────┐
│     预处理器      │  处理 #import, #include, export
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│      词法分析器    │  将源代码分词为 token
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│      语法分析器    │  从 token 构建 AST
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│     代码生成器     │  发出 LLVM IR
└────────┬─────────┘
         │
         ▼
      LLVM IR (.ll)
         │
         ▼
       LLC  （汇编为 .s）
         │
         ▼
       GCC  （链接为可执行文件）
```

## 项目结构

```
ultracpp/
├── src/                    # 编译器源码 (Rust)
│   ├── main.rs            # 入口点
│   ├── frontend/          # 词法分析、语法分析、AST
│   ├── codegen/           # LLVM IR 生成
│   ├── semantic/           # 类型检查、解析
│   └── preprocessor.rs     # #import, #include 处理
├── lib/                    # 标准库（UltraCPP 源码）
│   ├── io.uc              # I/O 库
│   └── math.uc           # 数学库
├── test/                   # 测试套件
│   ├── test_t1/           # 基础测试
│   ├── test_t2/           # 变量测试
│   ├── test_hello_world/  # Hello World 测试
│   └── README.md          # 测试文档
├── scripts/               # 构建工具
│   └── uc-build           # 构建脚本
├── docs/                  # 项目文档
├── .sisyphus/             # Agent 工作文件
│   ├── plans/             # 设计规范
│   └── drafts/            # 研究笔记
└── Cargo.toml            # Rust 项目配置
```

## 语言语法

### Hello World

```c
#import "lib/io"

int main() {
    io.print_str("Hello, World!\n");
    return 0;
}
```

### 变量和函数

```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 10;
    int y = add(x, 5);
    return y;
}
```

### 模块导入

```c
#import "lib/io"

int main() {
    int val = io.getValue();
    return val;
}
```

### 模块包含

```c
#include "lib/math.uc"

int main() {
    int result = add(5, 3);  // add 来自 math.uc
    return result;
}
```

## 符号命名规则

UltraCPP 使用 `$` 作为模块-函数分隔符，避免与 C 命名约定冲突：

| 表达式 | LLVM 符号 |
|--------|-----------|
| `io.getValue()` | `@io$getValue` |
| `sys$strlen(s)` | `@strlen` (内置) |
| `main` | `@main` |

## 测试

### 测试分类

| 目录 | 用途 |
|------|------|
| `test/test_t1/` | 基本空程序 |
| `test/test_t2/` | 变量声明 |
| `test/test_io/` | 通过 `#import` 使用 I/O 库 |
| `test/test_hello_world/` | 字符串字面量 + 输出 |
| `test/test_import/` | `#include` 指令 |

### 运行测试

```bash
# 先构建所有库
./scripts/uc-build lib lib/io.uc

# 编译测试
./scripts/uc-build compile test/test_hello_world/main.upp

# 手动运行
llc build/uc/*/test_*.ll -o /tmp/test.s
gcc build/uc/lib_io/lib_io.o /tmp/test.o -no-pie -o /tmp/test
/tmp/test
```

## 文档

### 项目文档

- [docs/UltraCPP-v0.1.0-spec-en.md](docs/UltraCPP-v0.1.0-spec-en.md) - Language Specification (English)
- [docs/UltraCPP-v0.1.0-spec-zh-CN.md](docs/UltraCPP-v0.1.0-spec-zh-CN.md) - 语言规范（中文）

### Agent 文档（内部）

- `.sisyphus/plans/` - 设计规范和 RFC
- `.sisyphus/drafts/` - 研究笔记和探索

### 关键文档

| 文档 | 说明 |
|------|------|
| `AGENTS.md` | 项目 Agent 系统提示 |
| `CONTRIBUTING.md` | 贡献指南 |
| `Cargo.toml` | Rust 依赖和项目配置 |

## 开发语言

**主要语言**：Rust

编译器使用 Rust 实现，原因：
- 编译器开发过程中的内存安全
- 强大的类型系统用于 AST 操作
- 通过 `inkwell` 或 `llvm-sys` 优秀的 LLVM 绑定

**目标语言**：LLVM 支持的任何架构

## 贡献

欢迎贡献！请参阅 [CONTRIBUTING.md](CONTRIBUTING.md) 获取指南。

## 版本历史

### v0.1.0（当前）
- 工作的编译器前端（词法分析、语法分析）
- LLVM IR 代码生成
- 基础 I/O 库
- 模块导入/导出系统
- 字符串字面量支持

## 许可证

Apache License 2.0 - 参见 [LICENSE](LICENSE)

---

*"C++ 语法 × Rust 安全 = UltraCPP"*
