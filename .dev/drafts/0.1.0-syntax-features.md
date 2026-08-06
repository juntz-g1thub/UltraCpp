# C++ 语法特性梳理

> 本文档整理 C++ 近几个标准（C++11 到 C++23）中的核心语法特性，用于定义 UltraCPP 的目标语法风格。

---

## 1. 类型系统

### 1.1 基础类型

C++ 提供了一套丰富的基础类型，用于表示不同种类的数据。

```cpp
// 整数类型
int age = 25;
short small_num = 100;
long big_num = 100000L;
long long very_big = 10000000000LL;

// 浮点类型
float price = 19.99f;
double precision = 3.1415926535;
long double high_precision = 2.718281828459L;

// 字符和布尔
char grade = 'A';
bool is_active = true;

// 字符串
std::string name = "UltraCPP";
const char* c_str = "Hello";
```

### 1.2 指针和引用

指针存储变量的内存地址，引用是变量的别名。

```cpp
int x = 42;

// 指针
int* p = &x;        // 指向 int 的指针
*p = 100;           // 通过指针修改值

// 引用
int& r = x;         // x 的引用
r = 200;            // 通过引用修改值

// 常量指针和指针常量
const int* cp = &x;     // 指向常量的指针
int* const pc = &x;     // 常量指针
const int* const cpc = &x; // 指向常量的常量指针

// 多级指针
int** pp = &p;
```

### 1.3 类和结构体

结构体和类本质上相同，区别在于默认访问权限。

```cpp
// 结构体 - 默认 public
struct Point {
    double x;
    double y;
    
    Point() : x(0), y(0) {}
    Point(double x_, double y_) : x(x_), y(y_) {}
};

// 类 - 默认 private
class Circle {
private:
    double radius;
    Point center;
    
public:
    Circle() : radius(1.0), center(0, 0) {}
    Circle(double r, Point c) : radius(r), center(c) {}
    
    double area() const { return 3.14159 * radius * radius; }
};
```

---

## 2. 函数

### 2.1 基本函数

函数是组织代码的基本单元。

```cpp
// 基本函数定义
int add(int a, int b) {
    return a + b;
}

// 无返回值函数
void print_message(const char* msg) {
    std::cout << msg << std::endl;
}

// 默认参数
int multiply(int a, int b = 2) {
    return a * b;
}

// 函数重载
int max(int a, int b) { return a > b ? a : b; }
double max(double a, double b) { return a > b ? a : b; }

// 内联函数
inline int square(int x) {
    return x * x;
}

// constexpr 函数 - 编译期求值
constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}
```

### 2.2 lambda 表达式

lambda 是匿名函数对象，可以捕获周围作用域的变量。

```cpp
// 基本 lambda
auto add = [](int a, int b) { return a + b; };
int result = add(3, 4);  // 7

// 带参数类型
auto multiply = [](int a, int b) -> int { return a * b; };

// 捕获列表
int factor = 10;
auto scaled = [factor](int x) { return x * factor; };

// 按引用捕获（可以修改外部变量）
int count = 0;
auto increment = [&count]() { count++; };

// 混合捕获
auto demo = [=, &count](int x) { 
    count++; 
    return x * factor; 
};

// 泛化 lambda（C++14）
auto generic = [](auto x) { return x; };
```

---

## 3. 面向对象

### 3.1 继承

继承建立类之间的层次关系，实现代码复用和多态。

```cpp
// 基类
class Animal {
protected:
    std::string name;
    
public:
    Animal(const std::string& n) : name(n) {}
    
    virtual ~Animal() {}
    
    virtual void speak() const {
        std::cout << name << " makes a sound" << std::endl;
    }
    
    virtual void move() const = 0;  // 纯虚函数
};

// 公有继承
class Dog : public Animal {
private:
    std::string breed;
    
public:
    Dog(const std::string& n, const std::string& b) 
        : Animal(n), breed(b) {}
    
    void speak() const override {
        std::cout << name << " says woof" << std::endl;
    }
    
    void move() const override {
        std::cout << name << " runs" << std::endl;
    }
};

// 多重继承
class Flyable {
public:
    virtual void fly() const = 0;
};

class Bird : public Animal, public Flyable {
public:
    Bird(const std::string& n) : Animal(n) {}
    
    void move() const override {
        std::cout << name << " flies" << std::endl;
    }
    
    void fly() const override {
        std::cout << name << " is flying" << std::endl;
    }
};
```

