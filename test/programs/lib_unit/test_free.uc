// test_free: lib/alloc.uc::uc_free 释放 uc_alloc 分配内存
// 期望 exit code = 0 (无 crash)
#include "../../../lib/alloc.uc"

int main() {
    int* p = (int*)uc_alloc(16);
    uc_free(p);
    return 0;
}
