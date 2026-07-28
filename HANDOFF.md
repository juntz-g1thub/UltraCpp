# Worktree Handoff — `feature/borrow-check-verification`

> **TL;DR**：当前在做 UltraCPP 编译器从 Rust → C → asm → 自举的迁移。**Phase 1（C 词法器）+ Phase 1.1（字节级验证）已完成并提交**。下一步是 Phase 2（C AST + parser）。

---

## 一、回到这里时 30 秒内要做的 3 件事

```bash
# 1. 确认所在 worktree（避免误操作 main repo）
pwd                                        # 应输出 .../borrow-check-verification
git log --oneline -5                       # 应看到两个新提交 cefc1c5、fbcb260

# 2. 确认 Phase 1 仍能通过
make -C src-c test                         # 应输出 "69/69 tests passed"

# 3. 确认字节级验证仍通过
bash tools/tokenize_test.sh                # 应输出 "7 passed, 0 failed"
```

如果任何一步失败，不要继续操作，先看下面的"故障排查"。

---

## 二、当前进度

| Phase | 内容 | 状态 | 提交 |
|-------|------|------|------|
| 0 | 规划 | ✅ | `.sisyphus/plans/UltraCPP-v0.2.0-c-asm-bootstrap-zh-CN.md` |
| 1 | C-Lexer | ✅ | `cefc1c5` |
| 1.1 | 字节级验证 | ✅ | `fbcb260` |
| **2** | **C-AST + Parser** | **🔄 待启动** | — |
| 3 | C-Codegen | ⏳ | — |
| 4 | C-CLI | ⏳ | — |
| 5 | asm-Lexer | ⏳ | — |
| 6 | UC-Frontend（自举） | ⏳ | — |
| 7 | Bootstrap 验证 | ⏳ | — |

**下下阶段的具体任务见** `.sisyphus/plans/UltraCPP-v0.2.0-c-asm-bootstrap-zh-CN.md` §7。

---

## 三、目录结构速查

```
.worktrees/borrow-check-verification/
├── HANDOFF.md                              ← 你正在读的文件
├── src-c/                                  C 实现（Phase 1+）
│   ├── Makefile
│   ├── README.md                           C 端口详细说明
│   ├── include/uc_*.h                      公共头
│   ├── src/{error,token,lexer,main}.c      实现
│   └── tests/test_lexer.c                  69 个单元测试
├── src-asm/                                占位（Phase 5 用）
├── tools/
│   └── tokenize_test.sh                    端到端验证脚本
├── .sisyphus/plans/
│   └── UltraCPP-v0.2.0-c-asm-bootstrap-zh-CN.md   完整规划
├── src/                                    原 Rust 实现（不要改，除非迁移需要）
└── test/                                   5 个测试程序（不要改）
```

---

## 四、约定（迁移期 C 代码必须遵守）

| 项 | 约定 |
|----|------|
| C 标准 | C99（`cc -std=c99`） |
| 警告 | `-Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes`（全开，零警告） |
| 命名 | `snake_case`，公开符号前缀 `uc_`，枚举值全大写 `UC_*` |
| 头文件 | 公共 API 放 `src-c/include/uc_*.h` |
| 内存 | 显式 `malloc`/`free`；所有权在头文件注释中声明 |
| 错误 | `UCError*` out 参数；不抛异常、不 setjmp |
| 字符串 | `char*` + `size_t len`，NUL-terminated |
| 编译选项 | `-O2 -g`（release 模式 `-O3 -DNDEBUG`） |

**Makefile 自动发现** `src/*.c`，新增 `.c` 文件无需改 Makefile。**测试**用 `tests/test_*.c`，Makefile 的 `LIB_OBJECTS` 已经排除 `main.o`，新测试无需改 Makefile。

---

## 五、Phase 2 启动包（最小 AST）

### 任务清单

- [ ] 读 `src/frontend/ast.rs` 全部（90 行）
- [ ] 读 `src-c/README.md` 和 `src-c/src/lexer.c` 头部 50 行（学风格）
- [ ] 创建 `src-c/include/uc_ast.h`：所有 AST 节点的 typedef
- [ ] 创建 `src-c/src/ast.c`：构造器、释放、深拷贝
- [ ] 创建 `src-c/tests/test_ast.c`：≥ 5 个测试用例（创建+释放、所有权正确性）
- [ ] 运行 `make -C src-c test`，确保 69/69 + 新测试都过

### Rust → C 风格映射（参考）

| Rust | C |
|------|---|
| `Box<T>` | `struct T*` 显式 malloc/free |
| `Vec<T>` | `T* data; size_t len; size_t cap;` |
| `enum X { A(i32), B }` | tag + union: `struct UCX { UCXTag tag; union { int a; } as; }` |
| `Option<T>` | `bool has; T value;` |
| `Result<T,E>` | `UCError*`（仅失败路径） |
| `String` | `char* data; size_t len;` |

