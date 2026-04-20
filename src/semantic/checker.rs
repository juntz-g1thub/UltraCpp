use crate::error::CompileError;
use crate::frontend::ast::{BinaryOp, Expr, FuncDef, Literal, Module, Stmt, Type, UnaryOp};
use crate::semantic::resolver::{Resolver, SymbolTable};

#[derive(Debug, Clone)]
pub struct TypedExpr {
    pub expr: Expr,
    pub ty: Type,
}

pub struct TypeChecker {
    symbol_table: SymbolTable,
}

impl TypeChecker {
    pub fn new(resolver: Resolver) -> Self {
        TypeChecker {
            symbol_table: resolver.get_symbol_table().clone(),
        }
    }

    pub fn check_module(&self, module: &Module) -> Result<(), CompileError> {
        for decl in &module.declarations {
            self.check_top_level(decl)?;
        }
        Ok(())
    }

    fn check_top_level(&self, decl: &crate::frontend::ast::TopLevel) -> Result<(), CompileError> {
        match decl {
            crate::frontend::ast::TopLevel::FuncDef(func) => {
                self.check_function(func)?;
            }
            crate::frontend::ast::TopLevel::FuncDecl(_) => {}
            crate::frontend::ast::TopLevel::StructDef(_) => {}
            crate::frontend::ast::TopLevel::VarDecl(_) => {}
            crate::frontend::ast::TopLevel::ConstDecl(_) => {}
            crate::frontend::ast::TopLevel::Import(_) => {}
            crate::frontend::ast::TopLevel::Export(inner) => {
                self.check_top_level(inner)?;
            }
            crate::frontend::ast::TopLevel::Extern(_, _, _) => {}
        }
        Ok(())
    }

    fn check_function(&self, func: &FuncDef) -> Result<(), CompileError> {
        self.check_stmt(&func.body, &func.return_ty)?;
        Ok(())
    }

    fn check_stmt(&self, stmt: &Stmt, expected_return: &Type) -> Result<(), CompileError> {
        match stmt {
            Stmt::Block(stmts) => {
                for s in stmts {
                    self.check_stmt(s, expected_return)?;
                }
            }
            Stmt::If(_, then_stmt, else_stmt) => {
                self.check_stmt(then_stmt, expected_return)?;
                if let Some(else_s) = else_stmt {
                    self.check_stmt(else_s, expected_return)?;
                }
            }
            Stmt::While(_, body) => {
                self.check_stmt(body, expected_return)?;
            }
            Stmt::For(_, _, _, body) => {
                self.check_stmt(body, expected_return)?;
            }
            Stmt::Return(expr) => {
                if let Some(e) = expr {
                    let ty = self.infer_expr_type(e)?;
                    self.unify(&ty, expected_return, "return type mismatch")?;
                }
            }
            Stmt::Break | Stmt::Continue => {}
            Stmt::Expr(expr) => {
                if let Some(e) = expr {
                    self.infer_expr_type(e)?;
                }
            }
            Stmt::Free(expr) => {
                self.infer_expr_type(expr)?;
            }
            Stmt::Decl(var_decl) => {
                if let Some(init) = &var_decl.init {
                    let init_ty = self.infer_expr_type(init)?;
                    self.unify(
                        &init_ty,
                        &var_decl.ty,
                        "variable initialization type mismatch",
                    )?;
                }
            }
        }
        Ok(())
    }

