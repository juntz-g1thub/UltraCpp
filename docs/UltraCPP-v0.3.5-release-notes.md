# UltraCPP 0.3.5 Release Notes

> **Version**: 0.3.5
> **Date**: 2026-08-28
> **Status**: Released
> **Previous version**: 0.3.4 (see [UltraCPP v0.3.4 Specification §0.3.4 → 0.3.5 deltas](./UltraCPP-v0.3.4-spec-en.md))

## Highlights

UltraCPP 0.3.5 extends the language with function-pointer types and the
`#include` source-copy preprocessor directive, enables multi-TU
compilation via `#import` with end-to-end `llvm-link` resolution, and
integrates the M1 lexical extension (six new keywords plus compound
assignment operators). The version also performs the long-pending
`UC_TYPE_MUTABLE_POINTER` dead-code cleanup and formally closes spec
gap **§S7 (CJK identifier support)** as **permanently NO-OP**.

Net m0 baseline: **32/48 → 36/49** (+4 m0_37 / m0_40 / m0_44 / m0_07;
m0_39 consolidated by cleanup). C unit: **505 → 554/554 PASS** (+49).

## What's New

### Spec 0.3.5 — Bilingual First Change Group (commit 14a-errata)

New bilingual specifications have been published
([zh-CN](./UltraCPP-v0.3.5-spec-zh-CN.md) /
[en](./UltraCPP-v0.3.5-spec-en.md)) as a copy of the 0.3.4
specification with three first-group amendments:

- **§2.4 Identifier Rules** — explicit ASCII-only statement
  (`letter ::= 'a'..'z' | 'A'..'Z' | '_'`); non-ASCII identifiers
  (including CJK / UTF-8) are permanently out of scope.
- **§14 Spec Gap Index** — new `§S7` entry recording
  "permanently NO-OP" with reference to the 2026-08-21 user decision.
- **§11.0.2 lib/print.uc** — function-name correction
  (`uc_print` → `uc_print_str`); `uc_print_num` clarified as a complete
  implementation (not a stub); `lib/io.uc` documented as a 0.3.3-era
  test fixture, scheduled for deletion in 0.3.6.

The 14a-errata follow-up commit also removes four pre-existing
Rust-style syntax contaminations carried over from the 0.3.4 spec
baseline (`fn raw_syscall(...) → int {` → `int raw_syscall(int, int) {`
and prose `uc_abs(n: int) -> int` → `int uc_abs(int n)`).

### Function-Pointer Type Extension (commit 14b)

The C host compiler now parses and code-generates function-pointer
types in declaration position:

- AST: new `UC_TYPE_FN_PTR` enum entry.
- Codegen: indirect call via `UC_EXPR_CALL` resolved through
  `load` / `call` of a function-typed slot; parameter and return types
  carried through to LLVM IR.
- m0_37 (`function_pointer`) → PASS (+1).

### `#include` Source-Copy Preprocessor (commit 14c)

The parser now handles `#include "path"` directives by inlining the
referenced source at parse time:

- Parser: literal `path` resolution, recursive cycle guard, file
  inclusion stack maintained for diagnostics.
- Codegen: includes do not emit IR; the build system resolves the
  full translation unit before `llvm-link`.
- m0_40 (`module_include`) → PASS (+1).

### Multi-TU `#import` with `llvm-link` (commit 14d)

End-to-end multi-translation-unit compilation is now supported:

- Parser + codegen: `#import "module"` produces a sibling TU
  reference; the build system runs `uc_compiler` on each TU and
  joins them via `llvm-link`.
- Side fix: `UC_EXPR_FIELD` declaration lookup no longer assumes the
  struct was declared in the current TU (`28f8cd6` post-14d cleanup).
- m0_44 (`multiple_files`) → PASS (+1).

### M1 Keyword Lexer Integration (commit 14e)

Six new keywords are recognised at the lexer level and routed through
the parser:

- `shared` / `__thread` / `move_to_thread` / `typedef` (four new M1
  keywords; `mod` / `unmod` were already implemented in 0.3.3 commits
  4–5).
- C unit `test_lexer.c` extended; parser paths added for each new
  keyword.

### Compound Assignment Operators (commit 14f)

Five compound-assignment operators are now lexer-recognised and emit
their standard LLVM IR equivalents:

- `+=` / `-=` / `*=` / `/=` / `%=` (token kinds
  `UC_TOK_OP_PLUS_ASSIGN` … `UC_TOK_OP_MOD_ASSIGN`).
- Codegen: `x += e` emits `load` / `add` / `store` in-place.
- m0_07 (`compound_assign`) → PASS (+1).

### `UC_TYPE_MUTABLE_POINTER` Dead-Code Cleanup (commit 14g)

The obsolete `UC_TYPE_MUTABLE_POINTER` AST enum entry is removed:

- `src-c/include/uc_ast.h` — enum entry deleted.
- `src-c/src/ast.c` / `codegen.c` / `parser.c` — five reference sites
  removed.
- C unit: 554/554 PASS (no regressions; +49 cumulative over the 0.3.5
  implementation chain).

### `m0_39` `#import` Decorative Cleanup (commit 810a517)

m0_39 (`module_import`) is consolidated as PASS by removing a
decorative `#import` line that no longer served a semantic role under
the 0.3.5 `#import` semantics. `lib/io.uc` is preserved for now
(spec marks deprecated; deletion scheduled for 0.3.6).

## Migration from 0.3.4

