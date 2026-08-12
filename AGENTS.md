# UltraCPP Project Agent Guide

> Project-specific rules for agents working in this repository.
> 最后更新：2026-08-08

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

> 最后更新：2026-08-10

- [x] **Language design documentation**：0.3.0 完成（两条权限独立 + `#modlaw` + 线程模型）
- [x] **C port of the compiler**：Phase 1+1.1+2+3+4 完成，505 单元测试，5/5 byte-exact 端到端
- [x] ~~asm port of the lexer~~：2026-08-06 删除（asm 路径放弃，走 bootstrap）
- [ ] **Borrow-check implementation in C host**：**当前活跃方向**，按 `.dev/plans/0.4.0-test-milestones.md` M0-M5 推进
  - [x] **M0 P0-1**: const trio (m0_22/45/46) done; baseline 11/48 PASS as of 2026-08-10
  - [ ] **M0 P0-2**: alloc/free/move/unique PARSE-ONLY trio (m0_34/35/36) — pending
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

*Last updated: 2026-08-10*
