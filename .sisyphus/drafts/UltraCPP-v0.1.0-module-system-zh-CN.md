# UltraCPP 0.1 模块和预编译系统设计

> 本文档定义 UltraCPP 0.1 版本的模块系统和预编译指令。

---

## 1. 文件约定

### 1.1 文件扩展名

*.uc  — UltraCPP 源文件（推荐）
*.upp — UltraCPP 源文件（兼容旧格式）

### 1.2 文件命名规范

- 模块名使用小写字母和下划线：`math.uc`, `string_utils.uc`
- 避免使用特殊字符和空格
- 每个文件对应一个模块或一个功能集合

---

## 2. #import — 模块导入

### 2.1 语法

```cpp
#import "模块路径";
```

### 2.2 行为说明

- `#import` 执行单独编译：被导入的模块会独立编译为目标文件
- 链接器在链接阶段将各模块的目标文件合并
- 导入的符号通过 `模块名.符号名` 方式访问
- 编译器负责模块的依赖解析和符号导出

### 2.3 示例

```cpp
// math.uc
export int add(int a, int b) {
    return a + b;
}

export int multiply(int a, int b) {
    return a * b;
}
```

```cpp
// main.uc
#import "math";

int main() {
    int sum = math.add(40, 2);
    int product = math.multiply(6, 7);
    return sum + product;  // 返回 82
}
```

### 2.4 导出规则

- 使用 `export` 关键字导出的函数或变量才能被其他模块访问
- 未导出的符号仅限本文件内部使用，类似 C++ 的 `static`

---

## 3. #include — 代码包含

### 3.1 语法

```cpp
#include "文件路径";
```

### 3.2 行为说明

- `#include` 是源码级别的文本替换
- 预处理器将指定文件的内容直接插入到 `#include` 位置
- 不产生独立的编译单元，无需链接
- 包含的代码共享当前文件的命名空间

### 3.3 示例

```cpp
// utils.uc
int max_int(int a, int b) {
    return a > b ? a : b;
}

int min_int(int a, int b) {
    return a < b ? a : b;
}
```

```cpp
// main.uc
#include "utils.uc";

int main() {
    int maximum = max_int(10, 20);  // 返回 20
    int minimum = min_int(10, 20); // 返回 10
    return maximum - minimum;
}
```

### 3.4 使用场景

- 头文件替代方案：不想单独编译的通用代码
- 宏定义集合：常量、枚举、配置参数
- 内联函数库：性能敏感的小型函数

---

## 4. #import vs #include 对比

| 特性 | #import | #include |
|------|---------|----------|
| 编译方式 | 单独编译为目标文件 | 源码复制到当前位置 |
| 链接 | 链接器负责合并目标文件 | 无需链接，所有符号平铺 |
| 符号访问 | 模块名.符号名 | 直接使用符号名 |
| 编译时间 | 较快，支持增量编译 | 较慢，每次全量编译 |
| 循环依赖 | 支持检测并报错 | 可能导致无限展开 |
| 适用场景 | 大型项目，模块化开发 | 小型工具代码，内联优化 |

### 4.1 选择建议

- 大型项目用 `#import`：独立编译加快构建，支持增量更新
- 小型工具或性能关键代码用 `#include`：减少函数调用开销

---

## 5. 编译流程

UltraCPP 的编译流程分为三个主要阶段：

### 5.1 预处理器阶段

1. 解析 `#import` 指令，定位模块文件，递归处理依赖
2. 解析 `#include` 指令，将源码内容展开到当前位置
3. 检测循环依赖错误
4. 生成预处理后的完整源码

```
源文件 → 预处理器 → 预处理文件
a.uc   → #import b → 展开 b.uc 内容
       → #include c → 展开 c.uc 内容
```

### 5.2 编译器阶段

1. 词法分析：将源码转换为 token 流
2. 语法分析：构建抽象语法树（AST）
3. 语义分析：类型检查、作用域解析
4. 代码生成：生成中间代码或目标代码

