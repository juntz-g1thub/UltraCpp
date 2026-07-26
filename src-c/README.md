# UltraCPP C Port (Phase 1: Lexer)

C99 rewrite of the UltraCPP compiler's lexer, faithful to the Rust
implementation in `src/frontend/lexer.rs`.

## Why

The project is migrating from Rust → C → x86-64 asm → self-hosted
UltraCPP. The C port is the production implementation that all later
phases depend on.

See [`.sisyphus/plans/UltraCPP-v0.2.0-c-asm-bootstrap-zh-CN.md`](../.sisyphus/plans/UltraCPP-v0.2.0-c-asm-bootstrap-zh-CN.md)
for the full migration plan.

## Build

```bash
cd src-c
make            # build bin/uc_lexer
make test       # run unit tests
make clean
```

Requires a C99 compiler (gcc or clang). No external dependencies.

## What this build does

Only the **lexer** is implemented in C99. Other phases (parser, semantic
analysis, codegen, CLI for IR emission) still live in the Rust reference
implementation.

```
$ ./build/uc_lexer test/test_hello_world/main.upp
TOKEN PpImport        1:1    #import
TOKEN String          1:9    "lib/io"
TOKEN Ident           3:1    int
TOKEN Ident           3:5    main
TOKEN LParen          3:9    (
...
```

## Layout

```
src-c/
├── Makefile           GNU Make build
├── include/
│   ├── uc_version.h   version constants
│   ├── uc_error.h     error reporting types
│   ├── uc_token.h     token kinds + token struct
│   └── uc_lexer.h     lexer interface
├── src/
│   ├── error.c
│   ├── token.c        token operations + keyword table
│   ├── lexer.c        lexer (port of lexer.rs)
│   └── main.c         CLI entry, --dump-tokens
└── tests/
    └── test_lexer.c   smoke tests
```

## C interface (public API)

```c
#include "uc_lexer.h"

UCError err;
uc_error_init(&err);

UCLexer lex;
uc_lexer_init(&lex, source, source_len, "input.uc", &err);

for (;;) {
    UCToken tok = uc_lexer_next(&lex);
    if (tok.kind == UC_TOK_EOF) {
        uc_token_free(&tok);
        break;
    }
    /* consume tok.lexeme (NUL-terminated), tok.as.int_val, etc. */
    uc_token_free(&tok);
}
```

`uc_token_free` releases the `lexeme` string. For `UC_TOK_STRING` tokens
it also releases `as.string_val`. All strings are heap-allocated and owned
by the token.

## Verified equivalence

The lexer is designed for byte-for-byte equivalence with the Rust
reference. **Known divergences** (preserved from Rust source, documented
in `lexer.c`):

- Compound-assignment operators (`+=`, `-=`, `*=`, `/=`, `%=`, `&=`,
  `|=`, `^=`, `<<=`, `>>=`) are NOT produced; the lexer returns the bare
  operator. The Rust parser also doesn't consume them, so behavior is
  unchanged at the moment.

End-to-end comparison against the Rust reference is partial — the Rust
compiler does not yet expose `--dump-tokens`. The unit tests
(`tests/test_lexer.c`) cover all token kinds; `tools/tokenize_test.sh`
runs the C lexer on every program under `test/` and reports line counts.

## Next steps

- Phase 2: AST + parser (`.h` interfaces are reserved under `include/`)
- Phase 3: LLVM IR code generator
- Phase 4: CLI for end-to-end compilation (matching the current
  `./target/debug/ultracpp` interface)
- Phase 5: Re-implement the lexer in x86-64 AT&T asm
- Phase 6: Re-implement parser + codegen in UltraCPP itself
- Phase 7: Self-hosting — UltraCPP compiler compiles itself