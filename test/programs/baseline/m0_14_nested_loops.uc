// m0_14: Nested for loops (multiplication table sample)
int main() {
    int total = 0;
    for (int i = 1; i <= 3; i++) {
        for (int j = 1; j <= 4; j++) {
            total = total + i * j;
        }
    }
    // 1*1+1*2+1*3+1*4 + 2*1+...+2*4 + 3*1+...+3*4
    // = 10 + 20 + 30 = 60
    return total;
}
