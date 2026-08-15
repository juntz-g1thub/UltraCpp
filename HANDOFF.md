<!-- ============================================================
     CURRENT TASK IDENTIFICATION (session-resume aid)
     启动新会话时,先看这里识别当前任务范围。
     ============================================================ -->

# 🔖 当前任务:0.3.1 → 0.3.3 三轮 spec 迭代

**目标**:通过 3 轮 spec 修订(spec 0.3.1 → 0.3.3),让 src-c 编译器补全已声明但未实现的功能,逐步达到 baseline 27/48+。

**3 轮迭代范围**:
- **0.3.1**(S2 lvalue/rvalue)— 修编译期 lvalue/rvalue 概念缺失,m0_42 deref-assign 主要 fix
- **0.3.2**(S4 C-style cast + S5 函数返回类型 + §11.0 builtin 签名表)— 修 m0_42 第三 bug 和 m0_41 链接
- **0.3.3**(S1 一元 op 优先级表 + S3 deref 语义 + mod/unmod/move/clone 函数化)— 修 spec 内部矛盾(S4.1 vs §4.9/§4.10/§7.4/§7.5)

**当前状态**(2026-08-12 / HEAD `6d52770`):
- 0.3.1 / 0.3.2 / 0.3.3 草稿 **全部 commit**(7 个 .dev/drafts/ 文档)
- baseline:**27/48 PASS / 21 FAIL / 56.3%**
- 7 个 commits 跨 0.3.1 + 0.3.2 + 0.3.3 + 修正

**进行中/未来工作**(详细见 `.dev/drafts/0.3.3-unsolved.md`):
- **已识别但未起草的 21 个 spec 缺陷**(S6 FFI body / S7 CJK / S8 deref-assign 残余 / S9 exit code / S10-S26 各种细节)
- **0.3.2 实施**:S4 C-style cast + S5 return type / builtin sig ~ 0.5-1 天
- **0.3.3 实施**(plan A 简化版,无 runtime)~ 1-1.5 天
- **0.3.3+ 修订** = S10/S6/S12/S11/S26 优先级排序

**关联文档**(必读):
1. `HANDOFF.md`(本文件)— 整体状态
2. `.dev/drafts/0.3.1-spec-amendment-s2-lvalue.md` — S2 计划
3. `.dev/drafts/0.3.1-spec-text-changes.md` — 0.3.1 spec
4. `.dev/drafts/0.3.2-spec-modifications.md` — S4+S5 spec
5. `.dev/drafts/0.3.2-implementation-plan.md` — S4+S5 计划
6. `.dev/drafts/0.3.3-spec-text-changes.md` — S1+S3+func spec
7. `.dev/drafts/0.3.3-implementation-plan.md` — 0.3.3 计划(plan A 简化)
8. `.dev/drafts/0.3.3-unsolved.md` — 21 个待起草 spec 缺陷

**新会话应做**:
1. 读本任务标识 + HANDOFF §1 看当前 baseline
2. 选下一步:实施 0.3.2 / 0.3.3 / 修剩余 spec / 其他
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

**Milestone: P0 + P1 complete (27/48 = 56.3% PASS).**

We're validating the current C compiler (`src-c/`) against the new UltraCPP 0.3.0 specification by writing 48 .uc test programs and running them through the compiler. This establishes a baseline before incrementally implementing borrow-check features (M1-M5).

**Expected duration**: 1-2 sessions (test creation + script writing + execution + report).

Current progress (HEAD `02d7170`): M0 P0-1 (const trio m0_22/45/46) + P0-2 (alloc/free/move/unique trio m0_34/35/36) + **P0-3 (++/-- + ternary m0_47/48)** + **P0-4 (void + local/global m0_17/21)** all complete + **P1-2 ✅ done (m0_10 else-if chain)** + **P1-3 ✅ done (for + break/continue m0_13/14/15)** + **P1-4 partial done (m0_31 + m0_29 + m0_19; m0_41/m0_42 deferred to P3-5)** + **P1-1 ✅ done (m0_50 — CJK identifier rejection)**; baseline **27/48 PASS** + **21 FAIL** (m0_35 full e2e exit=0; m0_34/36 parser+IR OK, llc codegen gap `*p = X` deref-assign 是 known issue, P3-5 时回看; m0_47 exit=16 full PASS + m0_48 exit=20 **effective PASS** — runner 误判因 expected-code extraction 默认 0; P0-3 期间 5 codegen fixes (cf51f6f / f1f4214 / 3abe96b) 同时铺平 control flow, m0_09 等 control flow 测试也开始水落石出 PASS; **m0_17 flipped** with commit `c052d2c` (void 函数 return path 修复); m0_21 早已 PASS — P0-4 净增 1 通过; **m0_10 flipped** (commit `09b4df5`) — **不是 codegen bug**, 是 test runner awk 不解析函数调用式 return, 加 trailing `// 255` 注释修复; **m0_04 flipped** (commits `5abb262` codegen i1→i32 zext + `ef3352c` test trailing `// 3`); **m0_13 + m0_14 + m0_15 flipped** (commits `afae9c9` UC_STMT_FOR/BREAK/CONTINUE + loop scope 栈 + `c8492ef` UC_BIN_MOD + UC_EXPR_LITERAL last_expr_type per lit kind; m0_14 was always FAIL (for-loop codegen was a stub until afae9c9; P1-3 codegen change exposed latent literal-type leak, fixed in c8492ef's last_expr_type per lit kind); m0_15 originally 28 → 16 after c8492ef); **m0_31 + m0_29 flipped** (commit `a513226`, UC_UN_ADDR_OF codegen); **m0_19 flipped** (commit `985eade`, .uc trailing `// 208` — factorial(6)=720 但 bash `$?` 只存低 8 bit → 实际 exit 208); **m0_50 flipped** (commit `02d7170`, runner 加 `// expects_compiler_error` marker 支持 + m0_50 .uc 加 marker — 测试目的本就是验证编译器正确拒绝 CJK 标识符(违反 spec §2.4 ASCII 标识符约束)，编译器早已正确输出 `Lexer error at ...: unknown character`，runner 旧逻辑一律把 `compile_failed` 当 FAIL 是误判；新机制下 m0_50 → PASS exit=N/A，src-c 未改); 累计 baseline 自 11→14→18→19→20→23→26→**27**). **21 个 FAIL 详情 + 分类**见 priority doc §8.1. Next: **P3-5 pointer runtime**（m0_41 / m0_42 / m0_30 / m0_34 / m0_36 落在 P3-5 pointer runtime 范畴：call signature + deref-assign LHS + abs_int linker）或 **P2**（数组 / struct / 函数指针 / extern "C" / 多文件 module）。**m0_41 / m0_42 未完成 — deferred to P3-5，不是 P1-4 DONE。**

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

- **Language spec**: `docs/UltraCPP-v0.3.0-spec-zh-CN.md` (current authoritative)
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
| **Language specification** | `docs/UltraCPP-v0.3.0-spec-zh-CN.md` |
| Borrow-check decisions + 0.3.0 outcome | `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` |
| C compiler | `src-c/README.md` |
| Project landing page | `README.md` |
| Agent working rules | `AGENTS.md` |
| **Historical HANDOFF (archived)** | `.dev/_archive/v0.2.0/handoff.md` |
| Bootstrap roadmap (dormant) | `bootstrap/PLAN.md` |

---

*Last updated: 2026-08-12*
*M0 expected duration: 1-2 sessions (test creation + execution + report)*
