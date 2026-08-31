// test_print_float: lib/print.uc::uc_print_float 完整 ftoa
// 期望 stdout = "3.14\n"  -- 14g' 阶段: STUB (per 0.3.5 plan D7)
// runner 应 SKIP 此测试 (14h commit 后启用)
int main() {
    // 14g' 阶段: 不调用 uc_print_float (仍是 STUB 输出 "<float>")
    // 14h commit 后会改为: uc_print_float(3.14); return 0;
    return 0;  // STUB PASS (期望 stdout 为空)
}
