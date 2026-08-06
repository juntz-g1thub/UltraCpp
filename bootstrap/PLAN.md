# Bootstrap Plan — 用 UltraCPP 写 UltraCPP 编译器

> **目标**：在 `src-uc/` 用 UltraCPP 语言写一个 UltraCPP 编译器，最终能用 UltraCPP 自身编译。
>
> **当前状态（2026-08-06）**：
> - `src-c/` C 端口完整（Phase 1+1.1+2+3+4，505 单元测试，5/5 字节级 E2E）
> - `src-asm/` 已删除（asm 路径放弃，见 `HANDOFF.md` §六 推荐路径 B）
> - `src-uc/` 已创建但为空（待实际编写）
> - `bootstrap/baseline/` 存有 `test_t1` 的 C/Rust 字节级一致的基线 IR/asm

---

## 1. 目标与基线

**基线程序**：`test/test_t1/main.upp`（`int main() { return 0; }`）

**选定的目标编译器版本**：C 端口（`src-c/build/uc_lexer`）

**理由**：
- C 端口是 2026-08 之后的"主线"实现，Rust 端口作为参考逐步弃用
- C 端口与 Rust 端口的输出字节级一致（IR 完全相同，asm 仅 `.file` 指令差 1 字节）
- C 端口结构更清晰（无 Cargo 依赖，单二进制），适合作为长期基线

**基线文件**（`bootstrap/baseline/`）：
- `test_t1.c.ll` — 208 字节 — C 端口产出的 LLVM IR（**这是新编译器必须 byte-exact 产出的目标**）
- `test_t1.c.s` — 426 字节 — C 端口经 llc 产出的 x86-64 汇编
- `test_t1.rust.ll` — 与 `test_t1.c.ll` 字节级相同（参考）
- `test_t1.rust.s` — Rust 端口经 llc 产出的汇编

**验证**：`./bootstrap/baseline/regenerate.sh` 重新生成所有基线文件。

---

## 2. 自举的定义

> 严格自举（full bootstrap）：用 UltraCPP 编写的 UltraCPP 编译器 → 编译自己 → 产出二进制。
> 这里的"编译"=从 UltraCPP 源码到可执行文件的全过程（lex → parse → AST → IR → llc → gcc）。

**自举的层次**（由浅入深）：

| 阶段 | 描述 | 状态 |
|---|---|---|
| **Level 0** | 主机 = C 端口。子编译器 = UltraCPP 写的最小词法器（仅能 lex 一个测试程序）。用 C 端口编译子编译器。 | 未开始 |
| **Level 1** | 子编译器支持 lex + parse + 简单 codegen（足够端到端产出 `test_t1.exe`）。字节级匹配 C 端口输出。 | 未开始 |
| **Level 2** | 子编译器支持完整语言特性。 | 未开始 |
| **Level 3** | 子编译器能编译自己（**严格自举**）。 | 未开始 |

Level 3 是最终目标。Level 1 是工程上最大的里程碑（证明"UltraCPP 写 UltraCPP 编译器"可行）。

---

## 3. 实施路线

### 3.1 Level 0 — 最小词法器（**本周可达成**）

**目标**：用 UltraCPP 写一个仅能 lex `int main() { return 0; }` 的小词法器，编译并验证输出与 C 端口字节级一致。

```
src-uc/
├── README.md                  # src-uc 自述
├── Makefile                   # 调用 src-c/build/uc_lexer 编译
├── test_lexer.uc              # UltraCPP 源：最小词法器
├── bootstrap/                 # Level 0 子目标
│   ├── compare.sh             # 对比 src-uc/test_lexer 与 src-c 的输出
│   └── expected/              # Level 0 期望输出（从 src-c 拷贝）
│       └── test_t1.tokens
```

**具体任务**：
1. 在 `src-uc/test_lexer.uc` 写一个约 200-500 行的 UltraCPP 词法器
2. 处理 `int`/`main` 等 identifier、`(`/`)`/`{`/`}` 等 punct、空格
3. 输出与 `src-c/build/uc_lexer test/test_t1/main.upp --tokens` 字节级一致
4. 写 `bootstrap/compare.sh` 自动化对比

**风险**：UltraCPP 自身的 stdlib 还不全，可能需要先在 `src-uc/` 写一些基础类型（字符串、向量等）。如果太重，先在 `src-c/` 加新特性以支持 bootstrap。

### 3.2 Level 1 — 完整最小编译器（多周）

**目标**：`src-uc/` 包含完整的 lex + parse + codegen + CLI，能编译 `test_t1/main.upp` 并产出与 C 端口字节级一致的 `.ll`。

**结构**：
```
src-uc/
├── src/
│   ├── token.uc          # token 类型 + keyword 表
│   ├── lexer.uc          # 词法器
│   ├── ast.uc            # AST 类型
│   ├── parser.uc         # 解析器
│   ├── codegen.uc        # LLVM IR 生成
│   └── main.uc           # CLI
├── tests/                # 单元测试
└── Makefile
```

