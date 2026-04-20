# UltraCPP 0.1 指针类型设计讨论

> 本文档深入讨论 UltraCPP 0.1 版本中指针类型的设计，包括内存管理、函数接口、编译时操作等。

---

## 1. 指针类型分类

| 类型 | 语法 | 说明 |
|------|------|------|
| 原始指针 | T* | 直接内存地址 |
| 常量指针 | const T* | 指向常量的指针 |
|  指针常量 | T* const | 固定地址的指针 |
| 引用 | T& | 引用 |
| 函数指针 | fn(T) -> U | 指向函数的指针 |
| void 指针 | void* | 通用指针 |

### 1.1 原始指针

原始指针是最基础的指针类型，直接持有内存地址。

```cpp
int value = 42;
int* ptr = &value;
*ptr = 100;  // 通过指针修改值
```

### 1.2 常量指针与指针常量

```cpp
int value = 42;
const int* p1 = &value;   // 指向常量的指针：不能通过 p1 修改 value
int* const p2 = &value;   // 指针常量：p2 本身不能指向其他地址
const int* const p3 = &value;  // 两者皆不可变
```

### 1.3 引用

引用是变量的别名，在底层实现上与指针密切相关。

```cpp
int value = 42;
int& ref = value;
ref = 100;  // 直接修改 value
```

---

## 2. 内存管理模型

### 2.1 内存区域

UltraCPP 继承 C++ 的内存区域划分：

- **栈**：自动管理，函数返回时自动释放
- **堆**：通过 alloc/free 手动管理
- **静态存储区**：全局变量和静态变量
- **代码区**：只读，存放程序代码

### 2.2 unique 指针语义

UltraCPP 默认采用 unique 所有权模型，堆内存分配后归创建者所有。

```cpp
// 分配整型内存
int* p = alloc(int);

// 分配整型数组
int* arr = alloc(int, 10);

// 移动所有权
int* p2 = move(p);
// 此时 p 变为 null，p2 拥有所有权

// 使用前应检查
if (is_null(p)) {
    // 处理空指针情况
}

// 释放内存
free(p2);
```

### 2.3 move 语义

move 操作转移对象的所有权，源对象在 move 后变为 null。

```cpp
int* source = alloc(int);
*source = 42;

int* dest = move(source);

// 错误：source 现在是 null，解引用会导致运行时错误
// *source = 100;  // 危险！

// 正确做法
if (!is_null(source)) {
    *source = 100;
}

free(dest);
```

### 2.4 所有权与生命周期

每个堆内存分配都有一个明确的所有者，所有权可以转移但不能共享。

```cpp
// 函数返回时转移所有权
int* create_value() {
    int* p = alloc(int);
    *p = 42;
    return move(p);  // 所有权转移给调用者
}

void use_value(int* p) {
    if (is_null(p)) return;
    *p = *p + 1;
    free(p);  // 使用完毕后释放
}

int* global_owner = create_value();
use_value(global_owner);  // global_owner 变为 null
```

---

## 3. 指针相关函数接口

### 3.1 内置函数

UltraCPP 提供一组内置函数用于内存管理和指针操作：

| 函数 | 签名 | 说明 |
|------|------|------|
| alloc | `fn alloc<T>(count: int) -> T*` | 分配内存 |
| free | `fn free<T>(ptr: T*)` | 释放内存 |
| sizeof | `fn sizeof<T>() -> int` | 获取类型大小 |
| is_null | `fn is_null(ptr: T*) -> bool` | null 检查 |
| address_of | `fn address_of<T>(ref: T&) -> T*` | 获取地址 |

### 3.2 内存分配示例

```cpp
// 分配单个对象
int* single = alloc(int);
*single = 5;
free(single);

// 分配数组
int* array = alloc(int, 100);
for (int i = 0; i < 100; i++) {
    array[i] = i;
}
free(array);

// 分配结构体
struct Point {
    int x;
    int y;
};

Point* pt = alloc(Point);
pt->x = 10;
pt->y = 20;
free(pt);
```

### 3.3 sizeof 使用

```cpp
// 获取类型大小
int size_of_int = sizeof<int>();
int size_of_double = sizeof<double>();

// 获取变量大小
int arr[10];
int len = sizeof(arr);  // 40 (假设 int 为 4 字节)

// 计算数组元素个数
#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
```

### 3.4 地址操作

```cpp
int value = 42;
int* ptr = address_of(value);

// 指针算术运算
int* base = alloc(int, 10);
int* fifth = base + 5;
*fifth = 100;

int index = fifth - base;  // 5
```

---

## 4. 函数指针

### 4.1 函数指针类型

函数指针指向可执行代码，支持 C ABI 调用约定。

