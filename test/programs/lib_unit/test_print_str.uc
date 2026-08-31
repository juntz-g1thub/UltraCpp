// test_print_str: lib/print.uc::uc_print_str 输出字符串
// 期望 stdout = "hello\n"
#include "../../../lib/print.uc"

int main() {
    uc_print_str("hello\n");
    return 0;
}
