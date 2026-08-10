# UltraCPP 0.1 开发者手册

**版本：** 0.1.0-alpha  
**目标受众：** 编译器开发者  
**用途：** UltraCPP 编译器实现参考手册

---

## 目录

1. [编译器架构概述](#1-编译器架构概述)
2. [词法分析器设计](#2-词法分析器设计)
3. [语法分析器设计](#3-语法分析器设计)
4. [语义分析](#4-语义分析)
5. [中间表示](#5-中间表示)
6. [代码生成](#6-代码生成)
7. [预处理器](#7-预处理器)
8. [链接器](#8-链接器)
9. [标准库](#9-标准库)
10. [错误处理](#10-错误处理)
11. [工具链](#11-工具链)
12. [参考实现](#12-参考实现)

---

## 1. 编译器架构概述

### 1.1 流水线

```
源文件 (.upp) → [预处理器] → [词法分析器] → [语法分析器] → [AST] 
                → [语义分析] → [类型化 AST] 
                → [IR 生成] → [优化] 
                → [代码生成] → [汇编器] → [链接器] → 可执行文件
```

### 1.2 各阶段职责

| 阶段 | 输入 | 输出 | 主要职责 |
|------|------|------|----------|
| 预处理器 | .upp 源文件 | 展开后的源文件 | `#import`、`#include`、依赖解析 |
| 词法分析器 | 源文本 | Token 流 | 字符→Token 转换、关键字、字面量 |
| 语法分析器 | Token 流 | AST | 语法强制、表达式解析 |
| 语义分析 | AST | 验证后的 AST | 类型检查、所有权验证、符号解析 |
| IR 生成 | 验证后的 AST | IR | 展平为中间表示 |
| 优化 | IR | 优化后的 IR | 死代码消除、内联 |
| 代码生成 | IR | 目标代码 | LLVM IR 或原生汇编 |
| 链接 | 目标文件 | 可执行文件 | 符号解析、重定位 |

### 1.3 模块依赖

```
lib.rs
├── ast.rs           (类型定义、AST 节点)
├── lexer.rs         (词法分析)
├── parser.rs        (AST 构建)
├── type_check.rs    (类型推断和检查)
├── ownership.rs     (借用检查器、移动验证)
├── interpreter.rs   (AST 解释器，用于原型开发)
└── ffi.rs           (外部函数接口)
```

---

## 2. 词法分析器设计

### 2.1 Token 定义

```cpp
// From src/lexer.rs
enum Token {
    // 关键字
    Fn, Let, Mut, Struct, Trait, Impl, If, Else, While, Return,
    Extern, Unsafe, Const, Enum, As, In, For, Loop, Match, Pub,
    True, False, SelfKw, SelfType, Where, Move, Ref, Continue, Break,
    
    // 字面量
    Int(i64),
    Float(f64),
    String(char*),
    Char(char),
    
    // 运算符
    Plus, Minus, Star, Slash, Percent, Eq, EqEq, Not, NotEq,
    Lt, Gt, LtEq, GtEq, AndAnd, OrOr, And, Or, Xor, Tilde,
    Shl, Shr, PlusEq, MinusEq, StarEq, SlashEq, PercentEq,
    AndEq, OrEq, XorEq, ShlEq, ShrEq, Inc, Dec, Arrow,
    
    // 分隔符
    LBrace, RBrace, LParen, RParen, LBracket, RBracket,
    Semi, Comma, Colon, ColonColon, Dot, FatArrow, At, Quest,
    
    // 标识符和特殊类型
    Ident(char*),
    Comment(char*),
    Eof,
    Error(char*),
};
```

### 2.2 状态机

```
                     ┌─────────────┐
                     │   START     │
                     └──────┬──────┘
                            │
          ┌─────────────────┼─────────────────┐
          │                 │                 │
          ▼                 ▼                 ▼
    ┌──────────┐      ┌───────────┐     ┌──────────┐
    │ IDENT    │      │  NUMBER   │     │ OPERATOR │
    │ (keyword │      │ (int/float│     │ (+ - * / │
    │  lookup) │      │  hex/bin) │     │  etc.)   │
    └──────────┘      └───────────┘     └──────────┘
          │                 │                 │
          └─────────────────┼─────────────────┘
                            │
                            ▼
                       [Tokenize]
```

### 2.3 关键字识别

```cpp
// 直接查找表 (每个标识符 O(1))
switch (ident) {
    case "fn": return Token::Fn;
    case "let": return Token::Let;
    case "mut": return Token::Mut;
    case "struct": return Token::Struct;
    // ... 等等
    default: return Token::Ident(ident);
}
```

### 2.4 标识符规则

- 模式: `[a-zA-Z_][a-zA-Z0-9_]*`
- 关键字不能用作标识符
- 大小写敏感: `self` vs `Self` vs `selfKw`

### 2.5 字面量解析

```cpp
// 整数: 十进制、十六进制 (0x)、二进制 (0b)
"42"    → Token::Int(42)
"0xFF"  → Token::Int(255)
"0b1010"→ Token::Int(10)

// 浮点数: 小数点后需要数字
"3.14"  → Token::Float(3.14)
"0.0"   → Token::Float(0.0)

// 字符串: 支持转义序列
"hello\nworld" → Token::String("hello\nworld")

// 字符: 单引号
'A'     → Token::Char('A')
'\n'    → Token::Char('\n')
```

### 2.6 转义序列

| 序列 | 字符 |
|------|------|
| `\n` | 换行符 (0x0A) |
| `\r` | 回车符 (0x0D) |
| `\t` | 制表符 (0x09) |
| `\\` | 反斜杠 (0x5C) |
| `\"` | 双引号 (0x22) |
| `\'` | 单引号 (0x27) |
| `\xHH` | 十六进制字节 |

### 2.7 错误恢复

- 词法分析器为无效输入发出 `Token::Error(String)`
- 语法分析器在错误 token 后继续
- 常见错误:
  - 未终止的字符串/字符
  - 无效的十六进制/二进制数字
  - 未知字符

---

## 3. 语法分析器设计

### 3.1 AST 节点层次

```
Program
  └── items: std::vector<Item*>
        ├── Item::Function(Function*)
        ├── Item::StructDef(StructDef*)
        ├── Item::EnumDef(EnumDef*)
        ├── Item::TraitDef(TraitDef*)
        ├── Item::ImplBlock(ImplBlock*)
        ├── Item::ExternBlock(ExternBlock*)
        ├── Item::LetBinding { ... }
        ├── Item::Const { ... }
        ├── Item::Module { ... }
        └── Item::Use { ... }

Stmt
  ├── Stmt::Let { name, mutable, type_, value }
  ├── Stmt::Expr(Expr*)
  ├── Stmt::Return(Expr*)
  ├── Stmt::Continue
  └── Stmt::Break(Expr*)

Expr
  ├── Expr::Int(i64), Float(f64), String(char*), Bool(bool), Char(char)
  ├── Expr::Ident(char*)
  ├── Expr::BinaryOp(Expr*, BinOp, Expr*)
  ├── Expr::UnaryOp(UnOp, Expr*)
  ├── Expr::Assign(Expr*, AssignOp, Expr*)
  ├── Expr::Call(Expr*, std::vector<Expr*>)
  ├── Expr::Method(Expr*, char*, std::vector<Expr*>)
  ├── Expr::Field(Expr*, char*)
  ├── Expr::Index(Expr*, Expr*)
  ├── Expr::Block(std::vector<Stmt*>)
  ├── Expr::Tuple(std::vector<Expr*>)
  ├── Expr::Array(std::vector<Expr*>)
  ├── Expr::Struct { type_name, fields }
  └── Expr::ControlFlow(ControlFlow*)
```

### 3.2 递归下降解析

```cpp
// 语法分析器结构
struct Parser {
    Token* tokens;
    int token_count;
    int pos;
};

// 入口点
int parse(Parser* parser, Program** out_program) {
    return parse_program(parser, out_program);
}

// Program = item*
int parse_program(Parser* parser, Program** out) {
    std::vector<Item*> items;
    while (!is_at_end(parser)) {
        Item* item;
        int err = parse_item(parser, &item);
        if (err != 0) return err;
        items.push_back(item);
    }
    *out = program_create(items);
    return 0;
}

// Item = function | struct | enum | trait | impl | extern | let | const | module | use
int parse_item(Parser* parser, Item** out) {
    bool is_pub = parse_visibility(parser);
    Token* current = current_token(parser);
    switch (current->kind) {
        case Token::Fn: return parse_function(parser, is_pub, out);
        case Token::Struct: return parse_struct(parser, is_pub, out);
        // ... 等等
        default: return parse_error(parser, "Unexpected token");
    }
}
```

### 3.3 运算符优先级

语法分析器使用递归下降配合优先级攀升:

```cpp
// 从最低到最高优先级
parse_assign_expr()    // =, +=, -=, ...
parse_logic_or_expr()  // ||
parse_logic_and_expr() // &&
parse_bitwise_or_expr()// |
parse_bitwise_xor_expr()// ^
parse_bitwise_and_expr()// &
parse_equality_expr()  // ==, !=
parse_relational_expr()// <, >, <=, >=
parse_shift_expr()     // <<, >>
parse_additive_expr()  // +, -
parse_multiplicative_expr() // *, /, %
parse_unary_expr()     // -, !, &, &mut, *
parse_postfix_expr()   // ., (), []
parse_primary_expr()   // 字面量、标识符、分组
```

### 3.4 表达式解析示例

```cpp
// if 表达式
int parse_if(Parser* parser, Stmt** out) {
    expect(parser, Token::If);
    Expr* condition;
    int err = parse_expr(parser, &condition);
    if (err != 0) return err;
    
    Stmt* then_branch;
    err = parse_block(parser, &then_branch);
    if (err != 0) return err;
    
    Stmt* else_branch = nullptr;
    if (check(parser, Token::Else)) {
        advance(parser);
        if (check(parser, Token::If)) {
            err = parse_if(parser, &else_branch);
            if (err != 0) return err;
        } else {
            err = parse_block(parser, &else_branch);
            if (err != 0) return err;
        }
    }
    
    *out = stmt_create_control_flow(ControlFlow::If {
        condition,
        then_branch,
        else_branch,
    });
    return 0;
}
```

### 3.5 错误报告

```cpp
struct ParseError {
    char* message;
};

ParseError* parse_error_create(const char* message) {
    ParseError* err = (ParseError*)malloc(sizeof(ParseError));
    err->message = strdup(message);
    return err;
}
```

常见解析错误:
- 期望 token X，得到 Y
- 文件意外结束
- 表达式中意外的 token

---

## 4. 语义分析

### 4.1 类型系统

```cpp
// From src/type_check.rs
enum Type {
    // 原始类型
    I8, I16, I32, I64, I128,
    U8, U16, U32, U64, U128,
    F32, F64,
    Bool, Char, String, Unit,
    
    // 所有权类型
    Owned(Box<Type>),
    Ref(Box<Type>),
    RefMut(Box<Type>),
    RawPtr(Box<Type>),
    
    // 复合类型
    Struct { char* name, std::vector<std::pair<char*, Type*>> fields },
    Tuple(std::vector<Type*>),
    Array(Box<Type>, size_t),
    Fn { std::vector<Type*> params, Box<Type> ret },
    
    // 特殊类型
    Result(Box<Type>, Box<Type>),
    Option(Box<Type>),
    Var(char*),
    Error,
}
```

### 4.2 类型检查

```cpp
struct TypeChecker {
    std::unordered_map<char*, Type*> env;           // 变量类型
    std::unordered_map<char*, FuncSig*> functions;  // 函数签名
    std::vector<TypeError*> errors;
    int next_var_id;
};

// 类型检查入口
int check(TypeChecker* checker, AstNode* node, Type** out_type) {
    switch (node->kind) {
        case AstNode::Function: {
            Function* func = node->function;
            return check_function(checker, func->name, func->params, 
                                  func->ret_type, func->body, func->location, out_type);
        }
        case AstNode::Let: {
            Let* let_node = node->let;
            return check_let(checker, let_node->name, let_node->type_annot, 
                            let_node->value, let_node->location, out_type);
        }
        // ...
    }
}
```

### 4.3 符号表

```cpp
// 符号表条目
struct Symbol {
    char* name;
    Type* type_;
    SymbolKind kind;
    ScopeId scope;
};

enum SymbolKind {
    Variable,
    Function,
    Struct,
    Trait,
    Field,
    Param,
};

struct Scope {
    ScopeId parent;
    std::unordered_map<char*, Symbol*> symbols;
};
```

### 4.4 所有权跟踪

```cpp
// From src/ownership.rs
enum OwnershipState {
    Owned,    // 值被拥有且有效
    Moved,    // 所有权已转移
    Dropped,  // 已被显式 drop
};

struct OwnershipChecker {
    std::unordered_map<char*, OwnershipState> variables;
    std::unordered_map<char*, size_t> move_locations;
    std::unordered_map<char*, size_t> drop_locations;
    std::unordered_map<char*, std::vector<Borrow*>> borrows;  // 活跃借用
};

struct Borrow {
    char* variable;
    bool mutable_;
    size_t start;
    size_t end;
};
```

### 4.5 所有权规则

1. **单一所有者**: 每个值有且只有一个所有者
2. **移动转移**: 移动使源变量失效
3. **作用域退出时 drop**: 所有者超出作用域时 drop 值
4. **借用规则**:
   - 允许多个不可变借用
   - 允许一个可变借用
   - 不可变和可变借用不能同时存在

### 4.6 所有权错误

```cpp
enum OwnershipError {
    UseAfterMove { char* var, size_t location, size_t move_location },
    UseAfterDrop { char* var, size_t location, size_t drop_location },
    BorrowConflict { char* var, size_t location, size_t conflict_location },
    DanglingReference { size_t location, size_t owner_location },
    CannotMoveOutOfBorrow { char* var, size_t location },
    CannotMoveBorrowed { char* var, size_t location },
};
```

### 4.7 借用检查算法

```cpp
// 记录借用
int record_borrow(OwnershipChecker* checker, const char* var, bool mutable_, 
                  size_t start, size_t end) {
    // 检查变量是否有效
    OwnershipState* state = map_get(checker->variables, var);
    if (state != nullptr) {
        if (*state == Moved) return UseAfterMove;
        if (*state == Dropped) return UseAfterDrop;
    }
    
    // 检查借用冲突
    int err = check_borrow_conflict(checker, var, mutable_, start);
    if (err != 0) return err;
    
    // 记录借用
    Borrow* borrow = borrow_create(var, mutable_, start, end);
    map_get_or_create(checker->borrows, var)->push_back(borrow);
    return 0;
}

// 检查冲突: 可变借用阻塞所有其他借用
int check_borrow_conflict(OwnershipChecker* checker, const char* var, 
                          bool mutable_, size_t location) {
    std::vector<Borrow*>* borrows = map_get(checker->borrows, var);
    if (borrows == nullptr) return 0;
    
    if (mutable_) {
        if (!borrows->empty()) {
            return BorrowConflict;
        }
    } else {
        // 不可变: 检查是否存在可变借用
        for (Borrow* borrow : *borrows) {
            if (borrow->mutable_) {
                return BorrowConflict;
            }
        }
    }
    return 0;
}
```

### 4.8 移动语义

```cpp
// 赋值时
int record_move(OwnershipChecker* checker, const char* var, size_t location) {
    // 如果被借用则不能移动
    std::vector<Borrow*>* borrows = map_get(checker->borrows, var);
    if (borrows != nullptr && !borrows->empty()) {
        return CannotMoveBorrowed;
    }
    
    map_insert(checker->variables, var, Moved);
    map_insert(checker->move_locations, var, location);
    return 0;
}
```

---

## 5. 中间表示

### 5.1 IR 设计（简化版）

v0.1 使用简单的三地址代码 IR:

```cpp
enum Instruction {
    // 算术
    Add(Dest, Src1, Src2),
    Sub(Dest, Src1, Src2),
    Mul(Dest, Src1, Src2),
    Div(Dest, Src1, Src2),
    
    // 内存
    Load(Dest, Address),
    Store(Address, Src),
    Alloc(Dest, Size),        // 堆分配
    LoadField(Dest, Base, FieldIdx),
    StoreField(Base, FieldIdx, Src),
    
    // 控制流
    Br(LabelId),
    BrIf(cond, then_label, else_label),
    Label(LabelId),
    Return(Src),
    Call(Dest, Func, std::vector<Src>),
    
    // 比较
    Cmp(Dest, Src1, CmpOp, Src2),
    
    // 转换
    Cast(Dest, Src, CastOp),
    
    // 所有权
    Move(Dest, Src),          // 所有权转移
    Borrow(Dest, Src, Mut),  // 创建借用
    Drop(Src),                // 释放值
};
```

### 5.2 基本块

```cpp
struct BasicBlock {
    LabelId label;
    std::vector<Instruction*> instructions;
    Terminator* terminator;
};

enum Terminator {
    Br(LabelId),
    BrIf(cond, LabelId, LabelId),
    Return(Value*),
    Unreachable,
};
```

### 5.3 函数 IR

```cpp
struct FunctionIR {
    char* name;
    std::vector<std::pair<char*, Type*>> params;
    Type* return_type;
    std::vector<BasicBlock*> blocks;
    std::vector<std::pair<char*, Type*>> locals;
};
```

### 5.4 模块 IR

```cpp
struct ModuleIR {
    std::vector<FunctionIR*> functions;
    std::vector<std::tuple<char*, Type*, IRValue*>> globals;
    std::vector<StructDef*> structs;
    std::vector<char*> strings;  // 字符串常量
};
```

### 5.5 SSA 形式（v0.1 可选）

```cpp
// 简单 SSA: 每次赋值创建新版本
// %1 = Add(%0, 1)
// %2 = Add(%1, 1)  // %1 此后不可复用

// 用于控制流合并的 phi 节点
// %3 = phi(%1, %2)  // 根据控制流选择
```

### 5.6 优化 passes（可选）

| Pass | 描述 |
|------|------|
| DCE | 移除未使用的指令 |
| CSE | 公共子表达式消除 |
| ConstProp | 常量折叠 |
| Inlining | 小函数内联 |
| LoopInv | 移动循环不变代码 |

---

## 6. 代码生成

### 6.1 LLVM 集成（推荐）

```cpp
// 目标: 生成 LLVM IR
struct LLVMCodeGen {
    LLVMContext* context;
    LLVMModule* module;
    LLVMBuilder* builder;
    std::unordered_map<char*, LLVMValue*> functions;
    std::unordered_map<char*, LLVMType*> types;
};
```

### 6.2 类型映射

| UltraCPP 类型 | LLVM 类型 |
|---------------|-----------|
| i8 | i8 |
| i32 | i32 |
| i64 | i64 |
| f32 | float |
| f64 | double |
| bool | i1 |
| () | void |
| &T | 指向 T 的指针 |
| Owned<T> | 指向 T 的指针（堆分配） |

### 6.3 函数调用 ABI

```cpp
// C 兼容 ABI，用于 extern "C" 函数
// UltraCPP 内部: 通过指针传递

// 函数签名:
// int foo(int a, int* b, void* c)
// LLVM: i32 @foo(i32, ptr, ptr)
```

### 6.4 符号命名

```cpp
// 符号名称格式: _uc_<mangled_name>

// 函数:
// int foo(int x)  →  _uc_foo_i32_i32

// 结构体方法:
// struct Foo { int bar() }  →  _uc_Foo_bar__uc_Foo
```

### 6.5 名称修饰（简化版）

```cpp
char* mangle(const char* name, Type** params, int param_count) {
    std::string result = "_uc_";
    result += name;
    for (int i = 0; i < param_count; i++) {
        result += "_";
        result += mangle_type(params[i]);
    }
    return strdup(result.c_str());
}

const char* mangle_type(Type* t) {
    switch (t->kind) {
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::Ref: return mangle_type(t->inner);
        // ... 等等
        default: return "x";  // 未知类型
    }
}
```

### 6.6 代码生成算法

```cpp
LLVMValue* codegen_function(LLVMCodeGen* gen, FunctionIR* func) {
    // 1. 创建函数签名
    LLVMType* func_type = llvm_type(gen, func->return_type);
    std::vector<LLVMType*> param_types;
    for (auto& [_, t] : func->params) {
        param_types.push_back(llvm_type(gen, t));
    }
    
    // 2. 创建基本块
    for (BasicBlock* block : func->blocks) {
        LLVMBasicBlock* llbb = append_basic_block(gen->context, block->label);
        position_at_end(gen->builder, llbb);
        
        // 3. 生成指令
        for (Instruction* instr : block->instructions) {
            codegen_instr(gen, instr);
        }
        
        // 4. 生成终止符
        codegen_terminator(gen, block->terminator);
    }
    
    // 5. 验证并返回
    return build_function(gen->builder);
}
```

### 6.7 内存分配

```cpp
// Owned<T> 的堆分配
// libc::malloc(size)
void gen_alloc(LLVMCodeGen* gen, LLVMValue* dest, LLVMValue* size) {
    LLVMValue* malloc_fn = get_c_function(gen, "malloc");
    LLVMValue* ptr = build_call(gen->builder, malloc_fn, &size, 1);
    build_store(gen->builder, ptr, dest);
}

// 临时变量的栈分配
LLVMValue* gen_alloca(LLVMCodeGen* gen, LLVMType* ty) {
    return build_alloca(gen->builder, ty);
}
```

---

## 7. 预处理器

### 7.1 指令

```cpp
enum Directive {
    Import(Path),        // #import "module"
    Include(Path),       // #include "header.h"
    Ifdef(Symbol),       // #ifdef SYMBOL
    Ifndef(Symbol),      // #ifndef SYMBOL
    If(Expr*),            // #if EXPR
    Else,
    ElseIf(Expr*),
    Endif,
    Define(Symbol, Expr*),// #define SYMBOL EXPR
    Undef(Symbol),       // #undef SYMBOL
};
```

### 7.2 #import 处理

```cpp
int process_import(Preprocessor* preprocessor, Path* path, FileId* out_file_id) {
    // 1. 解析路径（相对于当前文件，然后搜索路径）
    Path* resolved = resolve_path(preprocessor, path);
    if (resolved == nullptr) return PREPROCESSOR_ERROR;
    
    // 2. 检查是否已导入（防止重复包含）
    if (is_imported(preprocessor, resolved)) {
        *out_file_id = get_file_id(preprocessor, resolved);
        return 0;
    }
    
    // 3. 添加到导入图
    dependency_graph_add(preprocessor->import_graph, 
                         preprocessor->current_file, resolved);
    
    // 4. 解析导入的文件
    char* content = read_file(preprocessor, resolved);
    if (content == nullptr) return PREPROCESSOR_ERROR;
    
    Token* tokens = lex_and_expand(preprocessor, content);
    
    // 5. 返回以供插入
    *out_file_id = add_file(preprocessor, resolved, tokens);
    return 0;
}
```

### 7.3 依赖图

```cpp
struct DependencyGraph {
    std::unordered_map<FileId, std::vector<FileId>> edges;  // file → dependencies
};

void dependency_graph_add(DependencyGraph* graph, FileId from, FileId to) {
    graph->edges[from].push_back(to);
}

// 拓扑排序以确定编译顺序
int topological_order(DependencyGraph* graph, std::vector<FileId>* out_order) {
    // Kahn 算法
    std::vector<FileId> result;
    std::unordered_map<FileId, int> in_degree;
    
    // 初始化入度
    for (auto& [file, _] : graph->edges) {
        in_degree[file] = 0;
    }
    
    // 统计入边
    for (auto& [_, deps] : graph->edges) {
        for (FileId dep : deps) {
            in_degree[dep]++;
        }
    }
    
    // 从入度为 0 的文件开始
    std::queue<FileId> queue;
    for (auto& [file, degree] : in_degree) {
        if (degree == 0) {
            queue.push(file);
        }
    }
    
    while (!queue.empty()) {
        FileId node = queue.front();
        queue.pop();
        result.push_back(node);
        
        if (auto deps = map_get(graph->edges, node)) {
            for (FileId dep : *deps) {
                int* d = map_get_mut(in_degree, dep);
                if (d) {
                    (*d)--;
                    if (*d == 0) {
                        queue.push(dep);
                    }
                }
            }
        }
    }
    
    if (result.size() != graph->edges.size()) {
        return CYCLE_ERROR;  // 检测到环
    }
    
    *out_order = result;
    return 0;
}
```

### 7.4 环检测

```cpp
// 导入时检测循环依赖
std::vector<FileId>* detect_cycle(DependencyGraph* graph, FileId start) {
    std::unordered_set<FileId> visited;
    std::vector<FileId> stack;
    
    std::vector<FileId> result;
    if (dfs_cycle(graph, start, &visited, &stack, &result)) {
        return new std::vector<FileId>(result);
    }
    return nullptr;
}

bool dfs_cycle(DependencyGraph* graph, FileId node,
               std::unordered_set<FileId>* visited, 
               std::vector<FileId>* stack,
               std::vector<FileId>* out_cycle) {
    if (vector_contains(stack, node)) {
        // 找到环
        size_t idx = vector_find(stack, node);
        *out_cycle = vector_slice(stack, idx, stack->size());
        return true;
    }
    if (set_contains(visited, node)) {
        return false;
    }
    
    set_insert(visited, node);
    vector_push(stack, node);
    
    std::vector<FileId>* deps = map_get(graph->edges, node);
    if (deps) {
        for (FileId dep : *deps) {
            if (dfs_cycle(graph, dep, visited, stack, out_cycle)) {
                return true;
            }
        }
    }
    
    vector_pop(stack);
    return false;
}
```

### 7.5 宏展开

```cpp
struct Macro {
    char* name;
    std::vector<char*> params;
    std::vector<Token*> body;
    bool is_function_like;
};

std::vector<Token*> expand_macros(Preprocessor* preprocessor, 
                                   std::vector<Token*> tokens) {
    std::vector<Token*> result;
    std::unordered_map<char*, Macro*>* macros = preprocessor->macros;
    
    for (Token* token : tokens) {
        if (token->kind == Token::Ident) {
            Macro* macro = map_get(*macros, token->ident);
            if (macro != nullptr) {
                if (macro->is_function_like) {
                    // 收集参数并展开
                    std::vector<std::vector<Token*>> args;
                    collect_args(preprocessor, &tokens, &args);
                    std::vector<Token*> expanded = expand(macro, args);
                    result.insert(result.end(), expanded.begin(), expanded.end());
                } else {
                    std::vector<Token*> expanded = expand(macro, {});
                    result.insert(result.end(), expanded.begin(), expanded.end());
                }
                continue;
            }
        }
        result.push_back(token);
    }
    
    return result;
}
```

---

## 8. 链接器

### 8.1 符号解析

```cpp
struct Linker {
    std::vector<ObjectFile*> object_files;
    std::vector<LibraryPath*> libraries;
    std::unordered_map<char*, SymbolDef*> symbols;  // name → definition
};

struct SymbolDef {
    char* name;
    uint64_t address;
    size_t size;
    SymbolKind kind;
    FileId file;
};

enum SymbolKind {
    Function,
    Global,
    Section,
    File,
};
```

### 8.2 符号解析算法

```cpp
int resolve(Linker* linker) {
    // 1. 收集所有已定义的符号
    for (ObjectFile* obj : linker->object_files) {
        for (SymbolDef* sym : obj->symbols) {
            if (sym->is_defined()) {
                linker->symbols[sym->name] = sym;
            }
        }
    }
    
    // 2. 解析未定义的引用
    for (ObjectFile* obj : linker->object_files) {
        for (size_t i = 0; i < obj->undefined_refs.size(); i++) {
            char* name = obj->get_ref_name(obj->undefined_refs[i]);
            SymbolDef** def = map_get(linker->symbols, name);
            if (def != nullptr) {
                obj->resolve_ref(i, (*def)->address);
            } else if (search_libraries(linker, name)) {
                // 符号在库中找到
            } else {
                return LINKER_ERROR_UNDEFINED(name);
            }
        }
    }
    
    return 0;
}
```

### 8.3 地址重定位

```cpp
enum Relocation {
    Absolute {
        size_t offset;
        char* target;
        int64_t addend;
    },
    Relative {
        size_t offset;
        char* target;
        int64_t addend;
    },
    BitField {
        size_t offset;
        size_t size;
        char* target;
    },
};

int apply_relocations(Linker* linker, Section* section) {
    for (Relocation* reloc : section->relocations) {
        uint64_t target_addr = linker->symbols[reloc->target]->address + reloc->addend;
        
        switch (reloc->kind) {
            case RelocationKind::Absolute: {
                uint8_t* ptr = section->data.data() + reloc->offset;
                uint64_t* slot = (uint64_t*)ptr;
                *slot = target_addr;
                break;
            }
            case RelocationKind::Relative: {
                // jmp/call 指令使用相对偏移
                int8_t* ptr = (int8_t*)(section->data.data() + reloc->offset);
                int32_t* slot = (int32_t*)ptr;
                int64_t relative = (int64_t)target_addr - (int64_t)reloc->offset + reloc->addend;
                *slot = (int32_t)relative;
                break;
            }
        }
    }
    return 0;
}
```

### 8.4 库搜索

```cpp
bool search_libraries(Linker* linker, const char* symbol) {
    for (LibraryPath* lib_path : linker->libraries) {
        Library* lib = library_load(lib_path);
        if (lib != nullptr && library_contains_symbol(lib, symbol)) {
            linker->object_files.push_back(library_get_object_for_symbol(lib, symbol));
            return true;
        }
    }
    return false;
}

// 搜索路径顺序:
// 1. -L 指定的路径（按顺序）
// 2. 编译器 sysroot lib 目录
// 3. 系统库路径 (/usr/lib 等)
```

### 8.5 静态链接

```cpp
int static_link(Linker* linker, const char* output) {
    // 1. 加载所有目标文件
    for (const char* obj_path : linker->input_files) {
        ObjectFile* obj = object_file_load(obj_path);
        if (obj == nullptr) return LINKER_ERROR;
        linker->object_files.push_back(obj);
    }
    
    // 2. 加载所需的归档成员
    int err = resolve_archive_dependencies(linker);
    if (err != 0) return err;
    
    // 3. 解析所有符号
    err = resolve(linker);
    if (err != 0) return err;
    
    // 4. 应用重定位
    for (ObjectFile* obj : linker->object_files) {
        err = object_file_apply_relocations(obj);
        if (err != 0) return err;
    }
    
    // 5. 合并节
    Section* image = create_image(linker);
    
    // 6. 写入可执行文件
    err = write_executable(linker, image, output);
    
    return err;
}
```

---

## 9. 标准库

### 9.1 最小启动 (crt0)

```asm
; Minimal C runtime for UltraCPP
; File: ucrt0.s

.section .text
.global _start
_start:
    ; 设置栈
    mov rsp, stack_top
    
    ; 调用全局构造函数
    call _uc_init_globals
    
    ; 调用 main
    call _uc_main
    mov edi, eax    ; exit code
    
    ; 退出
    mov eax, 60     ; sys_exit
    syscall

; For LLVM, use @llvm.startdbg.main for debug info
```

### 9.2 内置函数

```cpp
// 这些函数无需显式声明即可使用

// 打印函数
void print(const char* s);              // 打印字符串
void println(const char* s);            // 打印字符串并换行
void print_num(int n);                  // 打印整数
void print_float(double f);             // 打印浮点数
void print_bool(bool b);                // 打印布尔值

// 内存函数
void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);

// 字符串函数
size_t strlen(const char* s);
size_t strcpy(char* dst, const char* src);
int strcmp(const char* a, const char* b);

// 转换
int parse_int(const char* s);           // 返回值或错误码
double parse_float(const char* s);      // 返回值或错误码
char* to_string(int n);
char* to_string_f(double n);

// 所有权辅助函数
template<typename T>
void drop(T value);                     // 显式 drop

template<typename T>
void forget(T value);                   // 阻止 drop

template<typename T>
void leak(void* value);                 // 泄漏所有权

template<typename T>
void swap(T* a, T* b);

template<typename T>
T replace(T* a, T b);

template<typename T>
T take(T* a);                           // 取值，保留默认值
```

### 9.3 Option 和 Result

```cpp
template<typename T>
struct Option {
    bool is_some_;
    T value;
    
    static Option Some(T v) { Option o; o.is_some_ = true; o.value = v; return o; }
    static Option None() { Option o; o.is_some_ = false; return o; }
    bool is_some() const { return is_some_; }
    bool is_none() const { return !is_some_; }
    T unwrap() const { if (!is_some_) panic(); return value; }
    T unwrap_or(T default_val) const { return is_some_ ? value : default_val; }
};

template<typename T, typename E>
struct Result {
    bool is_ok_;
    T value;
    E error;
    
    static Result Ok(T v) { Result r; r.is_ok_ = true; r.value = v; return r; }
    static Result Err(E e) { Result r; r.is_ok_ = false; r.error = e; return r; }
    bool is_ok() const { return is_ok_; }
    bool is_err() const { return !is_ok_; }
    T unwrap() const { if (!is_ok_) panic(); return value; }
    E unwrap_err() const { if (is_ok_) panic(); return error; }
    T unwrap_or(T default_val) const { return is_ok_ ? value : default_val; }
};
```

### 9.4 Vec<T>（Phase 2 - 模板支持后）

> 注：0.1 版本不包含模板，以下为 Phase 2 的设计参考。

```cpp
// 使用 typedef 模拟泛型（0.1 临时方案）
typedef struct {
    int* data;
    size_t len;
    size_t capacity;
} Vec_int;

Vec_int* vec_create() {
    Vec_int* v = alloc(Vec_int);
    v->data = nullptr;
    v->len = 0;
    v->capacity = 0;
    return v;
}

Vec_int* vec_with_capacity(size_t cap) {
    Vec_int* v = alloc(Vec_int);
    v->data = (int*)malloc(sizeof(int) * cap);
    v->len = 0;
    v->capacity = cap;
    return v;
}
```
    
    void push(T value);
    T pop();
    size_t len() const { return len; }
    size_t capacity() const { return capacity; }
    bool is_empty() const { return len == 0; }
    void clear();
    T* get(size_t index);
    T* get_mut(size_t index);
    T* first() { return len > 0 ? &data[0] : nullptr; }
    T* last() { return len > 0 ? &data[len - 1] : nullptr; }
};
```

---

## 10. 错误处理

### 10.1 错误分类

```cpp
enum ErrorKind {
    // 词法分析器错误
    Lexer {
        LexerErrorKind kind;
        Location location;
    },
    
    // 语法分析器错误
    Parser {
        ParserErrorKind kind;
        Location location;
    },
    
    // 类型错误
    Type {
        TypeErrorKind kind;
        Location location;
    },
    
    // 所有权错误
    Ownership {
        OwnershipErrorKind kind;
        Location location;
    },
    
    // 链接器错误
    Linker {
        LinkerErrorKind kind;
        Location* location;
    },
    
    // IO 错误
    Io {
        IoErrorKind kind;
        PathBuf* path;
    },
}
```

### 10.2 错误种类详情

```cpp
enum LexerErrorKind {
    UnterminatedString,
    UnterminatedChar,
    InvalidHexEscape,
    InvalidNumber(char*),
    UnknownCharacter(char),
    InvalidOperator(char*),
}

enum ParserErrorKind {
    UnexpectedToken(Token*),
    ExpectedToken(Token*),
    UnexpectedEndOfFile,
    InvalidExpression(char*),
    InvalidStatement(char*),
}

enum TypeErrorKind {
    TypeMismatch { Type* expected, Type* found },
    UndefinedVariable(char*),
    UndefinedFunction(char*),
    InvalidOperand { char* op, std::vector<Type*> types },
    CannotInferType,
    CyclicType,
    DivisionByZero,
    IndexOutOfBounds,
}

enum OwnershipErrorKind {
    UseAfterMove { Location move_location },
    UseAfterDrop { Location drop_location },
    BorrowConflict { Location conflict_location },
    DanglingReference { Location owner_location },
    CannotMoveBorrowed,
    CannotMoveOutOfBorrow,
}
```

### 10.3 错误消息格式

```
error[E0001]: type mismatch
  --> src/main.uc:5:12
    |
 5  |     int x = "hello";
    |            ^^^ expected int, found const char*

error[E0002]: use-after-move
  --> src/main.uc:10:5
    |
 8  |     int* s = alloc(int);
 9  |     int* t = s;
    |             - value moved here
10 |     *s = 42;
    |         ^ value used after move

error[E0003]: borrow conflict
  --> src/main.uc:15:10
    |
13  |     int* r = &x;
14  |     int* w = &mut x;
    |                  - first borrow here
15  |     int* r2 = &x;
    |                  ^ second borrow

error[E0004]: undefined function
  --> src/main.uc:20:1
    |
 20 |     foo();
    |     ^^^ not found in this scope
```

### 10.4 源位置跟踪

```cpp
struct Location {
    size_t line;
    size_t column;
};

struct Span {
    Location start;
    Location end;
};

template<typename T>
struct Spanned {
    T value;
    Span span;
};
```

### 10.5 错误报告

```cpp
struct Diagnostic {
    Severity severity;
    char* code;
    char* message;
    Span location;
    std::vector<Note*> notes;
};

enum Severity {
    Error,
    Warning,
    Info,
    Help,
};

struct Note {
    Span* location;
    char* message;
};

void emit_error(DiagnosticConsumer* consumer, Diagnostic* diag) {
    const char* severity_str;
    switch (diag->severity) {
        case Error: severity_str = "error"; break;
        case Warning: severity_str = "warning"; break;
        default: severity_str = "note"; break;
    }
    
    consumer->output += format("%s[%s]: %s\n", 
                               severity_str, diag->code, diag->message);
    
    // 打印源上下文
    print_source_line(consumer, &diag->location);
    
    // 打印注释
    for (Note* note : diag->notes) {
        consumer->output += format("  = %s\n", note->message);
    }
}
```

---

## 11. 工具链

### 11.1 编译器驱动 (ucac)

**名称**: `ucac` (UltraCPP C Compiler)  
**理由**: 对 C 开发者熟悉，短小易记

### 11.2 命令行接口

```bash
# 基本用法
ucac [options] <input files>

# 输出选项
-o <file>              # 输出文件名
-c                      # 仅编译，不链接
-S                      # 输出汇编
--emit-llvm             # 输出 LLVM IR
--emit-obj               # 输出目标文件

# 语言选项
--std=<version>         # 语言标准 (例如 ultracpp0.1)
--no-stdlib            # 不链接标准库

# 优化
-O<level>              # 优化级别 (0, 1, 2, 3, s, z)
-Onone                 # 无优化（debug 默认值）
-O0, -O1, -O2, -O3
-Os                    # 优化大小
-Oz                    # 优化大小（激进）

# 调试
-g[level]              # 调试信息 (0, 1, 2)
--source-map           # 生成源映射

# 链接
-L<dir>                # 库搜索路径
-l<lib>                # 链接库
-static                # 静态链接
-shared                # 构建共享库

# 预处理器
-I<dir>                # 包含搜索路径
-D<name>[=<value>]     # 定义宏
-U<name>               # 取消定义宏
--preprocess           # 仅预处理

# 其他
-v, --verbose          # 详细输出
--version             # 打印版本
--help                 # 显示帮助
```

### 11.3 示例调用

```bash
# 编译并链接单个文件（推荐 .uc 扩展名）
ucac -o program main.uc

# 编译为目标文件
ucac -c lib.uc -o lib.o

# 编译并优化
ucac -O2 -c main.uc -o main.o

# 链接多个目标文件
ucac -o program main.o lib.o -lm

# 显示帮助
ucac --help

# 显示版本
ucac --version
```

### 11.4 输出格式

| 标志 | 输出 |
|------|------|
| (默认) | 可执行文件 |
| `-c` | 目标文件 (.o) |
| `-S` | 汇编 (.s) |
| `--emit-llvm` | LLVM IR (.ll) |
| `--emit=obj` | 目标文件 (.o) |
| `-E` | 预处理后的源文件 (.i) |

### 11.5 退出码

| 代码 | 含义 |
|------|------|
| 0 | 成功 |
| 1 | 编译错误 |
| 2 | 链接错误 |
| 3 | 内部编译器错误 |
| 4 | 无效参数 |
| 5 | IO 错误 |

### 11.6 工具组织

```
ucac          # 驱动（解析参数、调用工具）
├── ucpp      # 预处理器
├── ucc       # 编译器（词法分析 + 语法分析 + 代码生成）
├── ucld      # 链接器（可选，可使用系统 ld）
└── ucar      # 归档器（用于库）
```

---

## 12. 参考实现

### 12.1 符号表

```cpp
// 带作用域的简单符号表
struct SymbolTable {
    std::vector<Scope*> scopes;
};

struct Scope {
    std::unordered_map<char*, Symbol*> symbols;
};

SymbolTable* symbol_table_create() {
    SymbolTable* st = new SymbolTable();
    st->scopes.push_back(scope_create());  // 全局作用域
    return st;
}

void symbol_table_enter_scope(SymbolTable* st) {
    st->scopes.push_back(scope_create());
}

void symbol_table_exit_scope(SymbolTable* st) {
    st->scopes.pop_back();
}

int symbol_table_define(SymbolTable* st, char* name, Symbol* symbol) {
    Scope* current = st->scopes.back();
    if (current->symbols.find(name) != current->symbols.end()) {
        return SYMBOL_ERROR_ALREADY_DEFINED;
    }
    current->symbols[name] = symbol;
    return 0;
}

Symbol* symbol_table_lookup(SymbolTable* st, const char* name) {
    for (auto it = st->scopes.rbegin(); it != st->scopes.rend(); ++it) {
        auto found = (*it)->symbols.find(name);
        if (found != (*it)->symbols.end()) {
            return found->second;
        }
    }
    return nullptr;
}

Symbol* symbol_table_lookup_current(SymbolTable* st, const char* name) {
    Scope* current = st->scopes.back();
    auto found = current->symbols.find(name);
    return found != current->symbols.end() ? found->second : nullptr;
}
```

### 12.2 所有权跟踪

```cpp
// 完整的所有权检查器
struct OwnershipChecker {
    // 变量 → 所有权状态
    std::unordered_map<char*, OwnershipState> vars;
    // 变量 → 被移动的位置
    std::unordered_map<char*, size_t> move_locs;
    // 变量 → 被 drop 的位置
    std::unordered_map<char*, size_t> drop_locs;
    // 变量 → 活跃借用
    std::unordered_map<char*, std::vector<Borrow*>> borrows;
};

// 声明新变量
void ownership_checker_declare(OwnershipChecker* checker, const char* var, size_t loc) {
    checker->vars[var] = Owned;
}

// 检查变量是否可以使用
int ownership_checker_check_use(OwnershipChecker* checker, const char* var, size_t loc) {
    auto it = checker->vars.find(var);
    if (it == checker->vars.end()) {
        return 0;  // 类型错误，不是所有权错误
    }
    
    OwnershipState state = it->second;
    
    if (state == Owned) {
        // 检查没有可变借用
        auto borrows_it = checker->borrows.find(var);
        if (borrows_it != checker->borrows.end()) {
            for (Borrow* b : borrows_it->second) {
                if (b->mutable_) {
                    return BORROW_CONFLICT;
                }
            }
        }
        return 0;
    }
    
    if (state == Moved) {
        size_t move_loc = 0;
        auto loc_it = checker->move_locs.find(var);
        if (loc_it != checker->move_locs.end()) move_loc = loc_it->second;
        return OWNERSHIP_ERROR_USE_AFTER_MOVE(var, loc, move_loc);
    }
    
    if (state == Dropped) {
        return OWNERSHIP_ERROR_USE_AFTER_DROP(var, loc, 0);  // 需要修复
    }
    
    return 0;
}

// 记录所有权转移
int ownership_checker_move_var(OwnershipChecker* checker, const char* var, size_t loc) {
    // 不能移动被借用的变量
    auto borrows_it = checker->borrows.find(var);
    if (borrows_it != checker->borrows.end() && !borrows_it->second.empty()) {
        return CANNOT_MOVE_BORROWED;
    }
    
    checker->vars[var] = Moved;
    checker->move_locs[var] = loc;
    return 0;
}

// 记录借用
int ownership_checker_borrow(OwnershipChecker* checker, const char* var, 
                              bool mutable_, size_t loc) {
    // 检查冲突
    if (mutable_) {
        // 可变: 不允许其他借用
        auto borrows_it = checker->borrows.find(var);
        if (borrows_it != checker->borrows.end() && !borrows_it->second.empty()) {
            return BORROW_CONFLICT;
        }
    } else {
        // 不可变: 不允许可变借用
        auto borrows_it = checker->borrows.find(var);
        if (borrows_it != checker->borrows.end()) {
            for (Borrow* b : borrows_it->second) {
                if (b->mutable_) {
                    return BORROW_CONFLICT;
                }
            }
        }
    }
    
    Borrow* borrow = borrow_create(var, mutable_, loc, loc);
    checker->borrows[var].push_back(borrow);
    return 0;
}

// 结束作用域，释放借用
void ownership_checker_end_scope(OwnershipChecker* checker, size_t scope_end) {
    for (auto& [_, borrows] : checker->borrows) {
        std::vector<Borrow*> filtered;
        for (Borrow* b : borrows) {
            if (b->end > scope_end) {
                filtered.push_back(b);
            } else {
                free(b);
            }
        }
        borrows = filtered;
    }
    
    std::vector<char*> to_remove;
    for (auto& [var, borrows] : checker->borrows) {
        if (borrows.empty()) {
            to_remove.push_back(var);
        }
    }
    for (char* var : to_remove) {
        checker->borrows.erase(var);
    }
}
```

### 12.3 依赖图

```cpp
// 导入依赖图
struct DependencyGraph {
    std::unordered_map<FileId, std::vector<FileId>> edges;
    std::unordered_map<FileId, PathBuf*> files;
};

void dependency_graph_add_import(DependencyGraph* graph, FileId importer, FileId imported) {
    graph->edges[importer].push_back(imported);
}

// Kahn 算法拓扑排序
int dependency_graph_topological_sort(DependencyGraph* graph, 
                                       std::vector<FileId>* out_order) {
    std::unordered_map<FileId, int> in_degree;
    std::deque<FileId> zero_degree;
    
    // 初始化
    for (auto& [file, _] : graph->files) {
        in_degree[file] = 0;
    }
    
    // 统计入边
    for (auto& [_, deps] : graph->edges) {
        for (FileId dep : deps) {
            in_degree[dep]++;
        }
    }
    
    // 从没有依赖的文件开始
    for (auto& [file, degree] : in_degree) {
        if (degree == 0) {
            zero_degree.push_back(file);
        }
    }
    
    std::vector<FileId> result;
    while (!zero_degree.empty()) {
        FileId file = zero_degree.front();
        zero_degree.pop_front();
        result.push_back(file);
        
        if (auto deps = map_get(graph->edges, file)) {
            for (FileId dep : *deps) {
                int* d = map_get_mut(in_degree, dep);
                if (d) {
                    (*d)--;
                    if (*d == 0) {
                        zero_degree.push_back(dep);
                    }
                }
            }
        }
    }
    
    if (result.size() != graph->files.size()) {
        return CYCLE_ERROR;
    }
    
    *out_order = result;
    return 0;
}

// 使用 DFS 检测环
std::vector<FileId> dependency_graph_find_cycle(DependencyGraph* graph) {
    std::unordered_set<FileId> visited;
    std::vector<FileId> stack;
    std::vector<FileId> result;
    
    for (auto& [file, _] : graph->files) {
        if (dfs_cycle(graph, file, &visited, &stack, &result)) {
            return result;
        }
    }
    return {};
}

bool dfs_cycle(DependencyGraph* graph, FileId node,
               std::unordered_set<FileId>* visited, 
               std::vector<FileId>* stack,
               std::vector<FileId>* out_cycle) {
    if (vector_contains(stack, node)) {
        size_t start = vector_find(stack, node);
        *out_cycle = vector_slice(stack, start, stack->size());
        return true;
    }
    if (set_contains(visited, node)) {
        return false;
    }
    
    set_insert(visited, node);
    vector_push(stack, node);
    
    std::vector<FileId>* deps = map_get(graph->edges, node);
    if (deps) {
        for (FileId dep : *deps) {
            if (dfs_cycle(graph, dep, visited, stack, out_cycle)) {
                return true;
            }
        }
    }
    
    vector_pop(stack);
    return false;
}
```

### 12.4 借用检查算法

```cpp
// 带生命周期推断的借用检查
struct BorrowChecker {
    // 每个借用都有推断的生命周期（作用域）
    std::unordered_map<char*, Lifetime*> lifetimes;
    // 被借用 → 借用集合
    std::unordered_map<char*, std::unordered_set<BorrowId>> borrows;
};

struct Lifetime {
    size_t start;
    size_t end;
};

// 分析函数中的借用违规
std::vector<BorrowError*> borrow_checker_check_function(BorrowChecker* checker, 
                                                         Function* func) {
    std::vector<BorrowError*> errors;
    
    for (size_t idx = 0; idx < func->body.size(); idx++) {
        Stmt* stmt = func->body[idx];
        int err = borrow_checker_check_statement(checker, stmt, idx);
        if (err != 0) {
            errors.push_back(borrow_error_create(err, idx));
        }
    }
    
    return errors;
}

int borrow_checker_check_statement(BorrowChecker* checker, Stmt* stmt, size_t idx) {
    switch (stmt->kind) {
        case Stmt::Let: {
            Let* let = stmt->let;
            borrow_checker_declare(checker, let->name, let->mutable_);
            if (let->value != nullptr) {
                return borrow_checker_check_expr(checker, let->value, idx);
            }
            break;
        }
        case Stmt::Expr: {
            return borrow_checker_check_expr(checker, stmt->expr, idx);
        }
        case Stmt::Return: {
            if (stmt->return_expr != nullptr) {
                return borrow_checker_check_expr(checker, stmt->return_expr, idx);
            }
            break;
        }
        // ... 等等
    }
    return 0;
}

int borrow_checker_check_expr(BorrowChecker* checker, Expr* expr, size_t idx) {
    switch (expr->kind) {
        case Expr::Ident: {
            return borrow_checker_check_use(checker, expr->name, idx);
        }
        case Expr::Borrow: {
            return borrow_checker_record_borrow(checker, expr->name, expr->mutable_, idx);
        }
        case Expr::Assign: {
            // 检查移动语义
            if (expr->lhs->kind == Expr::Ident) {
                int err = borrow_checker_check_move(checker, expr->lhs->name, idx);
                if (err != 0) return err;
            }
            return borrow_checker_check_expr(checker, expr->rhs, idx);
        }
        // ... 等等
    }
    return 0;
}

// 用于生命周期推断的活跃性分析
Lifetime* borrow_checker_infer_lifetime(BorrowChecker* checker, const char* var, size_t from) {
    // 查找变量最后一次使用的位置
    size_t end = from;
    // 简单启发式: 扩展到当前块末尾
    return lifetime_create(from, end);
}
```

---

## 附录 A: 快速参考

### 文件扩展名

| 扩展名 | 描述 |
|--------|------|
| `.upp` | UltraCPP 源文件 |
| `.uppi` | UltraCPP 包含（头文件） |
| `.uppo` | UltraCPP 目标文件 |
| `.uplib` | UltraCPP 静态库 |
| `.upll` | LLVM IR 输出 |

### 编译器流水线

```
源文件 → 词法分析 → 语法分析 → 语义分析 → IR → 优化 → 代码生成 → 链接 → 可执行文件
  │        │        │        │        │       │         │        │        │
  └────────┴────────┴────────┴────────┴───────┴─────────┴────────┴────────┘
                     错误向后传播
```

### 关键数据结构

- `Token` - 词法分析器输出
- `Program` - AST 根节点
- `Type` - 类型表示
- `OwnershipChecker` - 借用和移动跟踪
- `SymbolTable` - 作用域和标识符解析

---

## 附录 B: 未来考虑

### 阶段 B 功能（0.1 后）

1. **模板/泛型** - 类型参数化
2. **异常** - 通过 `throw`/`catch` 进行错误处理
3. **Async/Await** - 异步编程
4. **线程** - 并发支持
5. **GC 选项** - 不安全代码的可选垃圾回收
6. **模块** - 导入/导出可见性
7. **标准库扩展** - 更多集合、IO

### 性能目标

- 编译速度 10,000 行/秒
- 链接速度 50,000 行/秒
- 二进制大小在等效 C 的 2 倍以内

---

*文档版本: 0.1.0-alpha*  
*最后更新: 2026-04-13*  
*UltraCPP 项目: https://github.com/ultracpp/ultracpp*