```cpp
// 基本函数指针类型
typedef int (*BinaryOp)(int, int);

// 直接声明
int (*callback)(int, int);

// 使用别名
using Comparator = int (*)(int, int);
Comparator comp;
```

### 4.2 函数指针赋值

```cpp
int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

// 赋值
int (*operation)(int, int) = &add;
int result = operation(3, 4);  // 7

// 切换函数
operation = &multiply;
result = operation(3, 4);  // 12
```

### 4.3 回调函数示例

```cpp
typedef void (*Callback)(int);

void for_each(int* arr, int len, Callback cb) {
    for (int i = 0; i < len; i++) {
        cb(arr[i]);
    }
}

void print_int(int value) {
    printf("%d\n", value);
}

int* numbers = alloc(int, 5);
for (int i = 0; i < 5; i++) {
    numbers[i] = i + 1;
}

for_each(numbers, 5, &print_int);
free(numbers);
```

### 4.4 lambda 与函数指针

```cpp
// C++14 的 lambda 可以转换为函数指针
auto add = [](int a, int b) -> int { return a + b; };

typedef int (*BinaryOp)(int, int);
BinaryOp op = +add;  // 将 lambda 转换为函数指针
```

---

## 5. FFI 外部函数接口

### 5.1 C ABI 映射

UltraCPP 支持通过 extern "C" 块声明外部 C 函数。

```cpp
extern "C" {
    fn strlen(s: const char*) -> int;
    fn malloc(size: int) -> void*;
    fn free(ptr: void*);
    fn memcpy(dest: void*, src: void*, n: int) -> void*;
}
```

### 5.2 使用示例

```cpp
extern "C" {
    fn puts(s: const char*) -> int;
    fn printf(format: const char*, ...) -> int;
}

int main() {
    puts("Hello, UltraCPP!");
    printf("Value: %d\n", 42);
    return 0;
}
```

### 5.3 C++ 与 C 互操作

```cpp
// 在 UltraCPP 中调用 C++ 类（通过 extern "C" 导出）
extern "C" {
    fn create_object() -> void*;
    fn destroy_object(ptr: void*);
    fn process_object(ptr: void*, data: int) -> int;
}

class NativeObject {
public:
    int value;
    NativeObject() : value(0) {}
    int process(int data) { return value + data; }
};

void* create_object() {
    return new NativeObject();
}

void destroy_object(void* ptr) {
    delete static_cast<NativeObject*>(ptr);
}

int process_object(void* ptr, int data) {
    return static_cast<NativeObject*>(ptr)->process(data);
}
```

### 5.4 函数指针的 FFI

```cpp
extern "C" {
    typedef int (*C_Callback)(int);
    fn set_callback(cb: C_Callback);
}
```

---

## 6. 错误检测

### 6.1 运行时检测

UltraCPP 在 Debug 模式下提供全面的运行时检测：

| 检测类型 | 说明 | 行为 |
|----------|------|------|
| double-free | 重复释放同一内存 | 终止程序，打印错误 |
| use-after-free | 使用已释放内存 | 终止程序，打印错误 |
| null 解引用 | 解引用 null 指针 | 终止程序，打印错误 |
| 内存泄漏 | 未释放的堆内存 | 退出时警告 |

### 6.2 double-free 检测

```cpp
int* p = alloc(int);
*p = 42;

free(p);
// 模拟 double-free 场景
free(p);  // 检测到重复释放，程序终止
```

### 6.3 use-after-free 检测

```cpp
int* p = alloc(int);
*p = 42;

free(p);
// 模拟 use-after-free
*p = 100;  // 检测到访问已释放内存，程序终止
```

### 6.4 null 指针检测

```cpp
int* p = alloc(int);
// p 未赋值时默认为随机值，可能导致问题
free(p);  // p 现在是 null

// 检测 null
if (is_null(p)) {
    // 安全处理
} else {
    *p = 42;  // 实际不会执行
}

// 内置 null 检查
free(p);  // free 函数内部会检查 null
```

### 6.5 编译时安全增强

```cpp
// 建议：使用前检查
int* safe_ptr = alloc(int);
if (is_null(safe_ptr)) {
    // 处理分配失败
    return ERROR_OUT_OF_MEMORY;
}
*safe_ptr = 42;
free(safe_ptr);

// 建议：RAII 模式
struct AutoFree {
    int* ptr;
    AutoFree() { ptr = alloc(int); }
    ~AutoFree() { free(ptr); }
};

void example() {
    AutoFree af;
    *af.ptr = 100;
    // 自动在作用域结束时释放
}
```

---

## 7. 设计原则总结