### 验证清单

- [ ] 不引入任何 memory leak（用 `valgrind` 或 `ASAN` 跑测试）
- [ ] 不引入 double-free
- [ ] 节点树的深释放能正确清理所有子节点

### 完成后

提交：
```bash
git add -A
git -c user.name="UltraCPP Bot" -c user.email="bot@ultracpp.local" \
    commit -m "Phase 2.1: C AST type definitions and helpers"
```

然后开始 Phase 2.2：parser skeleton（仅 top-level declarations，能 parse 空 main.upp）。

---

## 六、踩过的坑（避免重复）

### 6.1 Sub-agent 任务规模

**问题**：一次 dispatch 让 worker sub-agent 完成 parser.rs（880 行 → ~1500 行 C），sub-agent 在产出可编译代码前 OOM/超时（exit 137）。

**结论**：**单 sub-agent 任务 ≤ 2 文件 ≤ 200 行 C**。超出就拆。

**Phase 2 的拆分**（已写入主计划）：
- 2.1 AST 类型（独立，最小）
- 2.2 parser skeleton + top-level declarations
- 2.3 parser statements（if/while/for/return）
- 2.4 parser expressions binary（按优先级分层）
- 2.5 parser expressions unary/postfix/literals
- 每个块单独 sub-agent + 单独验证

### 6.2 Rust lexer 列号 bug 不在迁移期修

**问题**：Rust lexer 在 identifier/number/string/char 扫描时不更新列号。

**决策**：在 `tools/tokenize_test.sh` 中归一化列号。Phase 1.1 已记录。Phase 6+（UC 自举）时统一处理列号。

**不要**为了列号字节级匹配去改 Rust lexer，那是独立改进。

### 6.3 不要碰的目录

- `src/`（Rust 实现）—— 只在**迁移需要**时改（如加 `--dump-ast` flag）
- `test/` —— 测试程序，不要改
- `docs/` —— 用户/语言规范，更新是单独任务

### 6.4 不要碰的文件

- `src/frontend/lexer.rs` 的列号跟踪逻辑（已知 bug，独立修复）
- `src/codegen/generator.rs` 中吞掉 `Stmt::For/Break/Continue` 的 `_ => {}`（独立 bugfix）

---

## 七、故障排查

| 症状 | 原因 | 解决 |
|------|------|------|
| `pwd` 显示 `/home/zjtti/Coding/UltraCpp` | bash 每次调用 cwd 重置 | 命令前加 `cd .../.worktrees/borrow-check-verification &&` |
| `make -C src-c` 在 `src-c/src/main.c` 报 `multiple definition of main` | 测试和 main 都链接了 | 不需要改，Makefile 已经排除 `main.o` |
| `cargo test --offline` 报 `assert_cmd` 下载失败 | dev-deps 不在本地缓存 | 联网时 `cargo fetch`；或暂时删 `Cargo.toml` 中 dev-deps |
| C 编译警告：`/*` within comment | 我在注释里写了 `/* ... * /` | 改成 `forward-slash-star ... star-slash` 文字描述 |
| Rust `--dump-tokens` 列号与 C 不同 | Rust 列号 bug（已知） | 见 `tools/tokenize_test.sh` 的 normalize 步骤 |

---

## 八、git 工作流建议

```bash
# 每次开始新工作
cd /home/zjtti/Coding/UltraCpp/.worktrees/borrow-check-verification
git status                                # 看是否有遗留
git fetch origin                          # 拿 main 最新
git rebase main                           # 基础对齐（可选）
git log --oneline -5                      # 看历史
make -C src-c test                        # 确认基线通过

# 提交粒度
# - 一个逻辑改动 = 一个 commit
# - 测试和实现放同一个 commit

# 推送（可选）
git push -u origin feature/borrow-check-verification
```

---

## 九、相关文档指针

| 需求 | 看哪里 |
|------|--------|
| 完整迁移规划（含阶段图、风险、决策） | `.sisyphus/plans/UltraCPP-v0.2.0-c-asm-bootstrap-zh-CN.md` |
| C 端口代码结构、API 用法 | `src-c/README.md` |
| UltraCPP 语言规范 | `docs/UltraCPP-v0.1.0-spec-zh-CN.md`（中文）、`docs/UltraCPP-v0.1.0-spec-en.md`（英文） |
| 原 Rust 实现（迁移参照） | `src/frontend/{ast,parser,lexer,token,mod}.rs` |
| 测试程序（验证目标） | `test/test_*/main.upp` |
| 端到端验证工具 | `tools/tokenize_test.sh` |

---

*最后更新：Phase 1 + 1.1 已提交（`fbcb260`），准备启动 Phase 2.1*
