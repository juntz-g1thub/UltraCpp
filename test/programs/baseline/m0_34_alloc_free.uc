// m0_34: PARSE-ONLY — alloc() and free() keyword parsing
// PARSE-ONLY: src-c/ has the alloc/free keywords but 0.3.0 spec §7.1
// ownership semantics (single-owner / move / free-balance) are not yet
// implemented, so we only check that the syntax parses + codegens.
//
// NOTE: this program may leak or fail at runtime — that is expected.
// Semantic checks live in M2.
int main() {
    int* p = alloc(int);
    *p = 42;
    int v = *p;
    free(p);
    return v;                 // 42
}
