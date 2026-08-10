# Worktree Handoff — `feature/borrow-check-verification`

> **TL;DR**：把 UltraCPP 编译器从 Rust 改写到 C 的迁移已完成到 Phase 4；asm 路径已放弃；**bootstrap 启动**。
> - **Phase 1+1.1+2+3+4**：C 端口完整、可用、与 Rust 字节级对齐、端到端产出可执行。
> - **Phase 5（asm-Lexer）**：工具链 stub 阶段。**2026-08-06 决定删除整个 `src-asm/`**，走"用 C 端口做底层、用 UltraCPP 写自己"的 bootstrap 路径。
> - **Phase 6+（UC-Frontend / Bootstrap）**：**已启动**。见 `bootstrap/PLAN.md`。
>
> **接下来的工作**：见本文末「项目状态总结」与 `bootstrap/PLAN.md`。

---

## 一、30 秒内恢复工作状态

```bash
cd /home/zjtti/Coding/UltraCpp/.worktrees/borrow-check-verification

# 1. 确认在正确 worktree
pwd                                        # → .../borrow-check-verification
git log --oneline -10                       # 看最近 10 个 commit

# 2. 跑全部冒烟测试
make -C src-c test                          # 单元测试 4 个 binary，505/505
bash tools/tokenize_test.sh                 # 字节级 token diff: 7/7
bash tools/ast_test.sh                      # 字节级 AST  diff: 5/5
bash tools/codegen_test.sh                  # 字节级 IR   diff: 5/5
bash tools/build_test.sh                    # 端到端 build : 3/3
# 全部 4 个脚本的预期输出："N passed, 0 failed"

# 3. 端到端构建一个可执行文件
src-c/build/uc_lexer --build test/test_t1/main.upp -o /tmp/test_t1
/tmp/test_t1; echo "exit: $?"               # 应该是 0

# 4. 确认 asm 工具链 stub 还能跑
./src-asm/uc_lexer_asm                      # 输出 banner，退出 0
```

如果上面任何一步失败，先看本文「故障排查」节。

---

## 二、当前进度

| Phase | 内容 | 状态 | 提交 | 备注 |
|---|---|---|---|---|
| 1 | C-Lexer | ✅ | `cefc1c5` | `src-c/src/lexer.c`，69 单元测试 |
| 1.1 | 字节级 tokenize 对齐 | ✅ | `fbcb260` | `tools/tokenize_test.sh` 7/7（column normalize） |
| 2.1 | C-AST 类型定义 | ✅ | `6ece689` | `src-c/include/uc_ast.h` + `src-c/src/ast.c`，73 单元测试 |
| 2.2 | parser skeleton + top-level | ✅ | `55390ad` | 8 种 TopLevel + 类型 + 最小 stmt |
| 2.3 | parser statements | ✅ | `0c8143f` | if/while/for/return/break/continue/expr/decl/free/unsafe |
| 2.4 | parser 二元表达式 | ✅ | `83645f6` | 11 级 precedence climbing 递归下降 |
| 2.5 | parser 一元 + postfix | ✅ | `ce95497` | unary/postfix 11 + move/clone，341 单元测试 |
| 2.6 | 与 Rust `--dump-ast` 字节级对齐 | ✅ | `b3f3b63` | `tools/ast_test.sh` 5/5，**顺带修了 Rust `parse_comparison` bug**（`<=/>` 全部错为 `<`） |
| 3 | C-Codegen（LLVM IR） | ✅ | `cb8bfae` | `src-c/src/codegen.c` (~770 行)，`tools/codegen_test.sh` 5/5 |
| 4 | C-CLI 端到端 | ✅ | `b8204fc` | `--build` 模式；`tools/build_test.sh` 3/3（test_t1/t2/t3 退出码与 Rust 编译产物一致） |
| **5** | **asm-Lexer** | ❌ 删除 | — | `src-asm/` 已删除（2026-08-06 决策走 bootstrap 路径 C） |
| 6 | UC-Frontend（自举） | — | — | 未启动 |
| 7 | Bootstrap 验证 | — | — | 未启动 |

### 已完成的核心能力

```
.upp 源码
  │
  ├─→ Rust 编译器：lex → parse → codegen → .ll → llc → gcc → native exec
  │
  └─→ C 端口：    lex → parse → codegen → .ll → llc → gcc → native exec
                  两者输出字节级一致（除 libio 链接限制外）
```

**端到端验证（test_t1/t2/t3）**：

