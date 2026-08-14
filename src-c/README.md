# UltraCPP C Port

C99 rewrite of the entire UltraCPP compiler (lexer + parser + AST + codegen + CLI).
Faithful to the Rust reference in `src/`, producing byte-identical LLVM IR
for the existing test programs.

> **Status (2026-08-08)**: Phase 1 + 1.1 + 2 + 3 + 4 complete. See `.dev/_archive/v0.2.0/handoff.md` for the historical project-state snapshot.
> The current active implementation direction is borrow-check (M0-M5) per `.dev/plans/0.3.0-borrow-check-milestones.md`.
>
> The C port is the **host compiler** for the bootstrap: the new UltraCPP
> compiler written in UltraCPP itself (in `src-uc/`) is compiled by this
> C port and must produce LLVM IR byte-equal to the reference baseline in
> `bootstrap/baseline/`.

## Why

The project is migrating from Rust → C → **UltraCPP** (self-hosted). The C
port replaces the Rust implementation as the production target. The
asm-Lexer plan (Phase 5) was deferred because the C port can already
serve as the bootstrap host.

See:
- [`.dev/_archive/v0.2.0/handoff.md`](../.dev/_archive/v0.2.0/handoff.md) — historical project-state snapshot (2026-08-06)
- [`.dev/plans/0.3.0-borrow-check-milestones.md`](../.dev/plans/0.3.0-borrow-check-milestones.md) — current active implementation plan (borrow-check M0-M5)
- [`bootstrap/PLAN.md`](../bootstrap/PLAN.md) — bootstrap roadmap (currently dormant)
- [`docs/UltraCPP-v0.3.0-spec-zh-CN.md`](../docs/UltraCPP-v0.3.0-spec-zh-CN.md) — current authoritative language spec

## Build

```bash
cd src-c
make            # build bin/uc_lexer
make test       # run unit tests (4 binaries, 505/505 assertions)
make clean
```

Requires a C99 compiler (gcc or clang). No external dependencies.

## What this build does

The full C port — **not just the lexer** anymore.  The binary supports:

| Mode | Flag | Output |
|---|---|---|
| Tokenize | `--tokens` (default) | one line per token (used by `tools/tokenize_test.sh`) |
| Dump AST | `--ast` | AST tree (used by `tools/ast_test.sh`) |
| Emit LLVM IR | `--emit-ll` / `-S` | LLVM IR text (used by `tools/codegen_test.sh`) |
| End-to-end build | `--build` | `.ll` → `llc` → `.s` → `gcc` → native executable (used by `tools/build_test.sh`) |

Example end-to-end:

```
$ src-c/build/uc_lexer --build test/test_t1/main.upp -o /tmp/test_t1
Wrote IR to /tmp/uc_build_main.ll
Running: llc /tmp/uc_build_main.ll -o /tmp/uc_build_main.s
Compiled to /tmp/uc_build_main.s
Running: gcc -no-pie /tmp/uc_build_main.s -o /tmp/test_t1
Built executable /tmp/test_t1
$ /tmp/test_t1; echo "exit: $?"
exit: 0
```

## Layout

```
src-c/
├── Makefile                       GNU Make build
├── include/                       public API
│   ├── uc_version.h
│   ├── uc_error.h
│   ├── uc_token.h
│   ├── uc_lexer.h
│   ├── uc_ast.h
│   ├── uc_parser.h
│   └── uc_codegen.h
├── src/                           implementation
│   ├── error.c
│   ├── token.c
│   ├── lexer.c
│   ├── ast.c
│   ├── parser.c
│   ├── codegen.c
│   └── main.c                     CLI entry
└── tests/                         unit tests (4 binaries)
    ├── test_lexer.c
    ├── test_ast.c
    ├── test_parser.c
    └── test_codegen.c
```

## C interface (public API)

Same conventions across all modules: `UCLexer` / `UCParser` /
`UCCodeGenerator` structs with `_init` / `_reset` / `_free` entry points;
errors propagated via `UCError*` out parameter; never `setjmp`/throw.
See the individual `uc_*.h` files for details.

## Code conventions

- C99 (`-std=c99`)
- `-Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes` (zero warnings)
- `snake_case`; public symbols prefixed `uc_`; enum values `UC_*`
- Public API in `include/uc_*.h`; ownership declared in header comments
- `char*` + `size_t len` for strings; NUL-terminated
- Release flags: `-O2 -g` (or `-O3 -DNDEBUG` for production)

## Verified equivalence

Byte-exact match with the Rust port for the 5 existing test programs:

| Test | C port | Rust port | Notes |
|---|---|---|---|
| `tools/tokenize_test.sh` | 7/7 ✅ | (same) | byte-exact after column normalization |
| `tools/ast_test.sh` | 5/5 ✅ | (same) | byte-exact (after fixing 2 Rust bugs) |
| `tools/codegen_test.sh` | 5/5 ✅ | (same) | byte-exact LLVM IR |
| `tools/build_test.sh` | 3/3 ✅ | (same) | end-to-end exec, exit codes match |

The IR for `test_t1/main.upp` is checked in to `bootstrap/baseline/`
as the bootstrap target — the new UltraCPP compiler (in `src-uc/`) must
produce the same `.ll` byte-for-byte.

## Next steps

- **Borrow-check implementation (M0-M5)**: the current active direction.
  See `.dev/plans/0.3.0-borrow-check-milestones.md` for the milestone breakdown
  and `.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` for the 21+ design decisions.
- The Rust port at `src/` is the reference implementation; expect
  gradual retirement as the UltraCPP port matures.
