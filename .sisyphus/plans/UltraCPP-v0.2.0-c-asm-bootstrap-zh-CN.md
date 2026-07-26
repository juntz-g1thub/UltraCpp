# UltraCPP 编译器迁移与自举规划

> **版本**: v0.2.0（自 v0.1.0 alpha 起，迁移目标）
> **日期**: 2026-04-18
> **作者**: UltraCPP Team
> **状态**: 起草（实施中）

---

## 1. 目标

将 UltraCPP 编译器从 **Rust 实现**逐步迁移到 **C 实现**，再迁移到 **汇编实现**，最终通过 **UltraCPP 自身**实现除最核心语法特性之外的所有扩展功能，实现自举（self-hosting）。

**最终架构**：
```
┌─────────────────────────────────────┐
│  src/         — Rust 版（参考实现，逐步弃用）  │
│  src-c/       — C 版（生产实现，先行版本）     │
│  src-asm/     — asm 版（仅核心 lexer）        │
│  src-uc/      — UltraCPP 版（自举后的扩展）   │
└─────────────────────────────────────┘
```

---

## 2. 阶段划分

| 阶段 | 名称 | 目标 | 入口语言 |
|------|------|------|----------|
| **Phase 0** | 规划 | 完成本计划 + 建立目录结构 | — |
| **Phase 1** | C-Lexer | 把 token + lexer 改写到 C | C99 |
| **Phase 2** | C-Frontend | 补完 AST + parser | C99 |
| **Phase 3** | C-Codegen | 完成 codegen，能生成 5 个测试程序的 IR | C99 |
| **Phase 4** | C-CLI | main + Makefile + 端到端验证 | C99 |
| **Phase 5** | asm-Lexer | 把 lexer 改写到 x86-64 汇编 | AT&T asm |
| **Phase 6** | UC-Frontend | 把 parser + codegen 改写到 UltraCPP | UltraCPP |
| **Phase 7** | Bootstrap | UltraCPP 编译器编译自己 | UltraCPP |

---

## 3. 目录结构

```
.worktrees/borrow-check-verification/
├── src/                    # 原 Rust 实现（参考，逐步弃用）
├── src-c/                  # NEW: C 实现
│   ├── include/            # 公共头文件
│   │   ├── uc_token.h
│   │   ├── uc_lexer.h
│   │   ├── uc_ast.h
│   │   ├── uc_parser.h
│   │   ├── uc_codegen.h
│   │   ├── uc_error.h
│   │   └── uc_version.h
│   ├── src/                # 实现文件
│   │   ├── token.c
│   │   ├── lexer.c
│   │   ├── ast.c
│   │   ├── parser.c
│   │   ├── codegen.c
│   │   ├── error.c
│   │   └── main.c
│   ├── tests/              # C 版单元测试
│   │   ├── test_token.c
│   │   ├── test_lexer.c
│   │   └── ...
│   └── Makefile
├── src-asm/                # NEW: asm 实现（仅 lexer）
│   ├── lexer.s
│   └── README.md
├── src-uc/                 # NEW: UltraCPP 实现的扩展
│   ├── parser.uc           # 用 UltraCPP 重写 parser
│   ├── codegen.uc
│   ├── typechecker.uc
│   └── ...
├── bootstrap/              # 自举实验
│   ├── stage0/             # C 编译器
│   ├── stage1/             # C 编译器编译 src-uc/parser.uc
│   └── stage2/             # UltraCPP 编译器编译自己
├── tools/                  # 验证脚本
│   ├── tokenize_test.sh    # C lexer vs Rust lexer 输出对比
│   ├── ir_diff_test.sh     # C vs Rust 编译器生成的 IR 对比
│   └── parse_dump.sh       # AST dump 对比
├── .sisyphus/
│   ├── plans/              # 设计文档（本文件）
│   └── drafts/
└── test/                   # 现有测试程序保持不变
```

---

## 4. 核心 vs 非核心语法特性

### 4.1 核心（在 C/asm 中实现）

