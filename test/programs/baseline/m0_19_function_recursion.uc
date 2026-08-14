// m0_19: Recursive function (factorial)
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    return factorial(6);   // 208
}
