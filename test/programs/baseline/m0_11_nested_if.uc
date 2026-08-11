// m0_11: Nested if statements
int main() {
    int a = 5;
    int b = 10;
    int r = 0;
    if (a > 0) {
        if (b > 0) {
            r =  1;
        } else if (b == 0) {
            r =  0;
        } else {
            r = -1;
        }
    } else {
        r = -2;
    }
    return r;     // 1
}
