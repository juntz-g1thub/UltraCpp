#[derive(Debug, Clone)]
pub struct Module {
    pub declarations: Vec<TopLevel>,
}

#[derive(Debug, Clone)]
pub enum TopLevel {
    FuncDef(FuncDef),
    FuncDecl(FuncDecl),
    StructDef(StructDef),
    VarDecl(VarDecl),
    ConstDecl(ConstDecl),
    Import(Import),
    Export(Box<TopLevel>),
    Extern(Type, String, Vec<Param>),
}

#[derive(Debug, Clone)]
pub struct FuncDef {
    pub name: String,
    pub params: Vec<Param>,
    pub return_ty: Type,
    pub body: Box<Stmt>,
}

#[derive(Debug, Clone)]
pub struct FuncDecl {
    pub name: String,
    pub params: Vec<Param>,
    pub return_ty: Type,
}

#[derive(Debug, Clone)]
pub struct Param {
    pub name: String,
    pub ty: Type,
}

#[derive(Debug, Clone)]
pub struct StructDef {
    pub name: String,
    pub fields: Vec<StructField>,
}

#[derive(Debug, Clone)]
pub struct StructField {
    pub name: String,
    pub ty: Type,
}

#[derive(Debug, Clone)]
pub struct VarDecl {
    pub name: String,
    pub ty: Type,
    pub init: Option<Expr>,
}

#[derive(Debug, Clone)]
pub struct ConstDecl {
    pub name: String,
    pub ty: Type,
    pub value: Expr,
}

#[derive(Debug, Clone)]
pub struct Import {
    pub path: String,
    pub alias: Option<String>,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Type {
    Void,
    Bool,
    Char,
    Int,
    I8,
    I16,
    I32,
    I64,
    UInt,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    USize,
    ISize,
    Pointer(Box<Type>),
    MutablePointer(Box<Type>),
    Ref(Box<Type>),
    Array(Box<Type>, usize),
    Function(Box<Type>, Vec<Type>),
    Named(String),
}

#[derive(Debug, Clone)]
pub enum Stmt {
    Block(Vec<Stmt>),
    If(Expr, Box<Stmt>, Option<Box<Stmt>>),
    While(Expr, Box<Stmt>),
    For(Option<Box<Stmt>>, Option<Expr>, Option<Expr>, Box<Stmt>),
    Return(Option<Expr>),
    Break,
    Continue,
    Expr(Option<Expr>),
    Free(Expr),
    Decl(VarDecl),
}

#[derive(Debug, Clone)]
pub enum Expr {
    BinaryOp(BinaryOp, Box<Expr>, Box<Expr>),
    UnaryOp(UnaryOp, Box<Expr>),
    Call(Box<Expr>, Vec<Expr>),
    Index(Box<Expr>, Box<Expr>),
    FieldAccess(Box<Expr>, String),
    Assign(Box<Expr>, Box<Expr>),
    Move(Box<Expr>),
    Clone(Box<Expr>),
    Sizeof(Type),
    Ident(String),
    Literal(Literal),
    Block(Vec<Stmt>, Option<Box<Expr>>),
    Null,
}

#[derive(Debug, Clone)]
pub enum BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Shl,
    Shr,
    Lt,
    Gt,
    Le,
    Ge,
    Eq,
    Ne,
    BitAnd,
    BitOr,
    BitXor,
    And,
    Or,
    Assign,
    AddAssign,
    SubAssign,
    MulAssign,
    DivAssign,
    ModAssign,
    BitAndAssign,
    BitOrAssign,
    BitXorAssign,
    ShlAssign,
    ShrAssign,
}

#[derive(Debug, Clone)]
pub enum UnaryOp {
    Neg,
    Not,
    BitNot,
    Deref,
    AddrOf,
}

#[derive(Debug, Clone)]
pub enum Literal {
    Int(i64),
    Float(f64),
    Char(char),
    String(String),
    True,
    False,
}
