<!-- ============================================================
     CURRENT TASK IDENTIFICATION (session-resume aid)
     启动新会话时,先看这里识别当前任务范围。
     ============================================================ -->

# 🔖 当前任务：0.3.6 实施中（commit 16-0 已落地，下一 commit 16-1a）

**目标**：15 commits 完整收 0.3.6（per `.dev/drafts/0.3.6-implementation-plan.md`）

**已落地**：
- `07e6fe8` 0.3.6 plan 写盘
- `86c2159` 0.3.6 spec-text-changes 写盘
- `16-0` docs refresh + 0.3.5 phase 5+6 + .gitignore（本 commit）

**下一 commit**：16-1a `lib/alloc.uc` rewrite uc_alloc/uc_free 用 uc_syscall(SYS_BRK) brk 链
- HEAD: 见 `git log --oneline -1`
- 完整 plan: `.dev/drafts/0.3.6-implementation-plan.md` (15 commits 16-0 ~ 16-12)
- spec 改动依据: `.dev/drafts/0.3.6-spec-text-changes.md` (commit 16-11 实施)
- 基线: m0 **36/49** PASS, C unit **554/554**, lib tests **0/5**
- 末态预期: m0 **46/49**, C unit **~580-600**, lib tests **5/5**

**关键决策**：
- D1-D11 (per 0.3.6 plan §2)：范围 / m0 baseline / lib 改动 / 文档 / m0_38 / 数组 / struct / spec / .gitignore
- S1-S5 (G1 sys:: 解读B)：保留 sys:: 公共 API + lib 内部绕开直接 uc_syscall

**新会话接手步骤**（如需继续 0.3.6 实施）：
1. `cd /home/zjtti/Coding/UltraCpp/.worktrees/borrow-check-verification`
2. `git log --oneline -3` — 确认 HEAD (应为本 16-0 commit)
3. 读本 HANDOFF.md + `.dev/drafts/0.3.6-implementation-plan.md` (15 commits 规划)
4. 读 `.dev/drafts/0.3.6-spec-text-changes.md` (commit 16-11 spec 改动依据)
5. 确认 0.3.6 plan §3 commit 16-1a 描述，dispatch orch-implementer 启动
6. 重复 16-N → commit 验证 → 16-(N+1) 流程直到 16-12 docs sync 收官

<!-- ============================================================
     END CURRENT TASK IDENTIFICATION
     ============================================================ -->

<!-- ============================================================
     END CURRENT TASK IDENTIFICATION
     ============================================================ -->

# HANDOFF — M0 Baseline Verification

> **Active since**: 2026-08-08
> **Status**: Ready to execute
> **Next session should**: Run M0 baseline verification

## 1. TL;DR

**Milestone: M0 baseline + 0.3.1 → 0.3.5 implementation complete (36 of 49 = 73.5% PASS, +9 since M0 close 2026-08-12). 0.3.6 plan + spec-text-changes 写盘 @ `07e6fe8` + `86c2159`; 0.3.6 实施启动 (commit 16-0); 末态预期 m0 46/49 + lib tests 5/5. Next: 0.3.6 commits 16-1a ~ 16-12 (15 commits per plan). See §1.1.**

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
| 2026-08-28 | 0.3.6 plan 写盘 (commit `07e6fe8`) | **36/49** | done | 15 commits 完整规划（16-0 ~ 16-12）；11 D + 5 S 决策；零代码改动 |
| 2026-08-28 | 0.3.6 spec-text-changes 写盘 (commit `86c2159`) | **36/49** | done | §10.4 注 1 段 + §11.0.2 注 2 删 + §14 刷 + §6.5 注 1 + header swap；spec 双语留给 commit 16-11 |
| 2026-08-28 | **0.3.6 启动 (commit 16-0)** | **36/49** | done | 本 commit：HANDOFF + AGENTS 刷 0.3.6 启动 + 0.3.5 phase 5+6 收尾 + .gitignore 加 .pi/ 规则；下一 commit 16-1a lib/alloc.uc rewrite |

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

- **Language spec**: `docs/UltraCPP-v0.3.5-spec-zh-CN.md`（活跃版本, commit 14a, 2026-08-28; 0.3.6 阶段未替换, 0.3.6 spec 双语 commit 16-11 实施才写）；前一版本 `docs/UltraCPP-v0.3.4-spec-{zh-CN,en}.md` immutable
- **C compiler**: `src-c/` complete (Phases 1+1.1+2+3+4), 554/554 unit tests
- **Documentation**: README (design journey), AGENTS.md, test-outline.md, 0.3.0-borrow-check-milestones.md all in sync
- **Borrow-check decisions**: 21+ decisions logged in `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` §10-7 (0.3.0 outcome)
- **0.3.6 实施**: 计划 `.dev/drafts/0.3.6-implementation-plan.md`（15 commits, @ `07e6fe8`）+ spec 改动 `.dev/drafts/0.3.6-spec-text-changes.md`（@ `86c2159`）；启动 commit 16-0 = 本 HANDOFF 刷新

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

# 0.3.6 16-N verification (per .dev/drafts/0.3.6-implementation-plan.md)
git log --oneline -1                                            # HEAD pointer
git diff --stat HEAD~1                                          # 单 commit LOC 检查
git grep -nE 'authoritative|companion|配套译本|翻译滞后' HEAD -- '*.md'  # banned-word 审计 (0 命中)
bash test/e2e/run_baseline.sh                                  # m0 baseline; 16-N 期望净增益
bash test/e2e/run_lib_tests.sh                                 # lib tests; 16-3 / 16-6 激活后运行
make -C src-c test                                              # C unit; 16-N 期望保持 +增量
ls docs/UltraCPP-v0.3.6-spec-{zh-CN,en}.md 2>/dev/null         # 16-11 写盘后检查
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
| **0.3.4 实施过程 (SUPERSEDED by 0.3.6)** | `.dev/drafts/0.3.4-implementation-process.md` |
| **0.3.6 实施计划（current, 15 commits 16-0 ~ 16-12）** | `.dev/drafts/0.3.6-implementation-plan.md` (写盘 @ `07e6fe8`） |
| **0.3.6 spec 改动依据（commit 16-11 实施）** | `.dev/drafts/0.3.6-spec-text-changes.md` (写盘 @ `86c2159`） |
| **0.3.4 release notes** | `docs/UltraCPP-v0.3.4-release-notes.md` |
| **0.3.4 spec (immutable)** | `docs/UltraCPP-v0.3.4-spec-{zh-CN,en}.md` |
| **Historical HANDOFF (archived)** | `.dev/_archive/v0.2.0/handoff.md` |
| Bootstrap roadmap (dormant) | `bootstrap/PLAN.md` |

---

*Last updated: 2026-08-28 (0.3.6 实施启动; commit 16-0 = HANDOFF/AGENTS refresh + 0.3.5 phase 5+6 收尾 + .gitignore 加 .pi/; HEAD 见 `git log --oneline -1`)*
*M0 expected duration: 1-2 sessions (test creation + execution + report)*
