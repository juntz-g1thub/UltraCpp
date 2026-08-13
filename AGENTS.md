# UltraCPP Project Agent Guide

> Project-specific rules for agents working in this repository.
> 最后更新：2026-08-12

---

## 📍 当前工作状态（Status Pointer）

完整 M0 进展请查看 **`HANDOFF.md`**（项目根目录）。**AGENTS.md 仅作索引 / 入口**，
不重复维护具体进度数字，以避免与 HANDOFF 不一致。涉及决策/计划/历史/
当下 milestone 的所有问题，以 **`HANDOFF.md` 为单一权威来源（single source of truth）**。

> 当前快速摘要（最新一次 baseline，2026-08-12 HEAD `ef3352c`）：M0 **21/48 PASS / 27 FAIL**；
> P0-1 → P0-2 → P0-3 → P0-4 全部完成 + **P1-2 done** (m0_10 commit `09b4df5`) + **P1-4 partial** (m0_04 commits `5abb262` + `ef3352c`)；下一站 **P1-3** (m0_13 + m0_15)。
> 详情与分类见 `HANDOFF.md` §1 + `.dev/drafts/0.4.0-m0-priority.md` §8。

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
| **当前活跃计划** | `.dev/plans/0.4.0-test-milestones.md` |
| **0.3.0 语言规范（用户面向，当前版）** | `docs/UltraCPP-v0.3.0-spec-zh-CN.md`、`docs/UltraCPP-v0.3.0-spec-en.md` |
| **.uc 测试目录**（102 个测试） | `docs/test-outline.md` |
| 借用检查审计 + 21+ 决策 + 0.3.0 outcome | `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` |
| 0.4.0 待办（UC_TYPE_MUTABLE_POINTER 清理） | `.dev/drafts/0.4.0-mutable-pointer-todo.md` |
| 模块系统设计（0.3.0 仍适用） | `.dev/drafts/0.1.0-module-system.md` |
| `.dev/` 索引 | `.dev/README.md` |
| C 端口代码说明 | `src-c/README.md` |
| Bootstrap 路线图（**当前休眠**） | `bootstrap/PLAN.md`（2026-08-06 写；当前活跃方向是 M0-M5 借用检查） |
| 项目根 README（含设计历程） | `README.md`、`README-zh-CN.md` |
| 历史规范（已不再活跃） | `docs/UltraCPP-v0.{1,2}.0-spec-{zh-CN,en}.md` |
| 历史快照 | `.dev/_archive/{v0.1.0,v0.2.0}/`（含已归档的 HANDOFF、设计文档等） |

> **2026-08-08 状态**：原 `.sisyphus/{plans,drafts}/` 文档已迁至 `.dev/`，0.1.0/0.2.0 时代文档归档至 `.dev/_archive/`。
> 用户面向的 `docs/`（语言规范）按版本保留。
> 当前活跃方向：**借用检查在 `src-c/` 中的实现**（M0-M5 增量开发）。

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

> 最后更新：2026-08-12

- [x] **Language design documentation**：0.3.0 完成（两条权限独立 + `#modlaw` + 线程模型）
- [x] **C port of the compiler**：Phase 1+1.1+2+3+4 完成，505 单元测试，5/5 byte-exact 端到端
- [x] ~~asm port of the lexer~~：2026-08-06 删除（asm 路径放弃，走 bootstrap）
- [ ] **Borrow-check implementation in C host**：**当前活跃方向**，按 `.dev/plans/0.4.0-test-milestones.md` M0-M5 推进
  - [x] **M0 P0-1**: const trio (m0_22/45/46) done; baseline 11/48 PASS as of 2026-08-10
  - [x] **M0 P0-2**: alloc/free/move/unique trio (m0_34/35/36) done as of 2026-08-12 (commits a173f07 + 66406da); m0_35 FULL PASS exit=0, m0_34/36 parse+IR OK; known llc-fail gap on deref-assign to be revisited at P3-5
  - [x] **M0 P0-3**: ++/-- + ternary (m0_47/48) done as of 2026-08-12; commits `cb07848` (m0_47 FULL PASS exit=16) + `70b8845` (m0_48 FULL PASS exit=20 — runner 误判为 FAIL 因 expected-code extraction 默认 0; **实际 effective PASS**); P0-3 期间 5 codegen fixes 同时铺平 control flow (`cf51f6f` label-ref `%%`→`%` strip 8 sites, `f1f4214` `emit_label` 前导 `%` strip, `3abe96b` `UC_STMT_IF` 3-label then/else/end 重构) — m0_09 等 control flow 测试水落石出 PASS
  - [x] **M0 P0-4**: void + local/global (m0_17/21) done as of 2026-08-12; **commit `c052d2c`** for m0_17 (void 函数 return path 修复，m0_17 翻转为 ✓ exit=0); m0_21 早已 PASS; baseline **19/48 PASS / 29 FAIL** (first complete post-P0-4 baseline run; 29 FAILs categorized in priority doc §8.1; m0_17 从 Parser 缺 bucket 移除，剩 9 个 parser 缺)
  - [/] **M0 P1 in-progress** (HEAD `ef3352c`，2026-08-12): **P1-2 ✅ done** (m0_10 else-if 链，commit `09b4df5` — **不是 codegen bug**，是 test runner awk 不解析函数调用式 return，加 trailing `// 255` 注释修复); **P1-4 partial done** (m0_04 comparison，commits `5abb262` codegen i1→i32 zext + `ef3352c` test trailing `// 3`); **remaining**: P1-3 (m0_13 for + m0_15 break/continue) + P1-4 (m0_19 recursion / m0_31 addr-of / m0_42 unsafe block) + P1-1 (m0_50 CJK identifiers)
- [ ] **Bootstrap**（UltraCPP 写 UltraCPP）：路线见 `bootstrap/PLAN.md`，**当前休眠**

> 当前焦点是**借用检查在 C 主机中的实现**：
> 1. **M0**（基线验证）——用 48 个 .uc 测试程序验证 `src-c/` 现状；出 `docs/m0-baseline-report.md`
> 2. **M1-M5**（增量实现）——按 0.3.0 spec 章节顺序：词法语法扩展 → 所有权 → 修改权 → 引用 const → 线程
>
> 每个里程碑开始前先设计测试项，跑现有测试看哪些通过/失败，写实现让失败测试通过，回归不破坏现有功能。
> 详细流程与测试清单见 `.dev/plans/0.4.0-test-milestones.md` 和 `docs/test-outline.md`。

---

## If Uncertain

1. Read `AGENTS.md` (this file)
2. Read `docs/UltraCPP-v0.3.0-spec-zh-CN.md` for current language semantics
3. Read `.dev/plans/0.4.0-test-milestones.md` for current implementation roadmap
4. Read `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` §10-7 for 0.3.0 outcome reconciliation
5. Ask user before making assumptions about language syntax

---

*Last updated: 2026-08-12*
