// m0_03: Operator precedence — * / % bind tighter than + -
int main() {
    int r1 = 2 + 3 * 4;       // 14  (not 20)
    int r2 = (2 + 3) * 4;     // 20
    int r3 = 10 - 6 / 2;      // 7
    int r4 = 10 - 2 * 3 + 1;  // 5
    return r1 + r2 + r3 + r4;
}