### 3.2 构造函数和析构函数

构造函数初始化对象，析构函数清理资源。

```cpp
class Resource {
private:
    int* data;
    size_t size;
    
public:
    // 默认构造函数
    Resource() : data(nullptr), size(0) {}
    
    // 参数构造函数
    Resource(size_t s) : size(s) {
        data = new int[s];
    }
    
    // 拷贝构造函数
    Resource(const Resource& other) : size(other.size) {
        data = new int[size];
        std::copy(other.data, other.data + size, data);
    }
    
    // 移动构造函数
    Resource(Resource&& other) noexcept : data(other.data), size(other.size) {
        other.data = nullptr;
        other.size = 0;
    }
    
    // 拷贝赋值
    Resource& operator=(const Resource& other) {
        if (this != &other) {
            delete[] data;
            size = other.size;
            data = new int[size];
            std::copy(other.data, other.data + size, data);
        }
        return *this;
    }
    
    // 移动赋值
    Resource& operator=(Resource&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            size = other.size;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }
    
    // 析构函数
    ~Resource() {
        delete[] data;
    }
};
```

### 3.3 运算符重载

运算符重载让自定义类型支持自然语法。

```cpp
class Vec2 {
private:
    double x, y;
    
public:
    Vec2(double x_ = 0, double y_ = 0) : x(x_), y(y_) {}
    
    // 加法运算符
    Vec2 operator+(const Vec2& other) const {
        return Vec2(x + other.x, y + other.y);
    }
    
    // 一元负运算符
    Vec2 operator-() const {
        return Vec2(-x, -y);
    }
    
    // 标量乘法
    Vec2 operator*(double scalar) const {
        return Vec2(x * scalar, y * scalar);
    }
    
    // 下标运算符
    double operator[](int i) const {
        return i == 0 ? x : y;
    }
    
    double& operator[](int i) {
        return i == 0 ? x : y;
    }
    
    // 输出流运算符（友元函数）
    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << "(" << v.x << ", " << v.y << ")";
    }
};
```

---

## 4. 模板

模板支持泛型编程，代码可以处理多种类型。

### 4.1 函数模板

```cpp
// 基本函数模板
template<typename T>
T max(T a, T b) {
    return a > b ? a : b;
}

// 显式实例化
template int max<int>(int, int);

// 模板参数默认值
template<typename T, typename Container = std::vector<T>>
class Stack {
private:
    Container data;
    
public:
    void push(const T& item) {
        data.push_back(item);
    }
    
    T pop() {
        T item = data.back();
        data.pop_back();
        return item;
    }
};

// 特化
template<>
const char* max<const char*>(const char* a, const char* b) {
    return std::strcmp(a, b) > 0 ? a : b;
}

// 变量模板（C++14）
template<typename T>
constexpr T pi = T(3.1415926535897932385);

// C++20 模板 lambda
auto add = []<typename T>(T a, T b) { return a + b; };
```

### 4.2 类模板

```cpp
template<typename T>
class SmartPtr {
private:
    T* ptr;
    
public:
    SmartPtr(T* p = nullptr) : ptr(p) {}
    
    ~SmartPtr() { delete ptr; }
    
    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
    
    // 拷贝构造删除（独占所有权）
    SmartPtr(const SmartPtr&) = delete;
    SmartPtr& operator=(const SmartPtr&) = delete;
    
    // 移动语义
    SmartPtr(SmartPtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }
};

// 模板参数约束（C++20 concepts）
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<Numeric T>
T square(T x) {
    return x * x;
}
```

---

## 5. 内存管理

### 5.1 new 和 delete

手动内存分配需要配对使用。

```cpp
// 基本动态分配
int* p = new int(42);           // 分配并初始化
int* arr = new int[10];          // 动态数组
delete p;
delete[] arr;

// 定位 new（placement new）
char buffer[sizeof(int)];
int* q = new(buffer) int(100);

// 智能管理
int* safe = new (std::nothrow) int(42);  // 不抛异常版本
if (safe) {
    delete safe;
}
```

### 5.2 智能指针

智能指针自动管理内存，避免内存泄漏。

