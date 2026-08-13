// m0_04: Comparison operators (==, !=, <, >, <=, >=)
int main() {
    int a = 5;
    int b = 10;
    int eq  = (a == b);   // 0
    int ne  = (a != b);   // 1
    int lt  = (a <  b);   // 1
    int gt  = (a >  b);   // 0
    int le  = (a <= b);   // 1
    int ge  = (a >= b);   // 0
    return eq + ne + lt + gt + le + ge;  // 3
}
