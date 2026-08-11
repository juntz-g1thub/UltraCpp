// m0_44 (helper): exported multiplication — paired with m0_44_multiple_files.uc
// This file exists in the baseline/ directory solely to support the
// §3.14 multi-file test. Both files are expected to be compiled and
// linked together by the M0 test runner.
export int helper_mul(int a, int b) {
    return a * b;
}
