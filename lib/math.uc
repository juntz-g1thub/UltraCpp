// lib/math.uc - UltraCPP Math Library
// Per .dev/drafts/0.3.4-implementation-plan.md commit 10d.
// 0.3.4 §11.0.2 UltraCPP stdlib path materialization.
// Provides uc_abs as the target of abs_int → uc_abs migration (commit 11a).

export int add(int a, int b) {
    return a + b;
}

// uc_abs: integer absolute value
//   abs(n) = (n < 0) ? -n : n
// Replaces inline-LLVM-IR @builtin_abs_int per 0.3.4 spec §11.0.3.
export int uc_abs(int n) {
    if (n < 0) {
        return -n;
    }
    return n;
}