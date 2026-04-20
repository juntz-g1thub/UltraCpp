use crate::error::CompileError;
use crate::frontend::ast::{Expr, FuncDef, Module, Stmt, StructDef, Type};
use std::collections::HashMap;

#[derive(Debug, Clone)]
pub struct Symbol {
    pub name: String,
    pub ty: Type,
    pub kind: SymbolKind,
}

#[derive(Debug, Clone)]
pub enum SymbolKind {
    Variable,
    Function,
    Struct,
    Parameter,
    Field,
}

#[derive(Debug, Clone, Default)]
pub struct Scope {
    symbols: HashMap<String, Symbol>,
}

impl Scope {
    pub fn new() -> Self {
        Scope {
            symbols: HashMap::new(),
        }
    }

    pub fn insert(&mut self, symbol: Symbol) -> Option<Symbol> {
        self.symbols.insert(symbol.name.clone(), symbol)
    }

    pub fn lookup(&self, name: &str) -> Option<&Symbol> {
        self.symbols.get(name)
    }
}

#[derive(Debug, Clone, Default)]
pub struct SymbolTable {
    scopes: Vec<Scope>,
    struct_defs: HashMap<String, StructDef>,
}

impl SymbolTable {
    pub fn new() -> Self {
        SymbolTable {
            scopes: vec![Scope::new()],
            struct_defs: HashMap::new(),
        }
    }

    pub fn push_scope(&mut self) {
        self.scopes.push(Scope::new());
    }

    pub fn pop_scope(&mut self) {
        if self.scopes.len() > 1 {
            self.scopes.pop();
        }
    }

    pub fn insert(&mut self, symbol: Symbol) -> Option<Symbol> {
        self.scopes.last_mut().unwrap().insert(symbol)
    }

    pub fn lookup(&self, name: &str) -> Option<&Symbol> {
        for scope in self.scopes.iter().rev() {
            if let Some(symbol) = scope.lookup(name) {
                return Some(symbol);
            }
        }
        None
    }

    pub fn add_struct(&mut self, def: StructDef) {
        self.struct_defs.insert(def.name.clone(), def);
    }

    pub fn get_struct(&self, name: &str) -> Option<&StructDef> {
        self.struct_defs.get(name)
    }

    pub fn current_scope(&self) -> &Scope {
        self.scopes.last().unwrap()
    }
}

pub struct Resolver {
    symbol_table: SymbolTable,
    current_function: Option<String>,
}

impl Resolver {
    pub fn new() -> Self {
        Resolver {
            symbol_table: SymbolTable::new(),
            current_function: None,
        }
    }

    pub fn resolve_module(&mut self, module: &Module) -> Result<(), CompileError> {
        for decl in &module.declarations {
            self.resolve_top_level(decl)?;
        }
        Ok(())
    }

    fn resolve_top_level(
        &mut self,
        decl: &crate::frontend::ast::TopLevel,
    ) -> Result<(), CompileError> {
        match decl {
            crate::frontend::ast::TopLevel::FuncDef(func) => {
                self.resolve_function(func)?;
            }
            crate::frontend::ast::TopLevel::FuncDecl(_) => {}
            crate::frontend::ast::TopLevel::StructDef(struct_def) => {
                self.symbol_table.add_struct(struct_def.clone());
            }
            crate::frontend::ast::TopLevel::VarDecl(var_decl) => {
                self.resolve_var_decl(var_decl)?;
            }
            crate::frontend::ast::TopLevel::ConstDecl(const_decl) => {
                self.resolve_const_decl(const_decl)?;
            }
            crate::frontend::ast::TopLevel::Import(_) => {}
            crate::frontend::ast::TopLevel::Export(inner) => {
                self.resolve_top_level(inner)?;
            }
            crate::frontend::ast::TopLevel::Extern(_, _, _) => {}
        }
        Ok(())
    }

    fn resolve_function(&mut self, func: &FuncDef) -> Result<(), CompileError> {
        let func_symbol = Symbol {
            name: func.name.clone(),
            ty: Type::Function(
                Box::new(func.return_ty.clone()),
                func.params.iter().map(|p| p.ty.clone()).collect(),
            ),
            kind: SymbolKind::Function,
        };
        self.symbol_table.insert(func_symbol);

        let prev_function = self.current_function.take();
        self.current_function = Some(func.name.clone());

        self.symbol_table.push_scope();

        for param in &func.params {
            let param_symbol = Symbol {
                name: param.name.clone(),
                ty: param.ty.clone(),
                kind: SymbolKind::Parameter,
            };
            self.symbol_table.insert(param_symbol);
        }

        self.resolve_stmt(&func.body)?;

        self.symbol_table.pop_scope();
        self.current_function = prev_function;

        Ok(())
    }