**验证**：`tools/codegen_test.sh` 字节级对比 C 端口与 UltraCPP 编译器的输出。

### 3.3 Level 2 — 完整语言支持

支持所有 5 个测试程序（test_t1, test_t2, test_t3, test_hello_world, test_io）。功能上等价于当前的 C 端口。

### 3.4 Level 3 — 严格自举

```
uc1 = C 端口
uc2 = uc1 编译 src-uc 产生的 UltraCPP 编译器
uc3 = uc2 编译 src-uc 产生的 UltraCPP 编译器
验证 uc2 == uc3（byte-exact）
```

---

## 4. 文件结构总览

```
.worktrees/borrow-check-verification/
├── src/                    原 Rust 实现（参考，逐步弃用）
├── src-c/                  C 端口（主机编译器，完整）
├── src-uc/                 **新** UltraCPP 编译器（用 UltraCPP 写，bootstrap 目标）
├── test/                   测试程序（不变）
├── docs/                   用户面向文档（语言规范）
├── .dev/                   开发过程文档
│   ├── plans/
│   └── drafts/
├── bootstrap/             **新** 自举基础设施
│   ├── PLAN.md             本文件
│   └── baseline/
│       ├── regenerate.sh   重新生成基线
│       ├── test_t1.c.ll    C 端口 IR（目标）
│       ├── test_t1.c.s     C 端口 asm
│       ├── test_t1.rust.ll Rust 端口 IR（参考）
│       └── test_t1.rust.s  Rust 端口 asm
├── tools/                  验证脚本（4 个 E2E）
├── HANDOFF.md              项目状态
├── AGENTS.md               Agent 工作约定
├── CONTRIBUTING.md         贡献流程
└── README*.md              项目根 README
```

---

## 5. 当前问题与决策点

### 5.1 UltraCPP stdlib 不足

当前 C 端口（src-c/）只支持 `int main() { return 0; }` 这类最小子集。要在 `src-uc/` 写一个最小词法器，至少需要：
- 字符串操作（拷贝、比较、切片）
- 动态内存（alloc/free）
- 文件 I/O（读源文件、写 token 输出）
- 整数格式化（itoa 之类）

这些基础设施**在 C 端口中没有现成的**（因为 C 端口本身只产生 LLVM IR，不运行 UltraCPP 程序）。需要：
- 选项 A：在 `src-uc/` 用 UltraCPP 自身实现 stdlib（自举的核心问题之一）
- 选项 B：先在 C 端口添加必要的 stdlib 支持（让 `src-c/` 能编译稍微复杂的 UltraCPP 程序）
- 选项 C：用 C 写一个"垫片 stdlib"（非自举但能让 src-uc/ 启动）

**推荐 C**：从最小可行开始，先让 C 端口能编译 `src-uc/` 中的最小词法器；自举要求 stdlib 用 UltraCPP 写，可以分阶段。

### 5.2 字节级对比工具

Level 1+ 需要 `src-uc` 编译器的输出与 `bootstrap/baseline/test_t1.c.ll` 字节级一致。

**已有工具**：`tools/tokenize_test.sh`（token diff）、`tools/ast_test.sh`（AST diff）、`tools/codegen_test.sh`（IR diff）已能验证 C 端口。需要扩展这些工具以验证 `src-uc/` 输出与 C 端口一致。

### 5.3 暂不关心的

- 性能优化：bootstrap 阶段先求正确性，不求速度
- 错误信息友好度：Level 0 允许 panic-style 错误处理
- 多文件支持：Level 0+1 单文件足够

---

## 6. 与 `.dev/` 的关系

- 本文件（`bootstrap/PLAN.md`）是 `.dev/plans/0.2.0-c-asm-bootstrap.md` 的**后续**
- 原 0.2.0 计划已大幅完成（Phase 1-4），且 Phase 5（asm）放弃
- 本计划继承了"Path B：用 C 端做底层，跳过 asm"的决定
- 本文件应被考虑作为 0.3.0 计划的雏形（未来可重命名为 `.dev/plans/0.3.0-bootstrap.md`）

---

## 7. 决策记录

- **2026-08-06**：确定 bootstrap 目标 = C 端口输出的 `test_t1.c.ll`（与 Rust 端口字节级一致）。选择 C 端口作为未来主线的"自举主机"而非 Rust 端口。
- **2026-08-06**：删除 `src-asm/`（参见 `HANDOFF.md` §六 推荐路径 B）。asm 路径已放弃。
- **2026-08-06**：建立 `bootstrap/baseline/` 作为新编译器的字节级目标。
- **2026-08-06**：建立 `src-uc/` 作为新编译器源码目录（待编写）。
