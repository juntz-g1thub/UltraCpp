// lib/memory.uc - UltraCPP Memory Library
// Per .dev/drafts/0.3.4-implementation-plan.md commit 10c.
// 0.3.4 §11.0.2 UltraCPP stdlib path materialization.

export i8* uc_memcpy(i8* dst, i8* src, int n) {
    int i = 0;
    while (i < n) {
        dst[i] = src[i];
        i = i + 1;
    }
    return dst;
}

// memmove handles overlapping src/dst regions.
// If dst < src: copy forward (low to high).
// If dst > src: copy backward (high to low) to avoid clobbering src.
// If dst == src: no-op.
export i8* uc_memmove(i8* dst, i8* src, int n) {
    if (dst == src) {
        return dst;
    }
    if (dst < src) {
        // forward copy (no overlap concern: copying low→high before src region)
        int i = 0;
        while (i < n) {
            dst[i] = src[i];
            i = i + 1;
        }
    } else {
        // backward copy (dst > src; copy high→low to avoid clobbering src)
        int i = n - 1;
        while (i >= 0) {
            dst[i] = src[i];
            i = i - 1;
        }
    }
    return dst;
}

export i8* uc_memset(i8* dst, int c, int n) {
    int i = 0;
    while (i < n) {
        dst[i] = (i8)c;
        i = i + 1;
    }
    return dst;
}
