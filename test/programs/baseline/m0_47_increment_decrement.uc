// m0_47: Pre/post increment (++) and decrement (--)
int main() {
    int x = 5;
    x++;                 // 6
    int y = x++;         // y=6, x=7
    y--;                 // 5
    --x;                 // 6
    int z = --x;         // z=5, x=5
    z++;                 // 6
    return x + y + z;    // 5 + 5 + 6 = 16
}