1. **安全第一**：默认使用 unique 所有权，move 语义清晰
2. **简洁直观**：alloc/free 配对，与 C++ new/delete 对应
3. **兼容 C ABI**：函数指针和 FFI 无缝对接 C 代码
4. **可检测性**：运行时错误能被及时发现并定位
5. **性能优先**：零成本抽象，不引入不必要的运行时开销

---

## 8. 未来扩展方向

- **智能指针**：提供 shared 和 weak 指针支持
- **内存池**：优化高频分配场景
- **垃圾回收**：可选的 GC 模式
- **指针运算**：更安全的数组索引操作
- **泛型指针**：template 风格的指针操作

---

## 9. 待定问题（2026-04-16 讨论）

### 9.1 unique 默认化 ✅ 已决定

**决定**：所有指针**默认不可复制**，`=` 赋值操作直接触发**所有权转移**（move 语义）。

```cpp
// ✅ 赋值即转移：p1 的所有权转移给 p2，p1 变为 null
int* p1 = alloc(int);
int* p2 = p1;  // p1 现在是 null

// ✅ 显式 move 也允许（与上面等价）
int* p1 = alloc(int);
int* p2 = move(p1);  // p1 变为 null

// ✅ 如果想复制指针（旧值继续使用），需要显式克隆
int* p1 = alloc(int);
int* p2 = clone(p1);  // 克隆一份，两个指针独立
```

**编译错误示例**：
```cpp
int* p1 = alloc(int);
int* p2 = p1;  // ✅ 编译通过，p1 变为 null
int* p3 = p2;  // ✅ 编译通过，p2 变为 null
```

**关键语义**：
- `=` 在指针类型上**等同于 `move()`**
- 源指针在赋值后变为 `null`
- 需要保留原值时使用 `clone()`

### 9.2 嵌入汇编语法设计 ✅ 已确认

#### Rust asm! 语法（现代设计）

```rust
unsafe {
    asm!(
        "mov eax, {:e}",
        "add eax, {:e}",
        "mov {:e}, eax",
        in(eax) input1,
        in(ebx) input2,
        lateout(eax) result
    );
}
```

**特点：**
- 宏风格 `asm!("...", operands...)`
- 显式输入/输出约束：`in(reg)`, `out(reg)`, `inout(reg)`, `lateout`, `inlateout`
- 模板中使用 `{}` 占位符，自动匹配操作数类型
- 需要 `unsafe` 块包裹
- 支持 clobber 声明
- 支持显式寄存器命名

#### GCC/Clang __asm__ 语法（AT&T 风格）

```c
int result;
__asm__ __volatile__(
    "movl %1, %%eax\n\t"
    "addl %%eax, %0"
    : "=r"(result)
    : "r"(input)
    : "eax"
);
```

**特点：**
- `__asm__` 关键字
- 输出/输入顺序：`: "约束"(变量), : "约束"(变量)`
- 约束字符：`"r"` 寄存器，`"="` 输出，`"&"` early clobber，`"m"` 内存
- 模板中使用 `%0`, `%1` 数字占位符
- `__volatile__` 防止编译器优化

#### MSVC __asm 语法（Intel 风格）

```cpp
__asm {
    mov eax, [input]
    add eax, 10
    mov [result], eax
}
```

**特点：**
- 块式语法，更接近原生汇编
- Intel 语法
- 直接使用变量名，无需约束

#### 语法对比

| 特性 | Rust asm! | GCC __asm__ | MSVC __asm |
|------|-----------|-------------|------------|
| 语法风格 | 宏 + 约束 | 约束字符串 | 块式 |
| 操作数顺序 | 显式约束 | `: outputs : inputs : clobbers` | 直接变量名 |
| 占位符 | `{}` | `%0`, `%1` | 变量名 |
| 安全保证 | unsafe | 无 | 无 |
| 寄存器分配 | 自动/显式 | 自动/显式 | 自动 |
| 可读性 | 高（显式） | 中 | 高（接近汇编） |

#### UltraCPP 嵌入汇编设计提案

**设计原则：**
1. 保持 C++ 兼容性 — 语法接近 GCC/Clang
2. 类型安全 — 引入 Rust 风格的约束系统
3. 可读性 — 使用命名占位符而非数字
4. 默认 unsafe — 嵌入汇编本身就是 unsafe 行为

**提案语法：**

```cpp
unsafe {
    asm {
        "mov eax, %[input]"
        "add eax, 10"
        "mov %[result], eax"
        : [result] "=r"(out_var)
        : [input] "r"(in_var)
        : "eax"
    }
}
```

**关键要素：**
- `unsafe {}` 块包裹
- `asm {}` 块包含汇编代码
- 输出/输入节：`: [name] "constraint"(expr)`
- 命名占位符：`%[name]`
- clobber 节：`: "register"`
