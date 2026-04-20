# UltraCPP Project Agent Guide

> Project-specific rules for agents working in this repository.

---

## Version Naming Convention

**Naming Format**: `UltraCPP-vx.y.z-文件名-en/zh-CN.md`

| Document Type | Example |
|--------------|---------|
| Specification | `UltraCPP-v0.1.0-spec-zh-CN.md` |
| Quick Guide | `UltraCPP-v0.1.0-guide-zh-CN.md` |
| Developer Handbook | `UltraCPP-v0.1.0-devhandbook-zh-CN.md` |
| Research Notes | `UltraCPP-v0.1.0-module-system-zh-CN.md` |

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
| Design Specs | `.sisyphus/plans/UltraCPP-v0.1.0-*-zh-CN.md` |
| Research Notes | `.sisyphus/drafts/UltraCPP-v0.1.0-*-zh-CN.md` |
| This File | `Agent.md` (root) |

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

- [x] Language design documentation
- [ ] Compiler development (not started)

---

## If Uncertain

1. Read `Agent.md` (this file)
2. Read relevant spec in `.sisyphus/plans/`
3. Ask user before making assumptions about language syntax

---

*Last updated: 2026-04-14*