### 5.3 链接器阶段

1. 符号解析：将符号引用绑定到定义
2. 地址重定位：合并各模块的地址空间
3. 生成可执行文件或库文件

### 5.4 流程图

```
源文件 (*.uc)
    ↓
┌─────────────────┐
│   预处理器      │
│ 解析 import/include │
│ 检测循环依赖    │
└────────┬────────┘
         ↓
┌─────────────────┐
│   编译器        │
│ 词法/语法/语义   │
│ 生成目标文件    │
└────────┬────────┘
         ↓
┌─────────────────┐
│   链接器        │
│ 符号解析合并    │
│ 生成可执行文件  │
└─────────────────┘
```

---

## 6. 循环依赖检测

### 6.1 错误示例

```cpp
// a.uc
#import "b";

export int func_a() {
    return b.func_b() + 1;
}
```

```cpp
// b.uc
#import "a";

export int func_b() {
    return a.func_a() - 1;
}
```

上述代码会导致编译错误：

```
错误：检测到循环依赖
  a.uc → import b.uc
  b.uc → import a.uc
```

### 6.2 解决方法

1. 重构代码，消除相互依赖
2. 将共享部分提取到独立模块
3. 使用接口或回调函数打破循环

```cpp
// 正确示例：提取公共模块
// common.uc
export int shared_value = 100;

// a.uc
#import "common";

export int func_a() {
    return shared_value + 1;
}

// b.uc
#import "common";

export int func_b() {
    return shared_value - 1;
}
```

---

## 7. 模块搜索路径

### 7.1 搜索规则

1. 相对路径：相对于当前文件所在目录
2. 绝对路径：相对于项目根目录
3. 系统路径：编译器内置的标准库路径

### 7.2 示例

```cpp
// 当前文件：src/utils/math.uc
// 搜索顺序：

#import "helpers";        // src/utils/helpers.uc
#import "../core/base";    // src/core/base.uc
#import "/std/io";        // 标准库 src/core/base.uc
```

---

## 8. 预编译宏

### 8.1 内置宏

| 宏名 | 说明 |
|------|------|
| `__UCPP__` | UltraCPP 编译器标识 |
| `__UCPP_VERSION__` | 编译器版本号 |
| `__FILE__` | 当前文件路径 |
| `__LINE__` | 当前行号 |

### 8.2 示例

```cpp
#import "debug";

export void log_message(const char* msg) {
    #ifdef __UCPP_DEBUG__
        print("[");
        print(__FILE__);
        print(":");
        print(__LINE__);
        print("] ");
        print(msg);
        print("\n");
    #endif
}
```

---

## 9. 条件编译

### 9.1 语法

```cpp
#ifdef 宏名
    // 条件为真时编译
#elif 另一个宏
    // 否则如果条件为真
#else
    // 否则编译
#endif
```

### 9.2 示例

```cpp
// config.uc
const int MAX_CONNECTIONS = 100;
const int ENABLE_LOGGING = 1;
```

```cpp
#include "config.uc";

#ifdef ENABLE_LOGGING
    export void log(const char* msg) {
        print(msg);
    }
#else
    export void log(const char* msg) {
        // 空实现
    }
#endif
```

---

## 10. 最佳实践

### 10.1 模块设计

- 每个模块职责单一，功能清晰
- 使用 `export` 精确控制符号导出
- 避免过深的依赖层级

### 10.2 命名规范

- 模块名：小写 + 下划线 `string_utils.uc`
- 导出函数：`模块名_功能` 如 `str_util_trim`
- 内部函数：使用 `static` 或不导出

### 10.3 编译组织

```
project/
├── src/
│   ├── main.uc
│   ├── math.uc
│   └── utils/
│       ├── string.uc
│       └── array.uc
└── std/
    └── io.uc
```

```cpp
// main.uc
#import "math";
#import "utils/string";
#import "utils/array";
#import "std/io";

int main() {
    io.print("Hello UltraCPP\n");
    return 0;
}
```
