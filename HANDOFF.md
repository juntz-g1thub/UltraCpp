# HANDOFF — M0 Baseline Verification

> **Active since**: 2026-08-08
> **Status**: Ready to execute
> **Next session should**: Run M0 baseline verification

## 1. TL;DR

We're validating the current C compiler (`src-c/`) against the new UltraCPP 0.3.0 specification by writing 48 .uc test programs and running them through the compiler. This establishes a baseline before incrementally implementing borrow-check features (M1-M5).

**Expected duration**: 1-2 sessions (test creation + script writing + execution + report).

Current progress: M0 P0-1 (const trio m0_22/45/46) + P0-2 (alloc/free/move/unique trio m0_34/35/36) complete; baseline 14/48 PASS (m0_35 full e2e exit=0; m0_34/36 parser+IR OK, llc codegen gap `*p = X` deref-assign 是 known issue, P3-5 时回看). Next: M0 P0-3 (++/-- + ternary). See `.dev/drafts/0.4.0-m0-priority.md` §8 for live progress.

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
- **Documentation**: README (design journey), AGENTS.md, test-outline.md, 0.4.0-test-milestones.md all in sync
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

See **`.dev/plans/0.4.0-test-milestones.md` §3.0 (M0 section)** for:
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

Script templates are in `.dev/plans/0.4.0-test-milestones.md` §4.

### Output

Generate **`docs/m0-baseline-report.md`** containing:
- Per-test pass/fail summary (table format)
- Performance baseline CSV
- Known gaps (tests that fail and why)
- Link to `test/bench/baseline.csv`

## 5. Important Files (Read These First)

| File | Why |
|---|---|
| `.dev/plans/0.4.0-test-milestones.md` | M0 test items + implementation tasks + scripts |
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

2. **Don't write tests for features `src-c/` doesn't have yet**. The test list in 0.4.0-test-milestones.md was specifically designed to be runnable on CURRENT src-c/. If a test fails to compile, that IS the baseline — record it, don't try to fix the compiler.

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
bash test/e2e/run_baseline.sh                  # new M0 script
bash test/e2e/bench.sh > test/bench/baseline.csv

# Commit
git status
git add ...
git commit -m "..."
```

## 9. Decision Records (Why we are here)

- **2026-08-08**: Decided incremental borrow-check implementation via M0-M5 milestones (per `.dev/plans/0.4.0-test-milestones.md`)
- **2026-08-08**: Test-first methodology — define tests before implementing (per user directive)
- **2026-08-08**: M0 scope corrections: typedef and compound-assign moved to M1; alloc/free/unique/move marked PARSE-ONLY (audit findings)
- **2026-08-08**: Old HANDOFF.md archived; this NEW HANDOFF created for M0 execution

## 10. Reference Documents

| Need | Document |
|---|---|
| **Current implementation plan** | `.dev/plans/0.4.0-test-milestones.md` |
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
