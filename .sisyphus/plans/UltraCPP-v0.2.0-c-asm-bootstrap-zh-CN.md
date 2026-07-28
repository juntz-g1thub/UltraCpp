# UltraCPP 编译器迁移与自举规划

> **版本**: v0.2.0（自 v0.1.0 alpha 起，迁移目标）
> **日期**: 2026-04-18（创建）/ 持续更新
> **作者**: UltraCPP Team
> **状态**: 实施中 — Phase 1 ✅ 完成，Phase 1.1 ✅ 验证完成，Phase 2 进行中

---

## 0. 当前状态（每次回到这里必读）

| 阶段 | 状态 | 提交 | 备注 |
|------|------|------|------|
| Phase 0 规划 | ✅ | — | `.sisyphus/plans/UltraCPP-v0.2.0-c-asm-bootstrap-zh-CN.md` |
| Phase 1 C-Lexer | ✅ | `cefc1c5` | `src-c/` 完整 C99 词法分析器，69/69 单元测试 |
| Phase 1.1 字节级验证 | ✅ | `fbcb260` | Rust 加 `--dump-tokens`；7/7 测试程序 tokenize 等价 |
| **Phase 2.1 C-AST** | ✅ | **`6ece689`** | `src-c/include/uc_ast.h` + `src-c/src/ast.c` + `tests/test_ast.c`；73/73 单元测试，ASAN+UBSan 净 |
| Phase 2.2 parser skeleton + top-level | 🔄 待启动 | — | 见 §7 |
| Phase 2.3 parser statements | ⏳ | — | 见 §7 |
| Phase 2.4 parser expressions binary | ⏳ | — | 见 §7 |
| Phase 2.5 parser expressions unary/postfix/literals | ⏳ | — | 见 §7 |
| Phase 2.6 parser `--dump-ast` 字节级对齐 | ⏳ | — | 见 §7 |
| Phase 3 C-Codegen | ⏳ | — | 未开始 |
| Phase 4 C-CLI | ⏳ | — | 未开始 |
| Phase 5 asm-Lexer | ⏳ | — | 未开始 |
| Phase 6 UC-Frontend | ⏳ | — | 未开始 |
| Phase 7 Bootstrap | ⏳ | — | 未开始 |

**当前分支**：`feature/borrow-check-verification`
**最后提交**：`6ece689`（Phase 2.1）
**工作树位置**：`/home/zjtti/Coding/UltraCpp/.worktrees/borrow-check-verification`

**回到这里的快速命令**：
```bash
cd /home/zjtti/Coding/UltraCpp/.worktrees/borrow-check-verification
git log --oneline -5                    # 看看进度
make -C src-c test                       # 确认 Phase 1 + 2.1 通过（73 + 69）
bash tools/tokenize_test.sh              # 确认字节级验证通过
cat HANDOFF.md                           # 读快速恢复指南
```

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
├── HANDOFF.md                                  🆕 快速恢复指南（必读）
├── src/                    # 原 Rust 实现（参考，逐步弃用）
├── src-c/                  # 🆕 C 实现
│   ├── Makefile
│   ├── README.md
│   ├── include/
│   │   ├── uc_version.h
│   │   ├── uc_error.h
│   │   ├── uc_token.h
│   │   └── uc_lexer.h
│   ├── src/
│   │   ├── error.c
│   │   ├── token.c
│   │   ├── lexer.c
│   │   └── main.c          # --dump-tokens
│   └── tests/
│       └── test_lexer.c
├── src-asm/                # 📁 占位（Phase 5 用）
├── tools/
│   └── tokenize_test.sh    # 端到端 tokenize 字节级对比
├── .sisyphus/
│   ├── plans/              # 📋 本文件
│   └── drafts/
└── test/                   # 现有测试程序保持不变
```

---

## 4. 核心 vs 非核心语法特性

### 4.1 核心（在 C/asm 中实现）

| 特性 | Phase | 备注 |
|------|-------|------|
| Token 类型 | P1 ✅ | 必需 |
| 词法分析 | P1 ✅, P5 | C 版在 P1，asm 版在 P5 |
| AST 节点（基本类型） | P2 🔄 | int/char/void/pointer/function |
| 解析器（基本结构） | P2 🔄 | 函数定义、变量声明、表达式 |
| 代码生成（基本 LLVM IR） | P3 | 必须支持 5 个现有测试程序 |

### 4.2 非核心（UltraCPP 自己实现）

| 特性 | Phase | 备注 |
|------|-------|------|
| struct / 类型系统 | P6+ | 需要完整 named type 支持 |
| 借用检查器 | P6+ | 需要 OwnershipChecker |
| 预处理器 | P6+ | #include/#import 解析 |
| 模块系统 | P6+ | 单独编译 + 链接 |
| 类型检查器 | P6+ | Resolver + TypeChecker |
| 标准库 | P6+ | lib/io.uc, lib/math.uc 等 |

---

## 5. 决策记录（确立的约定）

| 决策 | 选择 | 理由 |
|------|------|------|
| C 标准 | **C99** | 兼容性好、`long long`、`<stdint.h>` |
| 汇编方言 | **AT&T**（Phase 5） | gcc 内联 asm 兼容 |
| 构建系统 | **GNU Make** | 简单，零外部依赖 |
| 命名风格 | **snake_case + `uc_` 前缀** | 避免与 libc 冲突 |
| 头文件位置 | **统一 `src-c/include/`** | 公共/私有分离 |
| 字符串内存 | **自管 + 长度字段** | 减少 malloc |
| 错误传播 | **`UCError*` out 参数** | C 标准做法 |
| 验证策略 | **字节级 diff（列数归一化）** | 见 §6 |
| Sub-agent 策略 | **任务 ≤ 2 文件 ≤ 200 行** | 大任务会超时（见 §8） |

---

## 6. 验证策略（关键工具）

### 6.1 字节级 tokenize 验证（✅ 已建立）

**工具**：`tools/tokenize_test.sh`

**流程**：
```
Rust 编译器 --dump-tokens → 字符串格式 A
C 词法器     --dump-tokens → 字符串格式 B
                    ↓
         normalize（去除列号）→ A' / B'
                    ↓
                diff A' B'（必须字节相等）