    fn infer_expr_type(&self, expr: &Expr) -> Result<Type, CompileError> {
        match expr {
            Expr::BinaryOp(op, left, right) => {
                let left_ty = self.infer_expr_type(left)?;
                let right_ty = self.infer_expr_type(right)?;
                self.binary_op_type(op, &left_ty, &right_ty)
            }
            Expr::UnaryOp(op, expr) => {
                let expr_ty = self.infer_expr_type(expr)?;
                self.unary_op_type(op, &expr_ty)
            }
            Expr::Call(func, args) => {
                let func_ty = self.infer_expr_type(func)?;
                if let Type::Function(ret_ty, param_tys) = func_ty {
                    if args.len() != param_tys.len() {
                        return Err(CompileError::Semantic(format!(
                            "expected {} arguments, got {}",
                            param_tys.len(),
                            args.len()
                        )));
                    }
                    for (arg, param_ty) in args.iter().zip(param_tys.iter()) {
                        let arg_ty = self.infer_expr_type(arg)?;
                        self.unify(&arg_ty, param_ty, "argument type mismatch")?;
                    }
                    Ok(*ret_ty)
                } else {
                    Err(CompileError::Semantic(
                        "cannot call non-function".to_string(),
                    ))
                }
            }
            Expr::Index(arr, index) => {
                let arr_ty = self.infer_expr_type(arr)?;
                let index_ty = self.infer_expr_type(index)?;
                if let Type::Pointer(inner) = arr_ty {
                    if !matches!(index_ty, Type::Int | Type::I32 | Type::ISize) {
                        return Err(CompileError::Semantic("index must be integer".to_string()));
                    }
                    Ok(*inner)
                } else if let Type::Array(inner, _) = arr_ty {
                    if !matches!(index_ty, Type::Int | Type::I32 | Type::ISize) {
                        return Err(CompileError::Semantic("index must be integer".to_string()));
                    }
                    Ok(*inner)
                } else {
                    Err(CompileError::Semantic(
                        "cannot index non-array/non-pointer".to_string(),
                    ))
                }
            }
            Expr::FieldAccess(expr, field_name) => {
                let expr_ty = self.infer_expr_type(expr)?;
                if let Type::Named(name) = expr_ty {
                    if let Some(struct_def) = self.symbol_table.get_struct(&name) {
                        for field in &struct_def.fields {
                            if field.name == *field_name {
                                return Ok(field.ty.clone());
                            }
                        }
                        return Err(CompileError::Semantic(format!(
                            "struct {} has no field {}",
                            name, field_name
                        )));
                    }
                    Err(CompileError::Semantic(format!(
                        "undefined struct: {}",
                        name
                    )))
                } else {
                    Err(CompileError::Semantic(
                        "cannot access field on non-struct".to_string(),
                    ))
                }
            }
            Expr::Assign(left, right) => {
                let left_ty = self.infer_expr_type(left)?;
                let right_ty = self.infer_expr_type(right)?;
                self.unify(&left_ty, &right_ty, "assignment type mismatch")?;
                Ok(Type::Void)
            }
            Expr::Move(expr) => self.infer_expr_type(expr),
            Expr::Clone(expr) => self.infer_expr_type(expr),
            Expr::Sizeof(ty) => Ok(Type::USize),
            Expr::Ident(name) => {
                if let Some(symbol) = self.symbol_table.lookup(name) {
                    Ok(symbol.ty.clone())
                } else {
                    Err(CompileError::Semantic(format!(
                        "undefined variable: {}",
                        name
                    )))
                }
            }
            Expr::Literal(lit) => Ok(self.literal_type(lit)),
            Expr::Block(stmts, expr) => {
                if let Some(e) = expr {
                    self.infer_expr_type(e)
                } else {
                    Ok(Type::Void)
                }
            }
            Expr::Null => Ok(Type::Pointer(Box::new(Type::Void))),
        }
    }

    fn binary_op_type(
        &self,
        op: &BinaryOp,
        left: &Type,
        right: &Type,
    ) -> Result<Type, CompileError> {
        match op {
            BinaryOp::Add | BinaryOp::Sub | BinaryOp::Mul | BinaryOp::Div | BinaryOp::Mod => {
                if self.is_numeric(left) && self.is_numeric(right) {
                    self.promote_numeric(left, right)
                } else {
                    Err(CompileError::Semantic(
                        "binary operation requires numeric operands".to_string(),
                    ))
                }
            }
            BinaryOp::Shl | BinaryOp::Shr => {
                if matches!(
                    left,
                    Type::Int | Type::I32 | Type::I64 | Type::ISize | Type::UInt | Type::USize
                ) {
                    Ok(left.clone())
                } else {
                    Err(CompileError::Semantic(
                        "shift requires integer operands".to_string(),
                    ))
                }
            }
            BinaryOp::Lt | BinaryOp::Gt | BinaryOp::Le | BinaryOp::Ge => {
                if self.is_numeric(left) && self.is_numeric(right) {
                    Ok(Type::Bool)
                } else {
                    Err(CompileError::Semantic(
                        "comparison requires numeric operands".to_string(),
                    ))
                }
            }
            BinaryOp::Eq | BinaryOp::Ne => {
                self.unify(left, right, "equality comparison type mismatch")?;
                Ok(Type::Bool)
            }
            BinaryOp::BitAnd | BinaryOp::BitOr | BinaryOp::BitXor => {
                if self.is_integer(left) && self.is_integer(right) {
                    self.promote_numeric(left, right)
                } else {
                    Err(CompileError::Semantic(
                        "bitwise operation requires integer operands".to_string(),
                    ))
                }
            }
            BinaryOp::And | BinaryOp::Or => {
                if matches!(left, Type::Bool) && matches!(right, Type::Bool) {
                    Ok(Type::Bool)
                } else {
                    Err(CompileError::Semantic(
                        "logical operation requires boolean operands".to_string(),
                    ))
                }
            }
            BinaryOp::Assign
            | BinaryOp::AddAssign
            | BinaryOp::SubAssign
            | BinaryOp::MulAssign
            | BinaryOp::DivAssign
            | BinaryOp::ModAssign
            | BinaryOp::BitAndAssign
            | BinaryOp::BitOrAssign
            | BinaryOp::BitXorAssign
            | BinaryOp::ShlAssign
            | BinaryOp::ShrAssign => {
                self.unify(left, right, "compound assignment type mismatch")?;
                Ok(Type::Void)
            }
        }
    }

