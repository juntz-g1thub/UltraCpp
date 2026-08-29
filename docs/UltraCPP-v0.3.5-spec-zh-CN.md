# UltraCPP 0.3.5 语言规范

> **版本**：0.3.5
>
> **上一版本**：0.3.4 — [`UltraCPP-v0.3.4-spec-zh-CN.md`](./UltraCPP-v0.3.4-spec-zh-CN.md)
>
> **状态**：草稿 (draft)
>
> **日期**：2026-08-21
>
> 同版本英文译本：[English](./UltraCPP-v0.3.5-spec-en.md)

---

## 修订摘要

本版本在 0.2.0 的基础上，落实 **21+ 条语言设计决策**（决策编号沿用历史 D-1 .. D-8，本版本新增 Rule 1, 2A, 2B, Q1 .. Q6, Rule 22 .. 28）。**两条权限彻底拆分**（Rule 22）与 **#modlaw 指令**（Rule 23）是本次修订的核心概念改动。

> **[0.3.1]** 本版本在 0.3.0 的基础上，**新增 §4.13「表达式分类：lvalue 与 rvalue」** 完整章节，并在 §4.1 优先级表新增 2.5 级一元运算符优先级行、§4.6 赋值运算符表 11 行统一补「LHS 必须是 lvalue」约束、§7.8 引用规则 1 改写并交叉引用 §4.13.2、§12.1 EBNF 增加 `lvalue` / `rvalue` 非终结符、§12.3 附录优先级表同步 §4.1 加 2.5 级。**不引入新语法、不修改现有语义**，仅补 lvalue 概念术语并修 m0_42 deref-assign bug。详见变更日志中 0.3.1 行。

> **[0.3.4 修订重点]** 在 0.3.3 的基础上，**完成 spec gap 收尾 + stdlib bootstrap 路径实化**：S6 FFI extern body 来源策略（§6 extern "C" 块体符号统一从 §11.0.2 `lib/sys.uc` 解析，规避与 §S8 deref-assign 边界冲突）；S8 deref-assign 类型安全（§4.13.5 deref 边界 + §4.13.2 lvalue 分类明确禁止 unsafe coerce）；S9 main exit code 8-bit 截断（§6.1 main 返回类型 infer + §11.0.1 builtin 表 `exit` 仅 low 8 bits 生效）；§11.0.2 UltraCPP stdlib 路径实化（`lib/*.uc` 6 文件落地：`print` / `string` / `memory` / `math` / `alloc` / `sys`）；§11.0.3 codegen 集成重写（`abs_int` 从 builtin 迁移到 `lib/math.uc`，走 LLVMBuildMemcpy + IR 标记）；§11.0.5 新增（UltraCPP stdlib 自举路径：`lib/print.uc` 等 6 文件自举 + bootstrap 脚本 + §11.0.2 双向引用；§11.0.4 "添加新 builtin 的流程"已存在，故 §11.0.5 编号因 §11.0.4 占用改用）。详见变更日志中 0.3.4 行。

| 主题 | 决策编号 | 一句话摘要 |
|------|---------|-----------|
| 基础 | Rule 1 | Owning 类型不限堆/栈；活跃期内不变；超出后随意；由初始化表达式推断存储位置。 |
| 引用 | Rule 2A | `&` 引用禁止 owning；编译期报 null ref；每线程作用域隔离。 |
| 指针 | Rule 2B | `T*` 由初始化表达式区分 owning 与 non-owning；赋值 `p1 = p2` **不复制所有权**，**给 p1 修改权**（隐式 `mod(p1)`）。 |
| 指针 | Q1 | `unique T` ≡ `T*`，可省略。 |
| 声明 | Q2 | 引用 `T& a = b`（右侧是左值）；指针 `T* a = &b`（右侧是取地址）。 |
| 引用 | Q3 | 单一引用类型 `T&`；独占性由 `#modlaw exclusive` + `mod()` 申请控制，不在类型层区分。 |
| 指针 | Q4 | `p1 = p2` 不 move，p1 得修改权（隐式 `mod(p1)`）。 |
| 指针 | Q5 | `move(p)` 仍然必要，显式移交所有权。 |
| 指针 | Q6 | `const T*` / `T* const` 自定义语义（与 C++ 相反）；owning heap 变量**不能**创建只读指针。 |
| 权限 | **Rule 22** | **两条权限彻底拆分**：所有权（决定谁 delete/free）和修改权（决定谁能改值）独立。 |
| 指令 | **Rule 23** | 新增 `#modlaw` 指令，6 种合法组合。 |
| 表达式 | **Rule 24** | 新增 `mod(ref_expr)` 申请修改权；`unmod(ref_expr)` 释放（一般省略）。 |
| 线程 | **Rule 25** | 跨线程访问必须 `shared` 标注（编译期强制）；编译器自动生成 mutex。 |
| 线程 | **Rule 26** | 显式 `move_to_thread(p, tid)` 或 `spawn_thread_with(tid, p)` 跨线程移交所有权。 |
| 线程 | **Rule 27** | `__thread int x;`（GCC 风格）线程局部存储。 |
| 线程 | **Rule 28** | 多线程内存布局：栈 / 堆 / 全局 / TLS 各自的生命域。 |

> **关于 0.2.0 vs 0.3.0 文本差异**：本版本对 0.2.0 §7.1 的赋值 move 规则**改写**为「赋值 = 隐式 `mod()` + 不转移所有权」（Q4=a），对 §3.3 的引用声明语法**改写**为「`T& a = b`（右侧是左值）」（Q2），并**删除 `T&mut` 类型**（Q3 反转）——引用统一为 `T&`，独占性改由 `#modlaw exclusive` + `mod()` 申请控制。其余章节保持 0.2.0 的设计，新增章节 §4.9–4.10、§9.9、§13。

---

## 变更日志（Changelog）

| 决策 | 章节 | 描述 |
|------|------|------|
| **0.3.4 本版本** | §S6, §S8, §S9, §11.0.2, §11.0.3, §11.0.5 | **本版本要点 (2026-08-21)** — spec gap 收尾 + stdlib bootstrap 路径实化：S6 FFI extern body 来源策略（§6 extern "C" 块体符号统一从 §11.0.2 `lib/sys.uc` 解析）；S8 deref-assign 类型安全（§4.13.5 + §4.13.2 明确禁止 unsafe coerce）；S9 main exit code 8-bit 截断（§6.1 + §11.0.1 `exit` 仅 low 8 bits 生效）；§11.0.2 UltraCPP stdlib 路径实化（`lib/*.uc` 6 文件落地：`print` / `string` / `memory` / `math` / `alloc` / `sys`）；§11.0.3 codegen 集成重写（`abs_int` 从 builtin 迁移到 `lib/math.uc`，走 LLVMBuildMemcpy + IR 标记）；§11.0.5 新增（UltraCPP stdlib 自举路径 + bootstrap 脚本 + §11.0.2 双向引用；§11.0.4 "添加新 builtin 的流程"已存在，故 stdlib 自举路径落到 §11.0.5）。详见本 spec 各章节修订 + 变更日志后续行。 |
| **S4 (0.3.2 新增)** | §4.1, §4.8 (修订), §4.8.1 (新增), §12.1, §12.3 | **C-style 显式类型转换 `(T)expr`**：新增 §4.8.1 完整子节（5 小节：语义 8 行类型转换表；与函数式 `T(expr)` 完全等价；4 类示例：整数↔指针 / 宽度 / FFI / 类型断言；编译期检查；交叉引用）。§4.8 加注提示 `(T)` 中 T 是类型名时为 cast；§4.1 主表 + §12.3 附录表第 2.5 级同步加 C-style cast 行；§12.1 EBNF 新增 `cast_expression` 产生式并把 `'cast' '(' type ',' expression ')'` 加入 `unary_expression`。**修复 m0_42 deref-assign** 中 `*((int*)malloc(8))` 编译失败，向后兼容。 |
| **S5 (0.3.2 新增)** | §6.1 (修订), §6.2 (修订), §6.2.1 (新增) | **函数返回类型 infer 规则**：新增 §6.2.1 完整子节（5 小节：3 级优先级 builtin > 用户 > extern；7 上下文传播表；void 函数约束；codegen 集成伪代码；交叉引用）。§6.1 加返回类型编译期检查（3 条）；§6.2 加 §6.2.1 引用 + §11.0 引用。**修复 m0_41 abs_int 链接**：extern 符号表由 parser 解析 `extern "C"` 块时填充（commit b45f851），codegen 通过 `lookup_extern_func` 查得返回类型，规避硬编码 i32 错位。 |
| **§11.0 (0.3.2 新增)** | §11 (修订), §11.0 (新增), §11.1-§11.6 (跨引用) | **builtin 签名总表**：新增 §11.0（4 小节：16 行 builtin 表覆盖 I/O / 字符串 / 内存 / 工具 / 数学；LLVM IR 类型映射 12 行；codegen 集成 `builtin_sigs[]` 数组伪代码；添加新 builtin 流程；交叉引用）。§11 标题加 0.3.2 修订注 + §11.0 总览引用；§11.1-§11.6 各子节加 §11.0 双向引用注。`move` / `alloc` 标记为类型参数化 builtin。 |
| Rule 1 | §3.2 | Owning 类型不限堆/栈；由 init 推断存储位置。 |
| Rule 2A | §3.3 | 引用 `&` 禁止 owning；编译期报 null ref。 |
| Rule 2B | §3.2 | `T*` 由 init 决定 owning 或 non-owning；赋值不转所有权，给修改权。 |
| Q1 | §3.7 | `unique T` ≡ `T*`，可省略 `unique`。 |
| Q2 | §3.3, §12.1 | 引用声明右侧是左值；指针声明右侧是取地址。 |
| Q3 | §3.3 | **单一引用类型** `T&`：所有引用统一为 `T&`；独占性由 `#modlaw exclusive` + `mod()` 申请控制，不在类型层区分。 |
| Q4 | §3.2, §7.1 | 赋值 `p1 = p2` **不复制所有权**，**给 p1 修改权**（隐式 `mod(p1)`）。 |
| Q5 | §7.4 | `move(p)` 仍然必要，显式移交所有权。 |
| Q6 | §3.8, §7.10 | `const T*` / `T* const` 自定义语义（与 C++ 相反）；owning heap 禁止。 |
| **Rule 22** | §1.2, §3, §7 | 所有权与修改权彻底拆分。 |
| **Q3 反转** *(0.3.0 修订)* | §3.3, §7.8, §7.9 (删除), §12.1, §12.2, §2.3, §2.6 | **删除 `T&mut` 类型**：引用统一为 `T&`；独占性改由 `#modlaw exclusive` + `mod()` 申请控制，不再是类型层面区分。 |
| **Rule 23** | §9.9 | 新增 `#modlaw <perm> <scope>` 指令（6 种合法组合）。 |
| **Rule 24** | §4.9, §4.10 | 新增 `mod(ref_expr)` 和 `unmod(ref_expr)` 表达式。 |
| **Rule 25** | §13.1, §13.3 | 跨线程访问必须 `shared` 标注；mutex 编译器自动 + 程序员显式。 |
| **Rule 26** | §13.2 | `move_to_thread(p, tid)` / `spawn_thread_with(tid, p)`。 |
| **Rule 27** | §13.5 | `__thread` 线程局部存储。 |
| **Rule 28** | §13.4 | 多线程内存布局：栈（thread-local）/ 堆（shared）/ 全局（shared）/ TLS（thread-local）。 |
| **S2 (0.3.1 修订)** | §4.1, §4.6, §4.13 (新增), §7.8, §12.1, §12.3 | **lvalue / rvalue 概念明确化**：新增 §4.13 完整章节（定义 + 分类表 + 赋值上下文约束 + codegen 实现约束 + 交叉引用）；§4.1 优先级表补 2.5 级一元 op（` * ` ` & ` ` + ` ` - ` ` ! ` ` ~ ` `mod` `unmod` 前缀 `++` `--`）；§4.6 表格 11 行统一加「LHS 必须是 lvalue (§4.13.2)」约束；§7.8 创建规则 1 引用 §4.13.2 + 加合法/非法示例；§12.1 EBNF 加 `lvalue` / `rvalue` 规则 + `assignment_expression` LHS 标注；§12.3 附录优先级表同步加 2.5 级。**修复 m0_42 deref-assign bug**（`*view = payload`）。无新语法、无语义变更、向后兼容。 |
| **S1 (0.3.3 修订)** | §4.1, §4.9, §4.10, §12.3 | **运算符优先级表完整化 + mod/unmod/move/clone 函数化**：§4.1 优先级表显式区分 2 级（后缀，左到右）与 2.5 级（一元前缀，右到左），列 `*` `&` `+` `-` `!` `~` `++` `--`；删除原 15 级 `move clone`（它们是 builtin 函数，不是运算符）；改注脚说明 `mod` / `unmod` 是 builtin 函数，`&` 的 2.5 级（addr-of）与 8 级（按位与）同名异义。§4.9 / §4.10 加 §7.0 + §11.0.1 双向引用；明确 `mod` / `unmod` 现在 emit `@uc_borrow_mod_{enter,exit}` IR marker（per runtime-architecture §3.2），不再是「编译期决策、无 runtime」。§12.3 附录优先级表同步修订。 |
| **S3 (0.3.3 新增)** | §4.13.5 (新增) | **deref 语义边界**：新增 §4.13.5 完整子节（6 小节）：§4.13.5.1 基本 deref `*p`（lvalue 类型）；§4.13.5.2 复合 deref 形式（`&*p` / `*&x` 恒等、`**pp` 多级、`p->field` ≡ `(*p).field`）；§4.13.5.3 与 §3.5 指针类型关系；§4.13.5.4 优先级（2.5 级 vs 3 级同名异义）；§4.13.5.5 错误情况（`InvalidDereferenceError`）；§4.13.5.6 交叉引用。补全 0.3.1 S2 lvalue / rvalue 框架下 deref 总是 lvalue 的语义边界。 |
| **§7.0 (0.3.3 新增)** | §7 (修订), §7.0 (新增) | **概念清晰化：运算符 vs builtin 函数**：新增 §7.0 完整子节（5 小节）：§7.0.1 运算符（§4.1 优先级表）；§7.0.2 builtin 函数（§11.0.1 签名总表 + 4 项 intrinsic + §11.0.2 UltraCPP stdlib 路径 `lib/*.uc`）；§7.0.3 关键澄清（`mod` `unmod` `move` `clone` 是 builtin 函数，**不是**运算符；`mod` / `unmod` 是 builtin emit IR marker，不是「编译期决策」）；§7.0.4 历史回顾（0.2.0 → 0.3.3 演进表）；§7.0.5 交叉引用（11 章节）。修复 §4.1 与 §4.9/§4.10 内部矛盾。 |
| **§11.0 双分类 (0.3.3 修订, runtime-architecture 集成)** | §11 (修订), §11.0 (重写), §11.0.1 (新增), §11.0.2 (新增), §11.0.3 (修订), §11.0.4 (修订), §11.0.5 (修订) | **builtin 签名表双分类**：§11.0.1 **builtin 语言原语（6 项）** — `alloc` / `free` / `move` / `clone` / `mod` / `unmod`（**全部**进 `BUILTIN_SIGS[]`，**全部** codegen emit；`null` 退到 intrinsic）。§11.0.2 **UltraCPP stdlib 路径** — 0.3.2 原 14 项 stdlib 全部从 BUILTIN_SIGS[] 移除，改由 UltraCPP 写在 `lib/*.uc`（per `.dev/drafts/0.3.0-runtime-architecture.md` §3.1 + §5.3）：`lib/print.uc` / `lib/string.uc` / `lib/memory.uc` / `lib/math.uc` / `lib/alloc.uc` / `lib/sys.uc`。§11.0.3 codegen `BUILTIN_SIGS[]` 简化为 6 项（无 `is_type_parametric` 字段；stdlib 不进；`mod` / `unmod` 占位签名删除）。§11.0.4 添加新 builtin 流程（新增 intrinsic 路径）。§11.0.5 交叉引用加 §10.2 / §10.3 / §10.4。`free` 从 FFI 提升为 builtin（per runtime-arch §3.2）；`mod` / `unmod` 从 intrinsic 提升为 builtin（per runtime-arch §3.2）。 |
| **§10.2 / §10.3 / §10.4 (0.3.3 新增, runtime-architecture 集成)** | §10.2 (新增), §10.3 (新增), §10.4 (新增), §12.1 (5 个新产生式) | **跨平台宏 + 内联汇编 + sys:: 命名空间**：§10.2 **预编译宏机制**（C 风格 `@ifdef(MACRO)` / `@if defined()` / `@else` / `@end` / `@define` / `@error` / `@warning`）；预定义宏表（`TARGET_OS_LINUX` / `TARGET_ARCH_X86_64` 等）。§10.3 **内联汇编机制**（GCC 约束风格 `asm { "syscall" : "=a"(r) : "0"(n) : "cc" }`）。§10.4 **`sys::` 系统调用 namespace**（跨平台 API：`sys::read` / `sys::write` / `sys::open` / `sys::close` / `sys::mmap` / `sys::exit` / `sys::getpid` / `sys::raw_syscall` 等；目录组织 `lib/sys/`）。§12.1 EBNF 加 `preprocessor_directive` / `asm_block` / `qualified_call` 3 个新产生式。 |
| **§7.4 / §7.5 (0.3.3 修订)** | §7.4 (修订), §7.5 (修订) | **`move` / `clone` 函数化 + codegen 路径说明**：明确 `move` (§7.4) / `clone` (§7.5) 是 builtin 函数（§11.0.1），不是运算符（§4.1 优先级表 15 级已删除）。加 codegen emit 路径说明：`move(p)` 不走普通函数 emit，通过 `UCExprCall.ownership_xfer = MOVE` + IR 标记实现（约 30-50 LOC `emit_move_call`）；`clone(s)` 通过 alloc + LLVMBuildMemcpy 实现（约 50-80 LOC `emit_clone_call`）。概念分类见 §7.0.2 + §11.0.1。
| **§4.1 / §12.3 (0.3.3 修订)** | §4.1 (修订), §12.3 (修订) | **优先级表主表 + 附录表同步**：§4.1 主表 + §12.3 附录表加 2.5 级（显式标注一元前缀右到左），删 15 级 `move` / `clone` / `move_to_thread`，删 `mod` / `unmod` 一元运算符表行。`&` 在 2.5 级（addr-of）与 8 级（按位与）同名异义。
| **§12.1 (0.3.3 修订, runtime-architecture 集成)** | §12.1 (修订) | **EBNF 5 个新产生式**：① `builtin_function_call`（6 项语言原语 only：`alloc` / `free` / `move` / `clone` / `mod` / `unmod`；stdlib 完全移出）；② `intrinsic_call`（4 项：`null` / `is_null` / `sizeof` / `alignof`，per runtime-arch §5.2，不进 BUILTIN_SIGS[]）；③ `preprocessor_directive`（§10.2 C 风格 `@ifdef`）；④ `asm_block`（§10.3 GCC 约束风格）；⑤ `qualified_call`（§10.4 `sys::` namespace）。`mod` / `unmod` / `move` / `clone` 从 `unary_expression` 移到 `builtin_function_call`。 |
| **§7.8 (0.3.3 修订)** | §7.8 (修订) | **引用 + deref 边界交叉引用**：加 §4.13.5 deref 边界引用 + §7.0 概念分类引用；明确 `&` 既是一元运算符（2.5 级，引用类型）也是 §11.0.1 builtin 函数（`mod` / `unmod` / `move` / `clone`）的接受者。 |
| **§4.9 / §4.10 (0.3.3 修订)** | §4.9 (修订), §4.10 (修订) | **`mod` / `unmod` 函数化 + IR marker 说明**：明确 `mod` / `unmod` 是 builtin 函数（§11.0.1），**不是**运算符（§4.1 已删除 `mod` / `unmod` 一元位置）。调用形式始终是 `mod(ref)` / `unmod(ref)`。加 codegen emit 说明：`mod(ref)` call `@uc_borrow_mod_enter(%ref)` IR marker；`unmod(ref)` call `@uc_borrow_mod_exit(%ref)` IR marker；借用检查器同步维护 mod 状态机（per runtime-architecture §3.2）。 |

---

## 1. 概述

### 1.1 语言目标

UltraCPP 是一种系统级编程语言，将 **C++ 语法的熟悉度** 与 **Rust 风格的内存安全保证** 相结合，无需垃圾回收器。

**核心目标：**
- 零成本抽象
- 编译时内存安全
- C++ 兼容性，便于迁移
- 无垃圾回收器

### 1.2 设计原则（重点：两条权限独立）

1. **默认安全**：在可能的情况下，编译时强制内存安全
2. **显式优于隐式**：所有权、可变性和安全语义在语法中可见
3. **务实兼容**：利用 C++ 开发者的熟悉度
4. **最小运行时**：无重运行时，适用于系统编程
5. **两条权限独立** *(0.3.0 新增：Rule 22)*：
   - **所有权（ownership）**：决定谁负责 `delete` / `free`；**唯一所有者**；可转移（`move(p)`）；不可复制。
   - **修改权（modification right / `mod`）**：决定谁能通过引用**改值**；可多可少可独占；由程序员按需申请。

> **[0.3.0 · Rule 22]** 这是 0.3.0 最重要的概念变更。0.2.0 的「所有权」概念实际上混入了修改权，0.3.0 把两者拆开。举例：`int& r = x;`（共享引用）默认**有**所有权借阅权（C++ 风格的引用），但要改 `x`，需要**额外**调用 `mod(r)` 申请修改权（潜在可写引用语义见 §3.3、§4.9）。

### 1.3 符号约定

| 符号 | 含义 |
|------|------|
| `T` | owned 值（栈 / 全局 / TLS） |
| `unique T` ≡ `T*` | owning 指针（泛型形式，见 §3.7） |
| `T*` | 指针：右侧是 `&b` → 非 owning 栈/全局指针；右侧是 `alloc(T)` → owning 指针 |
| `T&` | **唯一引用类型**：必须初始化、不为 null、不 owns；可写（需 `mod()`）；多个可并存；线程可共享 |
| `T* p = &x` | p 是 non-owning 栈/全局指针（与引用规则类似） |
| `T* p = alloc(T)` | p 是 owning 指针 |
| `&expr` | 创建对 `expr` 的 `T&` 引用 |
| `mod(ref_expr)` | 申请修改权（按 `#modlaw` 策略） |
| `unmod(ref_expr)` | 释放修改权（一般省略，靠作用域） |
| `move(p)` | 移交所有权（p 失效） |
| `move_to_thread(p, tid)` | 跨线程移交所有权 |
| `shared` | 跨线程可见标注（编译期强制） |
| `__thread T x` | 线程局部存储（每线程独立） |
| `#modlaw <perm> <scope>` | 策略指令（6 种合法组合） |
| `const T*` | **指针锁定**（不能 rebind）+ 生命周期安全 + 通过它不能写（自定义，**与 C++ 相反**） |
| `T* const` | **数据只读视图**（可 rebind，但通过它不能写） |
| `alloc(T)` | 为类型 T 分配堆内存 |
| `free(ptr)` | 释放堆内存 |
| `clone(ptr)` | 克隆指针（复制一份，独立所有权） |

---

## 2. 词法结构

### 2.1 源文件约定

```
*.uc   — UltraCPP 源文件（推荐）
*.upp  — UltraCPP 源文件（兼容）
```

### 2.2 记号类型

| 类别 | 示例 |
|------|------|
| **关键字** | `if`, `else`, `while`, `for`, `return`, `struct`, `export`, `import`, `const`, `unique`, `move`, `free`, `alloc`, `null`, `true`, `false`, `void`, `extern`, `unsafe`, `typedef`, `clone`, `mod`, `unmod`, `shared`, `__thread`, `move_to_thread` |
| **标识符** | `foo`, `myVariable`, `_private`, `CamelCase` |
| **字面量** | `42`, `3.14`, `'x'`, `"hello"`, `true`, `false` |
| **运算符** | `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`, `&`, `|`, `^`, `~`, `<<`, `>>`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=` |
| **分隔符** | `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `::` |
| **预处理器** | `#import`, `#include`, `#ifdef`, `#ifndef`, `#endif`, `#define`, `#modlaw` *(0.3.0 新增)* |