| 测试程序 | Rust 退出码 | C 退出码 | C 端 `.ll` byte-exact |
|---|---|---|---|
| test_t1 (`return 0`) | 0 | 0 | ✅ |
| test_t2 (`return x=5`) | 5 | 5 | ✅ |
| test_t3 (`return 0`) | 0 | 0 | ✅ |
| test_hello_world | (link fail) | (link fail) | ✅ IR 正确，链接需 libio |
| test_io | (link fail) | (link fail) | ✅ IR 正确，链接需 libio |

### 单元测试覆盖（505/505）

| Binary | Tests | 范围 |
|---|---|---|
| `test_ast` | 73/73 | AST 类型 + 构造 + 深释放 + dump |
| `test_lexer` | 69/69 | C 词法（关键字、字符串、注释、整数） |
| `test_parser` | 339/339 | C 解析（top-level + 8 种 stmt + 11 级 binary + 一元 + postfix） |
| `test_codegen` | 24/24 | C codegen（builtin decls、函数、表达式、字段调用 declare） |

全部 ASAN+UBSan 净，零警告（`-Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes`）。

---

## 三、目录结构（实际状态）

```
.worktrees/borrow-check-verification/
├── HANDOFF.md                              ← 你正在读
├── README.md / README-zh-CN.md             项目根 README
├── AGENTS.md                               Agent 工作约定
├── CONTRIBUTING.md                         贡献流程
├── src/                                    原 Rust 实现（只读，迁移期间修改有限）
│   ├── frontend/{ast,lexer,parser,token,dump_ast}.rs
│   ├── codegen/{generator,builtin}.rs
│   ├── main.rs
│   └── ...
├── src-c/                                  C 端口（Phase 1+1.1+2+3+4 完整）
│   ├── Makefile
│   ├── README.md
│   ├── include/uc_*.h                      公共头（8 个）
│   ├── src/{error,token,lexer,parser,ast,codegen,main}.c
│   ├── tests/test_*.c                      4 个测试 binary，505 单元测试
│   └── build/                              build artifacts（已 gitignore）
├── src-asm/                                asm 端口（Phase 5 工具链 stub）
│   ├── lexer.s                             ~30 行 stub：写 banner，exit 0
│   ├── README.md                           详细状态 + 失败分析
│   ├── .gitignore                          排除 build artifacts
│   ├── build/                              build artifacts（已 gitignore）
│   └── uc_lexer_asm                        已编译 stub（已 gitignore）
├── tools/                                  4 个端到端验证脚本
│   ├── tokenize_test.sh                    字节级 token diff
│   ├── ast_test.sh                         字节级 AST diff
│   ├── codegen_test.sh                     字节级 IR diff
│   └── build_test.sh                       端到端 native exec + 退出码对比
├── .dev/                                   开发过程专用文档（2026-08-06 重组）
│   ├── README.md                           索引
│   ├── plans/                              实施计划与设计文档
│   │   ├── 0.1.0-compiler-architecture.md
│   │   ├── 0.1.0-devhandbook.md
│   │   ├── 0.1.0-docs-update.md
│   │   ├── 0.1.0-guide.md
│   │   ├── 0.1.0-spec-snapshot.md
│   │   └── 0.2.0-c-asm-bootstrap.md         当前迁移规划
│   └── drafts/                             探索性草稿
│       ├── 0.1.0-module-system.md
│       ├── 0.1.0-pointer-design.md
│       └── 0.1.0-syntax-features.md
├── docs/                                   用户面向文档（**建议保留不动**）
│   ├── UltraCPP-v0.1.0-spec-en.md           0.1.0 语言规范（英文）
│   └── UltraCPP-v0.1.0-spec-zh-CN.md        0.1.0 语言规范（中文）
├── test/                                   现有测试程序（不要改）
│   ├── test_t1/main.upp                    `int main(){return 0;}`
│   ├── test_t2/main.upp                    `int x=5; return x;`
│   ├── test_t3/main.upp                    `int main(){return 0;}`
│   ├── test_hello_world/main.upp           `io.print_str("Hello...")`
│   ├── test_io/main.upp                     `io.getValue()`
│   ├── test_include/main.upp
│   └── test_simple.uc
└── (Cargo.lock / Cargo.toml / .gitignore / ...)
```

---

## 四、C 端口遵守的约定

| 项 | 约定 |
|---|---|
| C 标准 | C99（`cc -std=c99`） |
| 警告 | `-Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes`，零警告 |
| 命名 | `snake_case`，公开符号前缀 `uc_`，枚举值全大写 `UC_*` |
| 头文件 | 公共 API 放 `src-c/include/uc_*.h` |
| 内存 | 显式 `malloc`/`free`；所有权在头文件注释中声明 |
| 错误 | `UCError*` out 参数；不抛异常、不 `setjmp` |
| 字符串 | `char*` + `size_t len`，NUL-terminated |
| 编译选项 | `-O2 -g`（release 模式 `-O3 -DNDEBUG`） |

