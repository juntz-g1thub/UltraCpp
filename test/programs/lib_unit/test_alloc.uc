// test_alloc: lib/alloc.uc::uc_alloc 分配 16 字节
// 期望 stdout = "42\n" (写入值)
#include "../../../lib/alloc.uc"

int main() {
    int* p = (int*)uc_alloc(16);
    *p = 42;
    uc_print_num(*p);  // 输出 "42\n"
    return 0;
}
