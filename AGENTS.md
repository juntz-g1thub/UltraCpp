# UltraCPP Project Agent Guide

> Project-specific rules for agents working in this repository.
> 最后更新：2026-08-21

---

## 📍 当前工作状态（Status Pointer）

完整 M0 进展请查看 **`HANDOFF.md`**（项目根目录）。**AGENTS.md 仅作索引 / 入口**，
不重复维护具体进度数字，以避免与 HANDOFF 不一致。涉及决策/计划/历史/
当下 milestone 的所有问题，以 **`HANDOFF.md` 为单一权威来源（single source of truth）**。

> 当前快速摘要（最新一次 baseline，2026-08-21 HEAD `e035922`）：M0 **32/48 PASS / 16 FAIL**（=66.7%）；
> P0-1 → P0-2 → P0-3 → P0-4 全部完成 + **P1-2 ✅ done** (m0_10 commit `09b4df5`) + **P1-3 ✅ done** (m0_13/14/15 commits `afae9c9` + `c8492ef`) + **P1-4 partial done** (m0_19 / m0_29 / m0_31，commits `a513226` + `985eade`；**m0_41 / m0_42 deferred to P3-5**) + **P1-1 ✅ done** (m0_50 commit `02d7170` — runner 加 `// expects_compiler_error` marker 支持 + m0_50 .uc 加 marker；测试目标是验证编译器正确拒绝 CJK 标识符，编译器一直正确输出 lexer error，runner 旧逻辑把 `compile_failed` 一律当 FAIL 是误判) + **P1-5 🟡 deferred** (CJK 真支持推迟到 未来 spec 修订) + **0.3.1 → 0.3.3 实施 ✅ done**（14 commits，baseline 27 → 32/48，5 FAIL 翻 PASS：m0_30 / m0_37 / m0_41 / m0_42 / m0_44 — 0.3.3 实施副产品，详见 `.dev/drafts/0.3.3-implementation-process.md`）；剩 **P2**（数组/struct/extern/include）+ 长期推 **P3-5 pointer runtime** (m0_34/m0_36)。
> 详情与分类见 `HANDOFF.md` §1 + `.dev/drafts/0.3.0-m0-priority.md` §8。

---

## Version Naming Convention

**Naming Format**: `UltraCPP-vx.y.z-文件名-en/zh-CN.md`

| Document Type | Example |
|--------------|---------|
| Specification | `docs/UltraCPP-v0.3.0-spec-zh-CN.md` |
| Quick Guide | `.dev/plans/0.1.0-guide.md` (historical) |
| Developer Handbook | `.dev/plans/0.1.0-devhandbook.md` (historical) |
| Research Notes | `.dev/drafts/0.1.0-module-system.md` |

> **历史约定**：旧文件名用 `UltraCPP-v0.x.y-...zh-CN.md` 格式存放在 `.sisyphus/`。
> 2026-08-06 重组后，开发过程文档迁到 `.dev/`，命名简化为 `<version>-<name>.md`。
> 用户面向的语言规范仍位于 `docs/`，保留全名格式。

---

## Git 操作约束 *(修订, 2026-08-17)*

代理（agent）的 git 操作权限按以下矩阵分配：

| 类别 | 操作 | 权限 |
|---|---|---|
| 本地只读 | `git status` / `git log` / `git diff` / `git show` / `git blame` | ✅ 自由执行 |
| 本地 commit | `git add` / `git commit` / `git commit --amend` | ✅ 默认自由 |
| 本地重构 | `git rebase` / `git reset` / `git cherry-pick` / `git stash` / `git revert`（破坏历史） | ❌ **直接禁止** |
| 远程写 | `git push` / `git fetch --write` / `git pull --rebase/merge` | ❌ **直接禁止** |
| 远程管理 | `git remote add/remove` / `git branch -u` / `git tag --push` | ❌ **直接禁止** |

### 原则