> **[0.3.0 · Rule 23, 24, 26, 27]** 新增关键字：`mod`、`unmod`、`shared`、`__thread`、`move_to_thread`。新增预处理器指令：`#modlaw`。

### 2.3 关键字（保留字）

```
if          else        while       for         return
struct      export      import      const       typedef
unique      move        free        alloc       null
true        false       void        extern      unsafe
as          static      clone
mod         unmod       shared      __thread    move_to_thread
```

> **[0.3.0 新增]** `mod`、`unmod`、`shared`、`__thread`、`move_to_thread`。详见 §4.9、§4.10、§13。

### 2.4 标识符规则

```
identifier ::= letter (letter | digit)*
letter     ::= 'a'..'z' | 'A'..'Z' | '_'
digit      ::= '0'..'9'
```

- 标识符大小写敏感
- 无长度限制（由实现定义）
- 不得与关键字冲突

**标识符字符仅限 ASCII 字母与下划线 `_`。非 ASCII 标识符（含 CJK / UTF-8）永久不支持（永久 NO-OP）—— 见 spec gap S7（NO-OP，2026-08-21 user 决策关闭）。**

### 2.5 字面量

#### 整数字面量
```
int-literal ::= decimal | hex | octal | binary
decimal     ::= [1-9][0-9]*
hex         ::= '0x'[0-9a-fA-F]+
octal       ::= '0'[0-7]+
binary      ::= '0b'[01]+
```

#### 浮点字面量
```
float-literal ::= [0-9]+'.'[0-9]+ | '.'[0-9]+ | [0-9]+'.'
```

#### 字符字面量
```
char-literal ::= "'" character "'"
```

#### 字符串字面量
```
string-literal ::= '"' (character | escape)* '"'
escape         ::= '\n' | '\t' | '\r' | '\\' | '\'' | '\"' | '\x'[0-9a-fA-F]{2}
```

#### 布尔字面量
```
true  // 布尔真
false // 布尔假
```

### 2.6 运算符和分隔符

| 运算符 | 描述 |
|--------|------|
| `+` | 加法 |
| `-` | 减法/取负 |
| `*` | 乘法/解引用 |
| `/` | 除法 |
| `%` | 取模 |
| `=` | 赋值（`T*` 之间触发**修改权转让**而非所有权 transfer，见 §7.1） |
| `==` | 相等 |
| `!=` | 不等 |
| `<` | 小于 |
| `>` | 大于 |
| `<=` | 小于等于 |
| `>=` | 大于等于 |
| `&&` | 逻辑与 |
| `\|\|` | 逻辑或 |
| `!` | 逻辑非 |
| `&` | 一元前缀：创建 `T&` 引用（见 §3.3）；二元中缀：按位与 |
| `\|` | 按位或 |
| `^` | 按位异或 |
| `~` | 按位非 |
| `<<` | 左移 |
| `>>` | 右移 |
| `++` | 递增 |
| `--` | 递减 |
| `->` | 箭头（指针成员访问） |
| `.` | 点号（成员访问） |
| `::` | 作用域解析 |

> **[0.3.0 修订]** 一元前缀 `&` 创建唯一引用类型 `T&`。是否实际可写由 `mod()` / `#modlaw` 决定（见 Rule 24）。0.3.0 不再区分「共享可写」与「独占可写」引用——`T&mut` 已删除。

### 2.7 注释

```cpp
// 单行注释

/*
 * 多行注释
 */
```

### 2.8 空白符

空格、制表符和换行符在分隔记号时被忽略。行尾保留用于错误报告。

---

## 3. 类型系统 *(0.3.0 重写)*

### 3.1 基本类型

| 类型 | 描述 | 大小 |
|------|------|------|
| `void` | 无值 | - |
| `bool` | 布尔 | 1 字节 |
| `char` | 字符 | 1 字节 |
| `int` | 有符号整数 | 4 字节 |
| `i8` | 8 位有符号 | 1 字节 |
| `i16` | 16 位有符号 | 2 字节 |
| `i32` | 32 位有符号 | 4 字节 |
| `i64` | 64 位有符号 | 8 字节 |
| `uint` | 无符号整数 | 4 字节 |
| `u8` | 8 位无符号 | 1 字节 |
| `u16` | 16 位无符号 | 2 字节 |
| `u32` | 32 位无符号 | 4 字节 |
| `u64` | 64 位无符号 | 8 字节 |
| `f32` | 32 位浮点 | 4 字节 |
| `f64` | 64 位双精度 | 8 字节 |
| `usize` | 无符号大小 | 平台相关 |
| `isize` | 有符号大小 | 平台相关 |

### 3.2 指针类型 *(0.3.0 改写：Rule 2B, Q4)*

**`T*` 由初始化表达式区分 owning 与 non-owning**（Rule 2B）。这是 0.3.0 与 0.2.0 在指针语义上的核心差异。

```cpp
T*              // 指针：右侧 &x → 非 owning 栈/全局指针；右侧 alloc(T) → owning 堆指针
T*              // 任何时刻唯一 owner；赋值 p1 = p2 不复制所有权，给 p1 修改权（隐式 mod(p1)）
```

**示例（必须区分两种形式）：**

```cpp
int x = 42;

// === Non-owning 栈指针：右侧是取地址 ===
int* p = &x;        // p 指向 x，p 本身不 owns x
*p;                 // ✅ 读
// *p = 100;        // ❌ 编译错：p 默认无 mod 权（需 mod(p) 申请，见 §4.9）
int* p2 = &x;       // ✅ 多 non-owning 指针可并存

// === Owning 堆指针：右侧是 alloc ===
int* h = alloc(int);   // h owns
*h = 100;              // ✅ h 默认拥有修改权（owning 指针 "mod-by-default"）
int* h2 = alloc(int);
int* h3 = h2;          // ⚠️ 隐式 mod(h3)：h3 获得修改权，h2 仍 owns
                      //    所有权并未 transfer；h3 不 own，所以不能 free(h3)
free(h2);              // ✅ h2 是 owner
```

**示例 A（规范声明语法，Q2）：**

```cpp
int x = 42;

// ✓ 引用声明：右侧是左值
int& r = x;       // r 是 x 的引用（潜在可写，C++ 风格）

// ✓ 指针声明：右侧是取地址
int* p = &x;      // p 是 x 的地址（非 owning）

// ✗ 类型不兼容（必须作为反例出现）
// int& r2 = &x;   // 错：T& 不能接 T*
// int* p2 = x;    // 错：T* 不能接 T
```

**关键规则（Rule 2B）：**

1. **由初始化表达式决定**：`T* p = &x` → p 不 own x；`T* p = alloc(T)` → p owns。
2. **任何时刻唯一 owner**：一个 allocated 对象最多有一个 owning 指针（`unique T` 等价于 `T*`）。
3. **赋值 `p1 = p2` 不复制所有权**，**给 p1 修改权**（路线 2：隐式 `mod(p1)`），owner 不变（Q4）。
4. **`move(p)` 显式**移交所有权（Q5）。
5. **`readonly` / `const T*` / `T* const` 控制只读权限**，见 §3.8 与 §7.10（**与 C++ 相反的语义**）。

### 3.3 引用类型 *(0.3.0 重写：删除 T&mut，Q2, Rule 22)*

UltraCPP 只有一个引用类型 `T&`——**必须初始化**、**不能为 null**、**不拥有**被指向的值，所有权始终留在 owner 处。

> **[0.3.0 · Rule 22 + Q3 反转]** 0.2.0 把引用分为 `T&`（不可变）和 `T&mut`（独占可变）两种。0.3.0 **删除 `T&mut`**，统一为单一引用类型 `T&`。**独占性**不再由类型区分，而由 `#modlaw exclusive` 策略 + `mod()` 申请控制。是否实际可写也由 `mod()` 与 `#modlaw` 决定。

```cpp
T&     // 潜在可写引用（latently-writable reference）：默认无 mod，需 mod() 才能写
```

**声明语法（Q2）：**

```cpp
int x = 42;
int& r = x;          // 引用声明：右侧是左值（不写 &）
int& r2 = x;         // ✅ 多个 T& 可并存
// int& r3 = &x;      // ❌ 错：T& 不能接 T*
// int& r4 = 42;      // ❌ 错：T& 右侧必须是左值
```

**可写性（Rule 24）：**

```cpp
int x = 42;
int& r = x;          // r 是 x 的引用，默认无 mod
int v = r;           // ✅ 读
// r = 100;          // ❌ 编译错：未申请 mod 权
mod(r);              // ✅ 申请修改权（按 #modlaw 决定行为）
*r = 100;            // ✅ 写入
```

**对比表（0.1.0 / 0.2.0 / 0.3.0）：**

| 版本 | 引用类型 | 独占机制 | 写权限 |
|---|---|---|---|
| 0.1.0 | T& (C++ 风格可变引用) | 无 | 直接写 |
| 0.2.0 | T& (不可变) + T&mut (独占可变) | 类型层面 | T& 只读；T&mut 独占写 |
| 0.3.0 | **T& 唯一** | `#modlaw exclusive` + `mod()` | 需 mod()，按 #modlaw 决定行为 |

### 3.4 函数指针类型

```cpp
int (*)(int, int)       // 函数类型：接受两个 int，返回 int（C++ 风格）
void (*)(const char*)   // 返回 void 的函数指针
```

**示例：**
```cpp
typedef int (*Comparator)(int, int);
typedef void (*Callback)(const char*);

Comparator cmp;
Callback cb;
```

### 3.5 数组类型

```cpp
T[n]  // 包含 n 个 T 类型元素的定长数组
```

**示例：**
```cpp
int[10] arr;           // 10 个 int 的数组
char[256] buffer;      // 256 字节的缓冲区
int[3] nums = [1, 2, 3];  // 初始化数组
```

### 3.6 结构体类型

```cpp
struct Point {
    double x;
    double y;
}

struct Rectangle {
    Point origin;
    double width;
    double height;
}
```

### 3.7 类型修饰符（unique T ≡ T*） *(Q1, 沿用 0.2.0)*

| 修饰符 | 含义 |
|--------|------|
| `const` | 值不可修改（与 `#modlaw` 配合，自定义语义见 §3.8） |
| `unique` | **泛型类型构造器**：`unique T` ≡ `T*`（Q1，可省略） |
| `static` | 内部链接 |
| `shared` | 跨线程可见标注（编译期强制，否则编译错，见 §13.1） |

```cpp
unique int      x1;   // ≡ int*
unique char     x2;   // ≡ char*
unique Point    x3;   // 用户定义 struct 同样合法
unique int*     x4;   // T 本身是指针类型时也合法
unique int[10]  x5;   // T 为数组类型
```

**规则（沿用 0.2.0 + Q1）：**

1. `unique T` 中的 `T` 可以是 §3.1–§3.6 的**任意类型**。
2. `unique T` 与 `T*` 在类型系统中**等价**；`unique` 可省略。
3. `unique` 不能作用于借用类型：`unique T&` 是**非法**的——借用不拥有值。
4. `unique` **不能写成** `shared unique T*`——`shared` 是运行时存储类（与 `__thread` 同级），不是类型构造器。

### 3.8 指针修饰符与所有权边界（Q6 自定义 const T* / T* const） *(0.3.0 新增)*

> **[0.3.0 · Q6 自定义语义，与 C++ 相反]** 本节是 0.3.0 新增的指针修饰符规则。0.2.0 沿用了 C++ 的 `const T*` / `T* const` 含义，0.3.0 **改写**为自定义语义。

| 声明 | 含义 | 可 rebind | 通过它写 | owning heap 允许 |
|------|------|-----------|----------|------------------|
| `const T*` | **指针锁定**：指针本身**不能 rebind** + 生命周期安全 + 通过它不能写 | ❌ | ❌ | ❌（owning heap 禁止） |
| `T* const` | **数据只读视图**：指针**可以 rebind**，通过它不能写 | ✅ | ❌ | ❌（owning heap 禁止） |
| `const T* const` | 双重锁定（既不能 rebind，也不能写） | ❌ | ❌ | ❌ |

> **与 C++ 相反**：
>
> | 类型 | C++ 含义 | UltraCPP 含义 |
> |------|----------|---------------|
> | `const T*` | 数据不能改（指针可改） | **指针不能改**（数据也不能写） |
> | `T* const` | 指针不能改（数据可改） | **数据不能写**（指针可改） |

**示例 D（Q6 自定义语义）：**

```cpp
int x = 42;

// === const T*：指针锁定 + 生命期 + 不可写 ===
const int* p = &x;
*p;                             // ✅ 读
// *p = 100;                    // ❌ 不可写
// p = &y;                      // ❌ 指针锁定，不可 rebind

// === T* const：数据只读视图 ===
T* const r = &x;
*r;                             // ✅ 读
// *r = 100;                    // ❌ 数据只读
r = &y;                         // ✅ 可以 rebind

// === owning heap 禁止 ===
unique int* h = alloc(int);
const int* bad = h;             // ❌ 错误：owning heap 不可造只读指针
T* const bad2 = h;              // ❌ 同样禁止
```

**为什么 owning heap 禁止？** 一个 owning 指针可能在未来被 `move()`、`free()` 走；那时所有 `const T*` / `T* const` 视图都会**悬垂**。owning 指针的释放时机由程序员控制，因此指向 owning heap 的「只读视图」无法在编译期保证安全。

进一步细节（违反时的诊断、解引用语义）见 §7.10。

---

## 4. 表达式 *(0.3.0 新增 §4.9, §4.10)*

### 4.1 运算符优先级和结合性 *(0.3.1 修订, 0.3.3 修订)*

| 优先级 | 运算符 | 结合性 |
|--------|--------|--------|
| 1 | `::` | 左到右 |
| 2 | `()` `[]` `.` `->` `++` `--` (后缀) | 左到右 |
| 2.5 | **一元 `*` `&` `+` `-` `!` `~` `++` `--` (前缀)** *(0.3.1 补, 0.3.3 删 `mod`/`unmod`)* | **右到左** |
| 2.5 | `(`*type*`)` | **C 风格强制类型转换** *(0.3.2 补)* |
| 3 | `*` `/` `%` (二元) | 左到右 |
| 4 | `+` `-` (二元) | 左到右 |
| 5 | `<<` `>>` (二元) | 左到右 |
| 6 | `<` `>` `<=` `>=` | 左到右 |
| 7 | `==` `!=` | 左到右 |
| 8 | `&` (二元,按位与) | 左到右 |
| 9 | `^` | 左到右 |
| 10 | `\|` | 左到右 |
| 11 | `&&` | 左到右 |
| 12 | `\|\|` | 左到右 |
| 13 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | 右到左 |
| 14 | `?:` （三元条件） | 右到左 |

> **[0.3.0]** 表中第 8 级的 `&` 指**二元中缀**按位与。一元前缀的 `&`（引用）属于一元运算符，详见 §3.3、§7.8。
> **[0.3.1 补]** 一元 `*`（解引用）见 §4.13.5；二元 `*`（乘法）见 §4.2。
> **[0.3.3 修订]** 表中第 8 级的 `&` 指**二元中缀**按位与。一元前缀的 `&`（引用）在 2.5 级。
>
> **`mod` `unmod` `move` `clone` 不在运算符优先级表** — 它们是 **builtin 函数调用**（`mod(ref)` / `unmod(ref)` 见 §4.9–§4.10；`move(p)` / `clone(s)` 见 §7.4–§7.5），与 `alloc` / `free` 同级（§11.0.1 builtin）。**完整概念分类**见 §7.0 运算符 vs builtin 函数。
>
> **0.3.3 删除**原 15 级 `move clone`（它们是 builtin 函数，不是运算符）。**0.3.3 删除** 2.5 级中的 `mod` / `unmod`（同理）。

### 4.2 算术运算符

| 运算符 | 描述 | 示例 |
|--------|------|------|
| `+` | 加法 | `a + b` |
| `-` | 减法 | `a - b` |
| `*` | 乘法 | `a * b` |
| `/` | 除法 | `a / b` |
| `%` | 取模 | `a % b` |

### 4.3 比较运算符

| 运算符 | 描述 | 示例 |
|--------|------|------|
| `==` | 相等 | `a == b` |
| `!=` | 不等 | `a != b` |
| `<` | 小于 | `a < b` |
| `>` | 大于 | `a > b` |
| `<=` | 小于等于 | `a <= b` |
| `>=` | 大于等于 | `a >= b` |

### 4.4 逻辑运算符

| 运算符 | 描述 | 示例 |
|--------|------|------|
| `&&` | 逻辑与 | `a && b` |
| `\|\|` | 逻辑或 | `a \|\| b` |
| `!` | 逻辑非 | `!a` |

### 4.5 位运算符

> **[0.3.0]** 本表仅描述**二元中缀**形式。一元前缀 `&` 不是位运算符，而是引用（见 §3.3）。

| 运算符 | 描述 | 示例 |
|--------|------|------|
| `&` | 按位与（二元中缀） | `a & b` |
| `\|` | 按位或 | `a \| b` |
| `^` | 按位异或 | `a ^ b` |
| `~` | 按位非 | `~a` |
| `<<` | 左移 | `a << 2` |
| `>>` | 右移 | `a >> 2` |

### 4.6 赋值运算符 *(0.3.1 修订)*

> **[0.3.1]** 赋值运算符的**左侧**必须为 lvalue（见 §4.13.2）。非 lvalue 作左侧
> 是编译期错误（`AssignmentToRvalueError`）。

| 运算符 | 描述 |
|--------|------|
| `=` | 简单赋值 — LHS 必须是 lvalue (§4.13.2);RHS 求值后赋给 LHS;`T*` 之间触发**隐式 `mod()`**，不转移所有权，见 §7.1 |
| `+=` | 加法赋值 — LHS 必须是 lvalue;`LHS = LHS + RHS` |
| `-=` | 减法赋值 — LHS 必须是 lvalue;`LHS = LHS - RHS` |
| `*=` | 乘法赋值 — LHS 必须是 lvalue;`LHS = LHS * RHS` |
| `/=` | 除法赋值 — LHS 必须是 lvalue;`LHS = LHS / RHS` |
| `%=` | 取模赋值 — LHS 必须是 lvalue;`LHS = LHS % RHS` |
| `&=` | 按位与赋值 — LHS 必须是 lvalue;`LHS = LHS & RHS` |
| `\|=` | 按位或赋值 — LHS 必须是 lvalue;`LHS = LHS \| RHS` |
| `^=` | 按位异或赋值 — LHS 必须是 lvalue;`LHS = LHS ^ RHS` |
| `<<=` | 左移赋值 — LHS 必须是 lvalue;`LHS = LHS << RHS` |
| `>>=` | 右移赋值 — LHS 必须是 lvalue;`LHS = LHS >> RHS` |

**结合性**：赋值运算符是**右结合**（`a = b = c` 等价于 `a = (b = c)`，内层
`(b = c)` 整体是 lvalue，可作外层 LHS — 见 §4.13.2 表）。

**lvalue 上下文**：赋值 LHS 走 lvalue 路径（§4.13.4），RHS 走 rvalue 路径。`=` 的结果
（整个赋值表达式）是 lvalue，值为赋值后的 LHS。

### 4.7 条件运算符

```cpp
condition ? expr1 : expr2
```

### 4.8 带括号表达式 *(0.3.2 修订)*

```cpp
(expr)  // 括号中的任意表达式
```

> **[0.3.2]** 当 `(T)` 中 `T` 是**类型名**而非**表达式**时，表示 **C-style 显式类型转换** (cast)，见 §4.8.1。C-style cast 与函数式 cast `T(expr)` 完全等价 (见 §4.8.1.2)。

#### 4.8.1 C-style 显式类型转换 `(`*type*`)`*expr* *(0.3.2 新增)*

C-style cast 是 UltraCPP 中最常用的显式类型转换形式，语法与 C/C++ 一致：

```
'(' type ')' unary_expression
```

其中 `type` 见 §3 (类型系统)，`unary_expression` 见 §4.1 (运算符优先级)。

##### 4.8.1.1 语义 *(0.3.2 新增)*

`(T)expr` 将表达式 `expr` 的值转换为类型 `T`。转换规则：

| 源类型 → 目标类型 | 行为 | codegen 层 |
|-------------------|------|-----------|
| `T*` → `T*` (同类型) | ✅ no-op (仅编译期确认) | 直接传递指针值 |
| `T*` → `void*` | ✅ 隐式允许 | 直接传递指针值 |
| `void*` → `T*` | ✅ 显式 cast (程序员担保类型正确) | bitcast (`bitcast T* ... to T*`) |
| `void*` → `int` (i64) | ✅ 显式 cast | ptrtoint (`ptrtoint T* ... to i64`) |
| `int` (i32/i64) → `T*` | ✅ 显式 cast | inttoptr (`inttoptr i64 ... to T*`) |
| `int` (i32) → `int` (i64) | ✅ 符号扩展 | sext (`sext i32 ... to i64`) |
| `int` (i64) → `int` (i32) | ✅ 截断 | trunc (`trunc i64 ... to i32`) |
| `int` → `float` / `double` | ✅ (若支持) | sitofp (`sitofp i32 ... to float`) |
| 不相关类型 (e.g. `int` → `Point`) | ❌ 编译期错误 `InvalidCastError` | — |

##### 4.8.1.2 与函数式 cast `T(expr)` 的关系 *(0.3.2 新增)*

UltraCPP 同时支持两种 cast 形式：

| 形式 | 名称 | 例子 |
|------|------|------|
| `T(expr)` | 函数式 cast (functional cast) | `int(3.14)` → 3 |
| `(T)expr` | C-style cast | `(int)3.14` → 3 |

两种形式**完全等价**。程序员可任选其一。UltraCPP 推荐使用 C-style cast (与 C/C++ 生态对齐，FFI 互操作更直观)。

> **历史**：0.3.0 / 0.3.1 spec 只显式支持 `T(expr)`，`(T)expr` 缺失，导致 m0_42 中 `*((int*)malloc(8))` 形式语义不清晰。0.3.2 加入 C-style cast 后两种形式等价。

##### 4.8.1.3 示例 *(0.3.2 新增)*

```cpp
// === 整数 ↔ 指针互转 (FFI / 系统调用常用) ===
int* p = alloc(int);          // p: int*
void* vp = (void*)p;          // T* → void*: 隐式允许
int addr = (int)vp;           // void* → int: ptrtoint
int* p2 = (int*)addr;         // int → int*: inttoptr

// === 整数宽度转换 ===
int big = (int)1000000L;      // i64 → i32: trunc (语义: 取低 32 位)
long huge = (long)42;         // i32 → i64: sext

// === FFI 场景 ===
extern "C" {
    void* malloc(int size);
}

// === 类型断言 (程序员担保类型正确) ===
void* raw = malloc(8);        // malloc 通过 §10 extern "C" 引入
int* typed = (int*)raw;       // void* → int*: bitcast，程序员担保 raw 实际是 int*

int* arr = (int*)malloc(8);   // 必须 cast void* → int*
*arr = 42;                     // 通过 typed 指针写
```

##### 4.8.1.4 编译期检查 *(0.3.2 新增)*

C-style cast 的编译期检查包括：

1. **`type` 必须是已知类型** (§3 类型系统中定义的类型，包括 built-in + 用户定义)。
2. **转换必须合法** (上表 8 行允许 + 1 行不相关类型拒绝)。
3. **指针 ↔ 整数转换需显式 cast** (不能隐式 —— 由 §3.5 类型转换规则约束；0.3.2 不变)。

错误类型：`InvalidCastError` (转换不合法) / `UnknownTypeError` (type 不识别)。

##### 4.8.1.5 与其他章节的交叉引用 *(0.3.2 新增)*

| 章节 | 关系 |
|------|------|
| §3 类型系统 | cast 目标 `T` 必须是 §3 定义的类型 |
| §4.1 优先级 | C-style cast 是 §4.1 第 2.5 级一元 op (与 `*` / `&` 同级，右到左) |
| §4.8 (本节) | C-style cast 是带括号表达式的特殊形式 |
| §6.2 函数调用 | 调用结果可作 cast 源：`(int)factorial(5)` |
| §7.4 `alloc` 与 C stdlib | `malloc` 返回 `void*`，通常需要 cast 到具体类型 |
| §10 FFI | FFI 函数返回 `void*` 必 cast |
| §11.0 builtin 签名表 | builtin 函数返回类型已知 (无需 cast) |
| §12.1 EBNF | `cast_expression` 产生式见 §12.1 (0.3.2 新增) |

