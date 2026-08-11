// m0_15: break and continue inside a loop
int main() {
    int sum = 0;
    for (int i = 0; i < 10; i++) {
        if (i == 8) {
            break;             // exit the loop entirely
        }
        if (i % 2 == 0) {
            continue;          // skip even i (i = 0, 2, 4, 6)
        }
        sum = sum + i;         // adds odd i = 1, 3, 5, 7
    }
    return sum;                // 1 + 3 + 5 + 7 = 16
}
