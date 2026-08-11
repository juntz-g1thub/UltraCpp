// m0_48: Ternary conditional (cond ? a : b)
int main() {
    int a = 5;
    int b = 10;
    int max_v  = (a > b) ? a : b;   // 10
    int min_v  = (a < b) ? a : b;   // 5
    int eq_v   = (a == b) ? 1 : 0;  // 0
    int abs_a  = (a < 0) ? -a : a;  // 5
    return max_v + min_v + eq_v + abs_a;
}