### 4.9 `mod()` 表达式 — 申请修改权 *(0.3.0 新增, 0.3.3 修订, Rule 24)*

> **[0.3.0 · Rule 24]** `mod(ref_expr)` 申请对**引用**所指对象的修改权。其行为由当前作用域的 `#modlaw` 策略决定。
>
> **[0.3.3 修订]** `mod` 是 **builtin 函数**（§11.0.1 builtin 语言原语），**不是**运算符（§4.1 优先级表中已删除 `mod` 一元位置）。调用形式始终是 `mod(ref)`，没有 prefix 形式。完整 builtin 签名见 §11.0.1；概念分类见 §7.0.2 builtin 函数。
>
> **`mod` 是 builtin 且 emit IR marker** — `mod(ref)` 调用 emit `call void @uc_borrow_mod_enter(%ref)` IR marker，由借用检查器同步维护 mod 状态机（Rule 24：申请中 → 已授予 → 已释放）。详见 §7.0.3 关键澄清 + §11.0.1 builtin 签名 + §11.0.3 codegen 集成。

**语法**：
```
mod '(' reference_expression ')'
```

**操作数**：`reference_expression` 必须是对引用类型 `T&` 的左值（即 `int&`、`Point&` 等）。注意：0.3.0 已删除 `T&mut` 类型，`mod()` 只接受 `T&` 引用。

**语义**（Rule 24）：行为由 `#modlaw` 当前策略决定。

| `#modlaw` 策略 | `mod(r)` 的行为 |
|----------------|------------------|
| `none` | ❌ 编译期错误：`mod() forbidden by #modlaw none policy` |
| `exclusive` | ✅ 编译期检查争用；若另一个 `T&` 已申请 mod，独占拒绝（`BorrowConflict`） |
| `shared` | ✅ 允许多个 `T&` 同时获得修改权（程序员责任处理数据竞争） |

**示例片段：**

```cpp
#modlaw shared module

int x = 42;
int& r = x;
mod(r);              // ✅ 共享策略下可申请
*r = 100;
mod(r);              // 作用域内可重复（编译器自动 unmod）
```

**隐式 `mod()`（Rule 24 + Q4）：**

赋值 `p1 = p2` 的 `T*` 版本会**自动**生成 `mod(p1)`，详见 §7.1。因此以下写法隐式成立：

```cpp
int* p1 = alloc(int);
int* p2 = p1;          // 隐式 mod(p1)：p1 获得修改权，p2 仍 owns
*p1 = 100;             // ✅ 不需显式 mod()
```

### 4.10 `unmod()` 表达式 — 释放修改权 *(0.3.0 新增, 0.3.3 修订, Rule 24)*

> **[0.3.0 · Rule 24]** `unmod(ref_expr)` 释放之前由 `mod()` 获取的修改权。**一般省略**——作用域结束或最后一次使用时编译器自动 unmod。
>
> **[0.3.3 修订]** `unmod` 是 **builtin 函数**（§11.0.1 builtin 语言原语），**不是**运算符（§4.1 优先级表中已删除 `unmod` 一元位置）。调用形式始终是 `unmod(ref)`，没有 prefix 形式。完整 builtin 签名见 §11.0.1；概念分类见 §7.0.2 builtin 函数。
>
> **`unmod` 是 builtin 且 emit IR marker** — `unmod(ref)` 调用 emit `call void @uc_borrow_mod_exit(%ref)` IR marker，与 `mod` 配对，由借用检查器关闭活跃的 mod 作用域。详见 §7.0.3 关键澄清 + §11.0.1 builtin 签名 + §11.0.3 codegen 集成。

**语法**：
```
unmod '(' reference_expression ')'
```

**使用场景**（可省略时的退化）：

```cpp
mod(r);                  // 申请
*r = compute();          // 用
unmod(r);                // ✅ 想立即释放，缩短临界区（可省略）

// 下面两段等价：
#modlaw exclusive module
{
    mod(m);
    *m = 100;            // m 的最后一次使用 → 编译器自动 unmod(m)
}
*m2 = 200;               // 解锁，m2 可申请
```

---

### 4.13 表达式分类：lvalue 与 rvalue *(0.3.1 新增)*

UltraCPP 中每个表达式属于以下两类之一：**lvalue**（左值）或 **rvalue**（右值）。这一分类是
赋值、地址运算、`mod()` / `unmod()` 申请、引用绑定等核心语义的判定基础。

#### 4.13.1 定义 *(0.3.1 新增)*

**lvalue**（左值）— 标识一个对象，具有持久身份：

- 有 identity（占据明确的存储位置）
- 可作为赋值 `=` 的左侧
- 可被取地址 `&` 运算
- 在 codegen 层：走 **lvalue 路径** — emit 取地址指令（`getelementptr`、`alloca`
  引用等），**不**自动 `load` 值

**rvalue**（右值）— 表示一个临时值，无持久身份：

- 没有 identity（临时计算结果）
- **不**可作为赋值 `=` 的左侧
- **不**可被取地址
- 在 codegen 层：走 **rvalue 路径** — emit 求值指令（`load`、立即数、算术运算结果等）

#### 4.13.2 哪些表达式是 lvalue *(0.3.1 新增)*

| 表达式形式 | 类别 | 原因 |
|-----------|------|------|
| 变量名 `x` | lvalue | 标识声明对象 |
| 解引用 `*p`（p 是 `T*` 指针类型） | lvalue | 标识 p 指向的对象 |
| 字段访问 `s.field` | lvalue | 标识结构体中的成员 |
| 索引 `a[i]` | lvalue | 标识数组中的元素 |
| 引用 `T& r` 的使用 `r` | lvalue | 引用是对象的别名 |
| 前缀自增 `++x` | lvalue | 修改后的对象 |
| 前缀自减 `--x` | lvalue | 修改后的对象 |
| 赋值表达式 `x = v` | lvalue | （整个赋值表达式的结果是 LHS） |
| 函数调用 `f()` 的结果 | rvalue | 临时返回值 |
| 字面量 `42`、`"hello"` | rvalue | 立即数 |
| 算术运算 `a + b`、`a * b` | rvalue | 计算结果 |
| 比较运算 `a < b`、`a == b` | rvalue | bool 值 |
| 逻辑运算 `a && b`、`a \|\| b` | rvalue | bool 值 |
| 后缀自增 `x++` | rvalue | 表达式的值是修改**前**的旧值 |
| 后缀自减 `x--` | rvalue | 同上 |
| 取地址 `&x` 的结果 | rvalue | 返回的是指针值（虽然操作 lvalue） |
| 条件运算 `c ? a : b` | rvalue | 计算结果 |

#### 4.13.3 赋值上下文约束 *(0.3.1 新增)*

赋值运算符（`=` 及 `+=` / `-=` / `*=` 等复合赋值）的**左侧**必须为 lvalue。
**非 lvalue 作左侧**是编译期错误（`AssignmentToRvalueError`）：

```cpp
42 = x;          // ❌ 字面量是 rvalue
x + 1 = 2;       // ❌ 算术表达式是 rvalue
f() = x;         // ❌ 函数调用结果是 rvalue
x++ = 1;         // ❌ 后缀自增结果是 rvalue

*x = v;          // ✅ 解引用是 lvalue（§4.13.2 表第 2 行）
s.field = v;     // ✅ 字段访问是 lvalue
a[i] = v;        // ✅ 索引是 lvalue
++x = 1;         // ✅ 前缀自增是 lvalue
x = y = z;       // ✅ 右结合，内层 `y = z` 整体是 lvalue 作为外层 LHS
```

复合赋值（`+=` / `-=` 等）的左侧同样必须为 lvalue，语义约束同 `=`。

#### 4.13.4 codegen 实现约束 *(0.3.1 新增)*

对 lvalue 表达式，调用 `gen_expr` 在不同上下文有不同行为。UltraCPP 编译器在 codegen
阶段对每个表达式维护两个上下文信息：

1. **value category**（lvalue / rvalue）— 由本节定义
2. **concrete type** — 具体类型（如 `int`、`int*`、`Point`）

| 调用上下文 | lvalue 表达式 codegen 行为 |
|-----------|--------------------------|
| 赋值 `=` 的 LHS（`UC_EXPR_ASSIGN.target`） | 走 lvalue 路径：emit 取地址（`getelementptr`、`alloca` 引用） |
| 取地址 `&x` 的操作数 | 走 lvalue 路径：emit 栈 / 全局 / 字段地址 |
| 表达式语句、函数实参、`=` 的 RHS、子表达式 | 走 rvalue 路径：emit `load` 或值复制 |

例：

```c
int x = 42;
int* p = &x;
*p = v;           // LHS: lvalue 路径 → emit gep,store v
y = *p;           // RHS: rvalue 路径 → emit load
int z = *p + 1;   // RHS: rvalue 路径 → emit load + add
&x;               // 取地址操作数: lvalue 路径 → emit 栈地址
```

**与 C/C++ 对比**：UltraCPP 的 lvalue / rvalue 二元分类与 C/C++ 一致（参见 K&R §A7.1、
C++17 [basic.lval]）。但 UltraCPP **不**区分 C++11 引入的 xvalue（eXpiring value）、
prvalue（pure rvalue）、glvalue（generalized lvalue）三分法——UltraCPP 只分两类，简单清晰。

#### 4.13.5 deref 语义 *(0.3.3 新增)*

UltraCPP 中 deref 操作符 `*`（一元前缀）的完整语义。本节补充 §4.13.2 表格第 2 行「解引用 `*p` 是 lvalue」的详细边界。

##### 4.13.5.1 基本 deref `*p`（p 是 `T*`） *(0.3.3 新增)*

```cpp
T* p = alloc(T);   // p: T*
T v = *p;          // 解引用：读 p 指向的对象
*p = new_val;      // 解引用：写 p 指向的对象
```

- **类型**：`T`（p 的目标类型）
- **类别**：**lvalue**（见 §4.13.2 lvalue 类别表 — deref 总是产生 lvalue，因为它表示内存位置）
- **含义**：访问 p 指向的对象
- **codegen**：emit `load T, T* p_slot`（读）或 `store T new_val, T* p_slot`（写）

##### 4.13.5.2 复合 deref 形式 *(0.3.3 新增)*

**`&*p`**（取地址后 deref）：

```cpp
T* p = alloc(T);
T** pp = &*p;  // 等价于 pp = p
```

- **等价**：`&*p ≡ p`（恒等）
- **codegen**：编译器优化，不 emit load + gep，直接返回 p 的 SSA 值

**`*&x`**（x 是 lvalue 时）：

```cpp
int x = 42;
int v = *&x;  // 等价于 v = x
```

- **前提**：`x` 必须是 lvalue（有内存地址）
- **等价**：`*&x ≡ x`（恒等）
- **codegen**：编译器优化，不 emit gep + load，直接返回 x 的 SSA 值

**多级 deref `**pp`**（pp 是 `T**`）：

```cpp
T** pp = alloc(T*);   // 二级指针
*pp = alloc(T);        // *pp: lvalue, 类型 T*
T* p = *pp;            // 读
**pp = new_val;        // 多级 deref + 写
```

- **类型**：`T`（剥皮 pp 的两层指针）
- **类别**：**lvalue**
- **codegen**：emit 两次 `load T*, T** pp_slot` + `load T, T* inner_slot`（读）；或 `getelementptr` + `store`（写）

**`p->field`**（deref + 字段访问）：

```cpp
struct Point { int x; int y; };
Point* p = alloc(Point);
int v = p->x;          // ≡ (*p).x
p->y = 100;            // ≡ (*p).y = 100
```

- **等价**：`p->field ≡ (*p).field`（§4.13.5.2 与 C/C++ 一致）
- **codegen**：emit `getelementptr` + `load` / `store`

##### 4.13.5.3 与 §3.5 指针类型的关系 *(0.3.3 新增)*

`p->field` 和 `(*p).field` 涉及 struct 字段访问：

- struct 字段由 §3.7 定义
- `Point* p` 通过 deref 得到 `Point` lvalue，再用 `.field` 访问
- `p->field` 是语法糖，codegen 与 `(*p).field` 等价

##### 4.13.5.4 优先级 *(0.3.3 新增)*

`*`（deref）作为一元前缀在 §4.1 的 **2.5 级**，**右到左**。

与算术 `*`（3 级，**左到右**）不同 — 语法上通过**操作数位置**区分：

```cpp
int a = *p;     // 2.5 级: * 是 deref, 一元前缀
int b = a * b;  // 3 级: * 是乘法, 二元中缀
int c = **pp;   // 2.5 级: 两个 deref, 右到左, 等价于 *(*pp)
```

##### 4.13.5.5 错误情况 *(0.3.3 新增)*

| 错误 | 错误类型 | 触发 |
|------|----------|------|
| `*p` 中 `p` 不是指针类型 | `InvalidDereferenceError` | e.g. `*42`（int 不能 deref） |
| `**pp` 中 `pp` 不是 `T**` | `InvalidDereferenceError` | e.g. `**42` |
| `*null` | 编译期合法，运行时 segfault | `null` 是 `T*`，可 deref，但 runtime 触发段错误 |

##### 4.13.5.6 与其他章节的交叉引用 *(0.3.3 新增)*

| 章节 | 关系 |
|------|------|
| §3.5 指针类型 | `p` 必须是指针类型才能 deref |
| §3.7 struct 字段 | `p->field` 访问 struct 字段 |
| §4.1 优先级表 | deref 在 2.5 级（一元前缀） |
| §4.13.2 lvalue 类别 | deref 结果是 lvalue |
| §4.13.3 rvalue | deref 不产生 rvalue（有内存位置） |
| §4.13.4 codegen 路径 | deref 走 lvalue 路径（load / store） |
| §7.10 自定义指针 | `const T*` deref 仍是 lvalue（但读受限） |
| §12.1 EBNF | `unary_expression` 中 `*` 产生式 |

#### 4.13.6 与其他章节的交叉引用 *(0.3.1 新增, 0.3.3 修订)*

| 章节 | 关系 |
|------|------|
| §4.1 优先级表 | 一元 `*` / `&` / `++` / `--` 见 §4.1 第 2.5 级（0.3.3 删 `mod` / `unmod`） |
| §4.6 赋值运算符 | "LHS 必须是 lvalue" 引用 §4.13.2 分类表 |
| §4.9 `mod()` | 操作数 lvalue 要求引用 §4.13.2（0.3.3 修订：mod 是 builtin 函数，见 §11.0.1） |
| §4.10 `unmod()` | 操作数 lvalue 要求引用 §4.13.2（0.3.3 修订：unmod 是 builtin 函数，见 §11.0.1） |
| §4.13.5 deref 语义 | deref 总是 lvalue 的完整边界见 §4.13.5 |
| §7.1 所有权 + 隐式 mod | 隐式 `mod()` 目标是 lvalue，引用 §4.13.2 |
| §7.8 引用 `&T` | "操作数必须是左值" 引用 §4.13.2 |
| §12.1 EBNF | `lvalue` / `rvalue` 规则见 §12.1 *(0.3.1 新增)*；`unary_expression` 含 `*` / `&` 产生式见 §12.1（0.3.3 修订 `builtin_function_call`） |

---

## 5. 语句

### 5.1 表达式语句

```cpp
x + y;        // 求值并丢弃
func(10);     // 函数调用
```

### 5.2 复合语句（块）

```cpp
{
    int x = 10;
    int y = 20;
    x = x + y;
}
```

### 5.3 if 语句

```cpp
if (condition) {
    // code
}

if (condition) {
    // code
} else {
    // code
}

if (a > b) {
    // code
} else if (a == b) {
    // code
} else {
    // code
}
```

### 5.4 while 语句

```cpp
while (condition) {
    // code
}
```

### 5.5 for 语句

```cpp
for (int i = 0; i < 10; i++) {
    // code
}

for (int x : array) {
    // code
}
```

### 5.6 return 语句

```cpp
return;              // 返回 void
return 42;           // 返回值
return x + y;        // 返回表达式结果
```

### 5.7 break 语句

```cpp
while (true) {
    if (condition) {
        break;       // 退出循环
    }
}
```

### 5.8 continue 语句

```cpp
for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;    // 跳过迭代
    }
}
```

### 5.9 free 语句

```cpp
int* p = alloc(int);
*p = 42;
free(p);  // 释放内存
```

### 5.10 声明语句

```cpp
int x;                // 变量声明
int x = 42;           // 带初始化器的声明
const int y = 100;    // 常量
int* p = alloc(int);  // owning 指针分配
int* p2 = &x;         // non-owning 指针（右侧 &x）
int& r = x;           // 唯一引用类型 T&（右侧是左值）
```

---

## 6. 函数

### 6.1 函数定义 *(0.3.2 修订)*

```cpp
int add(int a, int b) {
    return a + b;
}

void greet(const char* name) {
    print(name);
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

> **[0.3.2]** 函数定义时**必须声明返回类型** (`int` / `void` / 用户类型 / `T*` 等)。返回类型决定函数调用表达式的结果类型，见 §6.2.1。
>
> **编译期检查**：
>
> 1. 函数体所有 `return` 语句的返回值必须与声明的返回类型**匹配** (允许隐式数值拓宽，如 `int` → `long`；不允许窄化或类型无关)。
> 2. 若声明为非 `void` 返回类型，函数体**必须**至少有一个 `return` 语句 (或在控制流上保证到达末尾时已返回，编译器可放宽此检查)。
> 3. 若声明为 `void` 返回类型，函数体可有 `return;` (无值) 或省略 `return`。
>
> 错误类型：`ReturnTypeMismatchError` (return 类型不匹配)。

### 6.2 函数调用 *(0.3.2 修订)*

```cpp
int result = add(10, 20);
greet("Hello");
int fact = factorial(5);
```

> **[0.3.2]** 函数调用的**返回类型**由调用目标的**签名**决定。详见 §6.2.1 函数返回类型规则。简言之：
>
> - builtin 函数 (e.g. `print`, `abs_int`)：返回类型查 §11.0 builtin 签名总表
> - 用户函数 (e.g. `add`)：返回类型 = 函数定义中声明的返回类型 (§6.1)
> - extern "C" 函数 (e.g. `malloc`)：返回类型 = extern 声明中的返回类型 (§10)
>
> 返回类型传播规则见 §6.2.1.2 (赋值 RHS / 子表达式 / 函数实参 / cast 源 / return 值)。

#### 6.2.1 函数返回类型 *(0.3.2 新增)*

UltraCPP 中，函数调用的返回类型由以下优先级决定：

##### 6.2.1.1 返回类型确定规则 *(0.3.2 新增)*

1. 若被调用函数是 **builtin** (见 §11.0 builtin 签名表)
   → 返回类型 = §11.0 表中签名
   例：`factorial(5)` 的返回类型 = `int` (来自 §11.0 表)
       `abs_int(-5)` 的返回类型 = `int` (来自 §11.0 表)

2. 若被调用函数是**用户定义函数** (§6.1)
   → 返回类型 = 函数定义中声明的返回类型
   例：`int add(int a, int b)` → 返回类型 = `int`
       `void greet(const char* name)` → 返回类型 = `void`

3. 若被调用函数是 `extern "C"` (§10 FFI)
   → 返回类型 = 声明中的返回类型 (程序员担保)
   例：`extern "C" int abs_int(int x);` → 返回类型 = `int`

###### 6.2.1.1.1 extern 符号表的存储与填充时机 *(0.3.2 修订补充)*

§6.2.1.1 优先级 3 (extern 路径) 的实现需要 codegen 持有"extern 函数符号表"。本节明确该表的**结构**、**填充时机**和**查询接口**，避免 0.3.2 实施阶段因符号表缺失而 Bug C2 实质未修。

**1. 数据结构**：

```c
// 在 codegen 上下文（CGen / UCCodeGenerator）中新增：
typedef struct {
    char* name;            // 函数名，如 "malloc", "free"
    char* ret_type;        // LLVM IR 返回类型字符串，如 "i8*", "void"
    char** param_types;    // 参数类型数组（按声明顺序）
    int param_count;
} extern_func_sig_t;

// 全局表（编译单元级）：
extern_func_sig_t* g_extern_funcs;  // 动态数组
int g_extern_func_count;
int g_extern_func_capacity;
```

**2. 填充时机**：

- **时机**：parser 在解析 `extern "C" { ... }` 块时，**立即**把每个函数声明的（名字、返回类型、参数类型）记录到 `g_extern_funcs`。
- **触发点**：`parse_extern_decl` 函数末尾；具体位置见 `src-c/src/parser.c` 的 `extern "C"` 块解析路径。
- **若 extern 块内的声明不是函数**（例如 `extern "C" int errno;` 这种变量声明），跳过符号表记录 (codegen 走 `UC_EXPR_IDENT` 普通路径)。

**3. 查询接口**：

```c
// 在 codegen.c 中实现：
const extern_func_sig_t* lookup_extern_func(const char* name) {
    for (int i = 0; i < g_extern_func_count; i++) {
        if (strcmp(g_extern_funcs[i].name, name) == 0) {
            return &g_extern_funcs[i];
        }
    }
    return NULL;
}
```

**4. 与 UC_EXPR_CALL 的集成**：

```c
case UC_EXPR_CALL: {
    const char* fn_name = ...;
    const char* ret_type = NULL;

    // Priority 1: builtin（§11.0）
    const builtin_sig_t* bsig = lookup_builtin(fn_name);
    if (bsig) { ret_type = bsig->ret_type; }
    // Priority 2: 用户函数（§6.1）
    else {
        const char* user_ret = lookup_user_func_ret_type(fn_name);
        if (user_ret) { ret_type = user_ret; }
    }
    // Priority 3: extern（§10）—— **新增于 0.3.2**
    if (!ret_type) {
        const extern_func_sig_t* esig = lookup_extern_func(fn_name);
        if (esig) { ret_type = esig->ret_type; }
    }
    // Priority 4: fallback（仅前 3 级全 miss 时）
    if (!ret_type) { ret_type = "i32"; }

    emit_fmt_writeln(g, "%s = call %s %s(%s)", res, ret_type, fn_name, args);
    free(g->last_expr_type);
    g->last_expr_type = cgen_strdup(ret_type);
}
```

**5. void 返回类型的处理**：

- `void free(void* mem)` 的 `ret_type = "void"`
- emit `call void @free(i8* %mem)`（无 `%res =` 前缀）
- 不设置 `g->last_expr_type`（无值可传递）
- 若后续表达式使用 void 调用结果，编译器报错（per §6.2.1.3 void 约束）。

**6. 与既有设施的关系**：

- `g_extern_funcs` 与 `g->local_funcs`（用户函数表）独立存储。
- `lookup_user_func_ret_type()`（impl-plan:170）保持单一函数接口，内部仍按"先 user 后 extern"顺序查询 (合并两个表为统一接口)。
- §10 FFI 实施时需要确保 parser 与 codegen 间符号表共享 (可能需要 `CGEN` 结构体持有指针，或全局单例)。

##### 6.2.1.2 返回类型传播 *(0.3.2 新增)*

调用表达式的**结果**有以下用途：

| 上下文 | 行为 |
|--------|------|
| 赋值 RHS | `int x = factorial(5);` — `factorial(5)` 的返回类型 (`int`) 必须可赋给 `x` |
| 表达式语句 | `factorial(5);` — 丢弃返回值，合法 (类似 C) |
| 子表达式 | `int y = factorial(5) + 1;` — 返回值作算术运算输入 |
| 函数实参 | `print_num(factorial(5));` — 返回值传给 `print_num` |
| 条件 | `if (factorial(5) > 100)` — 返回值作 bool 上下文 |
| cast 源 | `(long)factorial(5)` — 见 §4.8.1 C-style cast |
| return 值 | `return factorial(5);` — 必须匹配函数声明返回类型 |

##### 6.2.1.3 void 函数调用 *(0.3.2 新增)*

返回类型为 `void` 的函数 (如 `print`, `print_num`)：

- 调用表达式的"值"是 `void`，没有具体类型。
- 不可作赋值 RHS，不可作子表达式，不可作 return 值 (除非外层函数也返回 void)。
- 可单独作语句：`print("hello");`

错误类型：
- `VoidUsedAsValueError` — void 结果被用作值。
- `ReturnTypeMismatchError` — return 值与函数声明返回类型不匹配。

##### 6.2.1.4 codegen 集成 *(0.3.2 新增)*

函数调用的 codegen 实现 (在 `src-c/src/codegen.c` 的 `UC_EXPR_CALL` case)：

```
1. 求值所有实参 (走 rvalue 路径，§4.13.4)
2. emit call 指令: ret_val = call i32 @factorial(i32 5)
3. 设置 g->last_expr_type 为函数签名返回类型 (从 §11.0 表或函数定义取)
4. 返回 ret_val (后续表达式可继续使用)
```

例：

```c
int x = factorial(5);
// codegen:
//   %1 = call i32 @factorial(i32 5)   ; §11.0 表查得 factorial 返回 int (i32)
//   store i32 %1, i32* %x
//   g->last_expr_type = "i32"          ; 由 §11.0 提供，修复 Bug C2 (硬编码 i32 错位)
```

##### 6.2.1.5 与其他章节的交叉引用 *(0.3.2 新增)*

| 章节 | 关系 |
|------|------|
| §6.1 函数定义 | 函数定义时声明的返回类型是 §6.2.1 优先级 2 的依据 |
| §10 FFI | extern 函数声明的返回类型是 §6.2.1 优先级 3 的依据 |
| §11.0 builtin 签名表 | builtin 函数返回类型是 §6.2.1 优先级 1 的依据 |
| §4.13.4 codegen 路径 | 调用表达式是 rvalue，返回值类型由 §11.0 提供 |
| §7.4 `alloc` 与 C stdlib | `malloc` 返回 `void*`，调用方需 cast 到具体类型 (§4.8.1) |
| §12.1 EBNF | 函数调用产生式见 §12.1 |

### 6.3 参数传递 *(0.3.0 改写：单一 T& + mod())*

**传值语义：**
- 参数被复制到函数形参
- 修改形参不影响实参

```cpp
void inc(int x) {
    x = x + 1;  // 不影响调用者
}