1. **commit 默认自由**：agent 可自行 `git add` + `git commit`，无需逐次确认。前提是改动计划**单一明确**（如"翻译 README 并 commit"）。
2. **多方案时停下询问**：当改动存在多种合理 commit 切分方式（按文件 / 按主题 / 拆分 / 合并 / squash / 原子化等），agent **不得自作主张**，必须列出候选方案让用户拍板。例：
   > 本次涉及 4 个文件：①全部合并为 1 commit；②按文件拆 4 commit；③按主题拆（spec 修复 vs README 改动）。请选择方案。
3. **本地重构完全交给用户**：`git rebase` / `git reset` / `git cherry-pick` / `git stash` 等可能改写历史的操作 agent **不得调用**。理由：
   - 这些操作一旦失误，恢复成本高（reflog 虽能救但非万无一失）
   - 常需结合 push 协作（force-push），与远程禁令冲突
   - 与"commit 默认自由"不同：commit 是追加，重构是改写，前者不可逆程度远低于后者
4. **远程操作完全禁止**：`git push` 及任何写入远程仓库的操作 agent **不得自行调用**，必须由用户手动执行。理由：
   - 远程操作不可逆（push 到共享分支后 force-push 会破坏他人工作）
   - 用户的 GitHub 凭据、SSH key、推送策略不在代理决策范围内
   - CI / pre-push hooks 行为可能与代理预期不符

### 例外

仅当用户在同一次对话中**明确授权**（如"push 吧" / "做 rebase"）时，agent 才可执行对应操作**一次**，并在输出中说明本次操作的目标与影响范围。授权不延续到后续操作。

---

## Critical Syntax Rules

**VIOLATION WILL CAUSE REJECTION**: All code examples MUST use C++ function syntax, NOT Rust.

| Wrong ❌ | Correct ✅ |
|-----------|-----------|
| `fn add(a: int) -> int` | `int add(int a, int b)` |
| `let x: int = 5;` | `int x = 5;` |
| `&str`, `i32`, `usize` | `const char*`, `int`, `size_t` |
| `-> void` (in definition) | Omit or use `void` |
| `mut` for variables | Use `const` for constants |

---

## File Extensions

| Extension | Status |
|------------|--------|
| `.uc` | **Recommended** for UltraCPP source |
| `.upp` | Supported (legacy) |

---

## Document Locations

| Type | Path |
|------|------|
| **当前活跃计划** | `.dev/plans/0.3.0-borrow-check-milestones.md` |
| **0.3.3 语言规范（用户面向，当前版）** | `docs/UltraCPP-v0.3.3-spec-zh-CN.md`、`docs/UltraCPP-v0.3.3-spec-en.md` |
| **0.3.3 实施过程（completed）** | `.dev/drafts/0.3.3-implementation-process.md` |
| **.uc 测试目录**（102 个测试） | `docs/test-outline.md` |
| 借用检查审计 + 21+ 决策 + 0.3.0 outcome | `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` |
| UC_TYPE_MUTABLE_POINTER 清理（M1） | `.dev/drafts/0.3.0-mutable-pointer-todo.md` |
| 模块系统设计（0.3.0 仍适用） | `.dev/drafts/0.1.0-module-system.md` |
| `.dev/` 索引 | `.dev/README.md` |
| C 端口代码说明 | `src-c/README.md` |
| Bootstrap 路线图（**当前休眠**） | `bootstrap/PLAN.md`（2026-08-06 写；当前活跃方向是 M0-M5 借用检查） |
| 项目根 README（含设计历程） | `README.md`、`README-zh-CN.md` |
| 历史规范（已不再活跃） | `docs/UltraCPP-v0.{1,2,3}.0-spec-{zh-CN,en}.md`、`docs/UltraCPP-v0.3.{1,2}-spec-{zh-CN,en}.md` |
| 历史快照 | `.dev/_archive/{v0.1.0,v0.2.0}/`（含已归档的 HANDOFF、设计文档等） |

> **2026-08-08 状态**：原 `.sisyphus/{plans,drafts}/` 文档已迁至 `.dev/`，0.1.0/0.2.0 时代文档归档至 `.dev/_archive/`。
> 用户面向的 `docs/`（语言规范）按版本保留。
> 当前活跃方向：**借用检查在 `src-c/` 中的实现**（M0-M5 增量开发）。