| 特性 | Phase | 备注 |
|------|-------|------|
| Token 类型 | P1 | 必需 |
| 词法分析 | P1, P5 | C 版在 P1，asm 版在 P5 |
| AST 节点（基本类型） | P2 | int/char/void/pointer/function |
| 解析器（基本结构） | P2 | 函数定义、变量声明、表达式 |
| 代码生成（基本 LLVM IR） | P3 | 必须支持 5 个现有测试程序 |

### 4.2 非核心（UltraCPP 自己实现）

| 特性 | Phase | 备注 |
|------|-------|------|
| struct / 类型系统 | P6+ | 需要完整 named type 支持 |
| 数组、字符串类型 | P6+ | 需要类型推导 |
| 借用检查器 | P6+ | 需要 OwnershipChecker |
| 错误类型系统 | P6+ | Result<T, E> |
| 预处理器 | P6+ | #include/#import 解析 |
| 模块系统 | P6+ | 单独编译 + 链接 |
| 类型检查器 | P6+ | Resolver + TypeChecker |
| 优化 pass | P6+ | 常量折叠、死代码消除 |
| 标准库 | P6+ | lib/io.uc, lib/math.uc 等 |

---

## 5. Phase 1 详细计划：C-Lexer（本会话执行）

### 5.1 任务清单

- [ ] **Task 1.1**：创建 `src-c/` 目录结构（已完成）
- [ ] **Task 1.2**：创建 `src-c/include/uc_version.h`（版本号）
- [ ] **Task 1.3**：创建 `src-c/include/uc_error.h` 和 `src-c/src/error.c`
- [ ] **Task 1.4**：创建 `src-c/include/uc_token.h` 和 `src-c/src/token.c`
- [ ] **Task 1.5**：创建 `src-c/include/uc_lexer.h` 和 `src-c/src/lexer.c`
- [ ] **Task 1.6**：创建 `src-c/src/main.c`（CLI 入口，支持 `--tokens` 选项只打印 token）
- [ ] **Task 1.7**：创建 `src-c/Makefile`
- [ ] **Task 1.8**：创建 `tools/tokenize_test.sh`（C vs Rust tokenization 对比）
- [ ] **Task 1.9**：运行测试，验证 5 个测试程序的 token 流一致
- [ ] **Task 1.10**：提交 Phase 1

### 5.2 C 接口设计

```c
// uc_token.h - Token 类型
typedef enum {
    UC_TOK_INT, UC_TOK_FLOAT, UC_TOK_CHAR, UC_TOK_STRING, UC_TOK_IDENT,
    UC_TOK_KW_IF, UC_TOK_KW_ELSE, UC_TOK_KW_WHILE, UC_TOK_KW_FOR, UC_TOK_KW_RETURN,
    UC_TOK_KW_STRUCT, UC_TOK_KW_EXPORT, UC_TOK_KW_IMPORT, UC_TOK_KW_CONST,
    UC_TOK_KW_UNIQUE, UC_TOK_KW_MOVE, UC_TOK_KW_FREE, UC_TOK_KW_ALLOC,
    UC_TOK_KW_NULL, UC_TOK_KW_TRUE, UC_TOK_KW_FALSE, UC_TOK_KW_VOID,
    UC_TOK_KW_EXTERN, UC_TOK_KW_UNSAFE, UC_TOK_KW_AS, UC_TOK_KW_STATIC,
    UC_TOK_KW_CLONE, UC_TOK_KW_ASM, UC_TOK_KW_BREAK, UC_TOK_KW_CONTINUE,
    // ... 运算符、分隔符
    UC_TOK_EOF, UC_TOK_ERROR
} UCTokenKind;

typedef struct {
    UCTokenKind kind;
    char* lexeme;      // 堆分配，需释放
    size_t lexeme_len;
    int line;
    int column;
    union {
        long long int_val;
        double float_val;
        char char_val;
        char* string_val;
    } as;
} UCToken;

void uc_token_init(UCToken* tok);
void uc_token_free(UCToken* tok);
const char* uc_token_kind_name(UCTokenKind kind);

// uc_lexer.h - 词法分析器
typedef struct UCLexer UCLexer;  // 不透明类型

UCLexer* uc_lexer_new(const char* source, size_t source_len, const char* filename);
void uc_lexer_free(UCLexer* l);
UCToken uc_lexer_next(UCLexer* l);  // 取下一个 token，需 uc_token_free

// uc_error.h - 错误处理
typedef enum {
    UC_ERR_NONE, UC_ERR_LEXER, UC_ERR_PARSER, UC_ERR_SEMANTIC, UC_ERR_CODEGEN
} UCErrorKind;

typedef struct {
    UCErrorKind kind;
    char message[512];
    int line;
    int column;
} UCError;

void uc_error_report(UCError* err);
```

