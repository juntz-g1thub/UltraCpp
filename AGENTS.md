# UltraCPP Project Agent Guide

> Project-specific rules for agents working in this repository.

---

## Version Naming Convention

**Naming Format**: `UltraCPP-vx.y.z-文件名-en/zh-CN.md`

| Document Type | Example |
|--------------|---------|
| Specification | `docs/UltraCPP-v0.1.0-spec-zh-CN.md` |
| Quick Guide | `.dev/plans/0.1.0-guide.md` |
| Developer Handbook | `.dev/plans/0.1.0-devhandbook.md` |
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
| 当前迁移规划 | `.dev/plans/0.2.0-c-asm-bootstrap.md` |
| 0.1.0 实施计划 | `.dev/plans/0.1.0-*.md` |
| 0.1.0 设计草稿 | `.dev/drafts/0.1.0-*.md` |
| `.dev/` 索引 | `.dev/README.md` |
| 语言规范（用户面向，当前版） | `docs/UltraCPP-v0.1.0-spec-zh-CN.md`、`docs/UltraCPP-v0.1.0-spec-en.md` |
| C 端口代码说明 | `src-c/README.md` |
| asm 端口状态 + 失败分析 | `src-asm/README.md` |
| 项目工作树 handoff | `HANDOFF.md`（30 秒恢复 + 完整状态） |

> **2026-08-06 重组说明**：原 `.sisyphus/{plans,drafts}/` 已迁至 `.dev/`。
> 详细结构与命名规则见 `.dev/README.md`。
> 用户面向的 `docs/`（语言规范）保留不动。

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

> 最后更新：2026-08-06（与 `HANDOFF.md` §二 同步）

- [x] Language design documentation（0.1.0 完成）
- [x] C port of the compiler（Phase 1+1.1+2+3+4 完成，505 单元测试，5/5 byte-exact 端到端）
- [ ] asm port of the lexer（Phase 5 工具链 stub 已提交，完整 lexer 留待后续）
- [ ] UC-Frontend / Bootstrap（Phase 6/7，未启动）

> 详细当前状态、Phase 5 失败分析、Phase 6+ 路径选项见 `HANDOFF.md`。

---

## If Uncertain

1. Read `AGENTS.md` (this file)
2. Read relevant spec in `.dev/plans/`（实施计划）或 `docs/`（语言规范）
3. Read `HANDOFF.md` for project status (30-second recovery + state)
4. Ask user before making assumptions about language syntax

---

*Last updated: 2026-08-06*
