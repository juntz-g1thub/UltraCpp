// lib/sys/raw.uc - UltraCPP Raw Syscall Interface
// Per .dev/drafts/0.3.4-implementation-plan.md commit 10e.
// 0.3.4 §10.4 sys:: namespace + runtime-architecture §6.
//
// Provides uc_syscall as the underlying primitive for sys::* API.
// Uses GCC inline asm syntax (per spec §10.3 / commit 8b codegen).

// x86_64 Linux syscall convention:
//   rax = syscall number
//   rdi = arg1, rsi = arg2, rdx = arg3
//   r10 = arg4, r8 = arg5, r9 = arg6
//   rcx, r11 are clobbered by syscall instruction

@ifdef(TARGET_ARCH_X86_64)
export int uc_syscall(int num, int arg1, int arg2, int arg3,
                       int arg4, int arg5, int arg6) {
    int result;
    asm {
        "syscall"
        : "=a"(result)
        : "0"(num), "D"(arg1), "S"(arg2), "d"(arg3),
          "r"(arg4), "r"(arg5), "r"(arg6)
        : "rcx", "r11", "cc", "memory"
    }
    return result;
}
@endif

// arm64 deferred to 0.3.5
// @ifdef(TARGET_ARCH_ARM64)
// ... uc_syscall via svc #0 ...
// @endif
