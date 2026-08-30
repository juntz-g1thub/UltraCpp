// m0_07: Compound assignment operators
// Spec: 0.3.0 §X.X (per .dev/plans/0.3.0-borrow-check-milestones.md §3.0,
// formerly MOVED to M1, now PASS via 0.3.5 commit 14f lexer/parser/codegen).
int main() {
    int a = 10;
    a += 5;     // 15
    a -= 3;     // 12
    a *= 2;     // 24
    a /= 4;     // 6
    a %= 5;     // 1
    return a;   // 1
}