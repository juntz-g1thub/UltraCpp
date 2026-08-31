# UltraCPP Project Agent Guide

> Project-specific rules for agents working in this repository.
> 最后更新：2026-08-28

---

## 📍 当前工作状态（Status Pointer）

完整 M0 进展请查看 **`HANDOFF.md`**（项目根目录）。**AGENTS.md 仅作索引 / 入口**，
不重复维护具体进度数字，以避免与 HANDOFF 不一致。涉及决策/计划/历史/
当下 milestone 的所有问题，以 **`HANDOFF.md` 为单一权威来源（single source of truth）**。

> 当前快速摘要（最新一次 baseline，2026-08-28 当前 HEAD 见 `git log --oneline -1`；0.3.6 启动 commit 16-0）；m0 baseline **36/49 PASS / 13 FAIL**（=73.5%）；C unit **554/554** PASS；
> 0.3.1 → 0.3.5 实施全部 ✅ done（baseline 27 → 32 → 36/49）；0.3.6 plan + spec-text-changes 写盘 @ `07e6fe8` + `86c2159`；**0.3.6 实施启动**（commit 16-0）— 15 commits 16-0 ~ 16-12 per `.dev/drafts/0.3.6-implementation-plan.md`；11 D + 5 S 决策；下一 commit **16-1a lib/alloc.uc rewrite** uc_alloc/uc_free 用 uc_syscall(SYS_BRK) brk 链 (per D3 + S5)；末态预期 m0 **46/49** = 94%，C unit **~580-600**，lib tests **5/5**。原地继承 0.3.5 plan: P2（数组/struct — fn-ptr 已 0.3.5 翻 PASS；剩数组/struct/m0_38）+ P3（main return code / 位运算 / 字符串 IO / 三元 runner 修复）。**永久 NO-OP**：CJK 标识符真支持（user 决策 2026-08-21，0.3.5 spec §2.4 + §S7 明文）。
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
| **0.3.5 语言规范（用户面向，当前版）** | `docs/UltraCPP-v0.3.5-spec-zh-CN.md`、`docs/UltraCPP-v0.3.5-spec-en.md`（commit 14a, 2026-08-28）|
| **0.3.4 语言规范（immutable, prior version）** | `docs/UltraCPP-v0.3.4-spec-zh-CN.md`、`docs/UltraCPP-v0.3.4-spec-en.md`（commit 11b, 2026-08-21）|
| **0.3.5 实施过程（completed）** | `.dev/drafts/0.3.5-implementation-process.md` |
| **0.3.5 实施计划** | `.dev/drafts/0.3.5-implementation-plan.md` (339 行) |
| **0.3.5 spec text changes** | `.dev/drafts/0.3.5-spec-text-changes.md` (213 行) |
| **0.3.5 release notes** | `docs/UltraCPP-v0.3.5-release-notes.md` |
| **0.3.6 实施计划（current, 15 commits 16-0 ~ 16-12）** | `.dev/drafts/0.3.6-implementation-plan.md` (写盘 @ `07e6fe8`) |
| **0.3.6 spec text changes（commit 16-11 实施）** | `.dev/drafts/0.3.6-spec-text-changes.md` (写盘 @ `86c2159`) |
| **0.3.4 实施过程（SUPERSEDED by 0.3.6）** | `.dev/drafts/0.3.4-implementation-process.md` |
| **0.3.4 release notes** | `docs/UltraCPP-v0.3.4-release-notes.md`（2026-08-21 release notes）|
| **0.3.4 spec text changes（immutable）** | `.dev/drafts/0.3.4-spec-text-changes.md` (1208 行) |
| **0.3.4 实施计划（immutable）** | `.dev/drafts/0.3.4-implementation-plan.md` (707 行) |
| **0.3.3 实施过程（SUPERSEDED by 0.3.4）** | `.dev/drafts/0.3.3-implementation-process.md` |
| **.uc 测试目录**（49 个 m0 测试 + 102 个 historical） | `docs/test-outline.md` |
| 借用检查审计 + 21+ 决策 + 0.3.0 outcome | `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` |
| UC_TYPE_MUTABLE_POINTER 清理（0.3.5 14g done） | `.dev/drafts/0.3.0-mutable-pointer-todo.md` |
| 模块系统设计（0.3.0 仍适用） | `.dev/drafts/0.1.0-module-system.md` |
| `.dev/` 索引 | `.dev/README.md` |
| C 端口代码说明 | `src-c/README.md` |
| Bootstrap 路线图（**当前休眠**） | `bootstrap/PLAN.md`（2026-08-06 写；当前活跃方向是 M0-M5 借用检查） |
| 项目根 README（含设计历程） | `README.md`、`README-zh-CN.md` |
| 历史规范（已不再活跃） | `docs/UltraCPP-v0.{1,2,3}.0-spec-{zh-CN,en}.md`、`docs/UltraCPP-v0.3.{1,2,3}-spec-{zh-CN,en}.md` |
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

