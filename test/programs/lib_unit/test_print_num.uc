// test_print_num: lib/print.uc::uc_print_num 输出多个 int 值
// 期望 stdout = "0\n42\n-7\n-2147483648\n"
#include "../../../lib/print.uc"

int main() {
    uc_print_num(0);
    uc_print_num(42);
    uc_print_num(-7);
    uc_print_num(-2147483648);  // INT_MIN
    return 0;
}