---

## 文档翻译规则 *(新增, 2026-08-17)*

目前**仅 spec 文档**（`docs/UltraCPP-vX.Y.Z-spec-*.md`）与 **`README`** 需要严格的中英文对照翻译。其他文档（设计草稿 `.dev/drafts/`、HANDOFF.md、AGENTS.md、本文件）保持中文单语即可。

### 双语对照原则

1. **独立、同行、无主从**：中英文版本是 sibling，不存在"权威版/译本"关系。任何文件不得出现 `authoritative / companion / 权威 / 配套 / 翻译滞后 / 配套译本 / lag` 等措辞。
2. **行数对齐**（强烈建议）：双语版本行数应大致一致，便于 cross-check。例外：翻译风格本身造成的长度差（如英文表格列宽窄）允许 ±5 行偏差。
3. **逐句翻译，结构镜像**：标题层级、章节编号、表格、列表、代码块、footnote 全部保留；代码块（除内嵌注释外）原文保留 C++ 语法。
4. **跨语言引用最小化**：除约定的两个 header 行（见下）外，双语文件不互相引用。

### 命名约定

| 类型 | 格式 | 示例 |
|---|---|---|
| spec 中文 | `docs/UltraCPP-vX.Y.Z-spec-zh-CN.md` | `docs/UltraCPP-v0.3.1-spec-zh-CN.md` |
| spec 英文 | `docs/UltraCPP-vX.Y.Z-spec-en.md` | `docs/UltraCPP-v0.3.1-spec-en.md` |
| README 中文 | `README-zh-CN.md` | `README-zh-CN.md` |
| README 英文 | `README.md` | `README.md` |

### 双语 spec 文件的 header 模板

每份双语 spec 文件的**前 15 行**应严格遵循以下模板（仅文件名/版本号/日期变化）：

中文版（zh-CN）line 5 + line 11：
```
> **上一版本**：X.Y.Z — [`UltraCPP-vX.Y.Z-spec-zh-CN.md`](./UltraCPP-vX.Y.Z-spec-zh-CN.md)
...
> 同版本英文译本：[English](./UltraCPP-vX.Y.Z-spec-en.md)
```

英文版（en）line 5 + line 11：
```
> **Previous version**: X.Y.Z — [`UltraCPP-vX.Y.Z-spec-en.md`](./UltraCPP-vX.Y.Z-spec-en.md)
...
> Same-version Chinese translation: [简体中文](./UltraCPP-vX.Y.Z-spec-zh-CN.md)
```

注意：
- line 5 "上一版本" **必须 self-language**（中文→中文前一版，英文→英文前一版）
- line 11 "同版本译本指针" **必须 cross-language**（唯一允许的跨语言引用）
- 任何其他位置的跨语言引用都属于违规

### 代码示例语法

所有 spec / README 中的代码示例必须使用 C++ 语法（参见上节 "Critical Syntax Rules"），包括 `int add(int a, int b)`、`const char*`、`int`、`size_t`、`const`。**严禁** Rust 风格（`fn`、`let x: int = 5;`、`&str`、`i32` 作关键字、`mut`、`-> void` 在定义中）。

例外：`i32` / `i64` / `i8` / `i16` / `usize` / `isize` 是 UltraCPP 自身的定宽整数类型，**不是** Rust 污染。

### 翻译工作流程（推荐）

1. **构造中文版**：先 cp 前一版 spec，再应用本版本 delta
2. **构造英文版**：从中文版 cp 后逐句翻译，应用对应英文 header 模板
3. **行数对齐验证**：`wc -l` 双语，差应在 ±5 内
4. **独立性审计**：
   - `grep -nE '权威|authoritative|companion|配套|翻译滞后' <file>` 必须 0 命中
   - `grep -nE '英文|English|en\.md' <zh-CN-file>` 必须仅命中 line 11
   - `grep -nE '中文|Chinese|zh-CN' <en-file>` 必须仅命中 line 5 与 line 11