```cpp
#include <memory>

// unique_ptr - 独占所有权
std::unique_ptr<int> up(new int(42));
std::unique_ptr<int[]> arr_up(new int[10]);

// 自定义删除器
auto deleter = [](int* p) { 
    std::cout << "deleting\n"; 
    delete p; 
};
std::unique_ptr<int, decltype(deleter)> custom_up(new int(10), deleter);

// make_unique（C++14）
auto up2 = std::make_unique<int>(100);
auto arr_up2 = std::make_unique<int[]>(10);

// shared_ptr - 共享所有权
std::shared_ptr<int> sp(new int(42));
auto sp2 = std::make_shared<int>(100);

// 引用计数
std::shared_ptr<int> sp3 = sp;  // 引用计数变为 2
sp.reset();                      // 引用计数变为 1
sp3.reset();                     // 引用计数变为 0，内存释放

// weak_ptr - 解决循环引用
class Node {
public:
    std::weak_ptr<Node> next;  // weak_ptr 避免循环引用
    ~Node() { std::cout << "destroyed\n"; }
};

auto n1 = std::make_shared<Node>();
auto n2 = std::make_shared<Node>();
n1->next = n2;
n2->next = n1;  // 循环引用，但 weak_ptr 不会增加引用计数
```

---

## 6. C++11/14/17/20/23 新特性

### 6.1 auto 关键字

auto 自动推导变量类型，简化声明。

```cpp
// 类型推导
auto x = 42;              // int
auto y = 3.14;            // double
auto name = "UltraCPP";   // const char*

// 复杂类型
auto p = std::make_unique<int>(10);
auto lambda = [](int x) { return x * 2; };
std::vector<int> v = {1, 2, 3};
for (auto it = v.begin(); it != v.end(); ++it) {
    // ...
}

// 泛化 lambda
auto identity = [](auto x) { return x; };
auto result = identity(42);    // int
auto result2 = identity(3.14); // double

// C++20: auto 模板参数
auto add = []<auto A, auto B>() { return A + B; };
```

### 6.2 范围 for 循环

简化容器遍历语法。

```cpp
std::vector<int> nums = {1, 2, 3, 4, 5};

// 基本遍历
for (int n : nums) {
    std::cout << n << " ";
}

// auto 遍历
for (auto n : nums) {
    std::cout << n << " ";
}

// 引用遍历（可修改）
for (auto& n : nums) {
    n *= 2;
}

// 常量引用遍历
for (const auto& n : nums) {
    std::cout << n << " ";
}

// 初始化（C++20）
for (auto v = std::vector{1, 2, 3}; auto& x : v) {
    std::cout << x;
}
```

### 6.3 nullptr

nullptr 替代 NULL 和 0，提供类型安全的空指针。

```cpp
int* p1 = nullptr;     // 指针初始化为空
int* p2 = NULL;         // 旧式写法
int* p3 = 0;            // 也可接受，但不推荐

void foo(int);
void foo(int*);

foo(0);      // 调用 foo(int)，歧义
foo(nullptr); // 调用 foo(int*)

// nullptr_t 类型
using ptr_t = std::nullptr_t;
```

### 6.4 初始化列表

统一初始化语法。

```cpp
// 大括号初始化
int arr[5] = {1, 2, 3, 4, 5};
std::vector<int> v = {1, 2, 3};

// std::initializer_list
void init(std::initializer_list<int> list) {
    for (auto x : list) {
        std::cout << x << " ";
    }
}
init({1, 2, 3, 4});

// 类内初始化
class Config {
    int port = 8080;
    std::string host = "localhost";
    std::vector<std::string> mods = {"a", "b"};
};

// 聚合初始化（C++20）
struct Point3D {
    double x = 0;
    double y = 0;
    double z = 0;
};

Point3D p{1.0, 2.0};  // z 使用默认值 0
```

### 6.5 作用域枚举

类型安全的枚举。

```cpp
// 基本作用域枚举
enum class Color { Red, Green, Blue };
enum class Priority { High, Medium, Low };

Color c = Color::Red;
Priority p = Priority::High;

// 指定底层类型
enum class Status : int { Ok = 200, Error = 500 };

// switch 支持
Status s = Status::Ok;
switch (s) {
    case Status::Ok: /* ... */ break;
    case Status::Error: /* ... */ break;
}

// 传统枚举（隐式转换）
enum OldStyle { A, B, C };  // 不会污染命名空间
OldStyle val = A;
```

### 6.6 强类型枚举

基于作用域枚举的增强。