**Makefile 自动发现**：`src/*.c` 与 `tests/test_*.c` 无需改 Makefile。
`LIB_OBJECTS` 排除 `main.o`，新测试无需改 Makefile。

---

## 五、项目状态总结（截至本次 commit）

### 已达成

1. **C 端编译器完整可用**（Phase 1+2+3+4）
   - 505/505 单元测试，4 个端到端 E2E 脚本全绿
   - 5/5 现有测试程序 IR 输出与 Rust 字节级一致
   - 3 个测试程序可端到端产出与 Rust 编译结果**退出码一致**的可执行文件

2. **迁移期修复的 Rust bug**（Phase 2.6 顺手）
   - `src/frontend/parser.rs::parse_comparison` 把 `<=/>=` 全部误为 `<` 的 bug
   - `x = y` 在 C 端改用专门的 `UC_EXPR_ASSIGN` 节点，与 Rust `Expr::Assign` 对齐

3. **完整的端到端验证套件**
   - 4 个 shell 脚本（`tools/{tokenize,ast,codegen,build}_test.sh`）
   - 任何 commit 都能用这 4 个脚本验证字节级 / 行为级正确性

### 未达成

1. **Phase 5（asm-Lexer）只到工具链 stub**
   - 详见下文「Phase 5 详细状态」

2. **Phase 6/7（UC-Frontend / Bootstrap）未启动**
   - 取决于 Phase 5 是否完成，或走 UltraCPP 自举路径

### Phase 5 详细状态

**已完成**（`329cd4b`）：

* `src-asm/lexer.s`（~30 行）：写一行 banner 到 stderr，exit 0
* `src-asm/README.md`：详细记录了 ~1500 行全量实现的尝试过程、3 类典型 bug、4 步恢复计划
* `src-asm/.gitignore`：排除 `lexer.o`、`uc_lexer_asm`
* 工具链验证：`as --64 -o lexer.o lexer.s && ld -o uc_lexer_asm lexer.o` 工作

**未完成**：完整 lexer 的 ~1500 行实现

* 之前的 ~1500 行实现能 build 但 runtime segfault
* 调试 3+ 小时后识别出 3 类 bug（详见 `src-asm/README.md`）：
  1. `.ascii` 数据放 `.text` 被当作指令执行（→ SIGILL）
  2. 4 字节 `.skip` 字段用 `movq` 写入（→ 破坏相邻 BSS 变量）
  3. RIP-relative `leaq` 在 call boundary 行为可疑
* 额外发现：GNU as 对 `movq mem(%rip), mem(%rip)` 报 "operand size mismatch"，必须 `lea + mov`
* **2026-08-05 二次尝试**：在 stub 基础上重写了 ~600 行可读版本，能 build，能 emit 10 个 token 但格式与 C 输出不一致（line/col、Int 值、lexeme 都有 bug），文件读路径（argv[1]）下完全不输出 tokens
* **本次 session 决定**：放弃 Phase 5 完整实现。已 `git checkout` 回到 stub 状态。`src-asm/lexer.s.stub` 临时备份文件已删除。

**Phase 5 完整实现的工作量估计**：根据二次尝试，~600 行 C-equivalent asm + 大量调试，预计 3-5 个 commit。

---

## 六、Phase 5 之后怎么办

剩余工作（Phase 6+）按计划是「用 UltraCPP 自己重写 parser + codegen（自举）」。

**两条路径**：

### 路径 A：补完 Phase 5 asm-Lexer

* 起点：`src-asm/lexer.s` stub（30 行已 build）
* 工作量：~3-5 个 commit 写完整 lexer 并过 `tools/tokenize_test.sh`
* 优点：完整 C → asm 路径；Phase 7 bootstrap 可用 asm 版本做底层
* 缺点：调试成本高（已两次尝试都遇到深层 bug）

### 路径 B：跳过 asm-Lexer，直接进 Phase 6

* C 端口（Phase 1-4）已经够用
* Phase 6：用 UltraCPP 语言重写 parser + codegen（自举的真正意义所在）
* 优点：跳过 asm 的痛苦；自举路径更直接
* 缺点：src-asm/ 永远停在 stub；自举的"底层"还是 C 端

### 推荐

**路径 B**。理由：

1. asm-Lexer 的边际收益低（C 端 100% 覆盖）
2. 自举的核心价值在 parser + codegen，不在 lexer
3. 两次 Phase 5 尝试都暴露了 asm 调试的时间成本
4. 如果未来真要 asm 端，可基于 stub 重新做（README 里有 4 步计划）

---

## 七、提交历史（已 merged）

