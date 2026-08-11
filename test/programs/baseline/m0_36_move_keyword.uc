// m0_36: PARSE-ONLY — move(p) expression
// PARSE-ONLY: `move` is the built-in primitive that explicitly transfers
// ownership (spec §7.4). This test only verifies the expression parses +
// codegens. Use-after-move / double-free detection is checked in M2.
int consume(unique int p) {
    return *p;
}

int main() {
    unique int p = alloc(int);
    *p = 7;
    int v = consume(move(p));
    free(p);
    return v;               // 7
}
