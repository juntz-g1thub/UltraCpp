# `.dev/` — 开发过程专用文档

> 本目录收纳项目开发过程中产生的**过程文档**：任务规划、实现计划、设计草稿。
> 区别于面向最终用户的文档（`README.md` / `HANDOFF.md` / `docs/` 等）。
>
> **位置**：`/dev`（相对项目根），原先在 `.sisyphus/` 下，2026-08-06 重组迁移而来。
>
> **原则**：这里的内容是**过程产物**，描述"我们怎么想、怎么做"，不是"这门语言/这个工具怎么用"。
> 用户面向的文档留在 `docs/` 和根目录。

---

## 目录结构

```
.dev/
├── README.md                       本文件（索引）
├── plans/                          实施计划与设计文档
│   ├── 0.1.0-compiler-architecture.md   编译器整体架构设计
│   ├── 0.1.0-devhandbook.md             开发者手册（2009 行，最详尽）
│   ├── 0.1.0-docs-update.md             文档更新与整理计划
│   ├── 0.1.0-guide.md                   快速指南
│   ├── 0.1.0-spec-snapshot.md          0.1.0 语言规范旧版快照
│   └── 0.2.0-c-asm-bootstrap.md         0.2.0 迁移与自举规划（当前）
└── drafts/                         探索性草稿与设计讨论
    ├── 0.1.0-module-system.md           模块/预编译系统设计
    ├── 0.1.0-pointer-design.md          指针类型设计讨论
    └── 0.1.0-syntax-features.md         C++ 语法特性梳理（设计输入）
```

---

## plans/ — 实施计划与设计文档

这些是相对正式的计划与设计文档。每个文件代表一个明确的工作方向。

| 文件 | 状态 | 用途 |
|---|---|---|
| `0.1.0-compiler-architecture.md` | 历史 | 0.1.0 编译器架构设计（Rust 实现的总体蓝图） |
| `0.1.0-devhandbook.md` | 历史 | 0.1.0 开发者手册（最详尽的内部参考，2009 行） |
| `0.1.0-docs-update.md` | 历史 | 0.1.0 文档更新与整理计划 |
| `0.1.0-guide.md` | 历史 | 0.1.0 快速指南 |
| `0.1.0-spec-snapshot.md` | 历史 | 0.1.0 语言规范早期快照（1102 行；当前规范在 `docs/`，1254 行） |
| `0.2.0-c-asm-bootstrap.md` | **当前** | 0.2.0 迁移与自举规划（Phase 1-5 的实施依据） |

> 注：0.1.0 文档为历史快照，**参考用**。当前语言规范请看 [`docs/UltraCPP-v0.1.0-spec-zh-CN.md`](../docs/UltraCPP-v0.1.0-spec-zh-CN.md) / [`-en.md`](../docs/UltraCPP-v0.1.0-spec-en.md)。

## drafts/ — 探索性草稿与设计讨论

这些是早期探索阶段的草稿，用于辅助决策。**已沉淀为正式计划或被放弃**。

| 文件 | 用途 |
|---|---|
| `0.1.0-module-system.md` | 模块/预编译系统设计探索（最终未在 0.1.0 实现） |
| `0.1.0-pointer-design.md` | 指针类型（unique/move/free）设计讨论（部分实现于 0.1.0） |
| `0.1.0-syntax-features.md` | C++11-23 语法特性梳理（0.1.0 语法选型的输入） |

---

## 历史

- **2026-08-06**：从 `.sisyphus/{plans,drafts}/` 重组而来。`.sisyphus/` 删除。
  改名规则：去掉 `UltraCPP-` 前缀和 `-zh-CN` 后缀，保留版本号作为文件名前缀。
  例：`UltraCPP-v0.2.0-c-asm-bootstrap-zh-CN.md` → `0.2.0-c-asm-bootstrap.md`。