```
33cecaf docs: fix Phase 2.6 commit hash reference
0655b14 docs: update HANDOFF and migration plan after Phase 2.6
b3f3b63 Phase 2.6: byte-level AST alignment between Rust and C parsers
423c7c4 docs: update HANDOFF and migration plan after Phase 2.5
ce95497 Phase 2.5: C parser unary + postfix expressions
c521da0 docs: update HANDOFF and migration plan after Phase 2.4
83645f6 Phase 2.4: C parser binary expressions
31eac55 docs: update HANDOFF and migration plan after Phase 2.3
0c8143f Phase 2.3: C parser statements
... (Phase 1, 1.1, 2.1, 2.2 ...)
1608926 docs: update HANDOFF and migration plan after Phase 5 (partial)
329cd4b Phase 5: asm-Lexer stub (toolchain verified, full lexer deferred)
2d44c5b docs: update HANDOFF and migration plan after Phase 4
b8204fc Phase 4: end-to-end CLI build
c0b0652 docs: update HANDOFF and migration plan after Phase 3
cb8bfae Phase 3: C LLVM IR code generator + end-to-end verification
```

---

## 八、故障排查

| 症状 | 原因 | 解决 |
|---|---|---|
| `pwd` 不是 worktree | bash 每次 cwd 重置 | 命令前 `cd .../.worktrees/borrow-check-verification &&` |
| `make -C src-c` 报 `multiple definition of main` | 测试和 main 都链接了 | 不需要改，Makefile 已排除 `main.o` |
| `cargo test --offline` 报 dev-deps 下载失败 | dev-deps 不在本地缓存 | 联网时 `cargo fetch`；或临时删 `Cargo.toml` dev-deps |
| C 编译警告 | 代码有问题 | 零容忍，警告即 fix |
| `tools/ast_test.sh` 不通过 | Rust `--dump-ast` 或 C `--ast` 输出改了 | 重新跑两者，diff 看差异 |
| `tools/codegen_test.sh` 不通过 | 同上 | 同上 |
| `tools/build_test.sh` 中 hello_world/io 失败 | 已知：libio 缺失 | 接受；Phase 6+ 再处理 |
| `src-asm/uc_lexer_asm` 跑 segfault | 完整 lexer 实现未提交；当前是 stub | 应该只跑 banner 然后 exit 0 |

---

## 九、相关文档指针

> **2026-08-08 更新**：0.1.0 / 0.2.0 时代的设计文档已迁移到 `.dev/_archive/`（commit `9f5297b`），本节路径同步更新。**当前活跃的实施计划**是 `.dev/plans/0.4.0-test-milestones.md`（借用检查实现测试里程碑）。

| 需求 | 看哪里 |
|---|---|
| **当前活跃计划** | **`.dev/plans/0.4.0-test-milestones.md`**（M0-M5 测试里程碑 + 借用检查实现路线） |
| **.uc 测试总览** | **`docs/test-outline.md`**（102 个 .uc 测试项目录 + 状态） |
| **Bootstrap 路线图** | **`bootstrap/PLAN.md`**（2026-08-06 新建） |
| 借用检查决策记录 | `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md`（21+ 决策 + 0.3.0 outcome） |
| 模块系统设计 | `.dev/drafts/0.1.0-module-system.md`（0.3.0 仍适用） |
| 0.2.0 dead code 清理 | `.dev/drafts/0.4.0-mutable-pointer-todo.md`（M1 处理） |
| 0.1.0 历史快照 | `.dev/_archive/v0.1.0/`（devhandbook §12.2/§12.4 算法伪代码仍可作 C 主机实现参考） |
| 0.2.0 历史 | `.dev/_archive/v0.2.0/c-asm-bootstrap.md`（Phase 1-5 已完成） |
| `.dev/` 索引 | `.dev/README.md` |
| C 端口代码结构 | `src-c/README.md` |
| **0.3.0 语言规范（用户面向，当前版）** | **`docs/UltraCPP-v0.3.0-spec-zh-CN.md`、`docs/UltraCPP-v0.3.0-spec-en.md`** |
| 原 Rust 实现（迁移参照） | `src/frontend/{ast,parser,lexer,token,dump_ast}.rs`、`src/codegen/{generator,builtin}.rs` |
| 测试程序 | `test/test_*/main.upp`、`test/test_simple.uc`（旧）；`test/programs/baseline/m0_*.uc`（M0 新） |

---

*最后更新：删除 `src-asm/`，启动 bootstrap（`src-uc/` + `bootstrap/PLAN.md` + `bootstrap/baseline/`）；Phase 1+2+3+4 完成；接下来按 `bootstrap/PLAN.md` §3 实施 Level 0。*
