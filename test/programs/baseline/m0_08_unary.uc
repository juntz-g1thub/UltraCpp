// m0_08: Unary prefix operators (-, !, ~)
int main() {
    int a = 5;
    int neg_a   = -a;            // -5
    int pos_a   = -neg_a;        // 5
    int not_a   = !a;            // 0
    int not_z   = !0;            // 1
    int bnot_z  = ~0;            // -1 (all bits set)
    int bnot_lo = bnot_z & 0xff; // low byte = 255
    return pos_a + not_a + not_z + bnot_lo;
}