```cpp
// 强类型
enum class Type : unsigned int {
    None = 0,
    Int = 1,
    Float = 2,
    String = 4
};

// 支持位运算
Type t = Type::Int | Type::Float;
bool is_set = (t & Type::Int) != 0;

// C++20 枚举类模板
enum class E<int N> : std::array<char, N> { 
    A = {'a'}, B = {'b'} 
};
```

### 6.7 委托构造函数

构造函数调用同一类的其他构造函数。

```cpp
class Widget {
private:
    int x, y;
    std::string name;
    
public:
    // 委托构造函数
    Widget() : Widget(0, 0, "default") {}
    
    Widget(int x) : Widget(x, 0, "default") {}
    
    Widget(int x_, int y_) : Widget(x_, y_, "default") {}
    
    // 主构造函数
    Widget(int x_, int y_, const std::string& n) 
        : x(x_), y(y_), name(n) {}
};
```

### 6.8 继承构造函数

子类直接使用父类构造函数。

```cpp
class Base {
protected:
    int value;
    
public:
    Base(int v) : value(v) {}
    
    void print() const {
        std::cout << value << std::endl;
    }
};

class Derived : public Base {
public:
    // 继承所有构造函数
    using Base::Base;
    
    // 添加新构造函数
    Derived() : Base(0) {}
    
    // 新方法
    void debug() const {
        std::cout << "Derived: " << value << std::endl;
    }
};

Derived d(42);
d.print();  // 42
d.debug();  // Derived: 42
```

### 6.9 override 和 final

明确意图，防止意外重写。

```cpp
class Base {
public:
    virtual void foo() {}
    virtual void bar() const {}
    void no_override() {}
};

class Derived : public Base {
public:
    // 正确重写
    void foo() override {}           // OK
    void bar() const override {}     // OK
    
    // 错误：基类没有虚函数，会导致隐藏而非重写
    // void no_override() override {}  // 编译错误
    
    // final - 禁止后续类重写
    void foo() final {}
};

class Blocked : public Derived {
public:
    // 错误：foo() 在 Derived 中已是 final
    // void foo() override {}  // 编译错误
};
```

### 6.10 static_assert

编译期断言。

```cpp
// 基本用法
static_assert(sizeof(int) == 4, "int must be 32 bits");

// 模板中的 static_assert
template<typename T>
class Vector {
    static_assert(std::is_default_constructible_v<T>,
                  "Vector requires default constructible type");
};

// C++17: 多参数版本
static_assert(true, "message") << "more info";  // C++17

// C++20: 常量表达式版本
consteval int sq(int x) {
    static_assert(x >= 0);  // 编译期检查
    return x * x;
}
```

---

## 7. 错误处理

### 7.1 异常

异常处理运行时错误。

```cpp
#include <stdexcept>

// 抛出异常
void divide(int a, int b) {
    if (b == 0) {
        throw std::runtime_error("division by zero");
    }
    std::cout << a / b << std::endl;
}

// 捕获异常
try {
    divide(10, 0);
} catch (const std::runtime_error& e) {
    std::cerr << "Runtime error: " << e.what() << std::endl;
} catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
} catch (...) {
    std::cerr << "Unknown exception" << std::endl;
}

// 自定义异常
class MyException : public std::exception {
    std::string msg;
public:
    MyException(const std::string& m) : msg(m) {}
    const char* what() const noexcept override {
        return msg.c_str();
    }
};

throw MyException("custom error");
```

### 7.2 异常规范

指定函数可能抛出的异常类型。

```cpp
// 旧式异常规范（C++11 弃用）
void old_style() throw(std::bad_alloc);

// noexcept 规范（C++11）
void safe_function() noexcept {
    // 保证不抛异常
}

// noexcept 条件（C++17）
void maybe_throw(bool flag) noexcept(flag) {
    if (flag) throw std::exception();
}

// noexcept 运算符
template<typename T>
void clone(T* src, T* dst) noexcept(std::is_copy_constructible_v<T>) {
    new(dst) T(*src);
}
```

---

## 8. 范围库和算法

### 8.1 迭代器

```cpp
std::vector<int> v = {5, 2, 8, 1, 9};

// 迭代器遍历
for (auto it = v.begin(); it != v.end(); ++it) {
    std::cout << *it << " ";
}

// 反向迭代器
for (auto rit = v.rbegin(); rit != v.rend(); ++rit) {
    std::cout << *rit << " ";
}

// C++17: 简化
for (auto [iter, end] = std::views::enumerate(v); iter != end; ++iter) {
    std::cout << *iter << " ";
}
```