5. **一次 commit 同时提交双语修改**（按"Git 操作约束"，不 push）

## 后续工作 *(待办)*

- ✅ `README.md` / `README-zh-CN.md` 已按本节规则清理（commit `b7ca430`，header 重写 + banned-word 移除 + 0.3.1 spec 引用更新）
- ⚠️ `.dev/_archive/` 历史归档不含 spec 双语文件，仅含 README + 设计笔记；本节翻译规则不追溯历史
- ⚠️ HANDOFF.md 当前仍含 `current authoritative` 措辞（line 96），是否同步清理待用户决定（不在本节强制范围）

---

## Language Design Core Decisions

### Memory Safety
- Default `unique` pointers: allocation is non-copyable, only movable
- `alloc(T)` / `free(ptr)` for heap memory
- `move(p)` transfers ownership, source becomes null

### Modules
- `#import "module"` — separate compilation + link
- `#include "file"` — source copy (no separate compilation)

### Constants
- Variables: `int x = 5;`
- Constants: `const int MAX = 100;`

---

## Current Phase

> 最后更新：2026-08-21

- [x] **Language design documentation**：0.3.0 完成（两条权限独立 + `#modlaw` + 线程模型）
- [x] **C port of the compiler**：Phase 1+1.1+2+3+4 完成，505 单元测试，5/5 byte-exact 端到端
- [x] ~~asm port of the lexer~~：2026-08-06 删除（asm 路径放弃，走 bootstrap）
- [ ] **Borrow-check implementation in C host**：**当前活跃方向**，按 `.dev/plans/0.3.0-borrow-check-milestones.md` M0-M5 推进
  - [x] **M0 P0-1**: const trio (m0_22/45/46) done; baseline 11/48 PASS as of 2026-08-10
  - [x] **M0 P0-2**: alloc/free/move/unique trio (m0_34/35/36) done as of 2026-08-12 (commits a173f07 + 66406da); m0_35 FULL PASS exit=0, m0_34/36 parse+IR OK; known llc-fail gap on deref-assign to be revisited at P3-5
  - [x] **M0 P0-3**: ++/-- + ternary (m0_47/48) done as of 2026-08-12; commits `cb07848` (m0_47 FULL PASS exit=16) + `70b8845` (m0_48 FULL PASS exit=20 — runner 误判为 FAIL 因 expected-code extraction 默认 0; **实际 effective PASS**); P0-3 期间 5 codegen fixes 同时铺平 control flow (`cf51f6f` label-ref `%%`→`%` strip 8 sites, `f1f4214` `emit_label` 前导 `%` strip, `3abe96b` `UC_STMT_IF` 3-label then/else/end 重构) — m0_09 等 control flow 测试水落石出 PASS
  - [x] **M0 P0-4**: void + local/global (m0_17/21) done as of 2026-08-12; **commit `c052d2c`** for m0_17 (void 函数 return path 修复，m0_17 翻转为 ✓ exit=0); m0_21 早已 PASS; baseline **19/48 PASS / 29 FAIL** (first complete post-P0-4 baseline run; 29 FAILs categorized in priority doc §8.1; m0_17 从 Parser 缺 bucket 移除，剩 9 个 parser 缺)
  - [x] **M0 P1 ✅ done** (HEAD `e035922`，2026-08-21，baseline **32/48 PASS / 16 FAIL**):
    - **P1-2 ✅ done** (m0_10 else-if 链，commit `09b4df5` — **不是 codegen bug**，是 test runner awk 不解析函数调用式 return，加 trailing `// 255` 注释修复)
    - **P1-3 ✅ done** (m0_13 for + m0_14 nested loops + m0_15 break/continue，commits `afae9c9` UC_STMT_FOR/BREAK/CONTINUE + loop scope 栈 + `c8492ef` UC_BIN_MOD + UC_EXPR_LITERAL last_expr_type per lit kind; m0_14 was always FAIL (for-loop codegen 在 afae9c9 前是 stub)，P1-3 codegen change exposed latent literal-type leak, fixed in c8492ef; m0_15 originally 28 → 16 after c8492ef)
    - **P1-4 partial done** (m0_04 comparison commits `5abb262` + `ef3352c`; **m0_31 + m0_29** commit `a513226` UC_UN_ADDR_OF codegen; **m0_19** commit `985eade` .uc trailing `// 208` — factorial(6)=720 但 bash `$?` 只存低 8 bit); **m0_41 / m0_42 deferred to P3-5** — `df29039` + `42d75f7` 推进了 `extern "C" { ... }` block 解析与 builtin 重声明跳过，但仍 fail on deeper bugs（call signature + deref-assign LHS + abs_int linker），属 P3-5 pointer runtime 范畴，**不算 P1-4 DONE**
    - **P1-1 ✅ done** (m0_50 CJK identifier rejection, commit `02d7170` — runner 加 `// expects_compiler_error` marker 支持 + m0_50 .uc 加 marker；测试目的是验证编译器正确拒绝 CJK 标识符(违反 spec §2.4 ASCII 标识符约束)，编译器一直正确输出 `Lexer error at ...: unknown character`，runner 旧逻辑一律把 `compile_failed` 当 FAIL 是误判；新机制下 m0_50 → PASS, src-c 未改)
    - **P1-5 🟡 deferred**（新条目，spec §2.4 范围外）— CJK identifier 真支持推迟到 未来 spec 修订；当前通过 runner `expects_compiler_error` marker 机制标 m0_50 PASS，编译器 src-c 未引入 UTF-8 支持；后续 spec 修订时再决定是否新增「CJK ident」语法点
    - **0.3.1 → 0.3.3 实施 ✅ done**（HEAD `e035922`，14 commits，baseline 27/48 → 32/48）—— 以 0.3.1/0.3.2/0.3.3 spec 为目标推进 lexer/parser/codegen 增量实现；5 FAIL 翻 PASS（m0_30 / m0_37 / m0_41 / m0_42 / m0_44 为 0.3.3 实施副产品）；完整 commit 序列与设计决策见 `.dev/drafts/0.3.3-implementation-process.md`
    - **remaining**: P2 (m0_26/27/28 array + m0_32/33 struct + m0_40 include) + 长期推 P3-5 pointer runtime (m0_34/m0_36)