int n = 10;
inc(n);
// n 仍然是 10
```

**引用传递用于修改（0.3.0 形式——唯一形式）：**

```cpp
// === 0.3.0 形式（唯一）：通过 mod() 申请 ===
void inc(int& x) {
    mod(x);
    *&x = *&x + 1;  // 或 x = x + 1（视具体语法）
}

int n = 10;
mod(n);  // 或在调用点 mod(n)
inc(&n);
// n 现在是 11
```

**只读形参继续用 `T&`（不需 `mod`）：**

```cpp
int read(int& x) {          // 只读参数
    return x;               // 不调用 mod()
}

int n = 10;
int v = read(n);            // v == 10
```

> **参数类型匹配**：参数类型的确定与调用检查遵循 §6.2.1 的优先级规则。
>
> **[0.3.0 · Q3 反转, Rule 22, Rule 24]** 0.2.0 提供 `void inc(int&mut x)`（独占 mod-by-default，调用点 `inc(&mut n)`）与 `void inc(int& x)`（只读）两种形参。0.3.0 **删除 `T&mut`**，统一为单一引用类型 `T&` 形参：写入需先 `mod(x)` 申请，由 `#modlaw` 决定是否允许多个 mod 并存。独占性通过 `#modlaw exclusive` + `mod()` 实现，不再是类型层面区分。

### 6.4 返回值

```cpp
int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

Point get_point() {
    Point p = {1.0, 2.0};
    return p;  // 按值返回（复制）
}
```

> **返回类型**：返回类型由 §6.2.1 决定。
>
> **[0.3.0 沿用 0.2.0 · D-7]** 按**值**返回局部变量（如上例的 `return p;`）始终合法。返回指向局部变量的**借用**则是悬垂引用，见 §7.9。

### 6.5 main 函数约定

```cpp
int main() {
    // program code
    return 0;
}
```

`main` 函数：
- 返回 `int`
- 可以不接受参数，或：
  - `int main(int argc, char** argv)`

---

## 7. 内存管理

> **[0.3.3 新增]** 本章开始前，先建立**核心概念分类**：
> - **运算符**（§4.1）vs **builtin 函数**（§11.0）是两类不同的语言原语
> - 历史上（0.2.0 / 0.3.0）`mod` `unmod` `move` `clone` 列为运算符，0.3.3 修正为 builtin 函数
> - 详见 §7.0 概念清晰化

### 7.0 概念清晰化：运算符 vs builtin 函数 *(0.3.3 新增)*

UltraCPP 中「语言原语」分两类：

#### 7.0.1 运算符（§4.1 优先级表）

一元 / 二元 / 三元运算符，有**结合性**，在优先级表 §4.1 中明确：

- **一元前缀**：`*`（deref）、`&`（addr-of）、`+` `-` `!` `~` `++` `--`（2.5 级，**右到左**）
- **一元后缀**：`++` `--`（2 级，**左到右**）
- **二元**：`*` `/` `%` `+` `-` `<<` `>>` `<` `>` `<=` `>=` `==` `!=` `&`（按位与）`^` `|` `&&` `||` `=` `+=` `-=` `*=` `/=` `%=` `&=` `|=` `^=` `<<=` `>>=`（3-13 级，**左到右**）
- **三元**：`?:`（14 级，**右到左**）
- **作用域**：`::`（1 级，**左到右**）

> **注意**：`*` 在 2.5 级（前缀，deref）和 3 级（二元，乘法）**同名异义**，语法上通过**操作数位置**区分。`&` 在 2.5 级（前缀，addr-of）和 8 级（二元，按位与）**同名异义**，同上。

#### 7.0.2 builtin 函数（§11.0 签名总表）

**函数调用语法**（带 `()` 括号），在编译器内部 codegen 路径处理：

> **[0.3.3 + runtime-architecture 集成]** §11.0.1 builtin 语言原语（**6 项**） + 编译器 intrinsic（4 项，不进 BUILTIN_SIGS[]） + §11.0.2 UltraCPP stdlib（`lib/*.uc`，不进 BUILTIN_SIGS[]）：

- §11.0.1 **builtin 语言原语（6 项，进 BUILTIN_SIGS[]）**：
  - §7.2 `alloc(T)` — 堆分配，返回 `T*`（owning）
  - §7.3 `free(p)` — 释放 owning 指针（per runtime-arch §3.2 提升从 FFI 到 builtin）
  - §7.4 `move(p)` — 所有权转移，源 moved-out
  - §7.5 `clone(s)` — 独立副本（深拷贝）
  - §4.9 `mod(ref)` — 申请修改权，codegen emit `@uc_borrow_mod_enter` IR marker
  - §4.10 `unmod(ref)` — 释放修改权，codegen emit `@uc_borrow_mod_exit` IR marker
- **编译器 intrinsic（4 项，不进 BUILTIN_SIGS[]，per runtime-arch §5.2）**：
  - §7.6 `null` — 空指针常量（codegen emit `i8* null`）
  - `is_null(p)` — codegen emit `icmp eq i8* %p, null`
  - `sizeof(T)` — 编译期常量
  - `alignof(T)` — 编译期常量
- §11.0.2 **UltraCPP stdlib 路径（由用户 `import "lib/*.uc"` 提供，不进 BUILTIN_SIGS[]）**：
  - `lib/print.uc` — `uc_print_str` / `uc_print_num` / `uc_print_float`
  - `lib/string.uc` — `uc_strlen` / `uc_strcpy` / `uc_strcmp`（UltraCPP 自己实现，不接 libc，per runtime-arch §3.1）
  - `lib/memory.uc` — `uc_memcpy` / `uc_memmove` / `uc_memset`
  - `lib/math.uc` — `uc_abs`（替代 `abs_int`）
  - `lib/alloc.uc` / `lib/sys.uc`（sys:: 命名空间，§10.4）
  - `lib/uc_runtime.c`（C 临时 bootstrap，0.4.0+ 替换）

**调用形式**：`name(arg1, arg2, ...)`（所有 6 项 builtin）或 `name`（intrinsic `null` 是常量）。

**返回值类型**：查 §11.0.1 签名表确定（与 §6.2.1 函数返回类型规则一致，由 0.3.2 引入）。stdlib 函数返回类型由 `lib/*.uc` 的函数定义决定（不进 builtin 表）。

#### 7.0.3 关键澄清 *(0.3.3 新增)*

`mod` `unmod` `move` `clone` 在 0.2.0 / 0.3.0 spec 中曾被列为「运算符」：

- 0.2.0 spec §4.1 第 15 级：`move` `clone` 是运算符
- 0.3.0 spec §4.1 注脚：`mod` / `unmod` 是一元运算符
- 0.3.0 spec §12.3 附录优先级表 15 级：`move` `clone` `move_to_thread` 是「所有权操作」

**0.3.3 修正**：这 4 个都是 **builtin 函数调用**，与 `alloc` / `free` / `move` / `clone` 同级（§11.0.1 语言原语 builtin）。

**修正原因**：

1. 它们**总是带括号调用**（`mod(ref)` 不是 `mod ref`），没有 prefix / postfix 形式
2. 它们**有签名**（返回类型 + 参数类型），符合 builtin 函数特征
3. 它们**优先级不固定** — `mod(ref)` 中 `ref` 是完整表达式，与 `+` `-` 等运算符的优先级语义不同
4. 把它们混在优先级表中造成**误读**（e.g. `a + move(b)` 是否要查表？）

**`mod` / `unmod` 的特殊性质** *(0.3.3 + runtime-architecture §3.2 + §5.1 集成)*：

- `mod` / `unmod` 是 **builtin（A 类）** — 不再是「编译期决策、无 runtime 函数」的半成品状态（2026-08-17 runtime-architecture 集成修订）
- 与 `alloc` / `free` / `move` / `clone` 同列 §11.0.1，进入 BUILTIN_SIGS[] 表（BUILTIN_SIGS[] 共 6 项）
- codegen 层**会** emit LLVM IR 调用 — 但发出的是 **IR marker**（`@uc_borrow_mod_enter` / `@uc_borrow_mod_exit` intrinsic），不是普通函数调用：
  - `@uc_borrow_mod_enter(%ref)` — 标计「进入可修改借权作用域」（该 marker 可被优化为 no-op，因为校验在编译期完成）
  - `@uc_borrow_mod_exit(%ref)` — 标计「退出可修改借权作用域」
- mod 状态由三个机制维护：
  1. **IR 标记**（codegen emit）— 每个 `UCExprBuiltinCall` 节点对应 `@uc_borrow_mod_enter` / `@uc_borrow_mod_exit` IR 调用
  2. **`UCExprCall.mod_state` 字段**（编译期）— 申请中 / 已授予 / 已释放
  3. **借用检查器**（compile-time）— 在作用域结束时校验 mod 配对（Rule 24）
- **提升为 builtin 的理由**（per runtime-architecture §3.2）：`mod` / `unmod` 是 borrow-checker 状态入口 / 出口，编译器必须**显式**在 IR 中记录；之前的「intrinsic / 编译期决策」路径无法保证所有 call site 都正确 emit，因此统一为 builtin
- **历史**（0.2.0-0.3.3）：mod / unmod 在 0.2.0 / 0.3.0 / 0.3.1 / 0.3.2 中均被反复重新分类（`运算符` → `intrinsic` → `builtin`），0.3.3 + runtime-architecture 集成版是最终版

**修正后**：

- §4.1 优先级表**删除**第 15 级 `move clone`，并从 2.5 级一元行中删除 `mod` / `unmod`
- §4.1 注脚**改写**，说明 `mod` / `unmod` 是函数调用
- §12.3 附录优先级表**删除** 15 级，改放到 §11.0.1 builtin 签名表
- §11.0 重新分组为 §11.0.1（语言原语 6 项 builtin：`alloc` / `free` / `move` / `clone` / `mod` / `unmod`） + §11.0.2（UltraCPP stdlib 路径 `lib/*.uc`，不进 BUILTIN_SIGS[]；per runtime-architecture §3.1 + §5.3）
- §4.9 / §4.10 加 [0.3.3 修订] 注：`mod` / `unmod` 是 **builtin 函数**，codegen emit IR marker（`@uc_borrow_mod_{enter,exit}`）；不是单纯编译期决策

#### 7.0.4 历史回顾 *(0.3.3 新增)*

| 版本 | §4.1 表第 15 级 | 注脚对 mod / unmod 描述 | §11.0 builtin 表 | 章节归属 |
|------|----------------|------------------------|-------------------|----------|
| 0.2.0 | `move` `clone` 是运算符 | （无 mod / unmod） | （无表） | §4.1 表 |
| 0.3.0 | `move` `clone` 是运算符（沿用） | 「mod / unmod 是一元运算符」 | （无表） | §4.1 表 + §12.3 |
| 0.3.1（S2） | （无变化） | （无变化） | （无表） | §4.1 表 + §12.3 |
| 0.3.2（S4 + S5） | （无变化） | （无变化） | 「单表 18 项」 | §4.1 表 + §12.3 + §11.0 |
| **0.3.3 + runtime-arch（本文档当前版）** | 删除 15 级 | 「mod / unmod 是 builtin 函数（A 类，codegen emit IR marker）」 | **§11.0.1（6 项语言原语：alloc / free / move / clone / mod / unmod） + §11.0.2（UltraCPP stdlib 路径 `lib/*.uc`，不进 BUILTIN_SIGS[]）** | §11.0.1 + §10.2 / §10.3 / §10.4 |

0.3.3 是首次明确「运算符 vs builtin 函数」分类的版本；runtime-architecture 集成修订（2026-08-17）进一步把 mod / unmod 从「半 builtin」提升为**完整** builtin（A 类，emit IR marker），并把 stdlib 从 BUILTIN_SIGS[] 表移走（UltraCPP 自带）。

#### 7.0.5 与其他章节的交叉引用 *(0.3.3 新增)*

| 章节 | 关系 |
|------|------|
| §3 类型系统 | 运算符和 builtin 函数操作数的类型约束来自 §3 |
| §4.1 运算符优先级 | 运算符完整列表见 §4.1 |
| §4.9 `mod(ref)` | `mod` 是 builtin 函数（§11.0.1，A 类，codegen emit IR marker），不是运算符 |
| §4.10 `unmod(ref)` | `unmod` 是 builtin 函数（§11.0.1，A 类，codegen emit IR marker），不是运算符 |
| §6.2.1 函数返回类型 | builtin 函数返回类型由 §11.0 签名表确定 |
| §7.1 所有权 | `move` / `clone` 是 §7.1 概念的核心 builtin（`null` 是 intrinsic，不在 §11.0.1 表，per runtime-arch §5.2） |
| §7.3 `free(p)` | `free` 是 builtin（§11.0.1，A 类，emit `@free` + 标记 dangling），不是 FFI |
| §7.4 `move(p)` | 完整语义见 §7.4（本节是分类，§7.4 是语义） |
| §7.5 `clone(s)` | 完整语义见 §7.5 |
| §11.0.1 builtin 语言原语（6 项） | §11.0.1 收录 **6** 个语言原语 builtin：`alloc` / `free` / `move` / `clone` / `mod` / `unmod`（per runtime-arch §5.1） |
| §11.0.2 UltraCPP stdlib 路径 | §11.0.2 指明 `lib/print.uc` / `lib/string.uc` / `lib/memory.uc` / `lib/math.uc` / `lib/alloc.uc` / `lib/sys.uc`；stdlib 不进 BUILTIN_SIGS[]（per runtime-arch §5.3 + §9.2） |
| §12.1 EBNF | `builtin_function_call` 产生式见 §12.1（0.3.3 新增） — 仅含 6 项语言原语；stdlib 走 `import "lib/*.uc"`，不在 builtin_function_call |
| §12.3 附录优先级表 | 同步 §4.1 修订 |

#### 7.0.6 abs_int → uc_abs 迁移说明 *(0.3.4 新增)*

> **[0.3.4 修订]** abs_int 已从 `BUILTIN_SIGS[]`（src-c/src/codegen.c）移除，由 UltraCPP 自实现的 `lib/math.uc::uc_abs` 提供（per commit 11a）。调用方仍可写 `abs_int(x)` 形式，codegen 通过 extern function lookup 链接到 lib/math.o::uc_abs。spec §11.0.3 codegen 集成相应修订。

**历史**：0.3.2 起 `abs_int` 收录 `BUILTIN_SIGS[]` 第 1 项，并以 `@builtin_abs_int` inline IR 实现（21 行）；0.3.3 阶段随 stdlib 整体移出 BUILTIN_SIGS[]，0.3.4 commit 11a 完全移除 inline IR 实现。

**0.3.4 迁移路径**：

1. `abs_int` 从 `BUILTIN_SIGS[]` **完全移除**（commit 11a）
2. codegen 在 `UCExprCall` 主流水中走 **P3 extern function lookup**（per §11.0.3 修订）：emit `declare external i32 @abs_int(i32)` + `call i32 @abs_int(i32 %x)`
3. 链接时由 linker 解析到 `lib/math.o::uc_abs`（来自 `lib/math.uc`）
4. `lib/math.uc` 提供 `int uc_abs(int n)` 实现：`n < 0 ? -n : n`

**回归保证**：`uc_abs` 行为必须与原 inline IR 字节一致（`uc_abs(-7) == 7` / `uc_abs(0) == 0` / `uc_abs(7) == 7`），m0_41_extern_c 测试在 commit 11a 后仍 PASS（per 0.3.4 plan §6 R2）。

**关联章节**：
- §11.0.3 codegen 集成 — 6 primitives 不再含 abs_int；走 P3 extern function lookup 路径
- §11.0.2 UltraCPP stdlib 路径 — `lib/math.uc` 提供 `uc_abs`
- §S6 FFI extern body 来源策略（新增）— extern lookup 三层优先级（stdlib → 用户代码 → libc fallback）

### 7.1 所有权语义（**两条权限拆分**） *(0.3.0 重写：Rule 22, Q4, Q5)*

> **[0.3.0 · Rule 22]** 0.3.0 把 0.2.0 的「所有权」拆成两个独立的权限：
>
> | 权限 | 决定什么 | 默认值 | 转移方式 |
> |------|---------|--------|----------|
> | **所有权** | 谁 delete / free | `T* p = alloc(T)` 时拥有；`T* p = &x` 不拥有 | 显式 `move(p)` |
> | **修改权** | 谁能改值 | owning 指针默认有；`T&` 默认无（需 `mod()`） | 隐式 `mod()`（按 `#modlaw`）|

**指针与所有权（Rule 2B, Q4）：**

- **任何时刻唯一 owner**：一个 allocated 对象最多有一个 owning 指针。
- **`T* p = alloc(T)`**：p 拥有对象的**所有权**和**修改权**。
- **`T* p = &x`**：p 仅持有 x 的地址，**不 owns x**，也**不**默认有 x 的修改权。
- **赋值 `p1 = p2` 不复制所有权**（Q4）：源 `p2` 仍然 owns；目标 `p1` 获得**修改权**（隐式 `mod(p1)`）。

#### 7.1.1 赋值与修改权（**0.3.0 改写 0.2.0 §7.1.1**）

**示例 C（所有权转移 vs. 修改权）：**

```cpp
unique int* p1 = alloc(int);  // p1 owns
*p1 = 100;

unique int* p2 = p1;           // 隐式 mod(p2)：p2 得修改权，p1 仍 owns
*p2 = 200;                      // ✅
// free(p2);                    // ❌ p2 不 owns

move(p2);                       // 显式移交
// p1 已 moved-out
free(p2);                       // ✅ 现在 p2 owns
```

**规则（0.3.0 与 0.2.0 关键差异）：**

> 设赋值语句为 `dst = src`，其中 `dst` 与 `src` 的类型均为 `unique T`（等价于 `T*`）。
>
> 0.3.0 行为：
> 1. `dst` 获得**修改权**（隐式 `mod(dst)`，按当前 `#modlaw` 决定是否多 mod 或独占）。
> 2. `src` 的**所有权不变**——它仍然 owns；除非显式 `move(src)` 才转。
> 3. `dst` 的所有权状态：不 owns。所以 `free(dst)` 在 **dest 不 owns** 的情况下是编译期错误。

> 0.2.0 行为（D-5）：`src` 置 null，`dst` 接管所有权。
>
> 0.3.0 **改写**为：`src` 不动，`dst` 获得修改权。`move(p)` 仍是显式移交所有权的唯一途径（Q5）。

**发生所有权转移 `move()` 的位置**（沿用 0.2.0，Q5）：

| 位置 | 行为 | 源 | 目标 |
|------|------|----|------|
| 显式 `move(p)` | 移交所有权 | `p` | `p2`（接收方） |
| 函数实参 `f(p)`（形参为 `unique T`） | 移交所有权 | `p` | 形参 |
| 函数返回 `return p;`（返回类型 `unique T`） | 移交所有权 | `p` | 调用方接收处 |

**发生修改权授予的位置**（Q4）：

| 位置 | 行为 | 源 | 目标 |
|------|------|----|------|
| 赋值 `dst = src`（类型为 `unique T`） | 隐式 `mod(dst)` | — | `dst` |
| 显式 `mod(ref_expr)`（§4.9） | 按 `#modlaw` 申请 | — | ref_expr |

**不发生任何转移的位置**：`clone(s)`、`&s`、`s == null` 等只读比较。

### 7.2 alloc 函数

```cpp
int* alloc(int)                  // 分配单个元素
int* alloc(int, int count)      // 分配 count 个元素的数组
```

**示例：**
```cpp
int* p = alloc(int);         // 单个 int
int* arr = alloc(int, 10);    // 10 个 int 的数组
char* buf = alloc(char, 256); // 256 字节的缓冲区
```

### 7.3 free 函数

```cpp
free(ptr)    // 释放内存
```

**示例：**
```cpp
int* p = alloc(int);
*p = 42;
free(p);

int* arr = alloc(int, 10);
free(arr);
```

> **[0.3.0 沿用 0.2.0 · D-6]** `alloc` 与 `free` 的配对由程序员负责，编译器**不强制**检查。

### 7.4 move(p) 表达式 *(0.3.0 改写, 0.3.3 修订, Rule 22, D-4, Q5)*

> **[0.3.0 · D-4, Q5]** `move(p)` 是语言的**内建 primitive**，**不是**语法糖，也**不是**普通函数。它是**显式移交所有权**的唯一途径。
>
> **[0.3.3 修订]** `move` 是 **builtin 函数**（§11.0.1 builtin 语言原语），**不是**运算符（§4.1 优先级表 15 级已删除）。调用形式始终是 `move(p)`，没有 prefix 形式。概念分类见 §7.0.2 builtin 函数；builtin 签名见 §11.0.1。
>
> **codegen 路径**（与简单函数调用**不同**）：`move(p)` 不走普通函数 emit，而是通过特殊 IR 标记（`UCExprCall.ownership_xfer = MOVE`）+ LLVM IR 重新绑定 SSA 值实现所有权转移。源 `p` 在 IR 中标记为 moved-out（后续 use 触发 use-after-move 错误）。codegen 代码集中在 `src-c/src/codegen.c` 的 `emit_move_call()`，约 30-50 LOC。详见 §11.0.3 codegen 集成。

**语法**（见 §12.1 `unary_expression`）：
```
move '(' expression ')'
```

**语义：**

1. 操作数必须是可寻址的左值，类型为 `unique T`（等价于 `T*`）。
2. `move(p)` 的**结果**是 `p` 原先持有的指针值，类型为 `unique T`。
3. **运行时效果**：求值 `move(p)` 后，编译器必须发出把 `p` 的存储单元写为 `null` 的代码。
4. **静态效果**：`p` 被标记为 moved-out，此后对 `p` 的任何使用都是编译期错误 `UseAfterMove`。
5. `move` 不产生新的分配，也不复制被指向的对象；它只转移所有权。
6. `move(p)` 与隐式 move（仅在 `dst = src` **不**触发 move 的 0.3.0 行为下，move(p) 是唯一显式手段）的关系：move 是显式手段；隐式赋值只授予修改权，不移交所有权。

