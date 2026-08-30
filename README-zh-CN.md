# UltraCPP

**熟悉的 C++ 表面，一套从自身问题里长出来的安全模型。**

> **语言版本**：[简体中文（本文件）](./README-zh-CN.md) · [English](./README.md)
>
> **当前规范**：[UltraCPP v0.3.5（简体中文）](./docs/UltraCPP-v0.3.5-spec-zh-CN.md) · [UltraCPP v0.3.5 (English)](./docs/UltraCPP-v0.3.5-spec-en.md)
>
> **状态**：草稿 · **最后修订**：2026-08-28

> **当前状态（2026-08-21）**
>
> 0.3.4 完成了 lib/*.uc 标准库引导（6 个文件：print/string/memory/math/alloc/sys.uc + sys/raw.uc）以及 codegen 中 abs_int → uc_abs 的迁移。0.3.3 阶段的工作（S1 一元运算符优先级 + S3 deref 语义 + mod/unmod/move/clone 函数化）与 0.3.4 规范缺口修复（S6/S8/S9）已写入代码。C99 端口 `src-c/` 是生产主机编译器；Rust 端口 `src/` 只作参考。当前基线：32/48 通过 / 16 失败。下一阶段 0.3.5 / M1 里程碑将聚焦 UC_TYPE_MUTABLE_POINTER 清理、剩余 m0 测试以及 6 关键字词法器集成。
>
> 请先阅读：
>
> - [UltraCPP v0.3.5 中文规范](docs/UltraCPP-v0.3.5-spec-zh-CN.md)
> - [UltraCPP v0.3.5 English Specification](docs/UltraCPP-v0.3.5-spec-en.md)
> - [0.1.0 借用检查规范与实现审计](.dev/drafts/0.1.0-borrowck-spec-vs-impl.md)，记录旧模型为何必须重做
> - [C 主机编译器说明](src-c/README.md)，当前实现能力

## 自我介绍 / Self-introduction

我是 UltraCPP，一个仍在学习怎样把 C++ 熟悉感和静态安全放在一起的系统语言实验。我的 C 编译器已经能完成词法分析、语法分析和 LLVM IR 生成，但我的 0.3.1 安全模型还在规范里，不能把设计当成实现。

## 设计历程 / Design Journey

### 第一阶段：天真 / Phase 1: Naivety

我一开始以为，把 Rust 的模型换成 C++ 的写法就能工作。那时我的口号是 "C++ syntax × Rust safety = UltraCPP"，映射表也很整齐：

| UltraCPP 写法 | 我当时赋予它的角色 |
|---|---|
| `T*` | owning pointer，近似 `Box<T>` |
| `T&` | immutable borrow |
| `T&mut` | mutable borrow |

纸面上很漂亮。三个写法，三个角色，似乎只要补上 move、borrow conflict 和 lifetime 检查，我就能得到熟悉的语法与现成的安全模型。我当时没有认真问一件事：这些符号在 C++ 里已经背负了什么含义？

### 第二阶段：裂缝 / Phase 2: Cracks

真正把规则写进规范后，裂缝很快出现了。

`T*` 被我同时当作 owner 和普通指针。owner 应该有唯一释放责任，普通指针却天然支持别名、重新绑定和指针算术。一个符号承担两套互相拉扯的义务。

`T&` 更乱。在一处，它是不可变借用；在另一处，它沿用 C++ 的可写引用和按引用修改参数；谈到一元 `&` 时，它又和 address-of / borrow 表达式混在一起。同一个符号系统在不同章节说着不同的话。

`unique T` 原本想显式表达独占所有权，可 `T*` 已经被定义成默认 owning。两者没有清楚分工，只是重叠。最普通的 C++ 写法也无处安放：

```cpp
int b = 42;
int* p = &b;
```

如果 `int*` 必然 owning，那么 `p` 是否负责释放栈变量 `b`？显然不应该。可如果它不 owning，旧规则对 `T*` 的承诺就不成立。

我那时其实是在假装自己是 Rust，但 C++ 语法不肯配合我。

### 第三阶段：怀疑 / Phase 3: Questioning

接下来的一次规范与实现审计把问题说得更直接。旧规范列出的 12 条所有权和借用规则，真正由两个编译器在语义阶段强制执行的数量是 **0/12**。有些关键字能被词法器识别，有些 AST 节点也能被 parser 构造，但 parse 完就直接进入 codegen，没有活着的 borrow checker。

这当然是实现缺口，但我不再愿意只把它当作工程欠账。即使把 12 条规则全部照旧实现，我仍会得到一个和 C++ 写法不断冲突的 Rust 模型。

"C++ syntax × Rust safety" 这个前提本身错了。把旧方案做对，结果仍然会错，因为那套模型是为 Rust 的语法和约束长出来的，不是为 C++ 的符号习惯长出来的。

### 第四阶段：转向 / Phase 4: The Turn

是用户把我从旧路线里拉了出来。问题不该是 "这个东西像 Rust 的哪一种 borrow"，而应该拆成两个互不代替的问题：

1. 谁最终负责释放资源？
2. 谁现在可以写这个值？

这成为 **Rule 22**。所有权和修改权不再捆绑。

| 权限 | 回答的问题 | 数量关系 | 如何变化 | 主要语法 |
|---|---|---|---|---|
| 所有权 `ownership` | 谁负责 `delete` / `free` | 同一资源唯一 owner | 只能显式移交，不可复制 | `T*`、`alloc(T)`、`move(p)` |
| 修改权 `modification right` | 谁可以通过引用改值 | 可以没有、共享或独占 | 按策略申请和释放 | `T&`、`mod(r)`、`unmod(r)` |

`T*` owner 按当前规则默认有修改权，但编译器在概念上分别记录两条状态；non-owner 也可以在策略允许时获得修改权。"谁释放" 不再暗中决定 "谁能写"。这不是换一套名字，而是我第一次把之前混在一起的两件事分开。

```cpp
int* owner = alloc(int);  // owner 负责 free
int& view = *owner;       // view 不拥有对象
mod(view);                // 单独申请修改权
*view = 42;
free(owner);              // 释放责任始终属于 owner
```

### 第五阶段：指令化 / Phase 5: Directive-ification

权限拆开后，我又发现修改权不应该继续塞进类型。不同项目对可写别名的容忍度不同，有的模块需要严格独占，有的愿意自己处理竞争，还有的只允许只读访问。如果把每一种策略都编码成新类型，类型系统会重新长回旧问题。

所以修改权成为用户配置的策略，由 `#modlaw` 指令选择。规范只允许 6 种组合：

| `perm` | `scope` | 行为 |
|---|---|---|
| `none` | `global` | 整个文件及其 include 内容禁止 `mod()` |
| `none` | `module` | 当前模块禁止 `mod()` |
| `exclusive` | `global` | 全局修改权独占，检查争用 |
| `exclusive` | `module` | 当前模块修改权独占，检查争用 |
| `shared` | `global` | 全局允许多个修改权，数据竞争由程序员负责 |
| `shared` | `module` | 当前模块允许多个修改权，数据竞争由程序员负责 |

没有指令时，默认是 `#modlaw shared module`。同一个 `T&` 不需要改变类型，只会在不同策略下表现不同：

```cpp
int value = 1;
int& r = value;
```

```cpp
#modlaw none module
mod(r);                 // 编译错误：策略禁止修改
```

```cpp
#modlaw exclusive module
mod(r);                 // 可以，但同一对象的第二个 mod 会冲突
```

```cpp
#modlaw shared module
mod(r);                 // 可以，多个引用可同时取得修改权
```

类型描述 "这是什么引用"，指令描述 "这个模块采用什么修改规则"。这条边界比 `T&` / `T&mut` 的二分更适合我。

### 第六阶段：简化 / Phase 6: Simplification

边界清楚以后，一些曾经看似必要的东西反而可以删除。`T&mut` 就是其中之一。独占性已经能由 `#modlaw exclusive` 和 `mod()` 完整表达，再保留一个独占可变引用类型只会重复同一信息。0.3.1 因此只保留 `T&`。

```cpp
#modlaw exclusive module

int n = 0;
int& r = n;
mod(r);                 // 这里申请独占修改权
*r = 1;
```

我也没有照搬 C++ 的 const 指针规则。0.3.1 明确采用相反的自定义语义：

| 声明 | UltraCPP 0.3.1 含义 |
|---|---|
| `const T*` | 指针锁定，不能 rebind，也不能通过它写 |
| `T* const` | 数据只读视图，可以 rebind，但不能通过它写 |
| `const T* const` | 指针和写入都锁定 |

```cpp
int x = 1;
int y = 2;

const int* pinned = &x;
// pinned = &y;         // 错：指针已锁定
// *pinned = 3;         // 错：只读

int* const view = &x;
view = &y;              // 可以 rebind
// *view = 3;           // 错：只读视图
```

这两种只读指针都不能从 owning heap 指针创建，因为 owner 之后可能 move 或 free，留下悬垂视图。

线程规则也沿用相同的拆分思路。跨线程可见的值必须标记 `shared`；线程私有存储使用 GCC 风格的 `__thread`；`mutex<T>` 和 `atomic<T>` 是标准库类型，不是编译器关键字。在 exclusive 策略下，普通 shared 值的 `mod()` 可由编译器合成 mutex；显式包装则由程序员控制同步，并抑制自动 mutex。

```cpp
#modlaw exclusive global

shared int counter = 0;
__thread int scratch = 0;

mutex<int> guarded;     // 标准库类型
atomic<int> ready;      // 标准库类型
```

### 第七阶段：当前形态 / Phase 7: Current Form

0.3.1 不是 "C++ × Rust"。它保留 C++ 的熟悉感，但安全模型属于 UltraCPP 自己：单一 `T&`、彼此独立的所有权和修改权、用户选择的 `#modlaw`，以及明确的跨线程可见性。

我付出的教训很具体。第一次，我把为另一套语法长出来的模型直接做了 1+1，结果每个熟悉符号都带来新的例外。第二次，我把实现缺口误当成唯一问题，直到审计显示即使补齐 checker，概念冲突仍然存在。用户反复问 "谁释放" 和 "谁能写" 时，答案其实已经藏在问题里了。我需要做的不是再发明一个映射，而是承认那是两条权限。

现在的模型更像我自己，但它还没有完成。0.3.1 规范给出了方向，下一步是让 C 主机编译器真的拥有对应的语义检查，而不是提前宣称安全已经实现。

## 当前架构 / Current Architecture (0.3.1)

```text
UltraCPP source (.uc / .upp)
             |
             v
+---------------------------+
| Preprocess / import scan  |  #include, #import, #modlaw
+-------------+-------------+
              |
              v
+---------------------------+
| Lexer                     |
+-------------+-------------+
              |
              v
+---------------------------+
| Parser + AST              |
+-------------+-------------+
              |
              v
+------------------------------------------------------+
| Semantic checks                                      |
| ownership | references | modlaw | lifetime | threads |
| NEW IN THE 0.3.1 DESIGN, NOT YET IMPLEMENTED IN CODE |
+-------------------------+----------------------------+
                          |
                          v
+---------------------------+
| LLVM IR code generation   |
+-------------+-------------+
              |
              v
        LLVM IR -> llc -> system linker -> executable
```

当前 `src-c/` 实际执行的是 lexer、parser、AST、codegen 和 CLI 路径。图中的 `#modlaw` 读取和语义检查阶段都是 0.3.1 新加入的架构要求，代码尚未接入，因此 README 不把规范中的安全规则标成已实现功能。

## 核心设计概念 / Core Design Concepts

### 1. 单一引用类型 `T&`

`T&` 必须初始化、不能为 null、也不拥有目标。它默认可读，写入前需要 `mod()`。

```cpp
int x = 10;
int& r = x;
int copy = r;           // 读
mod(r);
*r = 11;                // 写
```

### 2. 两条权限独立

所有权决定释放责任，修改权决定写入资格。移动 owner 不等于授予所有引用写权限，授予写权限也不改变谁负责 free。

```cpp
int* owner = alloc(int);
int& r = *owner;
mod(r);                 // 只改变修改权
*r = 7;
free(owner);            // owner 仍负责释放
```

### 3. `#modlaw` 指令

模块用 `none`、`exclusive` 或 `shared` 选择修改策略，再用 `global` 或 `module` 选择范围。

```cpp
#modlaw exclusive module

int value = 0;
int& r = value;
mod(r);                 // 当前作用域取得独占修改权
*r = 1;
```

### 4. 自定义 const 指针语义

`const T*` 锁定指针，`T* const` 提供可重新绑定的数据只读视图。两者都禁止通过指针写入，含义与 C++ 相反。

```cpp
const int* fixed_pointer = &x;  // 不能 rebind
int* const readonly_view = &x;  // 可以 rebind，不能写数据
```

### 5. 线程模型

跨线程访问必须显式 `shared`。`__thread` 提供每线程副本；`mutex<T>` 和 `atomic<T>` 由标准库提供。

```cpp
shared int jobs = 0;
__thread int local_jobs = 0;
mutex<int> jobs_lock;
atomic<int> stop_flag;
```

## 项目结构 / Project Structure

```text
UltraCpp/
├── src-c/       C99 生产主机编译器：lexer、parser、AST、codegen、CLI
├── src/         Rust 参考实现，不是当前生产目标
├── src-uc/      未来用 UltraCPP 编写的自举编译器，目前待实现
├── docs/        0.1.0、0.2.0、0.3.1、0.3.2、0.3.3、0.3.4 和 0.3.5 版本化语言规范
├── .dev/        设计计划、审计、草稿和开发过程记录
├── bootstrap/   自举计划与 C/Rust 基线产物
├── lib/         UltraCPP 标准库源码
├── test/        语言测试程序
└── tools/       C 与 Rust 实现的对比验证脚本
```

## 快速开始 / Quick Start

需要支持 C99 的编译器和 GNU Make。构建当前 C 主机编译器：

```bash
make -C src-c build/uc_lexer
src-c/build/uc_lexer --help
```

查看一个测试程序的 token：

```bash
src-c/build/uc_lexer --tokens test/test_t1/main.upp
```

运行 C 端口单元测试：

```bash
make -C src-c test
```

`src-c/build/uc_lexer` 还支持 `--ast`、`--emit-ll` / `-S` 和 `--build`。这些命令展示的是当前编译器能力，不代表 0.3.1 的语义检查已经实现。

## 关键文档 / Key Documents

| 文档 | 用途 |
|---|---|
| [UltraCPP v0.3.5 中文规范](docs/UltraCPP-v0.3.5-spec-zh-CN.md) | v0.3.5 语言规范的简体中文版本 |
| [UltraCPP v0.3.5 English Specification](docs/UltraCPP-v0.3.5-spec-en.md) | v0.3.5 语言规范的英文版本 |
| [0.1.0 借用检查审计](.dev/drafts/0.1.0-borrowck-spec-vs-impl.md) | 记录 0/12 执行现状、矛盾和后续决策 |
| [C 端口 README](src-c/README.md) | 当前生产主机编译器的构建方式和能力 |
| [Bootstrap Plan](bootstrap/PLAN.md) | 从 C 主机走向 `src-uc/` 自举编译器的路线 |
| [.dev README](.dev/README.md) | 设计计划、草稿和开发记录的索引 |

## 开发语言 / Development Languages

| 语言 | 角色 |
|---|---|
| C99 | 生产主机编译器，位于 `src-c/` |
| Rust | 参考实现，位于 `src/` |
| UltraCPP | 未来的自举编译器实现，位于 `src-uc/` |

编译目标继续使用 LLVM 工具链和系统链接器。

## 许可证 / License

Apache License 2.0。详见 [LICENSE](LICENSE)。
