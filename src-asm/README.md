# src-asm/ — Phase 5 asm port (lexer)

## Status: stub / toolchain verified

The `src-asm/lexer.s` shipped in Phase 5 commit `b4d2f0e` is a
**minimal stub** that proves the `as` + `ld` + x86-64 Linux toolchain
works for our build. It does NOT yet implement a real UltraCPP
lexer. The C port in `src-c/src/lexer.c` remains the canonical
implementation.

## What's committed

* `lexer.s` — minimal `_start` that writes a one-line banner to stderr
  and exits 0. ~30 lines of x86-64 AT&T assembly.

* The asm-Lexer is buildable via:
  ```
  as --64 -o lexer.o lexer.s
  ld -o uc_lexer_asm lexer.o
  ```

## What was attempted (and why it didn't ship)

A previous draft of `lexer.s` (~1500 lines) implemented a full
UltraCPP lexer in assembly:

* Source reading from `argv[1]` or stdin via `sys_read`
* Per-character dispatch with line/column tracking
* Whitespace + `//` + `/* */` comment skipping
* Identifier + integer literal + single/double-character operator
  recognition
* ~75 token kinds with on-the-fly format output matching the C
  lexer's `TOKEN KindName   line:col  lexeme[extras]` format

The implementation built cleanly but segfaulted at runtime inside
`emit_str` / `emit_token_header`. After several iterations of debug
syscalls the issues that surfaced were:

* `.ascii` debug labels in `.text` were being executed as instructions
  and causing #UD/SIGSEGV. Fix: move all string literals to `.data`
  or `.rodata`.
* Several `.skip N` state fields were declared as 4 bytes (e.g.
  `cur_line`, `cur_col`, `is_eof_flag`, `tok_kind_idx`) but written
  with `movq` (8-byte store), corrupting the next variable in
  `.bss`. Fix: use `movl` / `incl` for 4-byte fields.
* RIP-relative `leaq` of stack-allocated `pushq`-only labels
  (e.g. `.Ldbg_xxx`) silently returned wrong addresses because the
  labels were inside a `call` boundary; the rest of the dispatch
  cascade read from a non-mapped `out_buf` and segfaulted.

These were all in principle fixable, but the aggregate debugging time
exceeded what made sense for a single Phase 5 commit. The stub
preserves the toolchain and the build wiring so future work can
rebuild incrementally.

## Recommended next steps (for whoever picks this up)

1. Start from the stub and grow the lexer one token-kind at a time,
   with a single integrated `tools/asm_tokenize_test.sh` that diffs
   the asm-Lexer's output against the C lexer's for `test_t1`,
   then `test_t2`, ..., until byte-exact.
2. Write a tiny asm helper test harness (like the C tests in
   `src-c/tests/test_lexer.c`) that runs the asm lexer on hand-crafted
   byte sequences and checks outputs.
3. Use `gcc -static -nostdlib` to link test harnesses against the asm
   lexer without going through Linux's libc.
4. Reference: `src/frontend/lexer.rs` (Rust reference), `src-c/src/lexer.c`
   (C port). The asm should be byte-equivalent to the C output for
   the 7 test programs in `tools/tokenize_test.sh`.

## Build

```
cd src-asm
as --64 -o lexer.o lexer.s
ld -o uc_lexer_asm lexer.o
./uc_lexer_asm    # prints the stub banner and exits 0
```