- ✅ `README.md` / `README-zh-CN.md` 已按本节规则清理（commit `b7ca430` + `857ee1b` 双语 + commit 15 phase 2 v0.3.4 → v0.3.5 刷指针）
- ✅ HANDOFF.md §3 background spec pointer 已清除禁词措辞（commit 13 phase 1 at `5499a1a` + commit 15 phase 1 at `fe649be` 同步刷 0.3.5）
- ✅ **0.3.5 实施完成**（HEAD `fe649be`，2026-08-28）— 8 commits: 14a-errata spec 第 1 组变更 + 14b fn-ptr + 14c #include semantic + 14d multi-TU #import + m0_39 cleanup + 14e M1 关键字 lexer + 14f compound_assign + 14g UC_TYPE_MUTABLE_POINTER 死代码清理；baseline 32/48 → 36/49（+4 m0_37/40/44/07）；C unit 505/505 → 554/554 PASS（+49）；完整 commit 序列与设计决策见 `.dev/drafts/0.3.5-implementation-process.md`
- ✅ **0.3.6 启动** (commits `07e6fe8` plan + `86c2159` spec-text-changes + commit 16-0 HANDOFF/AGENTS refresh)：0.3.6 实施 15 commits 16-0 ~ 16-12 计划见 `.dev/drafts/0.3.6-implementation-plan.md`；11 D 决策 (范围/m0 baseline/lib 改动/文档/m0_38/数组/struct/spec/.gitignore) + 5 S 决策 (G1 sys:: 解读B: 保留公共 API + lib 内部绕开直接 uc_syscall)
- 🔄 **0.3.6 实施** (commits 16-0 已落地, 16-1a ~ 16-12 待执行, **当前下一 commit = 16-1a**): lib/alloc.uc rewrite + lib/print.uc rewrite (16-1b) + lib/sys/raw.uc arm64 (16-2) + run_lib_tests 激活 (16-3) + lib/io.uc DELETE (16-4) + lib/print_float 完整化 + 激活 (16-5/16-6) + m0_38 typedef codegen (16-7) + m0_48 runner regex (16-8) + m0_02/03/08 runtime return (16-9) + m0_26/27/28 array (16-10) + m0_32/33 struct (16-10') + 0.3.6 spec 双语新写 (16-11) + 0.3.6 release notes + process ledger (16-12)
- **0.3.6 末态预期**: m0 baseline **36 → 46/49** (=94%, +10: m0_02/03/08/26/27/28/32/33/38/48)；C unit **554 → ~580-600**；lib tests **0 → 5/5**；spec 0.3.6 双语新写 (cp 0.3.5 + header swap + §10.4 注 1 + §11.0.2 注 2 删)
- **0.3.7+ 路线提示**：剩余 3 个 m0 FAIL (P3 范畴: 位运算 m0_05/06 + 字符串 IO m0_24/25 等) 推 0.3.7 / 0.4.0；M1-M5 borrow-check 完整实施 推 0.4.0+；1.0 远期 = 全借用检查 + lib 完整化 (mutex/atomic/sync) + lib/io.uc 取消 (per 0.3.6 后)
- ⚠️ `.dev/_archive/` 历史归档不含 spec 双语文件，仅含 README + 设计笔记；本节翻译规则不追溯历史
- ⚠️ lib/io.uc 0.3.6 commit 16-4 `git rm`（per 0.3.5 spec §11.0.2 deprecate）；0.3.6 链之后 .uc 测试 m0_39 仍引 `lib/io.uc` 引用需同步清理（在 16-4 commit 处理）

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

> 最后更新：2026-08-28
>
> **0.3.6 实施中**（commit 16-0 已落地，14 commits 待实施 16-1a ~ 16-12 per `.dev/drafts/0.3.6-implementation-plan.md`）
>
> **关键决策**：11 D（范围 / m0 baseline / lib 改动 / 文档 / m0_38 / 数组 / struct / spec / .gitignore）+ 5 S（G1 sys:: 解读B）
>
> **末态预期**：m0 46/49, C unit ~580-600, lib tests 5/5, spec 0.3.6 双语新写

- [x] **Language design documentation**：0.3.0-0.3.5 完成（两条权限独立 + `#modlaw` + 线程模型 + CJK 永久 NO-OP + M1 关键字 lexer + compound_assign + fn-ptr + #include + multi-TU）
- [x] **C port of the compiler**：Phase 1+1.1+2+3+4 完成，554 单元测试 (0.3.4 末态 505 + 0.3.5 14b-14g 累计 +49)，5/5 byte-exact 端到端
- [x] ~~asm port of the lexer~~：2026-08-06 删除（asm 路径放弃，走 bootstrap）
- [x] **0.3.5 实施 ✅ done** (HEAD `fe649be`，2026-08-28，baseline **36/49 PASS / 13 FAIL**, C unit **554/554 PASS**):
  - [x] **0.3.5 commit 14a-errata** (spec 第 1 组变更) — spec 0.3.5 zh-CN + en 新写（cp 0.3.4 + header swap + §2.4 ASCII-only 明文 + §14 ADD §S7 NO-OP + §11.0.2 lib/print.uc 函数名校正 + lib/io.uc 处置 + uc_print_num 事实校正）；CJK 永久 NO-OP（user 决策 2026-08-21）；不动 0.3.4 spec / src-c / lib/uc_runtime.c
  - [x] **0.3.5 commit 14b** (fn-ptr 类型扩展) — src-c/ parser + codegen `UC_TYPE_FN_PTR` AST 节点；m0_37 翻 PASS (+1)
  - [x] **0.3.5 commit 14c** (#include 语义) — src-c/ parser + codegen `#include "path"`；m0_40 翻 PASS (+1)
  - [x] **0.3.5 commit 14d** (multi-TU #import + UC_EXPR_FIELD declare fix) — src-c/ multi-TU 端到端链接测试；m0_44 翻 PASS (+1)
  - [x] **0.3.5 commit m0_39 cleanup** — m0_39 runner 期望值修正（巩固 PASS）；lib/io.uc 保留（spec 标 deprecate，0.3.6 删除）
  - [x] **0.3.5 commit 14e** (M1 关键字 lexer) — mod/unmod/is_null/sizeof/alignof/volatile 关键字 lexer 集成（mod/unmod 0.3.3 已实施；本 commit 补 6 关键字 token + parser 路径）
  - [x] **0.3.5 commit 14f** (compound_assign) — `+= -= *= /= %=` 运算符 lexer 集成 + codegen emit；m0_07 翻 PASS (+1)
  - [x] **0.3.5 commit 14g** (UC_TYPE_MUTABLE_POINTER 死代码清理) — src-c/ ast.h enum 移除 + ast.c/codegen.c/parser.c 引用清理（5 处）；C unit 554/554 PASS（+49 累计）
  - 累计 baseline 提升：**32/48 → 36/49**（+4 m0_37/40/44/07；m0_39 由 cleanup 巩固）；C unit 提升：505 → 554/554
- [ ] **Borrow-check implementation in C host**：**当前活跃方向**，按 `.dev/plans/0.3.0-borrow-check-milestones.md` M0-M5 推进
  - [x] **M0 P0-1**: const trio (m0_22/45/46) done; baseline 11/48 PASS as of 2026-08-10
  - [x] **M0 P0-2**: alloc/free/move/unique trio (m0_34/35/36) done as of 2026-08-12
  - [x] **M0 P0-3**: ++/-- + ternary (m0_47/48) done as of 2026-08-12; P0-3 期间 5 codegen fixes 同时铺平 control flow
  - [x] **M0 P0-4**: void + local/global (m0_17/21) done as of 2026-08-12
  - [x] **M0 P1 ✅ done** (HEAD `e035922`，2026-08-21，baseline **32/48 PASS / 16 FAIL**)
  - [x] **0.3.4 实施 ✅ done** (HEAD `5499a1a`，2026-08-21，7 commits: 10a-10e lib/*.uc 6 文件 + 11a codegen abs_int 路由；baseline 32/48 unchanged)
  - [x] **0.3.5 实施 ✅ done** (HEAD `fe649be`，2026-08-28，8 commits: 14a-errata + 14b + 14c + 14d + m0_39 + 14e + 14f + 14g；baseline **36/49 PASS / 13 FAIL**, C unit **554/554 PASS**)
  - [ ] **0.3.5 docs sync** (commit 15 phase 1-6, **当前执行中**): HANDOFF+AGENTS (phase 1) + README 双语 (phase 2) + release notes (phase 3) + implementation process (phase 4) + 0.3.4 process SUPERSEDED (phase 5) + milestones + m0-priority refresh (phase 6)
  - **remaining**: P2 数组/struct (m0_26/27/28 + m0_32/33) + P3 runtime (m0_02/03/05/06/08/24/25 + m0_48 runner-misjudge)
- [ ] **Bootstrap**（UltraCPP 写 UltraCPP）：路线见 `bootstrap/PLAN.md`，**当前休眠**

> 当前焦点是 **commit 15 docs sync 6 phases**（在 0.3.5 实施完成 `fe649be` 基础上：HANDOFF/AGENTS 刷 0.3.5 → README 双语刷 spec 指针 → 新建 release notes → 新建 implementation process → 标 0.3.4 process SUPERSEDED → 刷 milestones + m0-priority；详见 `.dev/drafts/0.3.5-implementation-plan.md` §3.1 commit 15 + `.dev/drafts/0.3.4-implementation-process.md` 末段「Next Steps」经验）。M0 baseline verification 已 complete（baseline **36/49 PASS / 13 FAIL**；详见 `HANDOFF.md` §1）。
>
> 借用检查在 C 主机中的实现路线仍按 `.dev/plans/0.3.0-borrow-check-milestones.md` M0-M5 推进：
> 1. **M0**（基线验证）——✅ done（36/49 PASS = 73.5%）
> 2. **M1 词法扩展**（mod/unmod/is_null/sizeof/alignof/volatile + compound_assign + UC_TYPE_MUTABLE_POINTER 清理）——✅ done（commit 14e + 14f + 14g，HEAD `fe649be`）
> 3. **M2-M5**（增量实现）——按 0.3.3 spec 章节顺序：所有权 → 修改权 → 引用 const → 线程
>
> 每个里程碑开始前先设计测试项，跑现有测试看哪些通过/失败，写实现让失败测试通过，回归不破坏现有功能。
> 详细流程与测试清单见 `.dev/plans/0.3.0-borrow-check-milestones.md` 和 `docs/test-outline.md`。

---

## If Uncertain

1. Read `AGENTS.md` (this file)
2. Read `docs/UltraCPP-v0.3.4-spec-zh-CN.md` for current language semantics
3. Read `.dev/plans/0.3.0-borrow-check-milestones.md` for current implementation roadmap
4. Read `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` §10-7 for 0.3.0 outcome reconciliation
5. Ask user before making assumptions about language syntax

---

*Last updated: 2026-08-28 (0.3.6 启动 commit 16-0: HANDOFF + AGENTS refresh + 0.3.5 phase 5+6 收尾 + .gitignore 加 .pi/ 规则; HEAD 见 `git log --oneline -1`)*
