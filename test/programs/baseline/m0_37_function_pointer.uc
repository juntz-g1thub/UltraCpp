// m0_37: Function pointer — type and indirection call
int add(int a, int b) {
    return a + b;
}

int main() {
    int (*fp)(int, int) = add;
    int r1 = fp(2, 3);              // 5
    int r2 = (*fp)(10, 20);         // 30
    return r1 + r2;                 // 35
}
