// m0_05: Logical operators (&&, ||, !)
int main() {
    int a = 1;
    int b = 0;
    int and_v = a && b;       // 0
    int or_v  = a || b;       // 1
    int not_a = !a;           // 0
    int not_b = !b;           // 1
    // Mixed: (a && !b) || (!a && b)  ==  (1 && 1) || (0 && 0)  ==  1
    int mix = (a && !b) || (!a && b);
    return and_v + or_v + not_a + not_b + mix;
}