### 8.2 算法示例

```cpp
#include <algorithm>
#include <numeric>

std::vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

// 排序
std::sort(v.begin(), v.end());

// 查找
auto it = std::find(v.begin(), v.end(), 5);

// 计数
int cnt = std::count_if(v.begin(), v.end(), [](int x) { return x > 5; });

// 变换
std::transform(v.begin(), v.end(), v.begin(), [](int x) { return x * 2; });

// 累加
int sum = std::accumulate(v.begin(), v.end(), 0);

// 删除元素（注意：erase-remove 惯用法）
v.erase(std::remove_if(v.begin(), v.end(), [](int x) { return x < 5; }), v.end());
```

---

## 9. 总结

### 9.1 C++ 核心语法特征表

| 类别 | 特性 | 关键字/语法 | 标准版本 |
|------|------|-------------|----------|
| **类型系统** | 基础类型 | int, float, double, bool, char | C++98 |
| | 指针 | T* | C++98 |
| | 引用 | T& | C++98 |
| | auto 类型推导 | auto | C++11 |
| | decltype | decltype(expr) | C++11 |
| | 常量 | const, constexpr | C++98/C++11 |
| | 作用域枚举 | enum class | C++11 |
| **函数** | 默认参数 | void f(int x = 10) | C++98 |
| | 函数重载 | void f(int); void f(double); | C++98 |
| | inline 函数 | inline void f() | C++98 |
| | lambda 表达式 | [](int x) { return x; } | C++11 |
| | 泛化 lambda | [](auto x) { return x; } | C++14 |
| | 模板 lambda | []<typename T>(T x) { } | C++20 |
| **面向对象** | 类和结构体 | class, struct | C++98 |
| | 继承 | class D : public B { } | C++98 |
| | 虚函数 | virtual void f() = 0 | C++98 |
| | 构造/析构函数 | Class() { }; ~Class() { } | C++98 |
| | 拷贝/移动语义 | Class(const Class&), Class(Class&&) | C++11 |
| | override/final | void f() override; void g() final; | C++11/C++14 |
| | 委托构造函数 | Class() : Class(0, 0) { } | C++11 |
| | 继承构造函数 | using Base::Base; | C++11 |
| **模板** | 函数模板 | template<typename T> T f(T x) | C++98 |
| | 类模板 | template<typename T> class Vec { } | C++98 |
| | 变量模板 | template<typename T> T pi = ... | C++14 |
| | 概念 | template<Numeric T> | C++20 |
| | requires | template<typename T> requires ... | C++20 |
| **内存管理** | new/delete | new T; delete p; | C++98 |
| | 智能指针 | unique_ptr, shared_ptr, weak_ptr | C++11 |
| | make_unique | make_unique<T>(args) | C++14 |
| | make_shared | make_shared<T>(args) | C++11 |
| **错误处理** | 异常 | try { throw; } catch (...) { } | C++98 |
| | noexcept | void f() noexcept | C++11 |
| **新语法** | 初始化列表 | T x {1, 2}; | C++11 |
| | 范围 for | for (auto x : v) { } | C++11 |
| | nullptr | nullptr | C++11 |
| | 属性 | [[nodiscard]], [[maybe_unused]] | C++11/C++17/C++20 |
| **并发** | 线程 | std::thread | C++11 |
| | 互斥锁 | std::mutex, lock_guard | C++11 |
| | 异步 | std::async, std::future | C++11 |
| **模块化** | 模块 | import module; | C++20 |
| | 协程 | co_await, co_yield | C++20 |
| | 概念 | concept Addable = ... | C++20 |

### 9.2 UltraCPP 语法设计要点

基于以上分析，UltraCPP 语法设计应考虑：

1. **保持 C++ 基础语法兼容性**
   - 基础类型声明和初始化
   - 函数定义和调用语法
   - 类和结构体的基本形式

2. **简化或增强的语法**
   - 构造函数可以简化
   - 内存管理默认使用安全方式
   - 模块系统可以重新设计

3. **现代化语法借鉴**
   - 保留 auto 类型推导
   - 支持 lambda 表达式
   - 引入更安全的空指针概念

4. **可选增强**
   - 范围 for 循环
   - 智能指针作为默认
   - 错误处理机制

---

*文档版本: v0.1.0*
*创建日期: 2026-04-14*
