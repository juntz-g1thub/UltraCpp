// m0_50: Non-ASCII (Chinese) identifiers
// The lexer treats any non-control Unicode letter as a valid identifier
// start. This file uses CJK ideographs in identifier names.
// expects_compiler_error: lexer rejects unknown CJK char (spec §2.4 requires ASCII ident chars)
//   — this test verifies the compiler REJECTS the source with a proper
//   diagnostic, NOT that the source is accepted. Run as a negative-style
//   test via the `// expects_compiler_error` marker recognized by
//   test/e2e/run_baseline.sh.
int 计数() {
    int 总和 = 0;
    for (int 索引 = 1; 索引 <= 5; 索引++) {
        总和 = 总和 + 索引;
    }
    return 总和;                 // 15
}

int main() {
    return 计数();                // 15
}
