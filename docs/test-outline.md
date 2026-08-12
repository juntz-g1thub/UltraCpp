# UltraCPP Test Outline (.uc 源码测试)

> **目的**：记录全部使用 .uc 源文件进行的测试项目，作为借用检查实现的验证清单
> **范围**：M0 基线 + M1-M5 里程碑所有 .uc 测试
> **更新原则**：每个里程碑完成后更新对应表格的状态列
> **日期**：2026-08-08

## 1. 总览

| 里程碑 | 主题 | pass/ | fail/ | 小计 |
|---|---|---|---|---|
| **M0** | 基线验证（无新功能） | 47 + 3 PARSE-ONLY | 0 | **50**（2 已迁移到 M1） |
| **M1** | 词法 + 语法扩展 | 8 + 2 迁移 | 1 | **11** |
| **M2** | 两权限所有权模型 | 10 | 4 | **14** |
| **M3** | 修改权系统 | 8 | 3 | **11** |
| **M4** | 引用 + const 语义 | 5 | 7 | **12** |
| **M5** | 线程模型 | 7 | 1 | **8** |
| **合计** | | **90** | **16** | **106** |

**M0 实际执行数**：48（47 常规 + 3 PARSE-ONLY）—— 2 个测试（m0_07、m0_38）已迁移到 M1。
**最终激活测试数**：48 + 9 + 14 + 11 + 12 + 8 = **102 个 .uc 测试**（M0 完成后达成 48 / 102 激活）。

---

## 2. 状态图例

| 符号 | 含义 |
|---|---|
| ⬜ | 待写 / 待跑 |
| ⏳ | 编写中 |
| ✓ | 通过（编译 + 运行结果符合预期） |
| ✗ | 失败（编译失败 / 行为不符） |
| 🚧 | 部分通过（部分子场景通过） |
| ⏭ | 已迁移到其他里程碑 |
| ⚠ | PARSE-ONLY（仅验证 parse + codegen，语义检查未覆盖） |

---

## 3. 基线测试（M0 — 验证 src-c/ 现有功能）

**目标**：在不加新功能的前提下，跑通所有 0.3.0 spec 中"无需新机制即可工作"的语法点。失败 = src-c/ 已有 bug。
**范围**：47 常规 + 3 PARSE-ONLY（语义层未实现）+ 2 已迁移到 M1（lexer 缺 token）。

### 3.1 数据类型与运算符基础

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_01 | `m0_01_minimal_main.uc` | Minimal main | `int main() { return 0; }` | ⬜ |
| m0_02 | `m0_02_int_arithmetic.uc` | int +-*/% | `+ - * / %` | ⬜ |
| m0_03 | `m0_03_int_precedence.uc` | Operator precedence | `2 + 3 * 4` | ⬜ |
| m0_04 | `m0_04_comparison.uc` | Comparison ops | `== != < > <= >=` | ⬜ |
| m0_05 | `m0_05_logical.uc` | Logical ops | `&& \|\| !` | ⬜ |
| m0_06 | `m0_06_bitwise.uc` | Bitwise ops | `& \| ^ ~ << >>` | ⬜ |
| m0_07 | `m0_07_compound_assign.uc` | Compound assignment | `+= -= *= /=` | ⏭ **MOVED to M1**（lexer 无 token） |
| m0_08 | `m0_08_unary.uc` | Unary ops | `- ! ~` | ⬜ |
| m0_47 | `m0_47_increment_decrement.uc` | ++/-- | `++x; x--;` | ⬜ |
| m0_48 | `m0_48_ternary.uc` | Ternary | `cond ? a : b` | ⬜ |

### 3.2 控制流

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_09 | `m0_09_if_else.uc` | if/else | `if { } else { }` | ⬜ |
| m0_10 | `m0_10_if_else_chain.uc` | if/else if/else chain | `else if` | ⬜ |
| m0_11 | `m0_11_nested_if.uc` | Nested if | `if { if { } }` | ⬜ |
| m0_12 | `m0_12_while_loop.uc` | while | `while (cond)` | ⬜ |
| m0_13 | `m0_13_for_loop.uc` | for | `for (init; cond; step)` | ⬜ |
| m0_14 | `m0_14_nested_loops.uc` | Nested loops | `for { for { } }` | ⬜ |
| m0_15 | `m0_15_break_continue.uc` | break/continue | `break; continue;` | ⬜ |