    fn resolve_var_decl(
        &mut self,
        var_decl: &crate::frontend::ast::VarDecl,
    ) -> Result<(), CompileError> {
        let symbol = Symbol {
            name: var_decl.name.clone(),
            ty: var_decl.ty.clone(),
            kind: SymbolKind::Variable,
        };
        self.symbol_table.insert(symbol);

        if let Some(init) = &var_decl.init {
            self.resolve_expr(init)?;
        }

        Ok(())
    }

    fn resolve_const_decl(
        &mut self,
        const_decl: &crate::frontend::ast::ConstDecl,
    ) -> Result<(), CompileError> {
        let symbol = Symbol {
            name: const_decl.name.clone(),
            ty: const_decl.ty.clone(),
            kind: SymbolKind::Variable,
        };
        self.symbol_table.insert(symbol);
        self.resolve_expr(&const_decl.value)?;
        Ok(())
    }

    fn resolve_stmt(&mut self, stmt: &Stmt) -> Result<(), CompileError> {
        match stmt {
            Stmt::Block(stmts) => {
                self.symbol_table.push_scope();
                for s in stmts {
                    self.resolve_stmt(s)?;
                }
                self.symbol_table.pop_scope();
            }
            Stmt::If(_, then_stmt, else_stmt) => {
                self.resolve_stmt(then_stmt)?;
                if let Some(else_s) = else_stmt {
                    self.resolve_stmt(else_s)?;
                }
            }
            Stmt::While(_, body) => {
                self.symbol_table.push_scope();
                self.resolve_stmt(body)?;
                self.symbol_table.pop_scope();
            }
            Stmt::For(init, cond, update, body) => {
                self.symbol_table.push_scope();
                if let Some(init_stmt) = init {
                    self.resolve_stmt(init_stmt)?;
                }
                if let Some(c) = cond {
                    self.resolve_expr(c)?;
                }
                if let Some(u) = update {
                    self.resolve_expr(u)?;
                }
                self.resolve_stmt(body)?;
                self.symbol_table.pop_scope();
            }
            Stmt::Return(expr) => {
                if let Some(e) = expr {
                    self.resolve_expr(e)?;
                }
            }
            Stmt::Break | Stmt::Continue => {}
            Stmt::Expr(expr) => {
                if let Some(e) = expr {
                    self.resolve_expr(e)?;
                }
            }
            Stmt::Free(expr) => {
                self.resolve_expr(expr)?;
            }
            Stmt::Decl(var_decl) => {
                self.resolve_var_decl(var_decl)?;
            }
        }
        Ok(())
    }

    fn resolve_expr(&mut self, expr: &Expr) -> Result<(), CompileError> {
        match expr {
            Expr::BinaryOp(_, left, right) => {
                self.resolve_expr(left)?;
                self.resolve_expr(right)?;
            }
            Expr::UnaryOp(_, expr) => {
                self.resolve_expr(expr)?;
            }
            Expr::Call(func, args) => {
                self.resolve_expr(func)?;
                for arg in args {
                    self.resolve_expr(arg)?;
                }
            }
            Expr::Index(arr, index) => {
                self.resolve_expr(arr)?;
                self.resolve_expr(index)?;
            }
            Expr::FieldAccess(expr, _) => {
                self.resolve_expr(expr)?;
            }
            Expr::Assign(left, right) => {
                self.resolve_expr(left)?;
                self.resolve_expr(right)?;
            }
            Expr::Move(expr) => {
                self.resolve_expr(expr)?;
            }
            Expr::Clone(expr) => {
                self.resolve_expr(expr)?;
            }
            Expr::Sizeof(_) => {}
            Expr::Ident(name) => {
                if self.symbol_table.lookup(name).is_none() {
                    return Err(CompileError::Semantic(format!(
                        "undefined variable: {}",
                        name
                    )));
                }
            }
            Expr::Literal(_) => {}
            Expr::Block(stmts, expr) => {
                self.symbol_table.push_scope();
                for s in stmts {
                    self.resolve_stmt(s)?;
                }
                if let Some(e) = expr {
                    self.resolve_expr(e)?;
                }
                self.symbol_table.pop_scope();
            }
            Expr::Null => {}
        }
        Ok(())
    }

    pub fn get_symbol_table(&self) -> &SymbolTable {
        &self.symbol_table
    }
}