```cpp
unique int p1 = alloc(int);
*p1 = 42;

unique int p2 = move(p1);   // p1 运行时变为 null，静态标记为 moved-out

// int v = *p1;             // ❌ 编译期错误 UseAfterMove
int v = *p2;                // ✅ v == 42
free(p2);                   // ✅
```

### 7.5 clone(s) 函数 *(0.3.0 改写, 0.3.3 修订)*

> **[0.3.2]** `clone(s)` 创建 `s` 指向对象的**独立深拷贝副本**，返回新的 owning 指针。原 `s` 不受影响。
>
> **[0.3.3 修订]** `clone` 是 **builtin 函数**（§11.0.1 builtin 语言原语），**不是**运算符（§4.1 优先级表 15 级已删除）。调用形式始终是 `clone(s)`，没有 prefix 形式。概念分类见 §7.0.2 builtin 函数；builtin 签名见 §11.0.1。
>
> **codegen 路径**（与简单函数调用**不同**）：`clone(s)` 不走普通函数 emit，而是通过特殊 LLVM IR 生成（按 struct 大小 emit `load` + `malloc` + 逐字段 `store`）实现深拷贝。浅拷贝（如 `int*` + `size`）与深拷贝（struct 递归）走两套 emit。codegen 代码集中在 `src-c/src/codegen.c` 的 `emit_clone_call()`，约 50-80 LOC。详见 §11.0.3 codegen 集成。

```cpp
int* p1 = alloc(int);
*p1 = 42;
int* p2 = clone(p1);  // p1 和 p2 独立（不同地址），p2 == 42 拷贝，各自释放
```

### 7.6 null 常量

> **[0.3.3 修订]** `null` 是 **编译器 intrinsic**（per runtime-architecture §5.2），**不**在 §11.0.1 builtin 签名表。codegen 直接翻译为 `i8* null` LLVM 常量。详见 §7.0.2。

```cpp
int* p = null;      // 空指针
if (p == null) {    // 比较
    // p 为 null
}
```

### 7.7 指针运算

```cpp
int* ptr = alloc(int, 10);
ptr[0] = 1;          // 索引访问
ptr[5] = ptr[0];    // 复制值

int* p1 = alloc(int, 10);
int* p2 = p1 + 5;   // 偏移指针
```

### 7.8 引用 `&T`（重写 0.2.0 §7.8） *(0.3.0 改写, 0.3.1 修订, 0.3.3 修订, Rule 22, Rule 24, 删除 T&mut)*

> **[0.3.0 改写]** 0.3.0 删除 `T&mut` 类型。`T&` 是唯一的引用类型，是否可写由 `mod()` + `#modlaw` 决定。
> **[0.3.1 修订]** 0.3.1 显式定义 lvalue / rvalue 概念（§4.13），本节“操作数必须是左值”
> 的“左值”引用 §4.13.2 表达式分类表。
> **[0.3.3 修订]** `&` 既是一元运算符（`&T` 引用类型，§4.1 的 2.5 级），也是 §11.0.1 builtin 函数 `mod(ref)` / `unmod(ref)` / `move(p)` / `clone(s)` 的接受者（都接受 `T&` 或 `T*`）。`&` 与 `*` 的语义边界见 §4.13.5 deref 语义章节。`mod` / `unmod` 是 **builtin 函数**，不是运算符，详见 §7.0 概念清晰化。

**语法**（见 §12.1）：
```
'&' unary_expression
```

`&expr` 创建对 `expr` 所指对象的引用，结果类型为 `T&`。

**创建规则：**

1. 操作数必须是 lvalue（见 §4.13.2 表达式分类表 — 哪些表达式是 lvalue）。
   - ✅ 合法：`&x`（变量）、`&*p`（解引用）、`&s.field`（字段访问）、`&a[i]`（索引）、`&++x`（前缀自增）
   - ❌ 非法：`&42`（字面量）、`&x + 1`（算术结果）、`&f()`（函数调用结果）、`&x++`（后缀自增结果）
2. owner 必须处于活跃且已初始化的状态。
3. 引用不能超出 owner 的生命周期（违反时报 DanglingReference，见 §7.9）。
4. `&` 不改变 owner 的所有权状态。

**可写性（Rule 24）：**

5. 写入需先 `mod()`。`r = v;` 必须先用 `mod(r);` 申请修改权，否则是编译期错误。
6. 多个 `T&` 可并存。
7. 排他性由 `#modlaw exclusive` 控制（编译期检查 `mod()` 争用）。

```cpp
#modlaw shared module

int x = 42;
int& r1 = x;
int& r2 = x;        // ✅ 多个引用并存
int v = r1 + r2;    // ✅ 读
// r1 = 100;        // ❌ 编译错：没有 mod 权
mod(r1);            // ✅ 申请修改权（按 #modlaw 决定行为）
*r1 = 100;          // ✅ 写入

#modlaw exclusive module

int y = 10;
int& m = y;
mod(m);             // ✅ 独占 mod
*m = 100;
// mod(m2);         // ❌ 编译错：争用

#modlaw none module

int z = 5;
int& r = z;
// mod(r);         // ❌ 编译错：none 策略禁止
r;                  // ✅ 只读
// *r = 100;       // ❌ 无 mod 权
```

### 7.9 悬垂引用 (Dangling Reference) *(0.3.0 沿用 0.2.0 · D-7)*

定义与触发条件与 0.2.0 §7.10 相同，本节保留供查阅。

**定义**：一个 `T&` 借用是**悬垂 (dangling)** 的，当它在其 owner 的存储被销毁之后仍然可达。

**触发条件**（沿用 0.2.0）：

- **(a)** `&T` 借用的 owner 已离开作用域。
- **(b)** 函数返回引用，但 owner 是该函数的局部变量。

**示例（a）（沿用 0.2.0）**：

```cpp
int main() {
    int& r;
    {
        int x = 42;
        r = x;              // 借用 x（0.3.0 语法：右侧是左值，不是 &x）
    }                       // ← x 离开作用域
    return r;               // ❌ DanglingReference
}
```

**正例（沿用 0.2.0）**：

```cpp
int make_answer_by_value() {
    int x = 42;
    return x;               // ✅ 按值返回（复制）
}

unique int make_answer_owned() {
    unique int p = alloc(int);
    *p = 42;
    return p;               // ✅ move：所有权 transfer（见 §7.1.1）
}
```

### 7.10 const T* / T* const 自定义语义详解（Q6） *(0.3.0 新增)*

> **[0.3.0 · Q6 自定义语义，与 C++ 相反]** 详见 §3.8 概念部分，本节给出更深入的语义规则。

#### 7.10.1 `const T*`

- **指针锁定**：声明 `const T* p = &x;` 后，**不能再赋给 p**（`p = &y;` 是编译期错误）。
- **只读访问**：通过 `p` 不能写（`*p = v;` 是编译期错误）。
- **生命周期安全**：编译器会验证指向对象的生命周期 ≥ 借用的使用范围。
- **不允许指向 owning heap 变量**（owning heap 可能在未来被 move / free，导致悬垂）。

#### 7.10.2 `T* const`

- **数据只读视图**：通过 `r` 不能写（`*r = v;` 是编译期错误）。
- **允许 rebind**：`r = &y;` 合法。
- **同样不允许指向 owning heap 变量**——若 h 是 owning，`T* const r = h;` 在编译期被拒。

#### 7.10.3 owning heap 禁止的诊断

```
error[E0650]: cannot create read-only view of owning heap pointer
  --> src/main.uc:5:12
   |
 4 | unique int* h = alloc(int);
   | ----------------- `h` is an owning pointer
 5 | const int* p = h;
   |               ^ `h` may be moved or freed, so a `const T*` view of `h` would dangle
   |
   = help: copy the value first: `int v = *h; const int* p = &v;`
```

#### 7.10.4 与 `mod()` 的关系

`const T*` 与 `T* const` 都是**只读**的——`mod()` 不适用于它们，因为它们的设计意图就是「不让任何代码写它们」。

---

## 8. struct 类型

### 8.1 struct 定义

```cpp
struct Point {
    double x;
    double y;
}

struct Circle {
    Point center;
    double radius;
}
```

### 8.2 字段访问

```cpp
Point p;
p.x = 1.0;
p.y = 2.0;

Circle c;
c.center.x = 0.0;
c.center.y = 0.0;
c.radius = 5.0;
```

### 8.3 内存布局

结构体使用**顺序布局**，可能为对齐添加填充：

```cpp
struct Example {
    char  c;    // 1 字节
    int   i;    // 4 字节（可能添加 3 字节填充）
    short s;    // 2 字节
}
```

### 8.4 作为字段的函数指针

```cpp
struct Comparator {
    int (*compare)(int, int);
    const char* name;
}

int add(int a, int b) { return a + b; }

Comparator cmp = { add, "add" };
int result = cmp.compare(10, 20);  // result = 30
```

---

## 9. 模块和预处理器 *(0.3.0 新增 §9.9)*

### 9.1 预处理阶段

UltraCPP 在词法分析之前运行**预处理阶段**，处理以下指令：

| 指令 | 行为 |
|------|------|
| `#include "path"` | 展开文件内容到当前位置 |
| `#import "module"` | 记录模块依赖（不展开内容） |
| `#modlaw <perm> <scope>` *(0.3.0 新增)* | 设置当前文件 / 模块的修改权策略（见 §9.9） |
| `export <declaration>` | 标记导出的函数/变量 |

**预处理输出**：
1. 展开 `#include` 后的纯代码
2. 模块依赖列表
3. 当前 `#modlaw` 策略
4. 导出符号列表

### 9.2 #include 语法和语义

**源码复制**：`#include` 将指定文件的内容**逐字复制**到 `#include` 指令位置。

```cpp
#include "file_path"
```

**示例：**
```cpp
// main.upp
#include "../../lib/math.uc"

int main() {
    int result = add(5, 3);  // add() 定义在 math.uc 中
    return result;
}
```

**行为**：
- `#include` 后的代码直接获得被包含文件的所有定义
- 不需要额外的声明即可调用被包含文件中的函数
- 文件路径相对于**当前源文件目录**解析

**搜索路径顺序**：
1. 相对于当前源文件目录
2. 相对于项目根目录
3. 相对于指定的 include 路径

### 9.3 #import 语法和语义

**模块导入**：`#import` 记录对某个模块的依赖，**不展开其内容**。

```cpp
#import "module_path"
#import "module_path" as alias
```

**示例：**
```cpp
#import "math";           // 导入 math 模块
#import "io" as stdio;    // 导入 io 模块并起别名
```

**行为**：
1. 记录模块依赖，但不展开源代码
2. 被导入模块需要单独编译
3. 调用模块函数需要 `extern` 声明或 `#include` 其接口

**生成的依赖文件**：
```json
{
  "module": "math",
  "file": "lib/math.uc",
  "exports": ["add", "multiply"]
}
```

### 9.4 export 声明

标记函数或变量为**模块导出**，外部可以通过 `#import` 访问。

```cpp
export int add(int a, int b) {
    return a + b;
}

export const double PI = 3.14159;

export struct Point {
    double x;
    double y;
}
```

**规则**：
- 没有 `export` 的符号是**模块私有**的
- `export` 只能在模块顶层使用
- 导出的函数/变量可以被 `#import` 该模块的代码访问

### 9.5 模块设计规则

UltraCPP 的模块系统遵循以下规则：

1. **定义函数不需要声明** - 同文件内函数调用无需前置声明
2. **外部模块函数需要 import** - 调用其他模块的函数必须 `#import` 该模块
3. **内部函数不可外部访问** - 未 export 的函数只能模块内部使用

**示例**：
```cpp
// lib/math.uc
export int add(int a, int b);  // 导出接口

int _internal_helper(int x) {  // 内部函数，外部不可见
    return x + 1;
}

int add(int a, int b) {
    return _internal_helper(a) + _internal_helper(b);
}
```

```cpp
// test/main.upp
#include "../../lib/math.uc"

int main() {
    add(5, 3);           // ✅ 可以调用
    _internal_helper(1); // ❌ 不可调用（未导出）
    return 0;
}
```

### 9.6 模块搜索路径

| 路径类型 | 解析方式 |
|----------|----------|
| 相对路径 | 相对于当前源文件目录 |
| 绝对路径 | 相对于项目根目录 |
| 指定路径 | 通过 `-I` 参数添加的搜索路径 |

### 9.7 循环依赖检测

```cpp
// a.uc
#include "b.uc"

int func_a() {
    return func_b() + 1;
}
```

```cpp
// b.uc
#include "a.uc"  // 错误：检测到循环依赖

int func_b() {
    return 42;
}
```

**检测规则**：如果两个模块相互 `#include`，编译器应报告错误。

### 9.8 编译流程

```
源代码 (.uc/.upp)
    │
    ▼
┌─────────────────┐
│   预处理阶段     │
│  - 展开 #include │
│  - 记录 #import  │
│  - 解析 #modlaw  │ (0.3.0 新增)
│  - 解析 export   │
└─────────────────┘
    │
    ▼
┌─────────────────┐
│   词法分析       │
│   语法分析       │
│   代码生成       │
└─────────────────┘
    │
    ▼
  LLVM IR (.ll)
    │
    ▼
  llc (汇编)
    │
    ▼
  目标文件 (.o)
    │
    ▼
  链接器 (gcc/ld)
    │
    ▼
  可执行文件
```

### 9.9 `#modlaw` 指令 *(0.3.0 新增：Rule 23)*

> **[0.3.0 · Rule 23]** 关键字：`#modlaw`（全小写，下划线）。语法：`#modlaw <perm> <scope>`。

**语法**：
```
#modlaw <perm> <scope>
```

**合法组合（仅 6 种）：**

| `perm`     | `scope`    | 含义                                                      |
|------------|------------|-----------------------------------------------------------|
| `none`     | `global`   | 全局禁止 `mod()`；引用只读                                |
| `none`     | `module`   | 本模块禁止 `mod()`；引用只读                              |
| `exclusive`| `global`   | 全局 `mod()` 独占（编译期检查争用）                       |
| `exclusive`| `module`   | 本模块 `mod()` 独占（编译期检查争用）                     |
| `shared`   | `global`   | 全局 `mod()` 共享（多 mod 允许，程序员责任处理数据竞争）  |
| `shared`   | `module`   | 本模块 `mod()` 共享（多 mod 允许，程序员责任处理数据竞争）|

**非法组合**：其他 10 种组合是编译期错误。例如 `#modlaw none shared`、`#modlaw exclusive none`、`#modlaw shared exclusive` 等。

**作用域（`scope`）：**

- `global`：影响整个文件以及被 `#include` 引入的所有符号。
- `module`：仅影响当前 `.uc` 模块内的符号；被 `#include` 的文件不继承。
- 默认未指定 `#modlaw` 时，等价于 `#modlaw shared module`。

**位置**：`#modlaw` 指令必须出现在模块的**顶部**（在 `import` / `include` / `export` 之后，第一个函数 / 变量声明之前）。同一模块最多一条 `#modlaw`；重复出现是编译期错误。

**示例 B（三种合法策略）：**

```cpp
// === 共享模式（默认） ===
#modlaw shared module

int x = 42;
int& r1 = x;       // 共享引用
int& r2 = x;       // 也共享
mod(r1);            // ✅ r1 获得共享 mod
mod(r2);            // ✅ r2 也可获得（数据竞争程序员责任）
*r1 = 100; *r2 = 200;

// === 独占模式 ===
#modlaw exclusive module

int y = 10;
int& m = y;
mod(m);             // ✅ 独占 mod
// mod(m2);         // ❌ 编译错：争用
*m = 100;

// === none 模式 ===
#modlaw none module

int z = 5;
int& r = z;
// mod(r);         // ❌ 编译错：none 策略禁止
r;                  // ✅ 只读
// *r = 100;       // ❌ 无 mod 权
```

**错误诊断：**

```
error[E0701]: `#modlaw` policy forbids `mod()` under `none`
  --> src/main.uc:7:5
   |
 6 | int& r = z;
   |     - binding declared here
 7 | mod(r);
   | ^^^^^ `mod()` is forbidden by `#modlaw none module`
   |
   = help: change the policy with `#modlaw shared module` or `#modlaw exclusive module`
