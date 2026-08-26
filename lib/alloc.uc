// lib/alloc.uc - UltraCPP Allocation Wrappers
// Per .dev/drafts/0.3.4-implementation-plan.md commit 10d.
// 0.3.4 §11.0.2 UltraCPP stdlib path materialization.
// Thin wrappers over sys::malloc / sys::free for convenience.

export i8* uc_alloc(int n) {
    return sys::malloc(n);
}

export void uc_free(i8* p) {
    sys::free(p);
}