### 5.3 验证策略

**端到端 tokenize 对比**：

```bash
# 生成 Rust 版的 token dump
target/debug/ultracpp test/test_t1/main.upp --dump-tokens > /tmp/rust_t1.tokens

# 生成 C 版的 token dump
src-c/uc_lexer test/test_t1/main.upp > /tmp/c_t1.tokens

# diff
diff /tmp/rust_t1.tokens /tmp/c_t1.tokens && echo "OK"
```

**注意**：当前 Rust 版没有 `--dump-tokens` 选项，需要在 Phase 1.8 之前给 Rust 版加一个简单的 token dump 模式（或 C 端输出规范化格式后独立验证）。

---

## 6. 风险与决策

### 6.1 决策记录

| 决策 | 选项 | 选择 | 理由 |
|------|------|------|------|
| C 标准 | C89/C99/C11/C17 | **C99** | 兼容性好，支持 `//` 注释、`long long`、`<stdint.h>` |
| 汇编方言 | AT&T / Intel / NASM | **AT&T**（Phase 5） | gcc 内联 asm 兼容 |
| 构建系统 | Make / CMake / autotools | **GNU Make** | 简单，零外部依赖 |
| 字符串内存 | strdup / 自管 | **自管 + 长度** | 减少 malloc 调用，便于调试 |
| 错误传播 | errno / 返回码 / setjmp | **返回码 + out 参数** | 最清晰，C 标准做法 |
| 命名风格 | snake_case / camelCase | **snake_case + `uc_` 前缀** | 避免与 libc 冲突 |
| 头文件位置 | 单 include/ 与代码同目录 | **统一 `include/`** | 公共/私有分离 |

### 6.2 已知风险

| 风险 | 缓解 |
|------|------|
| 自举循环（需要 C 编译 UltraCPP） | Phase 6+ 之前不要求完整 UltraCPP；用最小子集 |
| x86-64 asm 跨平台 | Phase 5 只在 Linux x86-64 上做 |
| C 与 Rust 行为差异（如字符串字面量所有权） | 单元测试覆盖边界情况 |
| 工作量超出单会话 | 分阶段，每阶段独立可用 |

---

## 7. 完成标准

**Phase 1 完成**：✅
- `src-c/` 可独立 `make` 编译
- `uc_lexer` 二进制能 tokenize 5 个测试程序
- token 流与 Rust 版字面等价（规范化后）
- 所有代码提交到 `feature/borrow-check-verification` 分支

**Phase 7 完成**（终极目标）：
- `src-c/uc` 可以编译 `src-uc/parser.uc`
- `src-uc/parser.uc` 编译后得到的二进制可以编译 `src/lexer.rs` 等价的 UltraCPP 源文件
- 生成的可执行文件与原 Rust 版生成的 LLVM IR 等价

---

## 8. 当前会话执行范围

本会话内执行：
- ✅ Task 1.1：目录结构
- ⏳ Task 1.2-1.7：核心 C 代码（token/lexer/main/Makefile）
- ⏳ Task 1.8-1.9：验证脚本 + 端到端测试
- ⏳ Task 1.10：提交

后续会话执行：
- Phase 2-7

---

*Last updated: 2026-04-18 — Phase 1 起草*