### 3.3 函数

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_16 | `m0_16_function_simple.uc` | Function def | `int f(int x) { return x; }` | ✓ |
| m0_17 | `m0_17_function_void.uc` | Void function | `void f() {}` | ⬜ |
| m0_18 | `m0_18_function_multi_params.uc` | Multi params | `int add(int a, int b)` | ✓ |
| m0_19 | `m0_19_function_recursion.uc` | Recursion | factorial/fibonacci | ⬜ |
| m0_20 | `m0_20_function_nested_calls.uc` | Nested calls | `f(g(x))` | ✓ |

### 3.4 作用域与存储期

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_21 | `m0_21_local_global.uc` | Local vs global scope | `int g; int main() { int l; }` | ✓ |
| m0_22 | `m0_22_const_global.uc` | const global | `const int MAX = 100;` | ✓ |
| m0_45 | `m0_45_const_expr.uc` | const expr | `const int X = 1 + 2;` | ✓ |
| m0_46 | `m0_46_global_init.uc` | Global init | `int g = 42;` | ✓ |

### 3.5 字符串

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_23 | `m0_23_string_literal.uc` | String literal | `"hello"` | ✓ |
| m0_24 | `m0_24_string_escape.uc` | String escape | `"\n\t\\"` | ⬜ |
| m0_25 | `m0_25_string_io.uc` | String + sys$write | `sys$write(1, s, len)` | ⬜ |

### 3.6 数组

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_26 | `m0_26_array_basic.uc` | Array decl | `int arr[10];` | ⬜ |
| m0_27 | `m0_27_array_init.uc` | Array init | `int arr[3] = {1, 2, 3};` | ⬜ |
| m0_28 | `m0_28_array_index.uc` | Array index | `arr[i]` | ⬜ |

### 3.7 指针

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_29 | `m0_29_pointer_basic.uc` | Pointer decl | `int* p;` | ⬜ |
| m0_30 | `m0_30_pointer_deref.uc` | Pointer deref | `*p = 100;` | ⬜ |
| m0_31 | `m0_31_pointer_addr.uc` | Address-of | `&x` | ⬜ |

### 3.8 结构体

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_32 | `m0_32_struct_basic.uc` | Struct | `struct Point { int x; int y; }` | ⬜ |
| m0_33 | `m0_33_struct_field.uc` | Struct field | `p.x` | ⬜ |

### 3.9 内存管理关键字（⚠ PARSE-ONLY）

> **说明**：以下 3 个测试**仅验证** token / AST 节点存在 + 可 parse + 可 codegen。**不验证** 0.3.0 spec §7.1 的语义（unique owner / move / free balance）。语义验证见 **M2**。

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_34 | `m0_34_alloc_free.uc` | alloc/free | `alloc(int); free(p);` | ⚠ PARSE-ONLY（语义未检查） |
| m0_35 | `m0_35_unique_keyword.uc` | unique keyword | `unique int p;` | ⚠ PARSE-ONLY（语义未检查） |
| m0_36 | `m0_36_move_keyword.uc` | move keyword | `move(p)` | ⚠ PARSE-ONLY（语义未检查） |

### 3.10 函数指针

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_37 | `m0_37_function_pointer.uc` | Function pointer | `int (*fp)(int)` | ⬜ |

### 3.11 typedef（⏭ MOVED to M1）

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_38 | `m0_38_typedef.uc` | typedef | `typedef int MyInt;` | ⏭ **MOVED to M1**（lexer 无关键字） |

### 3.12 模块系统

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_39 | `m0_39_module_import.uc` | #import | `#import "lib/io"` | ✓ |
| m0_40 | `m0_40_module_include.uc` | #include | `#include "lib/math.uc"` | ⬜ |

### 3.13 FFI 与 unsafe

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_41 | `m0_41_extern_c.uc` | extern "C" | `extern "C" { ... }` | ⬜ |
| m0_42 | `m0_42_unsafe_block.uc` | unsafe block | `unsafe { ... }` | ⬜ |

### 3.14 main 变体

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_43 | `m0_43_main_with_args.uc` | main with args | `int main(int argc, char** argv)` | ⬜ |
| m0_44 | `m0_44_multiple_files.uc` | Multi-file | 2+ .uc files 互调 | ⬜ |

### 3.15 其他

