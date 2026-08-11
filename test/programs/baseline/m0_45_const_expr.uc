// m0_45: const with constant-expression initializer
const int X   = 1 + 2;       // 3
const int Y   = X * 4;       // 12
const int Z   = Y - X + 5;   // 14

int main() {
    return X + Y + Z;        // 3 + 12 + 14 = 29
}
