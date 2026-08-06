# Contributing to UltraCPP

Thank you for your interest in contributing to UltraCPP.

> **当前状态**：项目已远超 "feasibility research" 阶段。
> 详细见 `HANDOFF.md`。简要：编译器 C 端完整可用（505 单元测试，5/5 字节级 E2E 端到端）。
> asm 端有工具链骨架但完整 lexer 未实现。

## How to Contribute

1. Fork the repository
2. Create a branch for your work (`feature/<short-desc>` 或 `fix/<short-desc>`)
3. Make your changes
4. **跑全部冒烟测试**（见「验证」节）
5. 提交 + push，开 PR
6. PR 描述里写清楚：动了什么、为什么、测试结果

## 验证（commit 之前必跑）

```bash
make -C src-c test                    # 4 个 binary，505/505 单元测试
bash tools/tokenize_test.sh           # 字节级 token  diff: 7/7
bash tools/ast_test.sh                # 字节级 AST   diff: 5/5
bash tools/codegen_test.sh            # 字节级 IR    diff: 5/5
bash tools/build_test.sh              # 端到端 build: 3/3
# 全部应输出 "N passed, 0 failed"
```

任何一项失败 = 改动破坏了字节级 / 行为级契约，需要修。

## 文档约定

开发过程文档统一在 `.dev/` 下：

| 类型 | 路径 |
|---|---|
| 实施计划 / 设计 | `.dev/plans/<version>-<name>.md` |
| 探索性草稿 | `.dev/drafts/<version>-<name>.md` |
| 索引 | `.dev/README.md` |

新增开发过程文档时：
- 先看 `.dev/README.md` 了解结构
- 命名去掉 `UltraCPP-` 前缀和 `-zh-CN` 后缀
- 保留版本号作为文件名前缀（`0.1.0-` / `0.2.0-` 等）
- 提交时单独 commit，commit message 说明放进 `.dev/` 的理由

**不要**放到 `.sisyphus/`（已删除）或 `docs/`（用户面向，不放过程文档）。

## 代码约定

- C 端：`src-c/`，遵守 C99 + `-Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes`（零警告）
- 命名：`snake_case`，公开符号前缀 `uc_`，枚举值全大写 `UC_*`
- 头文件：公共 API 放 `src-c/include/uc_*.h`
- 错误：`UCError*` out 参数
- 字符串：`char*` + `size_t len`，NUL-terminated
- 编译选项：`-O2 -g`（release `-O3 -DNDEBUG`）

更详细的规范见 [AGENTS.md](AGENTS.md)。

## 关键文档指针

| 想了解什么 | 看哪里 |
|---|---|
| 项目当前状态 | `HANDOFF.md`（30 秒恢复 + 完整状态） |
| 迁移规划 | `.dev/plans/0.2.0-c-asm-bootstrap.md` |
| 0.1.0 详细内部参考 | `.dev/plans/0.1.0-devhandbook.md` |
| 语言规范 | `docs/UltraCPP-v0.1.0-spec-zh-CN.md` |
| C 端口代码结构 | `src-c/README.md` |
| asm 端口状态 | `src-asm/README.md` |

## Guidelines

- Document your reasoning for design decisions
- Include references to relevant research or prior art
- Keep commits focused and atomic（一个逻辑改动 = 一个 commit）

## Contact

Open an issue to discuss major changes before implementing.
