# UltraCPP 编译器架构设计

> 版本：0.1.0
> 状态：架构设计
> 更新日期：2026-04-17

---

## 1. 概述

### 1.1 目标

构建一个基于 Rust 的 UltraCPP 语言编译器，将 UltraCPP 代码编译为可执行文件。

### 1.2 技术选型

| 组件 | 选择 | 理由 |
|------|------|------|
| 前端 | 手写词法/语法分析 | 完全控制，适合学习 |
| IR | 中间表示 | 支持优化和多后端 |
| 后端 | LLVM (via inkwell) | 成熟生态，跨平台 |
| 构建工具 | Cargo | Rust 标准 |

### 1.3 编译流程

```
源代码 (.uc/.upp)
    │
    ▼
┌─────────┐
│  词法   │ → Token 流
│  分析   │
└─────────┘
    │
    ▼
┌─────────┐
│  语法   │ → AST
│  分析   │
└─────────┘
    │
    ▼
┌─────────┐
│  语义   │ → 检查后的 AST + 符号表
│  分析   │
└─────────┘
    │
    ▼
┌─────────┐
│  生成   │ → LLVM IR
│   IR    │
└─────────┘
    │
    ▼
┌─────────┐
│ LLVM    │ → 目标文件 / 可执行文件
│  优化   │
└─────────┘
```

---

## 2. 项目结构

```
ultracpp/
├── Cargo.toml
├── src/
│   ├── main.rs              # 入口点
│   ├── cli.rs               # 命令行参数
│   │
│   ├── frontend/
│   │   ├── mod.rs
│   │   ├── lexer.rs         # 词法分析器
│   │   ├── token.rs         # Token 定义
│   │   ├── parser.rs        # 语法分析器
│   │   └── ast.rs           # AST 节点定义
│   │
│   ├── semantic/
│   │   ├── mod.rs
│   │   ├── resolver.rs      # 符号解析
│   │   ├── checker.rs       # 类型检查
│   │   └── ownership.rs    # 所有权检查
│   │
│   ├── codegen/
│   │   ├── mod.rs
│   │   ├── generator.rs     # 代码生成器
│   │   ├── llvm_utils.rs    # LLVM 绑定工具
│   │   └── builtin.rs       # 内置函数实现
│   │
│   └── error/
│       ├── mod.rs
│       ├── lexer_error.rs   # 词法错误
│       ├── parse_error.rs   # 解析错误
│       └── type_error.rs    # 类型错误
│
└── tests/
    └── integration/         # 集成测试
```

---

## 3. 核心模块设计

### 3.1 词法分析器 (Lexer)

**职责**：将源代码字符串转换为 Token 序列

**Token 类型**：
```rust
pub enum TokenKind {
    // 字面量
    Int(i64),
    Float(f64),
    Char(char),
    String(String),
    Ident(String),

    // 关键字
    KwIf, KwElse, KwWhile, KwFor, KwReturn,
    KwStruct, KwExport, KwImport, KwConst,
    KwUnique, KwMove, KwFree, KwAlloc, KwNull,
    KwTrue, KwFalse, KwVoid, KwExtern, KwUnsafe,
    KwAs, KwStatic, KwClone,

    // 运算符
    OpPlus, OpMinus, OpStar, OpSlash, OpPercent,
    OpAssign, OpEq, OpNe, OpLt, OpGt, OpLe, OpGe,
    OpAnd, OpOr, OpNot,
    OpBitAnd, OpBitOr, OpBitXor, OpBitNot,
    OpShl, OpShr, OpInc, OpDec,

    // 分隔符
    LParen, RParen, LBrace, RBrace, LBracket, RBracket,
    Comma, Semicolon, Colon, Dot, Arrow, Scope,
    Pound,  // #

    // 预处理器
    PpImport, PpInclude, PpDefine, PpIfdef, PpIfndef, PpEndif,

    Eof,
}
```

### 3.2 语法分析器 (Parser)

**职责**：将 Token 序列解析为 AST

**主要 AST 节点**：
```rust
pub enum AstNode {
    // 程序
    Program(Vec<TopLevel>),

    // 声明
    FuncDef(FuncDefNode),
    StructDef(StructDefNode),
    VarDecl(VarDeclNode),
    ConstDecl(ConstDeclNode),
    Import(ImportNode),
    Export(Box<AstNode>),

    // 语句
    Block(Vec<AstNode>),
    If(IfNode),
    While(WhileNode),
    For(ForNode),
    Return(Option<Box<AstNode>>),
    Break,
    Continue,
    ExprStmt(Option<Box<AstNode>>),
    Free(Box<AstNode>),

    // 表达式
    BinaryOp(BinaryOpNode),
    UnaryOp(UnaryOpNode),
    Call(CallNode),
    Index(IndexNode),
    FieldAccess(FieldAccessNode),
    Assign(AssignNode),
    Move(Box<AstNode>),
    Clone(Box<AstNode>),
    sizeof(Type),
    Ident(String),
    Literal(Literal),
    BlockExpr(Vec<AstNode>, Option<Box<AstNode>>),
}
```