| ID | 程序名 | 测什么 | 关键语法 | 状态 |
|---|---|---|---|---|
| m0_49 | `m0_49_comments_mixed.uc` | Mixed comments | `// + /* */` | ✓ |
| m0_50 | `m0_50_chinese_identifiers.uc` | Chinese identifiers | `int 变量 = 42;` | ⬜ |

---

## 4. M1 测试

**目标**：在 lexer/parser/codegen 中加入 0.3.0 spec 缺失的 token / 语法节点（不做语义检查）。包含 m0_07 (compound_assign) + m0_38 (typedef) 从 M0 迁移过来的 2 个。

### 4.1 M1 通过测试

| ID | Program | Tests | Key Syntax | Status |
|---|---|---|---|---|
| m1_01 | `m1_01_mod_call.uc` | `mod(r);` syntax | `mod(expr)` | ⬜ |
| m1_02 | `m1_02_unmod_call.uc` | `unmod(r);` syntax | `unmod(expr)` | ⬜ |
| m1_03 | `m1_03_shared_decl.uc` | `shared T var` | `shared int* p;` | ⬜ |
| m1_04 | `m1_04_thread_local.uc` | `__thread T var` | `__thread int x;` | ⬜ |
| m1_05 | `m1_05_move_to_thread.uc` | `move_to_thread(p, tid)` | cross-thread move | ⬜ |
| m1_06 | `m1_06_combined.uc` | 5 new keywords together | full syntax | ⬜ |
| m1_07 | `m1_07_compound_assign.uc` | Compound assignment | `+= -= *= /= %=` | ⬜ (moved from M0) |
| m1_08 | `m1_08_typedef.uc` | typedef keyword | `typedef int MyInt;` | ⬜ (moved from M0) |

### 4.2 M1 失败测试

| ID | Program | Tests | Expected | Status |
|---|---|---|---|---|
| m1_n1 | `m1_n1_ampmut_rejected.uc` | `&mut` not recognized as keyword | 编译失败 | ⬜ |

---

## 5. M2 测试

**目标**：实现 0.3.0 spec §7.1 所有权语义（unique owner + 显式 move + clone + use-after-move + double-free 检测）。
**Spec 引用**：`.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` §10-7 Rule 22 + §3 R-OWN-1..8。

### 5.1 M2 通过测试

| ID | Program | Tests | Status |
|---|---|---|---|
| m2_o1 | `m2_o1_single_owner.uc` | Single owner uniqueness | ⬜ |
| m2_o2 | `m2_o2_move_transfer.uc` | `move(p)` transfers ownership | ⬜ |
| m2_o3 | `m2_o3_assign_implicit_mod.uc` | Q4: `p1 = p2` implicit mod, no ownership transfer | ⬜ |
| m2_o4 | `m2_o4_source_still_usable.uc` | After assignment, source still usable | ⬜ |
| m2_o5 | `m2_o5_move_source_unusable.uc` | After move, source can't be used | ⬜ |
| m2_o6 | `m2_o6_move_then_reassign.uc` | After move, source can be reassigned | ⬜ |
| m2_o7 | `m2_o7_return_owning.uc` | Return transfers ownership | ⬜ |
| m2_o8 | `m2_o8_arg_owning_transfer.uc` | Arg transfers ownership | ⬜ |
| m2_o9 | `m2_o9_stack_value_copy.uc` | Stack values are not subject to ownership | ⬜ |
| m2_o10 | `m2_o10_clone_independent.uc` | clone() creates independent owner | ⬜ |

### 5.2 M2 失败测试

| ID | Program | Tests | Expected Error | Status |
|---|---|---|---|---|
| m2_n1 | `m2_n1_double_free.uc` | Same alloc freed twice | `UseAfterFree` | ⬜ |
| m2_n2 | `m2_n2_assign_then_free_both.uc` | Both pointers freed after assignment | `DoubleOwner` or similar | ⬜ |
| m2_n3 | `m2_n3_use_after_move.uc` | Use source after move | `UseAfterMove` | ⬜ |
| m2_n4 | `m2_n4_double_free_after_pass.uc` | Free source after passing as arg | `UseAfterMove` | ⬜ |

---

## 6. M3 测试

**目标**：实现 0.3.0 spec §9.9 `#modlaw` 指令 + §7.1.1 `mod()` / `unmod()` 表达式的语义。
**Spec 引用**：`.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` §10-7 Rule 23 / Rule 24。

### 6.1 M3 通过测试

