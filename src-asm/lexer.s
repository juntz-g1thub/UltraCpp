# src-asm/lexer.s - x86-64 AT&T UltraCPP asm-Lexer (Phase 5 STUB)
#
# STATUS: Toolchain skeleton only.
#
# This commit proves the asm build pipeline works (assemble + link) but
# does NOT yet implement a real lexer. The main loop currently:
#   1. Prints a banner line to stderr identifying this as the asm build.
#   2. Exits with status 0 if it ran at all.
#
# The C port in src-c/src/lexer.c remains the source of truth for
# tokenization. A future Phase 5 commit will incrementally grow this
# file into a real lexer (callable from the same C harness via the
# same UCLexer struct).
#
# Build:
#   as --64 -o lexer.o lexer.s
#   ld -o uc_lexer_asm lexer.o

        .equ    SYS_WRITE,  1
        .equ    SYS_EXIT,   60

        .section .rodata
banner:  .ascii  "uc_lexer_asm: Phase 5 asm-Lexer stub (toolchain verified)\n"
banner_end:
banner_len = . - banner

        .section .text
        .global _start

_start:
        movq    $2, %rdi                  # fd = stderr
        leaq    banner(%rip), %rsi
        movq    $banner_end - banner, %rdx
        movq    $SYS_WRITE, %rax
        syscall

        movq    $0, %rdi
        movq    $SYS_EXIT, %rax
        syscall