- [ ] **Bootstrap**（UltraCPP 写 UltraCPP）：路线见 `bootstrap/PLAN.md`，**当前休眠**

> 当前焦点是 **0.3.4 规划**（在 0.3.3 实施完成 14 commits 基础上规划 `lib/*.uc` stdlib 引导 + `abs_int` 迁移；详见 `.dev/drafts/0.3.3-implementation-process.md` 末段「Next Steps」）。M0 baseline verification 已 complete（baseline **32/48 PASS / 16 FAIL**；详见 `HANDOFF.md` §1）。
>
> 借用检查在 C 主机中的实现路线仍按 `.dev/plans/0.3.0-borrow-check-milestones.md` M0-M5 推进：
> 1. **M0**（基线验证）——✅ done（32/48 PASS）
> 2. **M1-M5**（增量实现）——按 0.3.3 spec 章节顺序：词法语法扩展 → 所有权 → 修改权 → 引用 const → 线程
>
> 每个里程碑开始前先设计测试项，跑现有测试看哪些通过/失败，写实现让失败测试通过，回归不破坏现有功能。
> 详细流程与测试清单见 `.dev/plans/0.3.0-borrow-check-milestones.md` 和 `docs/test-outline.md`。

---

## If Uncertain

1. Read `AGENTS.md` (this file)
2. Read `docs/UltraCPP-v0.3.3-spec-zh-CN.md` for current language semantics
3. Read `.dev/plans/0.3.0-borrow-check-milestones.md` for current implementation roadmap
4. Read `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` §10-7 for 0.3.0 outcome reconciliation
5. Ask user before making assumptions about language syntax

---

*Last updated: 2026-08-21*
