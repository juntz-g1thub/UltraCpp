// m0_06: Bitwise operators (&, |, ^, ~, <<, >>) on int
int main() {
    int a = 0b1100;             // 12
    int b = 0b1010;             // 10
    int band  = a & b;          // 8
    int bor   = a | b;          // 14
    int bxor  = a ^ b;          // 6
    int bnot  = ~a;             // implementation-defined sign-extended
    int shl   = 1 << 4;         // 16
    int shr   = 64 >> 2;        // 16
    // bnot is masked to low byte via & 0xFF so it contributes usefully
    int bnot_lo = bnot & 0xff;  // low byte of ~12 = 243
    return band + bor + bxor + shl + shr + bnot_lo;
}