```

---

## 10. FFI（外部函数接口） *(0.3.0 引入, 0.3.3 修订)*

> **[0.3.0 引入]** FFI 是与外部代码（C / 汇编 / 系统调用）交互的机制。
>
> **[0.3.3 修订]** §10 补充 3 个子节以描述完整的 FFI / 系统调用机制：
> - §10.1 `extern "C"` 块 — 用户声明外部 C 函数
> - §10.2 （新）预编译宏机制（`@ifdef`，C 风格）
> - §10.3 （新）内联汇编机制（`asm { ... }`，C / GCC 风格）
> - §10.4 （新）`sys::` 系统调用 namespace

### 10.1 extern "C" 块

```cpp
extern "C" {
    // C 函数声明
}
```

> **[0.3.3 修订]** `extern "C"` 块**仅用于用户自己声明外部 C 函数**（少见场景，如调用系统库、遗留 C 代码）。**不用于 stdlib** — stdlib 由 UltraCPP 自己写在 `lib/*.uc`（per runtime-architecture §3.1 "UltraCPP 不使用 libc 作为 runtime"）。当 stdlib 已使用 `lib/*.uc` 后，`extern "C"` 仅用于 0.3.x 阶段临时 bootstrap 遗留调用。

#### 10.1.1 实现状态 *(0.3.4 新增, per 0.3.4 plan §1 + 0.3.4 spec-text-changes §3.2)*

> **[0.3.4 新增]** §10.1 描述的 `extern "C"` 块机制在 **0.3.4 commit 11b** 确认完成。

| 语法子集 | 实现状态 | 关联 commit |
|----------|----------|-------------|
| `extern "C" { fn_decl; }` 基本语法 | ✅ 已实现（per 0.3.3 commit 7d）| 7d |
| 块内多函数声明 | ✅ 已实现 | 7d |
| 函数声明跳过已声明 builtin（UC_TL_EXTERN 跳过）| ✅ 已实现（per 0.3.3 commit `42d75f7`）| `42d75f7` |
| 函数 body 来源（extern 三层查找）| ✅ 已实现（per 0.3.3 + 0.3.4 commit 11a）| 11a |
| body 来源策略完整规范 | ⚠️ partial（详见新增 §S6，0.3.4 Phase 3 落地）| 推 0.3.4 Phase 3 |

**0.3.4 修订**：规则不变；body 来源策略（extern 三层查找：UltraCPP stdlib → 用户代码 → libc fallback）由 0.3.4 commit 11b + 新增 §S6 章节明确定义。

### 10.2 预编译宏机制 *(0.3.3 新增, per runtime-architecture §7)*

UltraCPP 采用 **C 风格的预编译宏语法**，不采用 Rust 的 `#[cfg(...)]` 风格。理由：与 C 生态工具链兼容；与 `src-c/` 现有的 C 实现无缝；学习曲线友好。`@` 前缀避免与用户代码标识符冲突。

```c
@ifdef(TARGET_OS_LINUX)
    // Linux 实现
@end

@ifdef(TARGET_OS_DARWIN)
    // macOS 实现
@end

@if defined(TARGET_OS_LINUX) && defined(TARGET_ARCH_X86_64)
    // x86_64 Linux 特定代码
@end

@if !defined(TARGET_OS_LINUX)
    @error("lib/sys/raw.uc 仅支持 Linux，其他平台需单独实现")
@end
```

#### 10.2.1 语法子集 *(0.3.3 新增)*

| 语法 | 等效 C | 说明 |
|------|--------|------|
| `@ifdef(MACRO) ... @end` | `#ifdef MACRO ... #endif` | 单条件 |
| `@ifndef(MACRO) ... @end` | `#ifndef MACRO ... #endif` | 反向 |
| `@if defined(MACRO) && defined(MACRO2) ... @end` | `#if defined(MACRO) && defined(MACRO2) ... #endif` | 复合 |
| `@if !defined(MACRO) ... @end` | `#if !defined(MACRO) ... #endif` | 否定 |
| `@else` / `@elif(EXPR)` | `#else` / `#elif` | 分支 |
| `@define(NAME, VALUE)` | `#define NAME VALUE` | 宏定义 |
| `@error("msg")` | `#error "msg"` | 编译期错误 |
| `@warning("msg")` | `#warning "msg"` | 编译期警告 |

#### 10.2.2 预定义宏 *(0.3.3 新增, per runtime-architecture §7.2)*

| 宏 | 值 | 用途 |
|---|----|------|
| `TARGET_OS_LINUX` | 1 | Linux 平台 |
| `TARGET_OS_DARWIN` | 1 | macOS 平台 |
| `TARGET_OS_WINDOWS` | 1 | Windows 平台 |
| `TARGET_OS_FREEBSD` | 1 | FreeBSD 平台 |
| `TARGET_ARCH_X86_64` | 1 | x86_64 架构 |
| `TARGET_ARCH_AARCH64` | 1 | ARM 64 架构 |
| `TARGET_ARCH_RISCV64` | 1 | RISC-V 64 架构 |
| `TARGET_ULTRA_VERSION` | "0.3.3" | UltraCPP 编译器版本 |
| `TARGET_ULTRA_FEATURES` | 位标志 | 启用的语言特性 |

- **预定义宏来源**（per runtime-architecture §11）：uc compiler 启动时根据 `uname` 自动检测；可被 `-D NAME=VALUE` 显式覆盖
- **检测时机**：lexer / parser 阶段；解析 `@ifdef` / `@if defined()` 时立即求值，不进入 AST

#### 10.2.3 实现状态 *(0.3.4 新增, per 0.3.4 plan §1 + 0.3.3 process §3.1 commit 8a)*

> **[0.3.4 新增]** §10.2 描述的预编译宏机制在 **0.3.3 commit 8a** 完成 codegen 实现（per `.dev/drafts/0.3.3-implementation-process.md` §3.1）。

| 语法子集 | 实现状态 | 关联 commit |
|----------|----------|-------------|
| `@ifdef(MACRO) ... @end` | ✅ 已实现（per 0.3.3 commit 8a）| 8a |
| `@ifndef(MACRO) ... @end` | ✅ 已实现 | 8a |
| `@if defined(MACRO) && defined(MACRO2) ... @end` | ✅ 已实现 | 8a |
| `@if !defined(MACRO) ... @end` | ✅ 已实现 | 8a |
| `@else` / `@elif(EXPR)` | ✅ 已实现 | 8a |
| `@define(NAME, VALUE)` | ✅ 已实现 | 8a |
| `@error("msg")` | ✅ 已实现 | 8a |
| `@warning("msg")` | ✅ 已实现 | 8a |
| 预定义宏自动检测（`TARGET_OS_*` / `TARGET_ARCH_*`）| ✅ 已实现（per runtime-arch §11）| 8a |

**0.3.4 确认**：0.3.4 commit 11b 确认 0.3.3 commit 8a 全部落地，无未实现项。0.4.0+ Stage 1 自举阶段需用本机制跨平台分发 `lib/sys/raw.uc`（per §11.0.5）。

### 10.3 内联汇编机制 *(0.3.3 新增, per runtime-architecture §8)*

UltraCPP 采用 **C / GCC 风格的内联汇编语法**，不采用 Rust 的 `asm!` 宏。理由：LLVM 工具链直接理解；与 `src-c/` 当前 C 实现无缝；与 §10.2 预编译宏一致使用 C 生态约定。

```c
int raw_syscall(int num, int arg1) {
    int result;
    asm {
        "syscall"
        : "=a"(result)               // 输出操作数
        : "0"(num), "D"(arg1)         // 输入操作数（0 = 与输出 0 复用）
        : "rcx", "r11", "cc", "memory"  // clobbers
    }
    return result;
}
```

#### 10.3.1 语法格式 *(0.3.3 新增)*

格式（4 段, 引号分隔）：

1. **指令模板**：`"syscall"` —— GCC 风格的字符串字面量指令
2. **输出操作数**：`: "=a"(result)` —— `=` 表示只写
3. **输入操作数**：`: "0"(num), "D"(arg1)` —— `"0"` 表示与输出 0 同寄存器
4. **Clobbers**：`: "rcx", "r11", "cc", "memory"` —— 调用者保存 / 内存可能被改

#### 10.3.2 约束字符 *(0.3.3 新增, per runtime-architecture §8.2, 参考 GCC)*

| 约束 | 含义 |
|------|------|
| `r` | 任意通用寄存器 |
| `a` | rax / eax（x86_64 syscall num） |
| `D` | rdi / edi（x86_64 第 1 参） |
| `S` | rsi / esi（x86_64 第 2 参） |
| `d` | rdx / edx（x86_64 第 3 参） |
| `c` | rcx / ecx（x86_64 第 4 参，syscall 后被 clobber） |
| `b` | rbx / ebx（x86_64 第 5 参） |
| `=r` | 输出寄存器（只写） |
| `+r` | 读写操作数 |
| `0`-`7` | 与第 N 个操作数复用同一寄存器 |
| `r0`-`r7` | ARM / RISC-V 寄存器编号 |
| `m` | 内存操作数 |
| `i` | 立即数 |
| `cc` | 条件码寄存器（flags） |
| `memory` | 内存可能被改（编译器需刷新寄存器缓存） |

#### 10.3.3 vs Rust `asm!` *(0.3.3 新增)*

UltraCPP 选择 C 风格，不采用 Rust `asm!` 宏：

| UltraCPP（C 风格）| Rust `asm!` |
|---|---|
| `asm { "syscall" : "=a"(r) : "0"(n) : "cc" }` | `asm!("syscall", out("rax") r, in("rax") n, lateout("rcx") _, lateout("r11") _, options(preserves_flags))` |
| Clobber list 第四段 | `lateout(...)` / `options(...)` |
| `"0"` 复用前操作数 | 直接传同名变量 |
| `asm volatile { ... }` | `options(preserves_flags, nostack)` 等 |

#### 10.3.4 实现状态 *(0.3.4 新增, per 0.3.3 process §3.1 commit 8b)*

> **[0.3.4 新增]** §10.3 描述的内联汇编机制在 **0.3.3 commit 8b** 完成 codegen 实现（基本 GCC 约束子集）。

| 语法子集 | 实现状态 | 关联 commit |
|----------|----------|-------------|
| `asm { "syscall" : ... }` 基本格式 | ✅ 已实现（per 0.3.3 commit 8b）| 8b |
| 约束字符 `r` / `a` / `D` / `S` / `d` / `c` / `b` / `=r` | ✅ 已实现 | 8b |
| 约束字符 `+r` / `0`-`7`（寄存器复用）| ✅ 已实现 | 8b |
| 约束字符 `m` / `i` / `cc` / `memory` | ✅ 已实现 | 8b |
| `asm volatile { ... }` | ✅ 已实现 | 8b |
| 约束字符 `r0`-`r7`（ARM / RISC-V 寄存器编号）| ⚠️ 未实现（0.3.3 阶段 x86_64 only）| 推 0.3.5+ |

**0.3.4 确认**：基本 GCC 约束子集完整实施（通过 `@builtin_asm_*` 转发 LLVM IR），完整约束集留 0.4.0+。0.3.4 commit 10e `lib/sys/raw.uc` 用本机制实现 `sys::raw_syscall`（x86_64 only）。

### 10.4 `sys::` 系统调用 namespace *(0.3.3 新增, per runtime-architecture §6)*

UltraCPP 引入 `sys::` 命名空间作为跨平台系统调用抽象层。模仿 glibc 模式（high-level wrapper + low-level entry + inline assembly），但全部用 UltraCPP 自己写，不接 libc。

#### 10.4.1 架构 *(0.3.3 新增, per runtime-architecture §6.1)*

```
用户代码
    sys::read(fd, buf, len)
        ↓
UltraCPP stdlib 层 (lib/sys/sys.uc, UltraCPP 写)
        ↓
UltraCPP low-level 层 (lib/sys/raw.uc, asm { "syscall" ... })
        ↓
LLVM IR 层
        ↓
机器码 (syscall / svc / ecall)
        ↓
内核态 (Linux / macOS / Windows NT / FreeBSD)
```

#### 10.4.2 `sys::` API 第一版 *(0.3.3 新增, per runtime-architecture §6.2)*

| 类别 | 函数 | 说明 |
|------|------|------|
| **文件 I/O** | `sys::read(fd, buf, len)` | 从 fd 读 len 字节到 buf |
| | `sys::write(fd, buf, len)` | 写 len 字节从 buf 到 fd |
| | `sys::open(path, flags)` | 打开 / 创建文件，返回 fd |
| | `sys::close(fd)` | 关闭 fd |
| | `sys::lseek(fd, offset, whence)` | 移动文件指针 |
| **内存映射** | `sys::mmap(addr, len, prot, flags, fd, offset)` | 内存映射 |
| | `sys::munmap(addr, len)` | 取消映射 |
| | `sys::mprotect(addr, len, prot)` | 修改内存保护 |
| **进程** | `sys::exit(code)` | 终止进程 |
| | `sys::getpid()` | 获取进程 ID |
| | `sys::fork()` | fork 子进程 |
| | `sys::execve(path, argv, envp)` | 执行新程序 |
| | `sys::wait4(pid, status, options, rusage)` | 等待子进程 |
| **线程** | `sys::clone(...)` | 创建线程 / 子进程 |
| | `sys::futex_wait(addr, val)` | futex 等待 |
| | `sys::futex_wake(addr, max)` | futex 唤醒 |
| **时间** | `sys::clock_gettime(clock_id, ts)` | 获取时钟时间 |
| | `sys::nanosleep(req, rem)` | 高精度睡眠 |
| **raw** | `sys::raw_syscall(num, arg1, arg2, ...)` | low-level entry（直通内核） |

#### 10.4.3 跨平台统一命名原则 *(0.3.3 新增, per runtime-architecture §6.3)*

- **用户代码用 `sys::read(fd, buf, len)`** —— 所有平台相同
- **实现按 `@ifdef(TARGET_OS_*)` 区分** —— 同一接口名在不同平台分发到不同实现（using §10.2 macros）
- **实现按 `@ifdef(TARGET_ARCH_*)` 区分** —— 同一接口在不同架构分发到不同 `asm { ... }` 块（using §10.3 inline assembly）
- **目录组织**（`lib/sys/`）：
  ```
  lib/sys/
  ├── sys.uc          // sys:: namespace API 公共头
  ├── raw.uc          // sys::raw_syscall low-level 实现
  ├── linux_uc        // Linux-specific 实现（@ifdef 标记）
  ├── darwin_uc       // macOS-specific 实现
  ├── windows_uc      // Windows-specific 实现（Win32 syscalls）
  └── freebsd_uc      // FreeBSD-specific 实现
  ```

#### 10.4.4 与 stdlib 的关系 *(0.3.3 新增)*

- `sys::` 是 **UltraCPP 自带 stdlib**（写在 `lib/sys/*.uc`），**不**是 builtin（§11.0.1 不收录）
- 用户通过 `import "lib/sys/sys.uc"` 使用；不需要 `BUILTIN_SIGS[]` 表（per runtime-architecture §5.3）
- **raw_syscall**：`sys::raw_syscall` 接受 syscall number + args，由 `lib/sys/raw.uc` 用 `asm { "syscall" ... }`（x86_64） / `asm { "svc #0" ... }`（aarch64） / `asm { "ecall" ... }`（riscv64）实现（per §10.3 + runtime-architecture §6.1）

#### 10.4.5 实现状态 *(0.3.4 新增, per 0.3.3 process §3.1 commit 8c)*

> **[0.3.4 新增]** §10.4 描述的 `sys::` namespace 在 **0.3.3 commit 8c** 完成 codegen 实现（基本 `sys$*` 前缀映射）。

| API | 实现状态 | 关联 commit |
|-----|----------|-------------|
| `sys$write(1, s, len)`（`sys::write` 前缀形式）| ✅ 已实现（per 0.3.3 commit 8c）| 8c |
| `sys$read(fd, buf, len)` | ✅ 已实现 | 8c |
| `sys::write(fd, buf, len)` 完整形式 | ⚠️ partial（0.3.3 阶段为 `sys$*` 前缀映射）| 推 0.3.4 commit 10e |
| `sys::read(fd, buf, len)` 完整形式 | ⚠️ partial | 推 0.3.4 commit 10e |
| `sys::open(path, flags)` | ❌ 未实现（0.3.3 阶段无此函数调用）| 推 commit 10e |
| `sys::close(fd)` | ❌ 未实现 | 推 commit 10e |
| `sys::exit(code)` | ❌ 未实现（main 仍走 libc exit）| 推 commit 10e |
| `sys::raw_syscall(num, ...)` | ❌ 未实现 | 推 commit 10e |
| `sys::` 命名空间语法解析（`sys::name(...)` 形式）| ⚠️ partial（0.3.3 阶段为 `sys$*` 前缀而非 `sys::*`）| 推 0.3.5+ |
| 跨平台统一分发（`@ifdef(TARGET_OS_*)`）| ✅ 已实现（preprocessor 支持）| 8c |

**0.3.4 确认**：0.3.4 commit 11b 确认 0.3.3 commit 8c 全部落地。`sys::name(args)` 与 `sys$name(args)` 共存；底层走 `gen_syscall_call` 直接 emit libc `@syscall`。完整 `sys::*` 全 namespace 语法 + 6-8 个 `sys::*` 函数完整实现由 0.3.4 commit 10e（`lib/sys/sys.uc` + `lib/sys/raw.uc`）推进。

### 10.5 与其他章节的交叉引用 *(0.3.3 新增)*

| 章节 | 关系 |
|------|------|
| §10.1 `extern "C"` 块 | 用户声明外部 C 函数；**不用于 stdlib**（per runtime-arch §3.1） |
| §10.2 预编译宏 | C 风格 `@ifdef` / `@if defined()`；`sys::` 跨平台分发靠它 |
| §10.3 内联汇编 | C / GCC 风格 `asm { ... }`；`sys::raw_syscall` 实现靠它 |
| §10.4 `sys::` namespace | 跨平台系统调用抽象（用户在 `lib/` 里实现，不进 builtin 表） |
| §11.0.2 UltraCPP stdlib 路径 | `lib/sys.uc` 属于 UltraCPP stdlib（§11.0.2） |
| §12.1 EBNF | `preprocessor_directive` / `asm_block` / `sys_call` 产生式（0.3.3 加） |

#### 10.5.1 错误码映射 *(0.3.3 新增, per runtime-architecture §11)*

> **[0.3.3 新增]** 集成 runtime-architecture 决策后, 以下 6 个新错误码在 `src-c/include/uc_error.h` 加入枚举（与现有 `UC_ERR_*` 列表保持一致格式）。0.3.0 引入的 3 个错误码（`UC_ERR_MODLAW` / `UC_ERR_CROSS_THREAD_MOVE` / `UC_ERR_OWNING_HEAP_VIEW`）保留并在 §12.4 历史表中记录。

| 错误码 | 触发场景 | 关联章节 |
|---|---|---|
| `UC_ERR_ParserUnknownDirective` | parser 遇到未知 `@directive`（如 `@foo`，不在 `@ifdef` / `@if` / `@ifndef` / `@else` / `@elif` / `@end` / `@define` / `@error` / `@warning` 列表内） | §10.2 预编译宏 |
| `UC_ERR_CodegenAllocNoType` | `alloc(T)` 调用时 `T` 无法推断（无类型注解 + 无参数提示） | §7.2 / §11.0.1 |
| `UC_ERR_CodegenModNonRef` | `mod(expr)` / `unmod(expr)` 表达式的实参不是 `&T` 引用（如临时变量、字面量） | §4.9 / §4.10 / §11.0.1 / Rule 24 |
| `UC_ERR_CodegenAsmInvalidSegment` | `asm { ... }` 块的 4 段格式中某一区段语法错误（如输出约束不是 `=X` 形式） | §10.3 内联汇编 |
| `UC_ERR_CodegenSysInvalidName` | `sys::name(args)` 的 `name` 不在白名单（如 `sys::foo`） | §10.4 `sys::` namespace |
| `UC_ERR_AsmTooManyConstraints` | `asm` 块的操作数（outputs + inputs）超过 16 个（LLVMInlineAsm 上限） | §10.3 内联汇编 |

**实施位置**：`src-c/include/uc_error.h`（enum 增量 6 项）+ `src-c/src/error.c`（字符串映射 6 行）。

### 10.6 C 类型映射 *(0.3.0 引入, 0.3.3 修订)*

> **[0.3.3 修订]** 本节从 §10.2 重编号为 §10.6。新增 §10.2（预编译宏） / §10.3（内联汇编） / §10.4（`sys::` 命名空间）。

| C 类型 | UltraCPP 类型 |
|--------|---------------|
| `T*` | `T*` |
| `const T*` | UltraCPP 中含义**与 C 不同**（见 §3.8 / §7.10）；FFI 桥接时须显式标注 `readonly` |
| `void*` | `void*` |
| `int (*)(T)` | `int (*)(T)` |
| `int` | `int` |
| `double` | `double` |
| `char` | `char` |
| `char*` | `char*` |

> **[0.3.0 新增注记]** 在 FFI 边界，C 标准库的 `const char*`（指向 const data）在 UltraCPP 中既是"指针锁定"又是"数据只读"。两种语义都与 C 等价（都不能写），但 rebind 语义不同：如果 UltraCPP 代码持有来自 C 的 `const char*`，编译器把它视为**指针锁定**——这与 C 的 `const char*` 行为（指针可改）有差异，是 0.3.0 在 §3.8 自定义语义的直接后果。

> **FFI 返回类型**遵循 §6.2.1 的优先级 3（extern 声明）。

### 10.7 unsafe 块 *(0.3.0 沿用 0.2.0 · D-2, D-7, 0.3.3 重编号)*

> **[0.3.3 修订]** 本节从 §10.3 重编号为 §10.7。

```cpp
unsafe {
    // FFI 调用和原始指针操作
}
```

**示例：**
```cpp
extern "C" {
    void* malloc(int size);
    void free(void* ptr);
}

char* allocate_buffer(int size) {
    unsafe {
        return malloc(size);
    }
}
```

> **[0.3.0 沿用 0.2.0 · D-2, D-7]** `unsafe` 块内**抑制**所有权与借用检查：§7.1 的隐式 `mod()` 检查、§7.8 的借用规则、§7.9 的 `DanglingReference` 在 `unsafe` 块内不报告。`unsafe` **不改变**语义——`move` 仍然把源置 null（§7.4），`&` 仍然是引用（§7.8）——它只是把安全责任转移给程序员。

### 10.8 内联汇编（legacy 简单语法） *(0.3.0 引入, 0.3.3 重编号)*

> **[0.3.3 修订]** 本节从 §10.4 重编号为 §10.8。**主参考为 §10.3 内联汇编机制**（C / GCC 约束风格完整版）。本节保留 legacy 简化语法作为最小示例，仅供阅读。

嵌入汇编必须放在 `unsafe` 块中。

**语法（简化）**：
```cpp
unsafe {
    asm {
        "assembly instruction"
        : [name] "constraint"(output_var)
        : [name] "constraint"(input_var)
        : "clobbered_register"
    }
}
```

**示例：**
```cpp
int result;
int input = 10;

unsafe {
    asm {
        "mov eax, %[in_val]"
        "add eax, 5"
        "mov %[out_val], eax"
        : [out_val] "=r"(result)
        : [in_val] "r"(input)
        : "eax"
    }
}
```

**约束字符（节选，完整见 §10.3.2）：**
| 约束 | 含义 |
|------|------|
| `r` | 通用寄存器 |
| `=` | 输出寄存器 |
| `&` | early clobber（不得重用） |
| `m` | 内存引用 |

---

## 11. 标准库约定 *(0.3.2 修订, 0.3.3 修订, runtime-architecture 集成)*

> **[0.3.2]** 本章描述 UltraCPP 标准库函数的**签名约定**和**语义约定**。**builtin 签名总表**见 §11.0 (0.3.2 新增, 0.3.3 双分类, 2026-08-17 runtime-architecture 重修订)。编译器在 codegen 阶段查询 §11.0.1 表以确定 6 项 builtin 语言原语的返回类型；intrinsic 直接翻译；stdlib 走 `import "lib/*.uc"` 不查表。详见 §6.2.1。

> **[0.3.3 修订]** §11.0 重新分组为 (per `.dev/drafts/0.3.0-runtime-architecture.md` §5):
> - §11.0.1 **builtin 语言原语 (6 项)**: `alloc` / `free` / `move` / `clone` / `mod` / `unmod` (全部 codegen emit, 全部进 BUILTIN_SIGS[])
> - §11.0.2 **UltraCPP stdlib 路径** (`lib/*.uc`): 替代 0.3.2 原 14 项 stdlib 约定; stdlib 不再进 BUILTIN_SIGS[], 由 UltraCPP 自己写
>
> **退入 intrinsic 的项** (4 项, 不进 BUILTIN_SIGS[]): `null` / `is_null(p)` / `sizeof(T)` / `alignof(T)` — codegen 特殊处理 (per runtime-architecture §5.2)
>
> 概念分类详见 §7.0 (运算符 vs builtin 函数)。

### 11.0 builtin 签名总表 *(0.3.2 新增, 0.3.3 修订, 双分类, runtime-architecture 集成)*

UltraCPP 提供以下 **builtin 函数**。builtin 函数由编译器**内置识别**：

- 不需要 `#include` 任何头文件
- 不需要在用户代码中声明
- codegen 阶段直接 emit 对应的 LLVM IR 调用

> **历史**：0.3.1 spec 没有 builtin 签名总表，散落在 §11.1-§11.6 中。0.3.2 集中为 §11.0（单分类 18 项）。**0.3.3 重新分组**为 §11.0.1（6 项语言原语 builtin）+ §11.0.2（UltraCPP stdlib 路径 `lib/*.uc`），per `.dev/drafts/0.3.0-runtime-architecture.md` §5。

> **[0.3.3 修订 — runtime-architecture 集成]** 0.3.2 §11.0 是单分类（18 项）。0.3.3 按 §7.0 概念 + runtime-architecture §3-§5 重新分组为：
>
> - §11.0.1 **builtin 语言原语（6 项）** — 涉及所有权 / 内存 / 修改权：`alloc` / `free` / `move` / `clone` / `mod` / `unmod`（per runtime-architecture §5.1 表）
> - §11.0.2 **UltraCPP stdlib 路径** — stdlib 全部由 UltraCPP 自己写在 `lib/*.uc`（per runtime-architecture §3.1 "UltraCPP 不使用 libc 作为 runtime", §9.2 自举）；不在 BUILTIN_SIGS[] 表
>
> **退入 intrinsic 的项**（B 类，codegen 特殊处理，**不进 BUILTIN_SIGS[]**）：`null` / `is_null(p)` / `sizeof(T)` / `alignof(T)` — 编译期常量或一元指令，不需要函数调用（per runtime-architecture §5.2）
>
> **被废弃的项**（per runtime-architecture §5.3）：`sizeof_impl()` / `alignof_impl()` / `clone_impl()` / `print*`（作为 builtin） / `strlen` / `strcpy` / `strcmp` / `memcpy` / `memmove` / `memset` / `abs_int`（作为 builtin 或 FFI） — 一律改为 UltraCPP stdlib（`lib/*.uc`），或改为 intrinsic（sizeof(T) / alignof(T)）
>
> **遗留 libc 转发的项**：`malloc` — 仍走 `extern "C"`（§10.1 修订版），但仅作为**用户自己声明的 C 函数**，**不用于 stdlib**；`free` **不再走 extern "C"**，改为 builtin 收录 §11.0.1

#### 11.0.1 builtin 语言原语 *(0.3.3 新分类, runtime-architecture §5.1 集成)*

§7.0 描述的「语言原语」builtin，与内存所有权 / 借用检查直接相关。**共 6 项**，全部为 A 类 builtin，全部 codegen emit，全部进入 `BUILTIN_SIGS[]`：

| 函数 | 返回类型 | 参数签名 | spec 章节 | 备注 |
|------|----------|----------|----------|------|
| `alloc(T)` | `T*`（owning） | `T` 类型 | §7.2 | 堆分配，返回 owning 指针；codegen emit `call i8* @malloc(sizeof(T))` |
| `free(p)` | `void` | `T*`（owning） | §7.3 | 释放 owning 指针；codegen emit `call void @free(i8* %p)` + 标记 p 为 freed |
| `move(p)` | `T*`（新 owner，源 moved-out） | `T*` | §7.4 | 所有权转移；codegen emit IR 标记 + 加载 + 返回新 owner |
| `clone(s)` | `T*`（独立 owning 副本） | `T*` | §7.5 | 深拷贝 / 独立副本；codegen emit `alloc(T) + LLVMBuildMemcpy` |
| `mod(ref)` | `void` | `T&`（引用） | §4.9 | 申请修改权（Rule 24）；codegen emit IR marker（`@uc_borrow_mod_enter`，借用检查器状态入口） |
| `unmod(ref)` | `void` | `T&`（引用） | §4.10 | 释放修改权（Rule 24）；codegen emit IR marker（`@uc_borrow_mod_exit`，借用检查器状态出口） |

**类型参数化**：`alloc(T)` / `move(p)` / `clone(s)` 是类型参数化 builtin — 参数 `T` 在 codegen 阶段由上下文推导（e.g. `alloc(int)` → 分配 int 大小）。

**优先级**：`mod` `unmod` `move` `clone` **不在 §4.1 运算符优先级表**（0.3.3 修订），详见 §7.0.3 关键澄清。

**`free` 提升说明**（runtime-architecture §3.2 + §5.1）：
- 0.3.2 / 0.3.3 原计划中 `free` 走 `extern "C"` FFI；现在提升为 builtin
- 理由：free 是 borrow-checker 必须追踪的状态变更点（p 之后视为 dangling），不能透明穿过 libc（否则 IR 无法标记 freed 状态），必须由 compiler 显式 emit

**`mod` / `unmod` 提升说明**（runtime-architecture §3.2 + §5.1）：
- 0.3.2 / 0.3.3 原计划中 `mod` / `unmod` 是「编译期决策、无 runtime 函数」；现在提升为 builtin
- 理由：它们是 borrow-checker 状态入口 / 出口，编译器必须在 IR 中显式记录（emit `@uc_borrow_mod_enter` / `@uc_borrow_mod_exit` intrinsic）；不再需要专门的 `UC_INTRINSIC_MOD` / `UC_INTRINSIC_UNMOD` 间接路径

#### 11.0.2 UltraCPP stdlib 路径（0.3.4+ 实化） *(0.3.3 新分类, 0.3.4 实化, per runtime-architecture §3.1 + §9.2)*

> **[0.3.4 实化 — per D1 + 0.3.4 plan §3.1 commits 10a-10e]** 0.3.3 spec 把 §11.0.2 描述为「未来 UltraCPP stdlib 路径」 + 「0.3.x 阶段 `lib/uc_runtime.c` 是 C 临时 bootstrap」。0.3.4 调研（per 0.3.3 process §2.5）发现 **`lib/uc_runtime.c` 从未存在**；0.3.4 直接 bootstrap `lib/*.uc`，**不创建** C 临时 bootstrap。

**0.3.4 当前状态**（per commits 10a-10e）：

| 文件 | 内容 | 0.3.4 commit | LOC |
|---|---|---|---|
| `lib/print.uc` | `uc_print_str`（完整实现：`sys::strlen` + `sys::write`） / `uc_print_num`（完整 itoa：负数 + 16 字节缓冲 + 反转 + `sys::write`） / `uc_print_float`（STUB：写 `<float>` 字符串；commit 14h (0.3.5) 完整化为简单 ftoa（2 位小数））| 10a | ~50-80 |
| `lib/string.uc` | `uc_strlen` / `uc_strcpy` / `uc_strcmp` | 10b | ~40-60 |
| `lib/memory.uc` | `uc_memcpy` / `uc_memmove` / `uc_memset` | 10c | ~50-70 |
| `lib/math.uc` | `uc_add` / `uc_abs`（替代 abs_int）| 10d | ~15-20 |
| `lib/alloc.uc` | `uc_alloc` / `uc_free`（thin wrappers）| 10d | ~10 |
| `lib/sys/sys.uc` + `lib/sys/raw.uc` | `sys::write/read/open/close/exit/...` + `raw_syscall` | 10e | ~130-200 |

**注**：`lib/sync.uc`（`mutex<T>` / `atomic<T>`）留 0.3.5+（M5 前置，不在 0.3.4 范围）。
**注 2**：`lib/io.uc` 是 0.3.3 之前遗留的测试夹具文件（含旧式 `sys$` 前缀 print_str 与 getValue），功能与 `lib/print.uc::uc_print_str` 重叠；自 0.3.5 起 deprecate，**0.3.6 commit 删除**。`lib/io.uc` 不属于 §11.0.2 6 文件清单（print/string/memory/math/alloc/sys）。

**0.3.4 完成后，stdlib 调用走以下路径**：
- `BUILTIN_SIGS[]` 仅含 6 项 primitives（alloc/free/move/clone/mod/unmod，per §11.0.1）
- abs_int 等历史 builtin 名通过 extern function lookup 链接到 lib/math.o 等
- codegen 调用 emit `call void @uc_abs(i32 %x)` 或类似，链接时由 linker 解析

**与 0.3.3 文本的关系**：0.3.3 spec §11.0.2 描述「14 项 stdlib 删除 + `lib/uc_runtime.c` 临时 bootstrap」规划，0.3.4 把该规划实化为 6 文件落地。`BUILTIN_SIGS[]` **不再** 含 `abs_int` / `print` / `strlen` / `memcpy` 等历史 builtin，全部走 extern lookup（per §11.0.3）。

详见 §11.0.5 UltraCPP stdlib 自举路径（Stage 1 设计，0.4.0+ 启动）。

#### 11.0.3 codegen 集成（0.3.4 修订） *(0.3.2 §11.0.2 改, 0.3.3 §11.0.3 改, 0.3.4 重写, runtime-architecture 集成)*

`BUILTIN_SIGS[]`（`src-c/src/codegen.c`）现仅含 **6 项 primitives**（per §11.0.1）：
- alloc / free / move / clone / mod / unmod

**历史 builtin（如 abs_int）已从 BUILTIN_SIGS[] 移除**（per commit 11a）：
- abs_int 行为简单（`n < 0 ? -n : n`），由 UltraCPP `lib/math.uc::uc_abs` 实现
- 调用方仍可写 `abs_int(x)`，codegen 通过 extern function lookup 链接到 lib/math.o::uc_abs
- 其他历史 builtin（print / strlen / memcpy 等）随 stdlib 引导（`lib/*.uc`）走 extern 路径

**C bootstrap 不存在**：原计划假设的 `lib/uc_runtime.c` C bootstrap 已被证伪（per 0.3.3 process §2.5）。UltraCPP 直接 bootstrap `lib/*.uc`，详见 §11.0.5。

`src-c/src/codegen.c` 中，builtin 签名表用 `static const struct` 数组维护：

```c
typedef struct {
    const char* name;
    const char* ret_type;       // LLVM IR 类型字符串
    int is_void;
} BuiltinSig;

// [0.3.3 + runtime-architecture §5.1] BUILTIN_SIGS[] 简化为 **6 项** 语言原语:
// - alloc / free / move / clone / mod / unmod
// - null / is_null / sizeof / alignof 不在此表 (intrinsic, codegen 特殊处理, per runtime-architecture §5.2)
// - stdlib (print / strlen / memcpy / ...) 不在此表 (改 UltraCPP stdlib lib/*.uc, per runtime-architecture §5.3)
//
// [0.3.4 commit 11a — D2] abs_int 已从 BUILTIN_SIGS[] 移除（原第 1 项）：
// - 原 inline IR `@builtin_abs_int` 21 行实现从 `get_builtins_defs()` 完全删除
// - abs_int 调用走 codegen 主流水 P3 `lookup_extern_func("abs_int")` 路径
// - 链接时由 linker 解析到 lib/math.o::uc_abs（来自 lib/math.uc）
//
// 类型参数化由 `UCExprCall.return_type` 字段承担 (0.3.2 §6.2.1 已加);
// 不需要 `is_type_parametric` 字段.
//
// mod/unmod 是 **builtin**, emit IR marker (`@uc_borrow_mod_enter` / `@uc_borrow_mod_exit`),
// 不是占位签名; 也不再走单独的 UC_INTRINSIC_MOD / UC_INTRINSIC_UNMOD 路径.
static const BuiltinSig BUILTIN_SIGS[] = {
    /* [0.3.4 commit 11a] Removed abs_int per D2: migrate to lib/math.uc::uc_abs */
    { "alloc",   "T*",   0 },  // emit: call @malloc(sizeof(T)) + 类型标记
    { "free",    "void", 1 },  // emit: call @free(%p) + 标记 %p 为 freed (NEW, 从 FFI 提升为 builtin)
    { "move",    "T*",   0 },  // emit: IR 标记 + 加载 + 返回新 owner
    { "clone",   "T*",   0 },  // emit: alloc(T) + LLVMBuildMemcpy
    { "mod",     "void", 1 },  // emit: call @uc_borrow_mod_enter(%ref) IR marker (NEW, 从 intrinsic 提升为 builtin)
    { "unmod",   "void", 1 },  // emit: call @uc_borrow_mod_exit(%ref) IR marker (NEW, 从 intrinsic 提升为 builtin)
    { NULL, NULL, 0 }  // sentinel
};
```

**UCExprCall codegen 主流程**（per 0.3.2 §6.2.1 + 0.3.4 修订，主流水 4 级优先级链）：

```c
// 4 级优先级链（per 0.3.2 §6.2.1）
// P1: sys$* 前缀 → 直接映射 libc / sys:: (0.3.3 commit 8c)
// P2: BUILTIN_SIGS[] 查表 → 6 项语言原语 emit
// P3: lookup_extern_func(name) → extern lookup (用户 extern "C" 或 stdlib)
// P4: default → 假设返回 i32（fallback）

