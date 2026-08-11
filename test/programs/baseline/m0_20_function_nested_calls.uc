// m0_20: Nested function calls (one function used inside another)
int square(int x) {
    return x * x;
}

int dist_sq(int a, int b) {
    return square(a) + square(b);
}

int main() {
    return dist_sq(3, 4);   // 9 + 16 = 25
}
