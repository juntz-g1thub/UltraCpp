// lib/string.uc - UltraCPP String Library
// Per .dev/drafts/0.3.4-implementation-plan.md commit 10b.
// 0.3.4 §11.0.2 UltraCPP stdlib path materialization.

export int uc_strlen(i8* s) {
    int len = 0;
    while (s[len] != 0) {
        len = len + 1;
    }
    return len;
}

export i8* uc_strcpy(i8* dst, i8* src) {
    int i = 0;
    while (src[i] != 0) {
        dst[i] = src[i];
        i = i + 1;
    }
    dst[i] = 0;
    return dst;
}

export int uc_strcmp(i8* a, i8* b) {
    int i = 0;
    while (a[i] != 0 && b[i] != 0) {
        if (a[i] != b[i]) {
            return a[i] - b[i];
        }
        i = i + 1;
    }
    return a[i] - b[i];
}