static LLVMValueRef gen_call(CodeGen* g, UCExprCall* call) {
    const char* fn_name = call->fn_name;

    // P1: sys$* 前缀 (libc / sys:: 转发)
    if (strncmp(fn_name, "sys$", 4) == 0) {
        return gen_syscall_call(g, call, fn_name + 4);
    }

    // P2: BUILTIN_SIGS[] 查表 — 6 项语言原语
    const BuiltinSig* sig = lookup_builtin_sig(fn_name);
    if (sig != NULL) {
        return emit_builtin_call(g, call, sig);
    }

    // P3: extern lookup — [0.3.4 commit 11a] abs_int 在此分支
    // - 用户 extern "C" { int abs_int(int x); } 声明
    // - stdlib (lib/math.uc::uc_abs) 提供 @abs_int 符号
    // - libc fallback (0.4.0+ 完全移除)
    LLVMValueRef fn = lookup_extern_func(g, fn_name);
    if (fn != NULL) {
        return emit_extern_call(g, call, fn);
    }

    // P4: default — 假设返回 i32（fallback）
    return emit_default_call(g, call, "i32");
}
```

> **[0.3.3 plan A 简化 + runtime-architecture 集成 + 0.3.4 重写]**
>
> - `BUILTIN_SIGS[]` **仅收录 §11.0.1 语言原语 6 项**（此前为 7 项；`null` 改为 intrinsic, per runtime-architecture §5.2）
> - **stdlib 不进 BUILTIN_SIGS[]** — 改为 UltraCPP stdlib（`lib/*.uc`）；不再走 `extern "C" libc`
> - **不**需要 `is_type_parametric` 字段 — 类型参数化由 `UCExprCall.return_type` 字段承担（0.3.2 §6.2.1 已加）
> - **不**需要 stdlib 14 项 — 全部由 UltraCPP 自带 stdlib 提供
> - **不**需要 `UC_INTRINSIC_MOD` / `UC_INTRINSIC_UNMOD` 单独路径 — mod / unmod 移入 BUILTIN_SIGS[], 走 `BUILTIN_SIGS[]` 查表 + 统一的 `UC_EXPR_CALL` emit 路径（与 §6.2.1 优先级链一致）
> - **不再**有占位签名 — mod / unmod 全部是真实 emit（约 5 LOC each，输出 `@uc_borrow_mod_enter` / `@uc_borrow_mod_exit` IR marker）
> - **abs_int 已移除**（0.3.4 commit 11a）— 走 P3 extern lookup 路径，链接 lib/math.o::uc_abs

#### 11.0.4 添加新 builtin 的流程 *(0.3.2 §11.0.3 改, 0.3.3 §11.0.4 改, runtime-architecture 集成)*

未来要加新 builtin（语言原语，6 项之一）：

1. 在 §11.0.1 签名表中加一行（函数名 + 返回类型 + 参数签名 + spec 章节引用）
2. 在 `src-c/src/codegen.c` 的 `BUILTIN_SIGS[]` 加对应 C 结构项
3. codegen 层 emit 路径：
   - **alloc / free / move / clone**：emit runtime 调用（LLVM IR 指令 + 所有权标记）
   - **mod / unmod**：emit `call @uc_borrow_mod_{enter,exit}` IR marker（借用检查器状态入口 / 出口）
4. **stdlib 函数**（print / strlen / memcpy / abs_int 等）**不进 BUILTIN_SIGS[]** — 在 `lib/*.uc` 用 UltraCPP 自己实现（per runtime-architecture §3.1 + §5.3）；用户通过 `import "lib/print.uc"` 或类似机制使用
5. **intrinsic 函数**（null / is_null / sizeof / alignof）不进 BUILTIN_SIGS[] — codegen 直接翻译（per runtime-architecture §5.2）；在 AST / parser 用关键字 + codegen 用 `UC_EXPR_IS_NULL` / `UC_EXPR_SIZEOF` / `UC_EXPR_ALIGNOF` / `UC_EXPR_NULL` 特殊节点

#### 11.0.5 UltraCPP stdlib 自举路径（0.3.4+）

UltraCPP 编译器采用分阶段自举路径：

- **Stage 0（0.3.3，**已废弃**）**：C bootstrap（`lib/uc_runtime.c`）作为临时 stdlib。该路径已被证伪——实测 src-c/ 当前无 lib/uc_runtime.c，采用 inline LLVM IR + libc 链接的混合方案（per `.dev/drafts/0.3.3-implementation-process.md` §2.5）。

- **Stage 1（0.3.4，当前阶段）**：UltraCPP 编译 `lib/*.uc`，输出 LLVM IR，与 src-c/ 编译器 IR 链接。
  - **编译路径**：`uc_compiler` (src-c/build/uc_lexer → uc_compiler) 读 lib/*.uc → emit LLVM IR → `llc` → lib.o
  - **链接路径**：用户 .uc 编译 → emit LLVM IR → `llc` → user.o → 链接 lib.o + libc (for sys$*) → exe
  - **启动约束**：`lib/*.uc` 必须先于用户代码编译；用 `bootstrap=stage1` 标志区分

- **Stage 2（0.4.0+，**未来**）**：UltraCPP 编译自身 src-uc/。

详见 runtime-architecture §9。

#### 11.0.6 与其他章节的交叉引用 *(0.3.2 §11.0.4 改, 0.3.3 §11.0.5 改, 0.3.4 §11.0.5 → §11.0.6 改, runtime-architecture 集成)*

| 章节 | 关系 |
|------|------|
| §7.0 概念清晰化 | §11.0.1 收录的是 §7.0.2 builtin 函数（§6.2.1 优先级 1） |
| §7.2 / §7.3 / §7.4 / §7.5 alloc / free / move / clone | §11.0.1 对应行 |
| §4.9 / §4.10 mod / unmod | §11.0.1 列表行（**builtin** — codegen emit `@uc_borrow_mod_{enter,exit}` IR marker, 借用检查器状态由 codegen + borrow checker 同步维护） |
| §6.2.1 函数返回类型 | builtin 签名是优先级 1 的依据 |
| §7.6 `null` 常量 | `null` **不**在 §11.0.1 表（intrinsic, per runtime-architecture §5.2）；codegen emit `i8* null` 常量 |
| §7.4 / §11.3 `malloc`（C stdlib, 非 UltraCPP） | `malloc` 通过 §10.1 extern "C" 引入；**仅供用户自己声明外部 C 函数使用**，**不用于 stdlib**（per runtime-architecture §3.1） |
| `lib/*.uc`（UltraCPP stdlib 路径） | §11.0.2 改为指明 `lib/print.uc` / `lib/string.uc` / `lib/memory.uc` / `lib/math.uc` / `lib/alloc.uc` / `lib/sys.uc`（per runtime-architecture §9.2） |
| §10.4 `sys::` namespace | 系统调用抽象（per runtime-architecture §6）；stdlib/sys.uc / stdlib/sys/raw.uc |
| §10.2 跨平台宏 | `@ifdef(TARGET_OS_*)` / `@if defined(...)` 等（per runtime-architecture §7） |
| §10.3 内联汇编 | `asm { "..." : ... }` GCC 约束风格（per runtime-architecture §8） |
| §12.1 EBNF | `builtin_function_call` 产生式见 §12.1（0.3.3 新增）— 仅包含 6 项语言原语 + 0 项 stdlib（stdlib 走 `import "lib/*.uc"`, 不在 builtin_function_call） |

### 11.1 基本 I/O 函数

> **[0.3.2]** 本节详细说明以下 builtin：`print`, `print_num`, `print_float`。完整签名表见 §11.0。

```cpp
void print_num(int n);        // 打印整数
void print(const char* s);    // 打印字符串
void print_float(double f);   // 打印浮点数
```

### 11.2 字符串函数

> **[0.3.2]** 本节详细说明以下 builtin：`strlen`, `strcpy`, `strcmp`。完整签名表见 §11.0。

```cpp
int strlen(const char* s);  // 字符串长度
char* strcpy(char* dest, const char* src);
int strcmp(const char* a, const char* b);
```

### 11.3 内存函数

> **[0.3.2]** 本节详细说明以下 builtin：`memcpy`, `memmove`, `memset`, `alloc` (类型参数化), `move` (类型参数化)。完整签名表见 §11.0。

```cpp
void* memcpy(void* dest, const void* src, int n);
void* memmove(void* dest, const void* src, int n);
void* memset(void* s, int c, int n);
```

### 11.4 工具函数

> **[0.3.2]** 本节详细说明以下 builtin：`sizeof_impl`, `alignof_impl`, `is_null`, `clone_impl`, `abs_int`。完整签名表见 §11.0。

```cpp
int sizeof_impl();       // 类型大小
int alignof_impl();      // 类型对齐
bool is_null(int* ptr);  // 空检查
int* clone_impl(int* ptr);  // 指针克隆
```

### 11.5 线程原语 *(0.3.0 新增)*

```cpp
// --- 跨线程所有权 ---
move_to_thread(T* p, int tid);    // 显式移交所有权到线程 tid
spawn_thread(int tid, void fn()); // 启动无所有权传递的线程
spawn_thread_with(int tid, T* p); // 启动线程并隐式 move(p)
join_thread(int tid);             // 等待线程结束
join_all();                       // 等待所有已 spawn 的线程
current_tid();                    // 当前线程 id

// --- 线程局部存储 ---
__thread int x;                   // 每线程独立
__thread int buf[256];            // 每线程独立数组

// --- 共享原语 ---
mutex<T>          mx;             // 显式 mutex 包装（stdlib 类型，见 §11.5.1）
atomic<T>         at;             // 显式 atomic 包装（stdlib 类型，见 §11.5.1）
shared T*         sp = ...;       // 跨线程可见（编译期强制）

// --- 隐式线程支持 ---
thread1() { ... }                 // 由编译器自动生成 mutex / barrier
```

#### 11.5.1 共享同步类型 `mutex<T>` / `atomic<T>` *(0.3.0 新增)*

> **[0.3.0 新增]** `mutex<T>` 与 `atomic<T>` 是**标准库类型**（定义在 `lib/sync.uc`），**不是关键字**。它们提供跨线程修改权的显式包装。

**`mutex<T>`**：

| 成员 | 描述 |
|------|------|
| `lock()` | 加锁（阻塞直到获得锁） |
| `unlock()` | 解锁 |
| `try_lock()` | 尝试加锁，失败返回 `false` |
| `wait()` | 在条件变量上等待（与 `notify` 配合） |
| `notify()` / `notify_all()` | 唤醒等待者 |

```cpp
mutex<int> mx;

void worker() {
    mx.lock();
    // 临界区
    mx.unlock();
}
```

**`atomic<T>`**：

| 成员 | 描述 |
|------|------|
| `load()` | 原子读 |
| `store(v)` | 原子写 |
| `exchange(v)` | 原子交换 |
| `compare_exchange(expected, desired)` | CAS |

```cpp
atomic<int> state;

void worker() {
    int v = state.load();
    state.store(v + 1);
}
```

**与 `#modlaw` 的关系**：

- `mutex<T>` / `atomic<T>` 的 lock / load / store 操作**不**通过 `mod()` 系统——它们自带同步原语。
- 当变量类型为 `mutex<T>` 时，**编译器自动生成的 mutex 被抑制**（程序员已显式控制），避免双重加锁。
- `atomic<T>` 同理：硬件原子指令已保证可见性，无需编译器再生成同步代码。

### 11.6 `alloc` / `free` 配对约定 *(0.3.0 沿用 0.2.0 · D-6, 0.3.2 加 §11.0 跨引用)*

> **[0.3.0 沿用]** `alloc(T)` 与 `free(p)` 的配对由程序员负责，编译器**不强制**检查。
>
> **[0.3.2]** `alloc` 与 `move` 收录在 §11.0 builtin 签名总表 (类型参数化 builtin)；`free` / `malloc` 是 C 标准库函数，须通过 §10 `extern "C"` 块引入，**不在** §11.0 builtin 表中。详见 §11.0.4 与 §10.1。

**规范规则**：

1. 每一次成功的 `alloc(T)` 都产生一个新的分配，其所有权归接收该结果的 `unique T` 变量。
2. 每一个分配**应当**恰好被 `free` 一次。这是一条**程序员责任**。
3. 编译器**不**报告：未释放（内存泄漏）、重复释放、释放非 `alloc` 产物。
4. 编译器**仍然**报告 `UseAfterMove` / `UseAfterDrop` / `DanglingReference`。

---

## 12. 附录

### 12.1 完整 EBNF 语法 *(0.3.0 修订)*

> **[0.3.0]** 相对 0.2.0 的新增与改动（保留 0.2.0 全部产生式，新增 mod/unmod/move_to_thread/__thread/shared）：
> 1. 关键字列表新增 `mod`、`unmod`、`shared`、`__thread`、`move_to_thread`。
> 2. `unary_expression` 新增 `'mod' '(' expression ')'` 与 `'unmod' '(' expression ')'`（Rule 24）。
> 3. 新增 `preprocessor_directive` 顶层产生式并包含 `#modlaw` 规则（Rule 23）。
> 4. `unary_expression` 新增 `'move_to_thread' '(' expression ',' expression ')'`（Rule 26）。
> 5. `declaration`/`variable_declaration` 接受可选的存储类 `('shared' | '__thread')`（Rule 25, 27）。
> 6. `type` 的 `pointer_type` 增列 `const T*` 与 `T* const`（Q6，自定义语义与 C++ 相反）。

```
// === Program ===
program           ::= top_level_declaration*

top_level_declaration
                   ::= function_definition
                    | struct_definition
                    | export_declaration
                    | import_directive
                    | include_directive
                    | modlaw_directive           // [0.3.0 Rule 23]
                    | const_declaration
                    | ';'

// === Preprocessor ===
import_directive  ::= '#import' string_literal ('as' identifier)? ';'
include_directive ::= '#include' string_literal ';'

// [0.3.0 Rule 23] #modlaw directive — 6 legal combinations only
modlaw_directive  ::= '#modlaw' modlaw_perm modlaw_scope
modlaw_perm       ::= 'none' | 'exclusive' | 'shared'
modlaw_scope      ::= 'global' | 'module'

// === Export ===
export_declaration ::= 'export' declaration

// === Functions ===
function_definition
                   ::= type identifier '(' parameter_list? ')' compound_statement

parameter_list     ::= parameter (',' parameter)*
parameter          ::= identifier ':' type

// === Struct ===
struct_definition  ::= 'struct' identifier '{' struct_field_list '}'
struct_field_list ::= struct_field (';' struct_field)*
struct_field      ::= identifier ':' type

// === Declarations ===
declaration       ::= storage_class? variable_declaration
                    | const_declaration

// [0.3.0 Rule 25, 27] optional storage class on declarations
storage_class     ::= 'shared' | '__thread'

variable_declaration
                   ::= type identifier ('=' expression)? ';'

const_declaration  ::= 'const' type identifier '=' expression ';'

// === Types ===
type              ::= fundamental_type
                    | pointer_type
                    | unique_type              // [0.2.0 D-1]
                    | reference_type
                    | array_type
                    | function_type
                    | type_identifier

fundamental_type  ::= 'void' | 'bool' | 'char' | 'int' | 'i8' | 'i16' | 'i32' | 'i64'
                    | 'uint' | 'u8' | 'u16' | 'u32' | 'u64'
                    | 'f32' | 'f64' | 'usize' | 'isize'

// [0.3.0 Q6] custom `const T*` / `T* const` semantics — opposite of C++
pointer_type      ::= type '*'
                    | type '*' 'const'                              // [0.3.0 Q6] data read-only view
                    | 'const' type '*'                              // [0.3.0 Q6] pointer-locked read-only
                    | 'const' type '*' 'const'

// [0.2.0 D-1] `unique` is a generic type constructor: `unique T` for any T.
unique_type       ::= 'unique' type

// [0.3.0 Q2] reference declaration initializer is an lvalue (no `&` prefix)
reference_type    ::= type '&'

array_type        ::= type '[' integer_literal ']'
function_type     ::= type '(' type_list? ')'

// === Statements ===
statement         ::= expression_statement
                    | compound_statement
                    | selection_statement
                    | iteration_statement
                    | jump_statement
                    | free_statement
                    | declaration_statement

expression_statement
                   ::= expression? ';'

compound_statement ::= '{' statement* '}'

selection_statement
                   ::= 'if' '(' expression ')' statement ('else' statement)?

iteration_statement
                   ::= 'while' '(' expression ')' statement
                    | 'for' '(' for_init? expression? ';' expression? ')' statement
                    | 'for' '(' identifier ':' expression ')' statement

for_init          ::= declaration | expression

jump_statement    ::= 'return' expression? ';'
                    | 'break' ';'
                    | 'continue' ';'

free_statement    ::= 'free' '(' expression ')' ';'

declaration_statement
                   ::= declaration

// === Expressions ===
expression        ::= assignment_expression

// [0.3.1] Assignment LHS must be an lvalue (see §4.13.2). The grammar permits
// unary_expression on the LHS; the compiler enforces lvalue category in semantic
// analysis (or codegen for legacy paths).
assignment_expression
                   ::= lvalue '=' assignment_expression             // [0.3.1] LHS constraint
                    | lvalue compound_assignment assignment_expression

cast_expression    ::= '(' type ')' unary_expression
                    | unary_expression

// [0.3.1 Rule S2] lvalue / rvalue categorization — see §4.13 for full semantics.
lvalue            ::= identifier                                     // variable
                    | '*' cast_expression                            // unary deref
                    | postfix_expression '[' expression ']'         // index
                    | postfix_expression '.' identifier              // field access
                    | '(' lvalue ')'                                 // parenthesized lvalue
                    | lvalue '++'                                    // prefix increment (§4.13.2 row 6)
                    | lvalue '--'                                    // prefix decrement
                    | assignment_expression                          // result of `x = v` is lvalue

rvalue            ::= literal                                        // integer / float / string / char / bool / null
                    | postfix_expression '(' argument_list? ')'      // function call result
                    | postfix_expression '++'                        // postfix increment
                    | postfix_expression '--'                        // postfix decrement
                    | '(' rvalue ')'                                 // parenthesized rvalue
                    | '&' unary_expression                           // address-of result (pointer rvalue)
                    | 'move' '(' expression ')'
                    | 'clone' '(' expression ')'
                    | 'mod' '(' expression ')'                       // [0.3.0 Rule 24]
                    | 'unmod' '(' expression ')'                     // [0.3.0 Rule 24]
                    | 'move_to_thread' '(' expression ',' expression ')'

conditional_expression
                   ::= logical_or_expression ('?' expression ':' conditional_expression)?

logical_or_expression
                   ::= logical_and_expression ('||' logical_and_expression)*

logical_and_expression
                   ::= bitwise_or_expression ('&&' bitwise_or_expression)*

bitwise_or_expression
                   ::= bitwise_xor_expression ('|' bitwise_xor_expression)*

bitwise_xor_expression
                   ::= bitwise_and_expression ('^' bitwise_and_expression)*

// Binary infix '&' remains bitwise AND. Unary prefix '&' is a reference.
bitwise_and_expression
                   ::= equality_expression ('&' equality_expression)*

equality_expression
                   ::= relational_expression (('==' | '!=') relational_expression)*

relational_expression
                   ::= shift_expression (('<' | '>' | '<=' | '>=') shift_expression)*

shift_expression   ::= additive_expression (('<<' | '>>') additive_expression)*

additive_expression
                   ::= multiplicative_expression (('+' | '-') multiplicative_expression)*

multiplicative_expression
                   ::= unary_expression (('*' | '/' | '%') unary_expression)*

unary_expression   ::= postfix_expression
                    | '(' type ')' unary_expression                 // C-style cast [0.3.2]
                    | 'cast' '(' type ',' expression ')'              // explicit cast builtin [0.3.2]
                    | ('+' | '-' | '!' | '~' | '*' | '&') unary_expression
                    | 'sizeof' '(' type ')'
                    | 'move' '(' expression ')'
                    | 'clone' '(' expression ')'
                    | 'mod' '(' expression ')'                       // [0.3.0 Rule 24] acquire mod-right
                    | 'unmod' '(' expression ')'                     // [0.3.0 Rule 24] release mod-right
                    | 'move_to_thread' '(' expression ',' expression ')'  // [0.3.0 Rule 26]

postfix_expression ::= primary_expression
                    | postfix_expression '[' expression ']'
                    | postfix_expression '(' argument_list? ')'
                    | postfix_expression '.' identifier
                    | postfix_expression '->' identifier
                    | postfix_expression '++'
                    | postfix_expression '--'

primary_expression ::= identifier
                    | literal
                    | '(' expression ')'
                    | 'null'

argument_list      ::= expression (',' expression)*

literal            ::= integer_literal
                    | float_literal
                    | char_literal
                    | string_literal
                    | 'true'
                    | 'false'

// === Lexical ===
integer_literal    ::= decimal | hex | octal | binary
hex                ::= '0x' [0-9a-fA-F]+
octal              ::= '0' [0-7]+
binary             ::= '0b' [01]+
decimal            ::= [1-9] [0-9]* | '0'

float_literal      ::= [0-9]+ '.' [0-9]*
                    | '.' [0-9]+
                    | [0-9]+ '.'

char_literal       ::= "'" character "'"
string_literal     ::= '"' (character | escape)* '"'

identifier         ::= letter (letter | digit)*
letter             ::= 'a'..'z' | 'A'..'Z' | '_'                      // '__thread' uses '_'_'t'...
digit              ::= '0'..'9'
```

#### 12.1.1 关于 `&` 的解析注记 *(0.3.0 修订)*

0.3.0 **删除 `&mut` 产生式**。`'&' unary_expression` 是唯一引用产生式，无需前瞻——`&` 总是后接一个一元表达式。`mut` 不再是关键字。

#### 12.1.2 关于 `const T*` / `T* const` 的解析注记 *(0.3.0 新增：Q6)*

`const` 与 `*` 在 `type` 上的位置需要 **1 个记号的前瞻**：

- `const type *` → 指针锁定（指针不能 rebind）。
- `type * const` → 数据只读视图。

这与 C++ 解析规则**相反**——在 UltraCPP 里，`const` 永远修饰它**紧邻的右侧**或**紧邻的左侧**记号，但**不**传递到「指向的对象」上。具体语义见 §3.8 / §7.10。

### 12.2 保留关键字 *(0.3.0 修订)*

```
as         break      char       const      continue
else       export     extern     false      for
free       if         import     include    move
null       return     static     struct
true       typedef    unique     unsafe     void
while      clone
mod        unmod      shared     __thread   move_to_thread
```

> **[0.3.0 新增]** `mod`、`unmod`、`shared`、`__thread`、`move_to_thread`。**0.3.0 删除** `mut`（仅作为 `&mut` 记号的组成部分，删除 `T&mut` 后不再需要）。详见 §4.9、§4.10、§13。

### 12.3 运算符优先级表 *(0.3.0 修订,0.3.1 补 2.5 级)*

| 级别 | 运算符 | 描述 |
|------|--------|------|
| 1 | `::` | 作用域解析 |
| 2 | `()` `[]` `.` `->` `++` `--` | 后缀 |
| 2.5 | `*` `&` `+` `-` `!` `~` `mod` `unmod` `++` `--` (前缀) | 一元 *(0.3.1 补)* |
| 2.5 | `(`*type*`)` | **C 风格强制类型转换** *(0.3.2 补)* |
| 3 | `*` `/` `%` | 乘法 |
| 4 | `+` `-` | 加法 |
| 5 | `<<` `>>` | 移位 |
| 6 | `<` `>` `<=` `>=` | 关系 |
| 7 | `==` `!=` | 相等 |
| 8 | `&` | 按位与（**二元中缀**） |
| 9 | `^` | 按位异或 |
| 10 | `\|` | 按位或 |
| 11 | `&&` | 逻辑与 |
| 12 | `\|\|` | 逻辑或 |
| 13 | `?:` | 三元条件 |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | 赋值 |
| 15 | `move` `clone` `move_to_thread` *(0.3.0 新增)* | 所有权操作 |

**一元运算符**（优先级高于上表所有二元运算符，右到左结合）：

| 运算符 | 描述 |
|--------|------|
| `+` `-` | 正号 / 取负 |
| `!` | 逻辑非 |
| `~` | 按位非 |
| `*` | 解引用 |
| `&` | **引用**（§3.3） |
| `mod` `unmod` *(0.3.0 新增)* | 申请 / 释放修改权（Rule 24） |

### 12.4 附录 X：对未来工作的影响 *(0.3.0 修订)*

#### 12.4.1 实施策略（沿用 0.2.0）

C 主机（`src-c/`）是生产编译器；本规范的语义以其作为首个落地目标。具体算法参考 `.dev/plans/0.1.0-devhandbook.md`：

| 本规范章节 | devhandbook 参考 | 内容 |
|-----------|------------------|------|
| §3.2 / §7.1 所有权 + 修改权拆分 | §4.4 | 双状态表（所有权位 + mod 位） |
| §7.4 `move(p)` | §4.8 `record_move` | 移动点 |
| §7.8 引用（`T&`） | §4.5、§4.7 | 借用 + mod 规则 |
| §7.9 悬垂 | §4.6、§12.4 | 生命周期推断 |
| §9.9 `#modlaw` | §4.5（扩） | 修改权策略表 |
| §11.5.1 `mutex<T>` / `atomic<T>` | §4.6（扩） | 显式同步类型 |
| §13.1 跨线程可见 | §4.6（扩） | `shared` 强制标注 |
| §13.2 跨线程所有权 | §4.8（扩） | `move_to_thread` 实现 |

**新增产物（0.3.0）：**

1. 词法：`mod`、`unmod`、`shared`、`__thread`、`move_to_thread` 关键字与 `#modlaw` 指令（`src-c/src/lexer.c`）。
2. 文法：4 个新产生式 + `const T*` / `T* const` 解析新规则（`src-c/src/parser.c`）。
3. 错误种类：`UC_ERR_MODLAW`、`UC_ERR_CROSS_THREAD_MOVE`、`UC_ERR_OWNING_HEAP_VIEW`（`src-c/include/uc_error.h`）。
4. 编译阶段：所有权检查器升级为「双状态」——所有权位 + 修改权位（`src-c/src/ownership.c`）。
5. 线程支持：`spawn_thread`、`join_thread`、`mutex<T>`、`atomic<T>` 实现（`src-c/src/thread.c`）。
6. 测试语料：`test/modlaw/{pass,fail}/`、`test/threading/{pass,fail}/`、`test/const_ptr/{pass,fail}/`。

#### 12.4.2 未落入本版本的事项

| 事项 | 状态 | 去向 |
|------|------|------|
| `MissingFree` / `DoubleFree` | 不实现 | §11.6，候选可选诊断 |
| `Owned<T>` / `Ref<T>` / `RefMut<T>` 内部类型 | 不进入用户语言 | devhandbook §4.1 |
| 非词法生命周期的精确定义 | 仅描述为"到最后一次使用" | 待算法实现后回填 |
| `__thread` 在声明位置的具体语法 | 已支持 `__thread T x;` 形式 | 待后续实现细节 |
| `#modlaw` 的运行时切换 | 不实现 | 0.3.0 仅编译期策略；运行时切换延后 |

---

## 13. 线程模型 *(0.3.0 新增章节)*

> **[0.3.0]** 本章是 0.3.0 完整新增章节。UltraCPP 在 0.3.0 中加入线程模型：跨线程可见的 `shared` 标注（Rule 25）、跨线程所有权显式移交（Rule 26）、线程局部存储 `__thread`（Rule 27）、以及四种存储类（Rule 28）。

### 13.1 共享变量（`shared` 标注强制）

> **[0.3.0 · Rule 25]** 跨线程访问必须使用 `shared` 标注，否则编译错。

**语法**：
```cpp
shared T var;
shared T* p = alloc(T);    // owning + shared（堆对象的共享）
shared int* counter;       // 跨线程可见
```

**强制规则**：

- 任何在 `spawn_thread(...)` 函数体内访问的**全局 / 静态**变量，**必须**声明为 `shared`。否则编译期错误：`non-shared global accessed from spawned thread`。
- `__thread` 变量除外（每线程独立，天然不需要 `shared`）。

**编译器自动生成 mutex**：

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);  // 共享 owning 指针

thread1() {
    int& r = counter;
    mod(r);                          // 编译期 OK，运行时自动生成 mutex 包装
    *r = *r + 1;                     // 独占写
} // r 离开作用域 → 自动 unmod

thread2() {
    int& r = counter;
    mod(r);                          // 若 thread1 仍持锁则阻塞
    *r = *r + 1;
}
```

**程序员可选显式包装**：

```cpp
shared mutex<int*> counter;     // 显式 mutex 包装（mutex<T> 是 stdlib 类型，见 §11.5.1）
shared atomic<int> state;        // 显式 atomic 包装（atomic<T> 是 stdlib 类型，见 §11.5.1）
```

> **[0.3.0 修订]** `mutex<T>` 与 `atomic<T>` 是 **§11.5.1 标准库类型**，不是关键字。**当变量类型为 `mutex<T>` 或 `atomic<T>` 时，编译器自动生成的 mutex 被抑制**——程序员已显式控制同步，避免双重加锁。

### 13.2 跨线程所有权（Rule 26）

> **[0.3.0 · Rule 26]** 跨线程所有权必须显式 `move_to_thread(p, tid)` 或 `spawn_thread_with(tid, p)`。

**示例 F（跨线程所有权）：**

```cpp
int x = 42; // thread1 创建

move_to_thread(x, thread2_id);  // x 移交 thread2
// thread1 中 x 已 moved-out

// 或通过 spawn：
int y = 100;
spawn_thread_with(thread2_id, y);  // 隐式 move

// thread2 中：
void thread2() {
    // x 和 y 都可见且可访问
}
```

**规则**：

1. **`move_to_thread(p, tid)`**：显式移交 `p`（必须是 owning 或引用）的所有权到线程 `tid`；调用方此后无权访问 `p`。
2. **`spawn_thread_with(tid, p)`**：启动线程时隐式 move，等价于 `move_to_thread(p, tid)` + `spawn_thread(...)`。
3. **不可跨线程传栈引用**：`T&` 借用如果 owner 是栈变量，不可 `move_to_thread`——栈是 thread-local（见 §13.4）。
4. **跨线程传引用的限制**：跨线程传引用只能传：heap（shared owner）、global（shared）、TLS（thread-local，但接收方须正确匹配 TLS）。

### 13.3 跨线程修改权（mutex / atomic）

> **[0.3.0 · Rule 25 续]** 跨线程修改权由编译器自动生成的 mutex 或程序员显式 `mutex<T>` / `atomic<T>` 包装支持。`mutex<T>` 与 `atomic<T>` 是 §11.5.1 标准库类型，不是关键字。

**自动 mutex（编译期）：**

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);

void worker() {
    int& r = counter;
    mod(r);           // 编译器生成 pthread_mutex_lock
    *r = *r + 1;      // 临界区
}                     // 离开作用域 → pthread_mutex_unlock
```

**显式 `mutex<T>`：**

```cpp
mutex<int> mx;

void worker_explicit() {
    mx.lock();         // 显式加锁（不是 mod()——见 §11.5.1）
    // 临界区
    mx.unlock();       // 显式解锁
}
```

**显式 `atomic<T>`：**

```cpp
atomic<int> state;

void worker_atomic() {
    int v = state.load();    // 硬件原子加载
    state.store(v + 1);      // 硬件原子存储
}
```

> **与 `mod()` 的关系**：`mutex<T>` / `atomic<T>` 的 lock / load / store 操作**不**走 `mod()` 通道——它们自带同步原语。`mod()` 申请的是「修改权」，适用于普通 `T&` 引用。这两套机制互不冲突。

**`shared` 配合 `#modlaw`：**

| `#modlaw` | `shared` + mod 行为 |
|------------|----------------------|
| `shared` | 多个线程可同时 mod；程序员责任处理数据竞争 |
| `exclusive` | 编译期 / 运行时检查唯一持锁；不持锁时阻塞 |
| `none` | shared 不允许 mod（编译期错误） |

### 13.4 多线程内存布局（Rule 28）

> **[0.3.0 · Rule 28]** 四种内存区域各自的线程可见性。

| 存储类 | 线程可见性 | 跨线程引用能否传 | 备注 |
|--------|------------|------------------|------|
| **栈**（函数局部变量） | thread-local | ❌ 不允许 | owner 离开作用域即销毁 |
| **堆**（`alloc(T)`） | **shared** | ✅ 允许（须 `shared` 或 `move_to_thread`） | 单 owner，可 move |
| **全局 / 静态**（文件作用域 / `static`） | **shared** | ✅ 允许（须 `shared` 标注） | 进程级生命周期 |
| **TLS**（`__thread`） | thread-local | ❌ 不允许 | GCC 风格；每线程独立 |

**关键约束**：

- **栈** 引用**不能**传给其他线程（owner 在另一个线程离开作用域时销毁）。
- 跨线程传引用只允许 **heap / global / TLS**——其中 TLS 须确保发送方与接收方是同一线程。
- **不允许**通过 `spawn_thread` 隐式捕获栈变量：编译器报错 `cannot capture stack reference into spawned thread`。

### 13.5 `__thread` 线程局部存储（Rule 27）

> **[0.3.0 · Rule 27]** `__thread T x;`（GCC 风格）声明线程局部变量。

**语法**：
```cpp
__thread int x;               // 每线程独立的 int
__thread int buf[256];        // 每线程独立的数组
__thread int* p;              // 每线程独立的指针（指针本身是 thread-local，指向哪不一定）
```

**语义**：

- 每个线程都有 `x` 的**独立副本**，互不影响。
- 初始化仅发生一次（主线程），其它线程继承主线程的初值；这是 GCC `__thread` 的标准语义。
- 不需要 `shared` 标注（thread-local 天然隔离）。

**示例**：

```cpp
__thread int local = 42;     // 每线程独立

void worker() {
    local = local + 1;       // 仅影响当前线程
    print_int(local);
}
```

### 13.6 完整线程模型示例

> **示例 E（线程模型综合示例）：**

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);  // 跨线程可见（强制 shared）

thread1() {
    int& r = counter;
    mod(r);                          // 编译期 OK，运行时 mutex
    *r = *r + 1;                      // 独占写
} // r 离开作用域 → 自动 unmod

thread2() {
    int& r = counter;
    mod(r);                          // 若 thread1 仍持锁则阻塞
    *r = *r + 1;
}

main() {
    spawn_thread(thread1);
    spawn_thread(thread2);
    join_all();
    free(counter);                  // main 是 owner
    return 0;
}

// === __thread 线程局部 ===
__thread int local = 42;  // 每线程独立
```

---

## 14. Spec Gap Index *(0.3.4 新增, per `.dev/drafts/0.3.0-m0-priority.md` §9.x)*

> **[0.3.4 新增]** 本章集中列出 0.3.x 阶段 M0 baseline 中尚未在主章节里覆盖、但已在 spec 修订中明确定义的 spec gap 缺口。每节均按 m0-priority §9.x 原始编号（S6 / S8 / S9）保留编号，便于跨 spec gap 文档交叉引用。

### §S6 FFI extern body 来源策略（0.3.4 新增）

`extern "C" { int abs_int(int x); }` body 应来自哪里？

**三级查找路径**（按优先级）：
1. **链接的 UltraCPP stdlib**（`lib/*.uc` 编译产物）— 例：abs_int → lib/math.o::uc_abs
2. **链接的 UltraCPP 用户代码**（同一进程其他 .o）— 例：用户自定义 extern 函数
3. **libc**（作为 fallback）— 仅 Stage 1 自举需求保留

**codegen 实现**：
- emit `declare external fn <name>`（LLVM IR）
- 链接时由 linker 解析（按上述三级顺序）
- 运行时不需运行时代价（extern 是编译期 binding）

**m0 测试影响**：m0_41_extern_c 已 PASS（per commit 7d），但失败原因是 test runner exit code 提取 bug（per §S9 解决），非 §S6 路径问题。

**关联**：详见 §10.1 extern "C" 块；详见 runtime-architecture §3.1。

### §S7 CJK identifier support *(永久 NO-OP, 2026-08-21 user 决策关闭)*

非 ASCII 标识符（含 CJK / UTF-8）永久不支持。spec §2.4 显式声明 ASCII-only（per 0.3.5 修订）。

**Resolution**: ❌ Permanently NO-OP (user decision 2026-08-21).

**历史**：0.3.0 priority doc §9.x 列 S7 为 deferred（P1-5 范畴）；0.3.4 commit `02d7170` 通过 `expects_compiler_error` runner marker + m0_50 .uc 加 marker 关闭 m0 baseline 上的 CJK FAIL（编译器 lexer 早已正确输出 `unknown character` 错误，runner 旧逻辑把 compile_failed 一律当 FAIL 是误判）。0.3.5 spec 决议：S7 永远不进任何未来版本；spec §2.4 明文 ASCII-only 是唯一锚点。

**m0 测试影响**：m0_50 标 PASS（commit `02d7170`）。本决议对 m0 baseline 无新影响（已 PASS）。

**关联**：详见 spec §2.4 标识符规则（0.3.5 增补明文）；详见 `.dev/drafts/0.3.0-m0-priority.md` §9.x S7。

### §S8 deref-assign 类型安全（0.3.4 新增）

`*p = X` 中 `*p` 应满足 lvalue 分类 + pointee type 匹配：

**lvalue 分类**（per §4.13.5 deref 语义）：
- `*p` 是 lvalue（type = pointee type T）
- 赋值 `*p = X` 中 `*p` 是 lvalue（与 §4.13.3 赋值上下文约束一致）

**codegen emit 流程**：
- `*p` → 计算 `p` 的 SSA 值（type `T*`）
- emit `store T X, T* %p`（type 匹配）
- 当前 codegen UC_UN_DEREF emit `store i32 X, i8* %p`（类型不匹配）— **需修订为 commit 11c**

**实现细节**：
- codegen.c UC_UN_DEREF case 需检查 `*p` 的 pointee type（来自 `p->as.unary_expr` 类型推断）
- pointee type 推断：`p` 是 `int*` → pointee `int`；`p` 是 `MyStruct*` → pointee `%struct.MyStruct`
- store 指令 emit：`store <pointee_type> X, <pointee_type>* %p`

**m0 测试影响**：m0_30 + m0_34 + m0_36 + m0_42 deref-assign 翻 PASS（commit 11c）。

**关联**：详见 §4.13.5 deref 语义（deref 永远 lvalue）；详见 commit 11c 实施计划。

### §S9 main exit code 8-bit 截断语义（0.3.4 新增）

`int main()` 的返回值通过 OS exit 调用传递。8-bit 截断是 OS-level 行为（bash `$?` 取低 8 bit），不是 UltraCPP 行为。

**UltraCPP 编译器约束**：
- 不做截断；保留 main 返回完整 int 值
- 调用 `exit(return_value)` 系统调用传递完整 int

**测试 runner 约束**：
- 跑 main 进程 → `wait` 子进程 → 捕获 `wstatus` → `WEXITSTATUS(wstatus)` 取出**完整** exit code
- 不要取低 8 bit（OS/shell 行为）作为预期 exit code

**示例**：
- `int main() { return 720; }` — UltraCPP 调用 `exit(720)`
- bash `$?` 取低 8 bit → 720 % 256 = 208
- runner 应捕获 720（而非 208）

**m0 测试影响**：
- m0_19（factorial = 720）：当前 FAIL 因 runner 取低 8 bit；§S9 修复后 PASS

**关联**：详见 runtime-architecture §11 OS 集成；详见 commit 11b 实施计划。

---

## 文档历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 0.1 | 2026-04-13 | 初始规范 |
| 0.1 | 2026-04-17 | 更新所有权默认语义、添加 `clone()`、更新内联汇编语法 |
| 0.1.0 | 2026-04-18 | 0.1.0 定版 |
| 0.2.0 | 2026-08-07 | 落实 8 条设计决策 D-1 .. D-8：`unique` 泛型化、`&` 不可变借用、`&mut` 入语言、`move` 内建 primitive、显式化赋值 move 规则、`alloc`/`free` 不强制配对、`DanglingReference` 触发条件、C 主机优先决策。*（注：0.3.0 反转 D-3，删除 `&mut`；D-2 关于 `&` 语义被 Q3 反转覆盖。）* |
| **0.3.0** | **2026-08-07** | **本版本**：两条权限彻底拆分（Rule 22）；新增 `#modlaw` 指令（Rule 23）；新增 `mod()` / `unmod()` 表达式（Rule 24）；新增线程模型章节 §13（Rule 25–28）；改写 §3.3 引用类型为**单一 `T&`**（Q3 反转，**删除 `T&mut`**）；改写 §3.8 指针修饰符为「与 C++ 相反」的 Q6 自定义语义；改写 §7.1 赋值为「隐式 `mod()` + 不转所有权」（Q4=a）；重写 §3.2 区分 owning 与 non-owning 指针（Rule 2B）；§11.5.1 新增 `mutex<T>` / `atomic<T>` 标准库类型（修订 #5、#6）。状态：草稿。 |
| 0.3.4 | 2026-08-21 | 本版本（S6/S8/S9 spec gap + stdlib bootstrap 路径 + §11.0.2 实化 + abs_int → uc_abs 迁移） |
| **0.3.1** | **2026-08-12** | **本版本（lvalue / rvalue 概念明确化，S2）**：新增 §4.13 「表达式分类：lvalue 与 rvalue」完整章节（§4.13.1 定义 + §4.13.2 lvalue 分类表 + §4.13.3 赋值上下文约束 + §4.13.4 codegen 实现约束 + §4.13.5 交叉引用）；§4.1 优先级表补 2.5 级一元 op（`*` `&` `+` `-` `!` `~` `mod` `unmod` 前缀 `++` `--`，右到左）；§4.6 赋值运算符表 11 行统一加「LHS 必须是 lvalue (§4.13.2)」约束 + 头注 + 结合性 + lvalue 上下文说明；§7.8 引用创建规则 1 引用 §4.13.2 并加 5 合法 + 4 非法示例；§12.1 EBNF 加 `lvalue` / `rvalue` 非终结符并改 `assignment_expression` LHS 标注；§12.3 附录优先级表同步加 2.5 级。**修复 m0_42 deref-assign bug**（`*view = payload` 从 compile_failed → PASS）。不引入新语法、不修改现有语义、向后兼容。状态：草稿。 |

---

*UltraCPP 0.3.1 语言规范（草稿）*
