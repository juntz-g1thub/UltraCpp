# UltraCPP 测试项目

## 测试目录结构

```
test/
├── test_t1/           # 基础测试：空程序
├── test_t2/           # 变量声明测试
├── test_t3/           # 基础测试（与test_t1相同）
├── test_io/           # I/O库测试（#import语法）
├── test_import/       # #import指令测试
├── test_include/      # #include指令测试
├── test_hello_world/  # Hello World输出测试
└── test_simple.uc     # 简单测试
```

## 各测试项目说明

### test_t1 / test_t3 - 基础测试
- **文件**: `main.upp`
- **代码**:
  ```c
  int main() {
      return 0;
  }
  ```
- **目的**: 验证编译器能正确处理最简单的程序
- **预期**: 编译成功，链接后运行返回0

### test_t2 - 变量声明测试
- **文件**: `main.upp`
- **代码**:
  ```c
  int main() {
      int x = 5;
      return x;
  }
  ```
- **目的**: 测试变量声明和返回
- **预期**: 编译成功，链接后运行返回5

### test_io - I/O库测试
- **文件**: `main.upp`
- **代码**:
  ```c
  #import <io>

  int main() {
      int x = io.getValue();
      return x;
  }
  ```
- **依赖**: `lib/io.uc`
- **目的**: 测试 `#import` 语法和 `io.getValue()` 函数调用
- **预期**: 编译成功，链接后运行返回42

### test_import - #import指令测试
- **文件**: `main.upp`
- **代码**:
  ```c
  #include "../../lib/math.uc"

  int main() {
      int result = add(5, 3);
      return result;
  }
  ```
- **依赖**: `lib/math.uc`
- **目的**: 测试 `#include` 包含外部文件
- **预期**: 编译成功，链接后运行返回8

### test_include - #include指令测试
- **文件**: `main.upp`
- **代码**: 与 `test_import` 相同
- **目的**: #include 指令的另一个测试入口

### test_hello_world - Hello World输出测试
- **文件**: `main.upp`
- **代码**:
  ```c
  #import "lib/io"

  int main() {
      io.print_str("Hello, World!\n");
      return 0;
  }
  ```
- **依赖**: `lib/io.uc`
- **目的**: 测试字符串字面量和 I/O 输出
- **预期**: 输出 "Hello, World!"

## 测试执行

使用 `uc-build` 脚本编译测试：

```bash
# 编译单个测试
./uc-build build test/test_t1/main.upp

# 编译并运行
./uc-build build test/test_t1/main.upp
./test/test_t1/test_t1

# 清理构建
./uc-build clean
```

## lib/ - 库文件

| 文件 | 说明 |
|------|------|
| `math.uc` | 数学库，导出 `add(a, b)` 函数 |
| `io.uc` | I/O库，提供 `print_str()`, `getValue()` 函数 |

## 注意事项

1. `#import <io>` 和 `#include "path"` 是不同的：
   - `#import` - 模块导入，解析为 `module$function` 符号
   - `#include` - 源代码包含，直接插入被包含文件内容

2. `io.getValue()` 会解析为符号 `@io$getValue`
3. `math.add(a, b)` 通过 `#include` 直接包含后，符号为 `@math$add`（本地模块mangle）