| ID | Program | Tests | Status |
|---|---|---|---|
| m3_m1 | `m3_m1_mod_basic.uc` | `mod(r)` syntax | ⬜ |
| m3_m2 | `m3_m2_unmod_basic.uc` | `unmod(r)` syntax | ⬜ |
| m3_m3 | `m3_m3_modlaw_shared_global.uc` | `#modlaw shared global` | ⬜ |
| m3_m4 | `m3_m4_modlaw_exclusive_module.uc` | `#modlaw exclusive module` | ⬜ |
| m3_m5 | `m3_m5_modlaw_none_module.uc` | `#modlaw none module` | ⬜ |
| m3_m6 | `m3_m6_modlaw_combined.uc` | Multiple #modlaw in same file (scope) | ⬜ |
| m3_m7 | `m3_m7_modlaw_shared_multi_mod.uc` | Shared policy allows multiple mod() | ⬜ |
| m3_m8 | `m3_m8_modlaw_scope_isolation.uc` | Scope isolation between files | ⬜ |

### 6.2 M3 失败测试

| ID | Program | Tests | Expected Error | Status |
|---|---|---|---|---|
| m3_n1 | `m3_n1_illegal_combo.uc` | `#modlaw none shared` invalid | `IllegalModLaw` | ⬜ |
| m3_n2 | `m3_n2_exclusive_conflict.uc` | Exclusive policy, second mod() | `BorrowConflict` | ⬜ |
| m3_n3 | `m3_n3_none_mod_forbidden.uc` | None policy, mod() called | `ModForbidden` | ⬜ |

---

## 7. M4 测试

**目标**：实现 0.3.0 spec §3.3 + §7.8（单一 `T&`）+ §7.9（悬垂引用检查）+ §7.10（const 语义）。
**Spec 引用**：`.dev/drafts/0.1.0-borrowck-spec-vs-impl.md` §10-6（删除 `T&mut`）。

### 7.1 M4 通过测试

| ID | Program | Tests | Status |
|---|---|---|---|
| m4_r1 | `m4_r1_ref_decl_lvalue.uc` | `T& r = x` (lvalue) | ⬜ |
| m4_r2 | `m4_r2_pointer_addr_of.uc` | `T* p = &x` | ⬜ |
| m4_r3 | `m4_r3_ref_mod_shared.uc` | T& + mod() + #modlaw shared | ⬜ |
| m4_c1 | `m4_c1_const_rebind_ok.uc` | `T* const` rebindable | ⬜ |
| m4_c2 | `m4_c2_const_write_via_const_t.uc` | `T* const` data read-only | ⬜ |

### 7.2 M4 失败测试

| ID | Program | Tests | Expected Error | Status |
|---|---|---|---|---|
| m4_n1 | `m4_n1_ref_takes_addr.uc` | `int& r = &x;` | `TypeMismatch` | ⬜ |
| m4_n2 | `m4_n2_ref_write_no_mod.uc` | `int& r = x; r = 100;` (no mod) | `NoModRight` | ⬜ |
| m4_n3 | `m4_n3_dangling_local.uc` | Function returns reference to local | `DanglingReference` | ⬜ |
| m4_n4 | `m4_n4_dangling_after_free.uc` | Use reference after free | `DanglingReference` | ⬜ |
| m4_n5 | `m4_n5_const_rebind.uc` | `const int*` rebind | `CannotRebind` | ⬜ |
| m4_n6 | `m4_n6_const_write.uc` | `const int*` write | `ReadOnly` | ⬜ |
| m4_n7 | `m4_n7_owning_heap_const_view.uc` | Readonly view of owning heap | `OwningHeapNoReadonly` | ⬜ |

---

## 8. M5 测试

**目标**：实现 0.3.0 spec §10 线程模型（`shared` 跨线程 + `__thread` TLS + `move_to_thread` + stdlib `mutex<T>` / `atomic<T>`）。

### 8.1 M5 通过测试

| ID | Program | Tests | Status |
|---|---|---|---|
| m5_t1 | `m5_t1_shared_visible.uc` | shared visible across threads | ⬜ |
| m5_t2 | `m5_t2_thread_local.uc` | `__thread` TLS | ⬜ |
| m5_t3 | `m5_t3_move_to_thread.uc` | `move_to_thread` | ⬜ |
| m5_t4 | `m5_t4_spawn_with_move.uc` | `spawn_thread_with` implicit move | ⬜ |
| m5_t5 | `m5_t5_mutex_basic.uc` | `mutex<T>` stdlib type | ⬜ |
| m5_t6 | `m5_t6_atomic_basic.uc` | `atomic<T>` stdlib type | ⬜ |
| m5_t7 | `m5_t7_mutex_suppress_compiler_mutex.uc` | mutex<T> prevents double-lock | ⬜ |