```

**归一化的原因**：Rust lexer 的列号在多字符 token 后不更新（已知 bug），无法字节级对齐。Token kind / line / lexeme / 字面值**字节级比较**。

**当前结果**：7/7 测试程序通过。

### 6.2 字节级 parse 验证（待 Phase 2 建立）

**拟用方案**：
```
Rust 编译器 --dump-ast → 字符串格式 A
C 解析器    --dump-ast → 字符串格式 B
                    ↓
                 diff A B（必须字节相等）
```

实施细节见 §7。

### 6.3 字节级 codegen 验证（待 Phase 3 建立）

**拟用方案**：
```
Rust 编译器 main.upp -o main.ll → main_rust.ll
C 编译器   main.upp -o main.ll → main_c.ll
                    ↓
                diff main_rust.ll main_c.ll
```

---

## 7. 下一阶段具体任务（Phase 2 启动包）

### 7.1 本会话决定的方案

**方案 A**（最小可行）：
1. **Phase 2.1**：只做 AST 类型定义（`uc_ast.h` + `uc_ast.c`），约 200 行 C
   - 验证：写最小 parser stub 验证 AST 结构能被正确构造和释放
2. **Phase 2.2**：parser 拆分模块化（top-level / statements / expressions 分别实现）
3. **Phase 2.3**：end-to-end parser 验证（与 Rust `--dump-ast` 对比）

### 7.2 Phase 2.1 具体清单

**需创建的文件**：
```
src-c/include/uc_ast.h       # AST 类型定义
src-c/src/ast.c              # AST 分配/释放/深拷贝
```

**Rust 参考**（必读）：
- `src/frontend/ast.rs`（~90 行）：所有节点定义
- 重点：Type 枚举的对应、所有 Box<T> → 显式指针、所有 Vec<T> → 动态数组

**C 风格映射**：

| Rust | C |
|------|---|
| `enum Foo { A, B, C }` | `typedef enum { UC_FOO_A, UC_FOO_B, UC_FOO_C } UCFoo;` |
| `struct Foo { ... }` | `typedef struct UCFoo { ... } UCFoo;` |
| `Box<T>` | `struct T*`（手工管理） |
| `Vec<T>` | `struct { T* data; size_t len; size_t cap; }` 或单独 helper |
| `Option<T>` | `bool has_value; T value;`（或单独 type tag） |
| `Result<T, E>` | `UCError*`（仅记录错误，不返回值） |
| `String` | `char*` + `size_t len` |

**所有权约定**：
- 每个 AST 节点是 malloc'd 的，通过 `uc_*_new()` 创建
- `uc_*_free()` 深释放整棵树
- 父节点持有子节点的所有权
- 字符串字段（`Ident.name`, `Import.path`）是 malloc'd 副本

**验证清单**：
- [ ] `make -C src-c` 能编译
- [ ] 写 `tests/test_ast.c` 至少 5 个测试用例：
  - 创建+释放 `UCFuncDef`
  - 创建+释放 `UCIfStmt`
  - 创建+释放表达式树
  - 字符串字段的所有权正确性（不重复 free）
  - 深释放不会泄漏也不会 double-free
- [ ] 用 valgrind（或类似工具）跑测试，确认无内存错误

### 7.3 Phase 2.2 parser 拆分建议

**parser.rs 880 行的拆分点**（避免再次超时）：

| 拆分块 | Rust 行数 | 验证 |
|--------|----------|------|
| parser skeleton + top-level | ~150 | 能 parse 空 main.upp |
| declarations (func/var/struct/import) | ~200 | 能 parse 多个声明 |
| statements (if/while/for/return/block/decl) | ~200 | 能 parse 嵌套语句 |
| expressions binary ops (按优先级分层) | ~200 | 能 parse `1 + 2 * 3` |
| expressions unary/postfix/literals | ~100 | 能 parse `arr[i].field` 等 |
| types (parse_type) | ~50 | （types 已在 ast 中） |

每个块单独 sub-agent + 单独验证。

---

## 8. 经验教训（避免重蹈覆辙）

### 8.1 Sub-agent 任务规模上限

**问题**：一次 dispatch 让 sub-agent 完成 parser.rs（880 行 → ~1500 行 C），sub-agent 在产出可编译代码前 OOM/超时（exit 137）。

**结论**：**单 sub-agent 任务 ≤ 2 文件 ≤ 200 行 C**。超出后拆。

### 8.2 Rust lexer 列号 bug 不修复

**问题**：Rust lexer 在多字符 token（identifier/number/string/char）后列号不更新。修复需要重写 column 跟踪逻辑。

**决策**：**当前不修复**，在 `tools/tokenize_test.sh` 中归一化列号。Phase 1.1 注释里记录。Phase 6+（UC 自举）时统一处理。

理由：
- 列号仅用于错误消息
- Phase 1 已经达到目标（kind / line / lexeme / value 字节级匹配）
- 修复是 Rust lexer 的独立改进，不属于迁移任务
- C 版本列号正确，asm 版可以继承

### 8.3 Rust scan_string 的 lexeme 格式

**问题**：Rust lexer 把**解码后**的字符串值（如 `\n` → 实际换行）存入 lexeme，而 C 版本用**原始源文本**（保留 `\n` 字面）。

**决策**：**修复 Rust**（已提交于 `fbcb260`），用 `self.source[start..self.pos]` 作为 lexeme。

理由：
- lexeme 应该是原始源文本（语义正确）
- 与 C 版本字节级匹配
- 修复是 Rust 的独立改进，不影响行为（错误消息同样能用）

---

## 9. 已知的错误和待处理项

### 9.1 已知 Rust bug（迁移无关，独立修复）

- [ ] `scan_identifier/number/string/char` 列号不更新
- [ ] 复合赋值运算符 (`+=`, `-=`, 等) 在 lexer 中不识别（parser 也不消费）
- [ ] 编译 `test_hello_world/main.upp` 后 `io$print_str` 链接失败（声明无参数，调用有参数）

### 9.2 文档/代码不一致

- [ ] `README.md` 列出 `test/test_import/` 但目录不存在
- [ ] `test_t3` 与 `test_t1` 完全重复
- [ ] `Cargo.toml` 列出 `f64`，但 C 中已用 `double`
- [ ] AGENTS.md 提到 `typedef` 关键字，但 TokenKind 不存在

### 9.3 dev-dependency 缓存缺失

- [ ] `assert_cmd` / `predicates` / `tempfile` 不在 `~/.cargo/registry/cache/`，`cargo test --offline` 失败
- 解决路径：联网时 `cargo fetch` 补齐；或临时去掉 dev-deps

---

## 10. 完成标准

**Phase 2 完成**：✅
- AST 类型完整（mirror `ast.rs` 所有节点）
- parser 能 parse 5 个现有测试程序
- 与 Rust `--dump-ast` 字节级匹配

**终极目标（Phase 7）**：
- `src-c/uc` 可以编译 `src-uc/parser.uc`
- `src-uc/parser.uc` 编译后能编译等价的 UltraCPP 源文件
- 生成的可执行文件与原 Rust 版生成的 LLVM IR 等价

---

## 11. 回到此 worktree 时的 checklist

见 `HANDOFF.md`（工作树根目录）：
- 5 秒看完状态
- 复制粘贴可执行 build / test / verify 命令
- 找到下一步具体任务

---

*Last updated: Phase 1 + 1.1 + 2.1 已提交（`6ece689`），Phase 2.2 待启动*
