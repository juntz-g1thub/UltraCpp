// m0_35: PARSE-ONLY — `unique T` type constructor
// PARSE-ONLY: per spec §3.7 (Q1), `unique T` ≡ `T*`. The keyword must
// lex and parse correctly; deeper ownership semantics are checked in M2.
int main() {
    unique int p;          // ≡ int* p;  — owning pointer type
    return 0;
}