### 8.2 M5 失败测试

| ID | Program | Tests | Expected Error | Status |
|---|---|---|---|---|
| m5_n1 | `m5_n1_non_shared_global.uc` | Cross-thread access non-shared global | `NonSharedGlobal` | ⬜ |

---

## 9. 配套脚本索引

| 脚本 | 用途 | 用法 |
|---|---|---|
| `test/e2e/run_baseline.sh` | M0 基线 | `bash test/e2e/run_baseline.sh` |
| `test/e2e/run_m{1..5}.sh` | 各里程碑 | `bash test/e2e/run_mN.sh` |
| `test/e2e/run_negative.sh` | 负测试 | `bash test/e2e/run_negative.sh <dir>` |
| `test/e2e/bench.sh` | 性能基线 | `bash test/e2e/bench.sh > test/bench/X.csv` |
| `src-c/tests/test_*.c` | C 单元 | `make -C src-c test` |
| `tools/tokenize_test.sh` | tokenize 字节对齐 | `bash tools/tokenize_test.sh` |
| `tools/ast_test.sh` | AST 字节对齐 | `bash tools/ast_test.sh` |
| `tools/codegen_test.sh` | codegen 字节对齐 | `bash tools/codegen_test.sh` |
| `tools/build_test.sh` | build 字节对齐 | `bash tools/build_test.sh`（已迁移到 M0 路径） |

---

## 10. 回归执行矩阵

| 完成后 \ 重跑 | M0 | M1 | M2 | M3 | M4 | M5 |
|---|---|---|---|---|---|---|
| M0 | ✓ 新增 | | | | | |
| M1 完成 | 重跑全部 | ✓ 新增 | | | | |
| M2 完成 | 重跑 | 重跑 | ✓ 新增 | | | |
| M3 完成 | 重跑 | 重跑 | 重跑 | ✓ 新增 | | |
| M4 完成 | 重跑 | 重跑 | 重跑 | 重跑 | ✓ 新增 | |
| M5 完成 | 重跑 | 重跑 | 重跑 | 重跑 | 重跑 | ✓ 新增 |

**关键规则**：任一历史测试在新 Mx 下失败 = 该 Mx 未达完成标准。

---

## 11. 状态更新流程

每个 Mx 完成时：

1. 跑完 Mx 所有新增测试（pass/ + fail/）
2. 跑所有历史里程碑测试（按 §10 矩阵）
3. 更新本文档对应表格的状态列：
   - 通过 → ✓
   - 失败 → ✗（在 commit message 引用 issue）
   - 部分通过 → 🚧
4. 跑性能基线脚本：`bash test/e2e/bench.sh > test/bench/m{N}.csv`
5. commit：包含代码 + 测试 + 文档更新

**commit message 模板**：

```
Mx: <summary>

tests: <count> pass + <count> fail programs
- pass/<list> all ✓
- fail/<list> all ✓ (with correct error names)
- regression: M0..M{x-1} still ✓

bench: compile_ms delta = <N>% (threshold 20%)
docs: test-outline.md updated

Refs: spec §<section>
```

---

## 12. 已知未覆盖

以下场景在 0.4.0 计划中**不覆盖**（留待 0.5.0+ 评估）：

1. **错误路径细节**：
   - 错误码的 spans / suggestions（注：提示）
   - 跨文件错误的 source location 精确度
2. **复杂控制流**：
   - goto / labels
   - 嵌套 lambda / closure
   - 协程 / generator
3. **性能与压力**：
   - 大输入（> 10k 行）下的 compile time
   - 编译内存峰值
   - 并行编译
4. **跨平台**：
   - 当前 src-c/ 仅 Linux/x86_64 验证
   - macOS / Windows / ARM 未覆盖
5. **LLVM 后端**：
   - 当前 codegen 输出 IR 后由 llc + gcc 链接
   - 直接生成机器码 / 自定义后端未覆盖
6. **调试器集成**：
   - DWARF 调试信息
   - source map
7. **IDE 支持**：
   - LSP server
   - 语法高亮 / formatter

---

*最后更新：2026-08-08 — 初始草稿*
