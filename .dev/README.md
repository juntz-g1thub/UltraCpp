# `.dev/` — 开发过程专用文档

> 本目录收纳项目开发过程中产生的**过程文档**：任务规划、实现计划、设计草稿、决策记录。
> 区别于面向最终用户的文档（`README.md` / `HANDOFF.md` / `docs/` 等）。
>
> **历史归档**：v0.1.0 / v0.2.0 时代的设计草稿已移至 `.dev/_archive/`。仅 0.3.0 相关的活跃文档保留在主目录。
>
> **位置**：`/dev`（相对项目根），原先在 `.sisyphus/` 下，2026-08-06 重组迁移而来。
>
> **原则**：这里的内容是**过程产物**，描述"我们怎么想、怎么做"，不是"这门语言/这个工具怎么用"。
> 用户面向的文档留在 `docs/` 和根目录。

---

## 当前目录结构

```
.dev/
├── README.md                                  本文件（索引）
├── plans/                                     实施计划与设计文档
│   └── 0.4.0-test-milestones.md              借用检查实现测试里程碑规划（M0-M5）
└── drafts/                                    探索性草稿与决策记录
    ├── 0.1.0-borrowck-spec-vs-impl.md         借用检查审计 + 21+ 决策 + 0.3.0 对齐（活跃决策源）
    ├── 0.1.0-module-system.md                模块/预编译系统设计（仍适用 0.3.0）
    └── 0.4.0-mutable-pointer-todo.md         UC_TYPE_MUTABLE_POINTER 残留清理待办
```

## 当前活跃文档

| 文件 | 状态 | 用途 |
|---|---|---|
| `plans/0.4.0-test-milestones.md` | **活跃** | 借用检查实现测试里程碑规划（M0-M5）。102 个 .uc 测试项目录 + 状态更新流程 |
| `drafts/0.1.0-borrowck-spec-vs-impl.md` | **活跃** | 借用检查审计 + 21+ 决策记录 + 与 0.3.0 对齐（§10-7）。C 主机 ownership checker 实现以此为决策源 |
| `drafts/0.1.0-module-system.md` | **活跃** | 模块/预编译系统设计（#import / #include）。0.3.0 spec §9 沿用此设计 |
| `drafts/0.4.0-mutable-pointer-todo.md` | **活跃** | `UC_TYPE_MUTABLE_POINTER` 残留清理待办（M1 期间处理） |

## 归档（`.dev/_archive/`）

历史快照，保留以备查阅，**不再用于活跃开发**。

| 路径 | 内容 | 备注 |
|---|---|---|
| `.dev/_archive/v0.1.0/` | 0.1.0 时代的设计（编译器架构、devhandbook、guide、spec-snapshot、pointer-design） | 概念已与 0.3.0 不一致；devhandbook 的算法伪代码 (§12.2/§12.4) 仍可作 C 主机实现参考 |
| `.dev/_archive/v0.2.0/` | 0.2.0 迁移与自举规划 | Phase 1-5 已完成；当前活跃路线见 `bootstrap/PLAN.md` |

## 历史

- **2026-08-07**：归档 v0.1.0 / v0.2.0 文档；删除 `docs-update.md`（任务完成）和 `syntax-features.md`（与项目无关）。
- **2026-08-06**：从 `.sisyphus/{plans,drafts}/` 重组而来。`.sisyphus/` 删除。
  改名规则：去掉 `UltraCPP-` 前缀和 `-zh-CN` 后缀，保留版本号作为文件名前缀。
  例：`UltraCPP-v0.2.0-c-asm-bootstrap-zh-CN.md` → `0.2.0-c-asm-bootstrap.md`。