    fn unary_op_type(&self, op: &UnaryOp, expr: &Type) -> Result<Type, CompileError> {
        match op {
            UnaryOp::Neg => {
                if self.is_numeric(expr) {
                    Ok(expr.clone())
                } else {
                    Err(CompileError::Semantic(
                        "negation requires numeric operand".to_string(),
                    ))
                }
            }
            UnaryOp::Not => {
                if matches!(expr, Type::Bool) {
                    Ok(Type::Bool)
                } else {
                    Err(CompileError::Semantic(
                        "logical not requires boolean operand".to_string(),
                    ))
                }
            }
            UnaryOp::BitNot => {
                if self.is_integer(expr) {
                    Ok(expr.clone())
                } else {
                    Err(CompileError::Semantic(
                        "bitwise not requires integer operand".to_string(),
                    ))
                }
            }
            UnaryOp::Deref => {
                if let Type::Pointer(inner) = expr {
                    Ok(*inner.clone())
                } else {
                    Err(CompileError::Semantic(
                        "dereference requires pointer".to_string(),
                    ))
                }
            }
            UnaryOp::AddrOf => Ok(Type::Pointer(Box::new(expr.clone()))),
        }
    }

    fn literal_type(&self, lit: &Literal) -> Type {
        match lit {
            Literal::Int(_) => Type::Int,
            Literal::Float(_) => Type::F64,
            Literal::Char(_) => Type::Char,
            Literal::String(_) => Type::Pointer(Box::new(Type::Char)),
            Literal::True | Literal::False => Type::Bool,
        }
    }

    fn is_numeric(&self, ty: &Type) -> bool {
        matches!(
            ty,
            Type::Int
                | Type::I8
                | Type::I16
                | Type::I32
                | Type::I64
                | Type::UInt
                | Type::U8
                | Type::U16
                | Type::U32
                | Type::U64
                | Type::F32
                | Type::F64
                | Type::ISize
                | Type::USize
        )
    }

    fn is_integer(&self, ty: &Type) -> bool {
        matches!(
            ty,
            Type::Int
                | Type::I8
                | Type::I16
                | Type::I32
                | Type::I64
                | Type::UInt
                | Type::U8
                | Type::U16
                | Type::U32
                | Type::U64
                | Type::ISize
                | Type::USize
        )
    }

    fn promote_numeric(&self, left: &Type, right: &Type) -> Result<Type, CompileError> {
        let left_rank = self.type_rank(left);
        let right_rank = self.type_rank(right);
        if left_rank >= right_rank {
            Ok(left.clone())
        } else {
            Ok(right.clone())
        }
    }

    fn type_rank(&self, ty: &Type) -> i32 {
        match ty {
            Type::I8 | Type::U8 => 1,
            Type::I16 | Type::U16 => 2,
            Type::Int | Type::UInt | Type::ISize | Type::USize => 3,
            Type::I32 | Type::U32 => 4,
            Type::I64 | Type::U64 => 5,
            Type::F32 => 6,
            Type::F64 => 7,
            _ => 0,
        }
    }

    fn unify(&self, left: &Type, right: &Type, msg: &str) -> Result<(), CompileError> {
        if left == right {
            return Ok(());
        }
        if matches!((left, right), (Type::Pointer(_), Type::Pointer(_)))
            || matches!((left, right), (Type::Named(_), Type::Named(_)))
        {
            return Ok(());
        }
        Err(CompileError::Semantic(format!(
            "{}: {} vs {}",
            msg,
            type_name(left),
            type_name(right)
        )))
    }
}

fn type_name(ty: &Type) -> String {
    match ty {
        Type::Void => "void".to_string(),
        Type::Bool => "bool".to_string(),
        Type::Char => "char".to_string(),
        Type::Int => "int".to_string(),
        Type::I8 => "i8".to_string(),
        Type::I16 => "i16".to_string(),
        Type::I32 => "i32".to_string(),
        Type::I64 => "i64".to_string(),
        Type::UInt => "uint".to_string(),
        Type::U8 => "u8".to_string(),
        Type::U16 => "u16".to_string(),
        Type::U32 => "u32".to_string(),
        Type::U64 => "u64".to_string(),
        Type::F32 => "f32".to_string(),
        Type::F64 => "f64".to_string(),
        Type::USize => "usize".to_string(),
        Type::ISize => "isize".to_string(),
        Type::Pointer(inner) => format!("{}*", type_name(inner)),
        Type::MutablePointer(inner) => format!("{}* mut", type_name(inner)),
        Type::Ref(inner) => format!("{}&", type_name(inner)),
        Type::Array(inner, size) => format!("{}[{}]", type_name(inner), size),
        Type::Function(ret, params) => {
            let params_str = params
                .iter()
                .map(|p| type_name(p))
                .collect::<Vec<_>>()
                .join(", ");
            format!("fn({}) -> {}", params_str, type_name(ret))
        }
        Type::Named(name) => name.clone(),
    }
}