**No breaking changes** for user code:

- `lib/print.uc::uc_print_str` / `uc_print_num` / `uc_print_float` and
  the other 5 stdlib files continue to resolve at link time.
- `BUILTIN_SIGS[]` is internal; no user-facing impact from the table
  shape.
- The `lib/io.uc` deprecation is documented in 0.3.5 spec §11.0.2;
  removal is scheduled for 0.3.6.

**New capabilities available** (opt-in):

```c
import "lib/print.uc"

int add(int a, int b) { return a + b; }

int (*fp)(int, int) = add;       // function-pointer type (14b)

int main() {
    uc_print_str("add(2,3) = ");
    uc_print_num((*fp)(2, 3));   // indirect call
    uc_print_str("\n");
    return 0;
}
```

## Spec Gap Status

| Gap | Resolution |
|---|---|
| §S6 FFI extern body source policy | ✅ Closed in 0.3.4 |
| **§S7 CJK identifier support** | **❌ Permanently NO-OP** (user decision 2026-08-21; spec §2.4 explicit ASCII-only) |
| §S8 deref-assign type safety | ✅ Closed in 0.3.3 commit 6 |

## Known Limitations / Deferred

The following items remain deferred to 0.3.6 / M2+ or later:

- `lib/io.uc` deletion — scheduled for 0.3.6 (per spec §11.0.2
  deprecation).
- `lib/sync.uc` (mutex / atomic) — required for M5 threading tests.
- `lib/sys/raw.uc` arm64 / riscv64 support — x86_64 only in 0.3.5.
- `lib/print.uc::uc_print_float` complete ftoa — stub in 0.3.5;
  targeted for 0.3.6 / `14h` if reopened.
- `lib_unit` test infrastructure (per 0.3.5 plan §D7) — not landed in
  the 0.3.5 chain; deferred.
- 13 m0 baseline tests still FAIL (P2 array / struct / fn-ptr — fn-ptr
  now PASS; remaining = P3 runtime: m0_02 / m0_03 / m0_05 / m0_06 /
  m0_08 / m0_24 / m0_25 / m0_48 runner-misjudge + P2 array / struct:
  m0_26 / m0_27 / m0_28 / m0_32 / m0_33) — target 0.3.6.

## Baseline Status

- **M0 baseline**: 32/48 → **36/49 PASS / 13 FAIL** (+4:
  m0_37 / m0_40 / m0_44 / m0_07; m0_39 consolidated).
- **C unit tests**: 505 → **554/554 PASS** (+49 across the 0.3.5
  implementation chain 14b → 14g).
- **0.3.4 → 0.3.5 net test impact**: +4 m0 flips, +49 C unit tests,
  no regressions.

## Commit Manifest

8 implementation / spec commits in the 0.3.5 chain (newest first):

```
fe649be cleanup(0.3.5): remove UC_TYPE_MUTABLE_POINTER dead code (M1 范畴)
e773a77 lexer+parser+codegen(0.3.5): add compound assignment operators (m0_07 → PASS)
28f8cd6 test(0.3.5): fix test_codegen field-call IR expectations (post-14d)
0178e82 lexer(0.3.5): add M1 keywords (shared / __thread / move_to_thread / typedef)
810a517 test(0.3.5): remove decorative #import from m0_39 (per 0.3.5 spec Note 2)
eb4dab1 codegen+parser(0.3.5): multi-TU #import + UC_EXPR_FIELD declare fix (m0_44 → PASS)
5ab0bc5 parser(0.3.5): add #include semantic (source-copy preprocessor) (m0_40 → PASS)
1ff72e2 codegen+parser(0.3.5): add function pointer type + indirect call (m0_37 → PASS)
```

Plus 2 spec commits:

```
7c59ce2 docs(spec-0.3.5): errata — remove Rust-style syntax contamination
278f381 spec(0.3.5): bilingual spec 0.3.5 first change group (§2.4 ASCII-only + S7 NO-OP + §11.0.2 corrections)
```

Plus 2 docs prep commits (prior session):

```
4505b3e docs(0.3.5 prep): archive 0.3.4 drafts + ledger errata + 0.3.5 plan drafts
1beaf84 docs(0.3.5 prep): refresh HANDOFF.md + AGENTS.md for new session continuity
```

Plus 6 docs sync commits (commit 15 — phases 1–6):

```
Phase 1 — HANDOFF.md + AGENTS.md refresh
Phase 2 — README.md + README-zh-CN.md spec pointers
Phase 3 — this file (release notes)
Phase 4 — .dev/drafts/0.3.5-implementation-process.md
Phase 5 — .dev/drafts/0.3.4-implementation-process.md SUPERSEDED
Phase 6 — .dev/plans/0.3.0-borrow-check-milestones.md + .dev/drafts/0.3.0-m0-priority.md
```

## See Also

- [UltraCPP v0.3.5 Chinese Specification](./UltraCPP-v0.3.5-spec-zh-CN.md)
- [UltraCPP v0.3.5 English Specification](./UltraCPP-v0.3.5-spec-en.md)
- [0.3.5 Implementation Process](../.dev/drafts/0.3.5-implementation-process.md)
- [0.3.5 Implementation Plan](../.dev/drafts/0.3.5-implementation-plan.md)
- [0.3.5 Spec Text Changes](../.dev/drafts/0.3.5-spec-text-changes.md)
- [HANDOFF.md](../HANDOFF.md) — current session-resume state