### 3.3 语义分析 (Semantic Analysis)

**职责**：
1. 符号表管理
2. 类型检查
3. 所有权检查（move/clone 语义）

### 3.4 代码生成 (Codegen)

**职责**：将 AST 转换为 LLVM IR

**内置函数映射**：
| UltraCPP | LLVM IR |
|----------|---------|
| `alloc(T)` | `malloc` |
| `free(ptr)` | `free` |
| `move(ptr)` | 所有权转移 |
| `clone(ptr)` | `llvm.memcpy` |
| `is_null(ptr)` | `icmp eq` |
| `print(s)` | `puts` |

---

## 4. 所有权检查设计

### 4.1 指针赋值规则

```rust
// 赋值操作符处理
fn check_assignment(&mut self, lhs: &AstNode, rhs: &AstNode) -> Result<()> {
    match (lhs.ty(), rhs.ty()) {
        // 指针类型：赋值即 move
        (Ty::Pointer(_), Ty::Pointer(_)) => {
            // 检查 rhs 是否为 null
            // 标记 rhs 所有权转移
        }
        // 非指针类型：普通赋值
        _ => {}
    }
}
```

### 4.2 move 语义

```rust
// move 表达式检查
fn check_move(&mut self, expr: &AstNode) -> Result<Ty> {
    let ty = self.check_expr(expr)?;
    if let Ty::Pointer(inner) = ty {
        // 确保指针有效
        // 返回相同类型
    }
}
```

### 4.3 clone 语义

```rust
// clone 表达式检查
fn check_clone(&mut self, expr: &AstNode) -> Result<Ty> {
    let ty = self.check_expr(expr)?;
    if let Ty::Pointer(inner) = ty {
        // 生成 memcpy
        // 返回新指针
    }
}
```

---

## 5. 内联汇编设计

### 5.1 asm 块结构

```rust
pub struct AsmBlock {
    pub instructions: Vec<String>,
    pub outputs: Vec<AsmOperand>,
    pub inputs: Vec<AsmOperand>,
    pub clobbers: Vec<String>,
}

pub struct AsmOperand {
    pub name: String,
    pub constraint: String,
    pub expr: Box<AstNode>,
}
```

### 5.2 约束字符

| 约束 | 含义 |
|------|------|
| `r` | 通用寄存器 |
| `=` | 输出寄存器 |
| `&` | early clobber |
| `m` | 内存引用 |

---

## 6. 错误处理设计

### 6.1 错误类型

```rust
pub enum CompileError {
    Lexer(LexerError),
    Parse(ParseError),
    Semantic(SemanticError),
    Codegen(CodegenError),
}

pub enum LexerError {
    InvalidChar(char, Position),
    UnterminatedString(Position),
    UnterminatedComment(Position),
}

pub enum ParseError {
    UnexpectedToken(Token, Expected),
    Expected(String, Position),
}

pub enum SemanticError {
    UndefinedVar(String, Position),
    TypeMismatch(Ty, Ty, Position),
    InvalidMove(Position),
    // ...
}
```

### 6.2 错误报告格式

```
error[E0001]: undefined variable 'x'
  --> test.uc:5:3
   |
5  |     int y = x + 1;
   |           ^
```

---

## 7. 测试策略

### 7.1 单元测试

- Lexer 测试：Token 化正确性
- Parser 测试：AST 生成正确性
- Semantic 测试：类型检查、所有权检查

### 7.2 集成测试

```rust
// tests/integration/hello_world.rs
const SOURCE: &str = r#"
int main() {
    print("Hello, UltraCPP!");
    return 0;
}
"#;

#[test]
fn test_hello_world() {
    let result = compile(SOURCE);
    assert!(result.is_ok());
}
```

### 7.3 语法覆盖测试

每个语法结构至少有一个测试用例。

---

## 8. 下一步行动

1. **环境搭建**：安装 Rust toolchain、LLVM
2. **项目初始化**：创建目录结构、Cargo.toml
3. **词法分析器**：实现 Token 和 Lexer
4. **语法分析器**：实现 Parser 和 AST
5. **语义分析**：实现符号表、类型检查
6. **代码生成**：集成 LLVM，生成 IR
7. **测试**：编写测试用例

---

## 9. 文档历史

| 版本 | 日期 | 描述 |
|------|------|------|
| 0.1.0 | 2026-04-17 | 初始架构设计 |

---

*UltraCPP 编译器架构设计*
