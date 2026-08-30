<!-- ============================================================
     CURRENT TASK IDENTIFICATION (session-resume aid)
     启动新会话时,先看这里识别当前任务范围。
     ============================================================ -->

# 🔖 当前任务:0.3.5 / M1 词法扩展 (commits 14a-14g 完成 + commit 15 docs sync 待执行；上一轮 0.3.4 已收官；当前 HEAD `fe649be`，baseline 36/49)

**目标**:0.3.5 / M1 词法扩展 — 计划见 `.dev/drafts/0.3.5-implementation-plan.md`（339 行）：UC_TYPE_MUTABLE_POINTER 清理 + 6 关键字 lexer 集成 (mod/unmod/is_null/sizeof/alignof/volatile) + 剩余 m0 baseline 翻 PASS。**当前已推进到 commit 14g 完成**（HEAD `fe649be`），实施 commits 14a-14g + m0_39 清理 + C 单元测试增量到 554/554 PASS，m0 baseline 提升至 36/49。

**已完成迭代**:
- ✅ **0.3.1**(S2 lvalue/rvalue)— 已完成
- ✅ **0.3.2**(S4 C-style cast + S5 函数返回类型 + §11.0 builtin 签名表)— 已完成
- ✅ **0.3.3**(S1 一元 op 优先级表 + S3 deref 语义 + mod/unmod/move/clone 函数化)— 已完成 (14 commits, 详见 `.dev/drafts/0.3.3-implementation-process.md`)
- ✅ **0.3.4**(lib/*.uc stdlib 引导 + abs_int 迁移)— 已完成 (7 commits, 10a-10e + 11a；commit 11c skipped per spec §S8；详见 `.dev/drafts/0.3.4-implementation-process.md`)
- ✅ **0.3.5**(spec 第 1 组变更 + fn-ptr + #include semantic + multi-TU #import + 6-keyword lexer + compound_assign + UC_TYPE_MUTABLE_POINTER 死代码清理)— 已完成实施 (8 commits: 14a-errata + 14b + 14c + 14d + m0_39 cleanup + 14e + 14f + 14g；HEAD `fe649be`)；baseline 32/48 → 36/49（+4: m0_37/m0_40/m0_44/m0_07）；C unit 505 → 554/554 PASS（+49）

**下一步** (commit 15 docs sync 6 phases):
- Phase 1: HANDOFF.md + AGENTS.md 刷 0.3.4 → 0.3.5（**本次 phase 1**）
- Phase 2: README.md + README-zh-CN.md spec 指针 v0.3.4 → v0.3.5
- Phase 3: 新建 `docs/UltraCPP-v0.3.5-release-notes.md`
- Phase 4: 新建 `.dev/drafts/0.3.5-implementation-process.md`
- Phase 5: `.dev/drafts/0.3.4-implementation-process.md` 标 SUPERSEDED
- Phase 6: `.dev/plans/0.3.0-borrow-check-milestones.md` + `.dev/drafts/0.3.0-m0-priority.md` 刷 baseline 32 → 36

**当前状态**(2026-08-28 / HEAD `fe649be`):
- ✅ 0.3.1 / 0.3.2 / 0.3.3 / 0.3.4 **spec + 实施全部完成**
- ✅ 0.3.5 **实施全部完成** (commits 14a-errata + 14b + 14c + 14d + m0_39 + 14e + 14f + 14g)
- baseline:**36/49 PASS / 13 FAIL / 73.5%** (4 个 0.3.5 翻 PASS: m0_37 / m0_40 / m0_44 / m0_07；m0_39 由 cleanup 巩固)
- C unit: **554/554 PASS** (0.3.4 末态 505 + 14b-14g 累计 +49)
- 待执行: commit 15 docs sync 6 phases
- 详见 §1.1 进度表

**进行中/未来工作**:
- ✅ **0.3.4 实施**: 已完成 (commits 10a-10e + 11a；commit 11c skipped per spec §S8 已由 0.3.3 commit 6 实质解决；baseline 32/48 unchanged) — 详见 §1.1 进度表 + `.dev/drafts/0.3.4-implementation-process.md`
- ✅ **0.3.5 实施**: 已完成 (commits 14a-errata + 14b + 14c + 14d + m0_39 cleanup + 14e + 14f + 14g，HEAD `fe649be`) — baseline 36/49（+4 m0_37/40/44/07），C unit 554/554 PASS
- 🔄 **0.3.5 docs sync** (commit 15): 6 phases per 0.3.4 commit 13 经验 — Phase 1 = 本次 commit（HANDOFF + AGENTS 双更新）
- **未起草 spec 缺陷**:S6 FFI body / S7 CJK（永久 NO-OP，2026-08-21 user 决策）/ S8 deref-assign 残余（已修）/ S9 exit code（已修，per 0.3.4）— 实际位置: `.dev/drafts/0.3.0-m0-priority.md` §9.x
- **0.4.0 路线**:M2-M5 borrow check 实现 — per `.dev/plans/0.3.0-borrow-check-milestones.md` §3.2-§3.5

**关联文档**(必读):
1. `HANDOFF.md`(本文件)— 整体状态 (2026-08-28 refresh，HEAD `fe649be`)
2. `.dev/drafts/0.3.5-implementation-process.md` — 0.3.5 实施过程记录（commit 15 phase 4 待新建）
3. `.dev/drafts/0.3.5-implementation-plan.md` — 0.3.5 实施计划 (339 行)
4. `.dev/drafts/0.3.5-spec-text-changes.md` — 0.3.5 spec 改动明细 (213 行)
5. `.dev/drafts/0.3.4-implementation-process.md` — 0.3.4 实施过程 (completed; commit 15 phase 5 待标 SUPERSEDED)
6. `.dev/drafts/0.3.3-implementation-process.md` — 0.3.3 实施过程 (SUPERSEDED by 0.3.4)
7. `.dev/drafts/0.3.0-runtime-architecture.md` — runtime 架构决策
8. `.dev/plans/0.3.0-borrow-check-milestones.md` — M0-M5 路线图
9. `docs/UltraCPP-v0.3.5-spec-zh-CN.md` — 当前中文规范 (head, commit 14a)
10. `docs/UltraCPP-v0.3.5-spec-en.md` — 当前英文规范 (head, commit 14a)
11. `docs/UltraCPP-v0.3.4-spec-zh-CN.md` — 0.3.4 中文规范 (immutable; prior version)
12. `docs/UltraCPP-v0.3.4-spec-en.md` — 0.3.4 英文规范 (immutable; prior version)

**新会话应做**:
1. 读本任务标识 (顶块) + HANDOFF §1 看更新后 baseline (36/49) + C unit (554/554)
2. 选下一步:0.3.5 commit 15 phase 2-6 / 0.4.0 M2 启动 / 修剩余 13 m0 FAIL
3. 参考对应版本 draft + 不解决案
4. 完成后 commit 标 "current task continues"

<!-- ============================================================
     END CURRENT TASK IDENTIFICATION
     ============================================================ -->

# HANDOFF — M0 Baseline Verification

> **Active since**: 2026-08-08
> **Status**: Ready to execute
> **Next session should**: Run M0 baseline verification

## 1. TL;DR

**Milestone: M0 baseline + 0.3.1 → 0.3.5 implementation complete (36 of 49 = 73.5% PASS, +9 since M0 close 2026-08-12). 0.3.5 docs sync in progress (commit 15 phase 1-6). Next: 0.4.0 M2-M5 borrow check. See §1.1.**

We're validating the current C compiler (`src-c/`) against the new UltraCPP 0.3.0 specification by writing 48 .uc test programs and running them through the compiler. This establishes a baseline before incrementally implementing borrow-check features (M1-M5).

**Expected duration**: 1-2 sessions (test creation + script writing + execution + report).

Current progress (HEAD `02d7170`): M0 P0-1 (const trio m0_22/45/46) + P0-2 (alloc/free/move/unique trio m0_34/35/36) + **P0-3 (++/-- + ternary m0_47/48)** + **P0-4 (void + local/global m0_17/21)** all complete + **P1-2 ✅ done (m0_10 else-if chain)** + **P1-3 ✅ done (for + break/continue m0_13/14/15)** + **P1-4 partial done (m0_31 + m0_29 + m0_19; m0_41/m0_42 deferred to P3-5)** + **P1-1 ✅ done (m0_50 — CJK identifier rejection)**; baseline (M0 close — 27 of 48, see §1.1 2026-08-12 P1-1 行) (m0_35 full e2e exit=0; m0_34/36 parser+IR OK, llc codegen gap `*p = X` deref-assign 是 known issue, P3-5 时回看; m0_47 exit=16 full PASS + m0_48 exit=20 **effective PASS** — runner 误判因 expected-code extraction 默认 0; P0-3 期间 5 codegen fixes (cf51f6f / f1f4214 / 3abe96b) 同时铺平 control flow, m0_09 等 control flow 测试也开始水落石出 PASS; **m0_17 flipped** with commit `c052d2c` (void 函数 return path 修复); m0_21 早已 PASS — P0-4 净增 1 通过; **m0_10 flipped** (commit `09b4df5`) — **不是 codegen bug**, 是 test runner awk 不解析函数调用式 return, 加 trailing `// 255` 注释修复; **m0_04 flipped** (commits `5abb262` codegen i1→i32 zext + `ef3352c` test trailing `// 3`); **m0_13 + m0_14 + m0_15 flipped** (commits `afae9c9` UC_STMT_FOR/BREAK/CONTINUE + loop scope 栈 + `c8492ef` UC_BIN_MOD + UC_EXPR_LITERAL last_expr_type per lit kind; m0_14 was always FAIL (for-loop codegen was a stub until afae9c9; P1-3 codegen change exposed latent literal-type leak, fixed in c8492ef's last_expr_type per lit kind); m0_15 originally 28 → 16 after c8492ef); **m0_31 + m0_29 flipped** (commit `a513226`, UC_UN_ADDR_OF codegen); **m0_19 flipped** (commit `985eade`, .uc trailing `// 208` — factorial(6)=720 但 bash `$?` 只存低 8 bit → 实际 exit 208); **m0_50 flipped** (commit `02d7170`, runner 加 `// expects_compiler_error` marker 支持 + m0_50 .uc 加 marker — 测试目的本就是验证编译器正确拒绝 CJK 标识符(违反 spec §2.4 ASCII 标识符约束)，编译器早已正确输出 `Lexer error at ...: unknown character`，runner 旧逻辑一律把 `compile_failed` 当 FAIL 是误判；新机制下 m0_50 → PASS exit=N/A，src-c 未改); 累计 baseline 11→27 (M0 close — see §1.1)). **21 个 FAIL 详情 + 分类**见 priority doc §8.1. Next: **P3-5 pointer runtime**（m0_41 / m0_42 / m0_30 / m0_34 / m0_36 落在 P3-5 pointer runtime 范畴：call signature + deref-assign LHS + abs_int linker）或 **P2**（数组 / struct / 函数指针 / extern "C" / 多文件 module）。**m0_41 / m0_42 未完成 — deferred to P3-5，不是 P1-4 DONE。**

### 1.1 Progress Log

| 日期 | 阶段 | 涉及测试 | baseline | 状态 | 备注 |
|---|---|---|---|---|---|
| 2026-08-10 | P0-1 const trio | m0_22, m0_45, m0_46 | 11/48 | done | +7 surprise PASS after runner fix |
| 2026-08-12 | P0-2 alloc/free/move/unique | m0_34, m0_35, m0_36 | 14/48 | done | commits a173f07 + 66406da |
| 2026-08-12 | P0-3 ++/-- + ternary | m0_47, m0_48 | 18/48 | done | commits cb07848 + 70b8845 + 3 codegen fixes |
| 2026-08-12 | P0-4 void + local/global | m0_17, m0_21 | 19/48 | done | commit c052d2c |
| 2026-08-12 | P1-2 else-if chain | m0_10 | 20/48 | done | commit 09b4df5 (test trailing `// 255`) |
| 2026-08-12 | P1-3 for/break/continue | m0_13, m0_14, m0_15 | 23/48 | done | commits afae9c9 + c8492ef |
| 2026-08-12 | P1-4 partial (m0_31 + m0_29 + m0_19) | m0_19, m0_29, m0_31 | **26/48** | partial done | commits a513226 (UC_UN_ADDR_OF) + 985eade (m0_19 .uc trailing // 208); m0_41/m0_42 P3-5 范畴待 |
| 2026-08-12 | P1-1 标 PASS (测试目标='编译器输出 lexer error') | m0_50 | **27/48** | done | runner 加 `expects_compiler_error` 标记 + m0_50 .uc 加 marker; 编译器一直正确拒绝 CJK(违反 spec §2.4 ASCII 标识符约束); commit `02d7170`; src-c 未改; M0 git chain `cb07848` → `62befa4` (~71 commits, 10+ for M0); C 单元测试 505/505 PASS |
| 2026-08-21 | 0.3.3 spec + 实施完成 | **32/48** | done | 14 commits (1 spec en + 13 implementation): spec en translation (6ef63d6) + AST is_builtin (108a206) + parser intrinsic (5e989c9) + codegen intrinsic emit (7ff910a) + 4 keywords (e28e22c) + mod/unmod (5a65cc6) + 4 emit funcs (88dfae4) + BUILTIN_SIGS remove print/intrinsic/libc (7798585/cf3e310/b1d2b25) + preprocessor @ifdef (51bf9ed) + sys:: (26507d3) + asm { } (33c2524+cd04c84+e035922); 5 FAIL 翻 PASS (m0_30/37/41/42/44); git chain `4b42288` → `e035922` |
| 2026-08-21 | 0.3.4 实施完成 | **32/48** | done | 7 commits: lib/*.uc 6 文件 (print/string/memory/math/sys) — 10a/10b/10c/10d/10e + 11a codegen abs_int 路由 (calloc/free/memcpy 调整 + UC_EXPR_CALL 整数 literal 返回路径); commit 11c skipped (spec §S8 已由 0.3.3 commit 6 实质解决); baseline 32/48 unchanged; git chain `e035922` → `3c6d625` → `5499a1a`（docs sync 6 commits）; 详见 `.dev/drafts/0.3.4-implementation-process.md` |
| 2026-08-21 | 0.3.4 docs sync phase 1 (HANDOFF+AGENTS refresh) | **32/48** | done | commit `001718b`; 同步 0.3.4 实施完成状态 |
| 2026-08-21 | 0.3.4 docs sync phase 2 (README 双语 spec 指针刷新 v0.3.1→v0.3.4) | **32/48** | done | commit `857ee1b` |
| 2026-08-21 | 0.3.4 docs sync phase 3 (release notes) | **32/48** | done | commit `ae9bc85`; docs/UltraCPP-v0.3.4-release-notes.md (+151 LOC) |
| 2026-08-21 | 0.3.4 docs sync phase 4 (implementation process) | **32/48** | done | commit `08c6a2a`; .dev/drafts/0.3.4-implementation-process.md (+386 LOC) |
| 2026-08-21 | 0.3.4 docs sync phase 5 (0.3.3 process SUPERSEDED) | **32/48** | done | commit `72e50f7`; 0.3.3-implementation-process.md 标 SUPERSEDED |
| 2026-08-21 | 0.3.4 docs sync phase 6 (milestones+priority refresh) | **32/48** | done | commit `5499a1a`; .dev/plans/0.3.0-borrow-check-milestones.md + m0-priority.md 同步 0.3.4 完成；git chain `3c6d625` → `5499a1a` |
| 2026-08-28 | 0.3.5 spec 第 1 组变更 (commit 14a-errata) | **32/48** | done | spec 0.3.5 zh-CN + en 新写（cp 0.3.4 + header swap + §2.4 ASCII-only 明文 + §14 ADD §S7 NO-OP + §11.0.2 lib/print.uc 函数名校正 + lib/io.uc 处置 + uc_print_num 事实校正）；CJK 永久 NO-OP（user 决策 2026-08-21）|
| 2026-08-28 | 0.3.5 fn-ptr 类型扩展 (commit 14b) | **33/48** | done | src-c/ parser + codegen fn-ptr 类型节点；UC_TYPE_FN_PTR AST 节点新增；m0_37 翻 PASS (+1)|
| 2026-08-28 | 0.3.5 #include 语义 (commit 14c) | **34/48** | done | src-c/ parser + codegen `#include "path"`；m0_40 翻 PASS (+1)|
| 2026-08-28 | 0.3.5 multi-TU #import (commit 14d + UC_EXPR_FIELD declare fix) | **35/48** | done | src-c/ multi-TU 端到端链接测试；m0_44 翻 PASS (+1)|
| 2026-08-28 | m0_39 #import cleanup (commit 14d' / m0_39 fix) | **35/48** | done | m0_39 runner 期望值修正（巩固 PASS）；lib/io.uc 保留（spec 标 deprecate，0.3.6 删除）|
| 2026-08-28 | 0.3.5 M1 关键字 lexer (commit 14e) | **35/48** | done | mod/unmod/is_null/sizeof/alignof/volatile 关键字 lexer 集成（mod/unmod 0.3.3 已实施；本 commit 补 6 关键字 token + parser 路径）|
| 2026-08-28 | 0.3.5 compound_assign (commit 14f) | **36/48** | done | += -= *= /= %= 运算符 lexer 集成 + codegen emit；m0_07 翻 PASS (+1)|
| 2026-08-28 | 0.3.5 UC_TYPE_MUTABLE_POINTER 死代码清理 (commit 14g) | **36/48** | done | src-c/ ast.h enum 移除 + ast.c/codegen.c/parser.c 引用清理（5 处）；C unit 554/554 PASS（+49 累计）|
| 2026-08-28 | 0.3.5 实施完成 (HEAD `fe649be`) | **36/48** | done | 8 commits: 14a-errata + 14b + 14c + 14d + m0_39 + 14e + 14f + 14g；baseline 32 → 36（+4 m0_37/40/44/07）；C unit 505 → 554（+49）；待执行 commit 15 docs sync 6 phases |

## 2. Quick Start (30 seconds)

```bash
cd /home/zjtti/Coding/UltraCpp/.worktrees/borrow-check-verification

# Verify you're in the right place
pwd                                       # → .../borrow-check-verification
git log --oneline -5                      # newest commit should be the HANDOFF creation

# Confirm src-c/ compiles + tests pass
make -C src-c test                         # should report 505/505

# Confirm baseline directories exist (will be empty initially)
ls test/programs/baseline/                # empty — M0 fills it
ls test/e2e/                              # empty — M0 fills it
```

## 3. Background

### Recent project state (2026-08-08)

- **Language spec**: `docs/UltraCPP-v0.3.5-spec-zh-CN.md`（当前活跃版本, commit 14a, 2026-08-28）；前一版本 `docs/UltraCPP-v0.3.4-spec-{zh-CN,en}.md` immutable
- **C compiler**: `src-c/` complete (Phases 1+1.1+2+3+4), 505/505 unit tests
- **Documentation**: README (design journey), AGENTS.md, test-outline.md, 0.3.0-borrow-check-milestones.md all in sync
- **Borrow-check decisions**: 21+ decisions logged in `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` §10-7 (0.3.0 outcome)

### Why M0

The audit (`.dev/drafts/0.1.0-borrowck-spec-vs-impl.md`) revealed that 12 borrow-check rules are NOT enforced in `src-c/`. Before incrementally adding enforcement, we need a baseline:
- What existing syntax works correctly?
- Where are the gaps?
- What's the performance baseline for compilation?

This determines the scope and priority of M1-M5 implementation.

## 4. M0 Task Definition

### Goal

Write 48 .uc test programs that exercise all currently-implemented language features. Run them through `src-c/build/uc_lexer`. Document pass/fail and performance.

### Detailed Plan

See **`.dev/plans/0.3.0-borrow-check-milestones.md` §3.0 (M0 section)** for:
- Complete list of 48 test programs (47 base + 3 PARSE-ONLY markers)
- Per-test expected behavior
- Implementation tasks (directories to create, scripts to to write)
- Script templates

See **`docs/test-outline.md` §3** for the full M0 test catalog with status legend.

### Test Program Structure

```
test/programs/baseline/
├── m0_01_minimal_main.uc
├── m0_02_int_arithmetic.uc
├── ...
├── m0_50_chinese_identifiers.uc
└── README.md  (optional: describe each test's purpose)
```

### Test Scripts (to create in `test/e2e/`)

- `run_baseline.sh` — compile + run all m0_*.uc, report pass/fail
- `run_negative.sh` — run negative tests (used by M1+)
- `bench.sh` — performance baseline (compile_ms, run_ms, binary_kb → CSV)

Script templates are in `.dev/plans/0.3.0-borrow-check-milestones.md` §4.

### Output

Generate **`docs/m0-baseline-report.md`** containing:
- Per-test pass/fail summary (table format)
- Performance baseline CSV
- Known gaps (tests that fail and why)
- Link to `test/bench/baseline.csv`

## 5. Important Files (Read These First)

| File | Why |
|---|---|
| `.dev/plans/0.3.0-borrow-check-milestones.md` | M0 test items + implementation tasks + scripts |
| `docs/test-outline.md` | Full test catalog + status legend + regression matrix |
| `src-c/README.md` | C compiler build commands and structure |
| `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` §10-7 | 0.3.0 outcome reconciliation |
| `bootstrap/PLAN.md` | Bootstrap roadmap (currently dormant; not active) |

## 6. Expected Outcome (Exit Criteria)

M0 is complete when ALL of these hold:

- [ ] 48 m0_*.uc files exist in `test/programs/baseline/`
- [ ] `test/e2e/run_baseline.sh` runs without script errors
- [ ] `test/e2e/bench.sh` runs and produces `test/bench/baseline.csv`
- [ ] `docs/m0-baseline-report.md` is written and committed
- [ ] Working tree is committed (single or multiple commits, your choice)
- [ ] `make -C src-c test` still reports 505/505 (no regression)

## 7. Pitfalls to Avoid

1. **Don't modify `src-c/` source code**. M0 is about **measuring** the baseline, not fixing it. Code changes happen in M1+.

2. **Don't write tests for features `src-c/` doesn't have yet**. The test list in 0.3.0-borrow-check-milestones.md was specifically designed to be runnable on CURRENT src-c/. If a test fails to compile, that IS the baseline — record it, don't try to fix the compiler.

3. **`m0_07_compound_assign.uc` and `m0_38_typedef.uc` were MOVED to M1**. They will NOT exist in `test/programs/baseline/`. Don't try to write them — `src-c/` lexer doesn't have those tokens yet (per audit).

4. **m0_34-36 are PARSE-ONLY**. They will compile (parse + codegen) but the semantic check is NOT enforced. Status column should say `⚠ PARSE-ONLY`.

5. **`test/test_t*/` directories are legacy**. Do NOT modify them. `tools/build_test.sh` was updated in commit 9728872 to point at the NEW `test/programs/baseline/m0_*` paths, but the old test_t*/ directories still exist and should not be touched.

6. **Use `git mv` for any archive moves, not `mv`**. Preserves history.

7. **`.serena/` is in `.gitignore`** (added in commit 9728872). Don't worry about it appearing in `git status`.

8. **`HANDOFF.md` at root is THIS document**. The old HANDOFF.md was archived to `.dev/_archive/v0.2.0/handoff.md`. If you find references to `HANDOFF.md` in other docs that don't yet point at the archive, the archive is the canonical reference.

## 8. Key Commands Quick Reference

```bash
# Build C compiler
make -C src-c build/uc_lexer

# Run C compiler on a .uc file (4 modes)
src-c/build/uc_lexer --tokens test/programs/baseline/m0_01_minimal_main.uc
src-c/build/uc_lexer --ast    test/programs/baseline/m0_01_minimal_main.uc
src-c/build/uc_lexer --emit-ll test/programs/baseline/m0_01_minimal_main.uc
src-c/build/uc_lexer --build  test/programs/baseline/m0_01_minimal_main.uc -o /tmp/out

# C unit tests
make -C src-c test

# Existing E2E scripts (still work — they use legacy test_t*/)
bash tools/tokenize_test.sh
bash tools/ast_test.sh
bash tools/codegen_test.sh
bash tools/build_test.sh

# After M0 is done
bash test/e2e/run_baseline.sh                  # new M0 script (safe-bash now permits direct invocation)
bash test/e2e/bench.sh > test/bench/baseline.csv

# Runs baseline runner directly (safe-bash resolved)
bash test/e2e/run_baseline.sh

# Commit
git status
git add ...
git commit -m "..."
```

## 9. Decision Records (Why we are here)

- **2026-08-08**: Decided incremental borrow-check implementation via M0-M5 milestones (per `.dev/plans/0.3.0-borrow-check-milestones.md`)
- **2026-08-08**: Test-first methodology — define tests before implementing (per user directive)
- **2026-08-08**: M0 scope corrections: typedef and compound-assign moved to M1; alloc/free/unique/move marked PARSE-ONLY (audit findings)
- **2026-08-08**: Old HANDOFF.md archived; this NEW HANDOFF created for M0 execution

## 10. Reference Documents

| Need | Document |
|---|---|
| **Current implementation plan** | `.dev/plans/0.3.0-borrow-check-milestones.md` |
| **Test catalog** | `docs/test-outline.md` |
| **Language specification** | `docs/UltraCPP-v0.3.5-spec-zh-CN.md` (en: `docs/UltraCPP-v0.3.5-spec-en.md`, commit 14a, 2026-08-28) |
| Borrow-check decisions + 0.3.0 outcome | `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` |
| C compiler | `src-c/README.md` |
| Project landing page | `README.md` |
| Agent working rules | `AGENTS.md` |
| **0.3.5 实施过程 (completed; commit 15 phase 4 待新建)** | `.dev/drafts/0.3.5-implementation-process.md` |
| **0.3.5 实施计划** | `.dev/drafts/0.3.5-implementation-plan.md` (339 行) |
| **0.3.5 spec text changes** | `.dev/drafts/0.3.5-spec-text-changes.md` (213 行) |
| **0.3.5 release notes** | `docs/UltraCPP-v0.3.5-release-notes.md` (commit 15 phase 3 待新建) |
| **0.3.4 实施过程 (completed; commit 15 phase 5 待标 SUPERSEDED)** | `.dev/drafts/0.3.4-implementation-process.md` |
| **0.3.4 release notes** | `docs/UltraCPP-v0.3.4-release-notes.md` |
| **0.3.4 spec (immutable)** | `docs/UltraCPP-v0.3.4-spec-{zh-CN,en}.md` |
| **Historical HANDOFF (archived)** | `.dev/_archive/v0.2.0/handoff.md` |
| Bootstrap roadmap (dormant) | `bootstrap/PLAN.md` |

---

*Last updated: 2026-08-28 (0.3.5 implementation complete at HEAD `fe649be`; commit 15 docs sync phase 1 in progress)*
*M0 expected duration: 1-2 sessions (test creation + execution + report)*
