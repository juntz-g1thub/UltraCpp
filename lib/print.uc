// lib/print.uc - UltraCPP I/O Print Library
// Per .dev/drafts/0.3.4-implementation-plan.md commit 10a.
// 0.3.4 §11.0.2 UltraCPP stdlib path materialization.

export int uc_print_str(i8* s) {
    int len = sys::strlen(s);
    int result = sys::write(1, s, len);
    return 0;
}

export int uc_print_num(int n) {
    // simple itoa for non-negative numbers
    i8 buf[16];
    int i = 0;
    int is_neg = 0;

    if (n < 0) {
        is_neg = 1;
        n = -n;
    }
    if (n == 0) {
        buf[i] = '0';
        i = i + 1;
    } else {
        while (n > 0) {
            buf[i] = (n % 10) + '0';
            n = n / 10;
            i = i + 1;
        }
    }
    if (is_neg) {
        buf[i] = '-';
        i = i + 1;
    }
    // reverse buffer
    i8 out[16];
    int j = 0;
    while (i > 0) {
        i = i - 1;
        out[j] = buf[i];
        j = j + 1;
    }
    int result = sys::write(1, out, j);
    return 0;
}

export int uc_print_float(double f) {
    // PARSE-ONLY stub — full float formatting deferred to 0.3.5
    // (per implementation plan §7 scope-out)
    int result = sys::write(1, "<float>", 8);
    return 0;
}
