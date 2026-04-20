use crate::codegen::builtin::get_builtins_decls;
use crate::error::CompileError;
use crate::frontend::ast::{
    BinaryOp, Expr, FuncDef, Literal, Module as AstModule, Stmt, TopLevel, Type, UnaryOp,
};
use std::collections::{HashMap, HashSet};

pub struct CodeGenerator {
    output: String,
    extern_decls: String,
    global_strings: String,
    indent: usize,
    temp_counter: usize,
    label_counter: usize,
    string_counter: usize,
    module_name: String,
    local_types: HashMap<String, String>,
    local_vars: HashMap<String, String>,
    local_funcs: HashMap<String, String>,
    imported_modules: HashMap<String, String>,
    declared_externals: HashSet<String>,
    last_expr_type: String,
}

impl CodeGenerator {
    pub fn new(module_name: &str) -> Self {
        CodeGenerator {
            output: String::new(),
            extern_decls: String::new(),
            global_strings: String::new(),
            indent: 0,
            temp_counter: 0,
            label_counter: 0,
            string_counter: 0,
            module_name: module_name.to_string(),
            local_types: HashMap::new(),
            local_vars: HashMap::new(),
            local_funcs: HashMap::new(),
            imported_modules: HashMap::new(),
            declared_externals: HashSet::new(),
            last_expr_type: "i32".to_string(),
        }
    }

    pub fn add_imported_module(&mut self, module_path: &str) {
        let module_name = module_path
            .trim_end_matches(".uc")
            .trim_end_matches(".upp")
            .trim_end_matches('/')
            .trim_end_matches('\\');
        let name = std::path::Path::new(module_name)
            .file_stem()
            .and_then(|s| s.to_str())
            .unwrap_or(module_name);
        self.imported_modules
            .insert(name.to_string(), name.to_string());
    }

    pub fn generate(&mut self, ast_module: &AstModule) -> Result<String, CompileError> {
        let mut header = get_builtins_decls();

        for decl in &ast_module.declarations {
            if let TopLevel::FuncDef(f) = decl {
                self.local_funcs
                    .insert(f.name.clone(), self.module_name.clone());
            }
        }

        for decl in &ast_module.declarations {
            self.gen_toplevel(decl)?;
        }

        header.push_str(&self.global_strings);
        header.push_str(&self.extern_decls);
        header.push_str(&self.output);

        Ok(header)
    }

    fn mangle_name(&self, name: &str, local: bool) -> String {
        if name.starts_with("sys$") {
            format!("@{}", name)
        } else if name == "main" {
            "@main".to_string()
        } else if local {
            format!("@{}{}{}", self.module_name, "$", name)
        } else {
            format!("@{}", name)
        }
    }

    fn gen_toplevel(&mut self, decl: &TopLevel) -> Result<(), CompileError> {
        match decl {
            TopLevel::FuncDef(f) => self.gen_func(f, false),
            TopLevel::FuncDecl(d) => {
                let mangled = self.mangle_name(&d.name, false);
                let params_str: String = d
                    .params
                    .iter()
                    .map(|p| format!("{} %{}", self.llvm_type(&p.ty), p.name))
                    .collect::<Vec<_>>()
                    .join(", ");
                self.writeln(format!(
                    "declare {} {}({})",
                    self.llvm_type(&d.return_ty),
                    mangled,
                    params_str
                ));
                Ok(())
            }
            TopLevel::Export(inner) => {
                if let TopLevel::FuncDef(f) = inner.as_ref() {
                    self.gen_func(f, true)
                } else {
                    Ok(())
                }
            }
            TopLevel::Extern(ret_ty, name, params) => {
                let mangled = self.mangle_name(name, false);
                let params_str: String = params
                    .iter()
                    .map(|p| format!("{} %{}", self.llvm_type(&p.ty), p.name))
                    .collect::<Vec<_>>()
                    .join(", ");
                self.writeln(format!(
                    "declare {} {}({})",
                    self.llvm_type(ret_ty),
                    mangled,
                    params_str
                ));
                Ok(())
            }
            _ => Ok(()),
        }
    }

    fn gen_func(&mut self, func: &FuncDef, exported: bool) -> Result<(), CompileError> {
        self.local_types.clear();
        self.local_vars.clear();

        for p in &func.params {
            self.local_types
                .insert(p.name.clone(), self.llvm_type(&p.ty));
        }

        let is_local = self.local_funcs.contains_key(&func.name);
        let mangled_name = self.mangle_name(&func.name, is_local);
        let params: String = func
            .params
            .iter()
            .map(|p| format!("{} %{}", self.llvm_type(&p.ty), p.name))
            .collect::<Vec<_>>()
            .join(", ");

        self.writeln(format!(
            "define {} {}({}) {{",
            self.llvm_type(&func.return_ty),
            mangled_name,
            params
        ));
        self.writeln("entry:".to_string());
        self.indent += 1;
        self.gen_stmt(&func.body)?;
        self.indent -= 1;
        self.writeln("}".to_string());
        self.writeln(String::new());
        Ok(())
    }

    fn gen_stmt(&mut self, stmt: &Stmt) -> Result<(), CompileError> {
        match stmt {
            Stmt::Block(stmts) => {
                for s in stmts {
                    self.gen_stmt(s)?;
                }
            }
            Stmt::If(cond, then_s, else_s) => {
                let cond_val = self.gen_expr(cond)?;
                let else_lbl = self.mk_label("else");
                let end_lbl = self.mk_label("if_end");
                self.writeln(format!(
                    "br i1 {}, label %{}, label %{}",
                    cond_val, else_lbl, end_lbl
                ));
                self.writeln(format!("{}:", else_lbl));
                self.indent += 1;
                self.gen_stmt(then_s)?;
                self.indent -= 1;
                if else_s.is_some() {
                    self.writeln(format!("br label %{}", end_lbl));
                    self.writeln(format!("{}:", end_lbl));
                    self.indent += 1;
                    self.gen_stmt(else_s.as_ref().unwrap())?;
                    self.indent -= 1;
                } else {
                    self.writeln(format!("br label %{}", end_lbl));
                    self.writeln(format!("{}:", end_lbl));
                }
            }
            Stmt::While(cond, body) => {
                let c_lbl = self.mk_label("while_cond");
                let b_lbl = self.mk_label("while_body");
                let e_lbl = self.mk_label("while_end");
                self.writeln(format!("br label %{}", c_lbl));
                self.writeln(format!("{}:", c_lbl));
                let cv = self.gen_expr(cond)?;
                self.writeln(format!("br i1 {}, label %{}, label %{}", cv, b_lbl, e_lbl));
                self.writeln(format!("{}:", b_lbl));
                self.indent += 1;
                self.gen_stmt(body)?;
                self.indent -= 1;
                self.writeln(format!("br label %{}", c_lbl));
                self.writeln(format!("{}:", e_lbl));
            }
            Stmt::Return(Some(e)) => {
                let v = self.gen_expr(e)?;
                self.writeln(format!("ret i32 {}", v));
            }
            Stmt::Return(None) => {
                self.writeln("ret void".to_string());
            }
            Stmt::Expr(Some(e)) => {
                let _ = self.gen_expr(e)?;
            }
            Stmt::Free(e) => {
                let p = self.gen_expr(e)?;
                self.writeln(format!("call void @free(i8* {})", p));
            }
            Stmt::Decl(d) => {
                let alloc = format!("%{}", d.name);
                let ll_type = self.llvm_type(&d.ty);
                self.local_vars.insert(d.name.clone(), ll_type.clone());
                self.writeln(format!("{} = alloca {}", alloc, ll_type));
                if let Some(init) = &d.init {
                    let v = self.gen_expr(init)?;
                    let expr_type = &self.last_expr_type;
                    if expr_type != &ll_type {
                        let converted = self.mk_temp();
                        self.writeln(format!("{} = trunc i64 {} to i32", converted, v));
                        self.writeln(format!("store i32 {}, i32* {}", converted, alloc));
                    } else {
                        self.writeln(format!("store {} {}, {}* {}", expr_type, v, ll_type, alloc));
                    }
                }
            }
            _ => {}
        }
        Ok(())
    }

    fn gen_expr(&mut self, expr: &Expr) -> Result<String, CompileError> {
        match expr {
            Expr::BinaryOp(op, l, r) => {
                let lv = self.gen_expr(l)?;
                let rv = self.gen_expr(r)?;
                let res = self.mk_temp();
                let irop = match op {
                    BinaryOp::Add => "add",
                    BinaryOp::Sub => "sub",
                    BinaryOp::Mul => "mul",
                    BinaryOp::Div => "sdiv",
                    BinaryOp::Lt => "icmp slt",
                    BinaryOp::Gt => "icmp sgt",
                    BinaryOp::Le => "icmp sle",
                    BinaryOp::Ge => "icmp sge",
                    BinaryOp::Eq => "icmp eq",
                    BinaryOp::Ne => "icmp ne",
                    BinaryOp::And => "and",
                    BinaryOp::Or => "or",
                    _ => return Err(CompileError::Codegen(format!("unsupported op: {:?}", op))),
                };
                self.writeln(format!("{} = {} i32 {}, {}", res, irop, lv, rv));
                Ok(res)
            }
            Expr::UnaryOp(op, e) => {
                let v = self.gen_expr(e)?;
                let res = self.mk_temp();
                match op {
                    UnaryOp::Neg => {
                        self.writeln(format!("{} = sub i32 0, {}", res, v));
                    }
                    UnaryOp::Not => {
                        self.writeln(format!("{} = xor i32 {}, 1", res, v));
                    }
                    UnaryOp::Deref => {
                        self.writeln(format!("{} = load i32, i32* {}", res, v));
                    }
                    _ => {
                        return Err(CompileError::Codegen(format!(
                            "unsupported unary op: {:?}",
                            op
                        )))
                    }
                }
                Ok(res)
            }
            Expr::Call(f, args) => {
                let symbol = if let Expr::Ident(name) = &**f {
                    let is_local = self.local_funcs.contains_key(name);
                    self.mangle_name(name, is_local)
                } else if let Expr::FieldAccess(module_expr, func_name) = &**f {
                    if let Expr::Ident(module_name) = &**module_expr {
                        let full_symbol = format!("@{}${}", module_name, func_name);
                        if self.imported_modules.contains_key(module_name) {
                            if !self.declared_externals.contains(&full_symbol) {
                                self.declared_externals.insert(full_symbol.clone());
                                self.extern_decls
                                    .push_str(&format!("declare i32 {}()\n", full_symbol));
                            }
                        }
                        full_symbol
                    } else {
                        return Err(CompileError::Codegen(
                            "invalid module expression".to_string(),
                        ));
                    }
                } else {
                    return Err(CompileError::Codegen(
                        "indirect calls not supported".to_string(),
                    ));
                };

                let res = self.mk_temp();

                if symbol.starts_with("@sys$") {
                    self.gen_syscall_call(&symbol, args, &res)?;
                } else {
                    let mut arg_vals = Vec::new();
                    for a in args {
                        let val = self.gen_expr(a)?;
                        let arg_type = &self.last_expr_type;
                        arg_vals.push(format!("{} {}", arg_type, val));
                    }
                    self.writeln(format!(
                        "{} = call i32 {}({})",
                        res,
                        symbol,
                        arg_vals.join(", ")
                    ));
                }
                Ok(res)
            }
            Expr::Ident(n) => {
                if let Some(ll_type) = self.local_vars.get(n).cloned() {
                    let loaded = self.mk_temp();
                    self.writeln(format!(
                        "{} = load {}, {}* %{}",
                        loaded, ll_type, ll_type, n
                    ));
                    self.last_expr_type = ll_type;
                    Ok(loaded)
                } else {
                    Ok(format!("%{}", n))
                }
            }
            Expr::Literal(Literal::Int(n)) => {
                let t = self.mk_temp();
                self.writeln(format!("{} = add i32 0, {}", t, n));
                Ok(t)
            }
            Expr::Literal(Literal::True) => {
                let t = self.mk_temp();
                self.writeln(format!("{} = add i32 0, 1", t));
                Ok(t)
            }
            Expr::Literal(Literal::False) => {
                let t = self.mk_temp();
                self.writeln(format!("{} = add i32 0, 0", t));
                Ok(t)
            }
            Expr::Literal(Literal::Char(c)) => {
                let t = self.mk_temp();
                self.writeln(format!("{} = add i32 0, {}", t, *c as i32));
                Ok(t)
            }
            Expr::Literal(Literal::Float(f)) => {
                let t = self.mk_temp();
                self.writeln(format!("{} = fadd double 0.0, {}", t, f));
                Ok(t)
            }
            Expr::Literal(Literal::String(s)) => {
                let id = self.string_counter;
                self.string_counter += 1;
                let global_name = format!("@.str.{}", id);
                self.global_strings.push_str(&format!(
                    "{} = private constant [{} x i8] c\"{}\\00\"\n",
                    global_name,
                    s.len() + 1,
                    s
                ));
                let t = self.mk_temp();
                self.writeln(format!(
                    "{} = getelementptr [{} x i8], [{} x i8]* {}, i64 0, i64 0",
                    t,
                    s.len() + 1,
                    s.len() + 1,
                    global_name
                ));
                self.last_expr_type = "i8*".to_string();
                Ok(t)
            }
            Expr::Null => Ok("null".to_string()),
            _ => Err(CompileError::Codegen(format!(
                "unsupported expr: {:?}",
                expr
            ))),
        }
    }

    fn llvm_type(&self, ty: &Type) -> String {
        match ty {
            Type::Void => "void".to_string(),
            Type::Bool | Type::Char | Type::Int | Type::I32 | Type::UInt | Type::U32 => {
                "i32".to_string()
            }
            Type::I64 | Type::U64 | Type::ISize | Type::USize => "i64".to_string(),
            Type::F32 => "float".to_string(),
            Type::F64 => "double".to_string(),
            Type::Pointer(_) | Type::Ref(_) | Type::Named(_) => "i8*".to_string(),
            _ => "i32".to_string(),
        }
    }

    fn gen_syscall_call(
        &mut self,
        symbol: &str,
        args: &Vec<Expr>,
        res: &str,
    ) -> Result<(), CompileError> {
        let name = symbol.trim_start_matches('@');
        match name {
            "sys$strlen" => {
                if args.len() != 1 {
                    return Err(CompileError::Codegen("sys$strlen takes 1 arg".to_string()));
                }
                let arg0 = self.gen_expr(&args[0])?;
                self.writeln(format!("{} = call i64 @strlen(i8* {})", res, arg0));
                self.last_expr_type = "i64".to_string();
            }
            "sys$write" => {
                if args.len() != 3 {
                    return Err(CompileError::Codegen("sys$write takes 3 args".to_string()));
                }
                let arg0 = self.gen_expr(&args[0])?;
                let arg1 = self.gen_expr(&args[1])?;
                let arg2 = self.gen_expr(&args[2])?;
                let arg2_final = if self.last_expr_type == "i32" {
                    let ext = self.mk_temp();
                    self.writeln(format!("{} = sext i32 {} to i64", ext, arg2));
                    ext
                } else {
                    arg2
                };
                self.writeln(format!(
                    "{} = call i64 @write(i32 {}, i8* {}, i64 {})",
                    res, arg0, arg1, arg2_final
                ));
                self.last_expr_type = "i64".to_string();
            }
            "sys$read" => {
                if args.len() != 3 {
                    return Err(CompileError::Codegen("sys$read takes 3 args".to_string()));
                }
                let arg0 = self.gen_expr(&args[0])?;
                let arg1 = self.gen_expr(&args[1])?;
                let arg2 = self.gen_expr(&args[2])?;
                self.writeln(format!(
                    "{} = call i64 @read(i32 {}, i8* {}, i64 {})",
                    res, arg0, arg1, arg2
                ));
                self.last_expr_type = "i64".to_string();
            }
            _ => {
                return Err(CompileError::Codegen(format!("unknown syscall: {}", name)));
            }
        }
        Ok(())
    }

    fn mk_temp(&mut self) -> String {
        let t = format!("%t{}", self.temp_counter);
        self.temp_counter += 1;
        t
    }

    fn mk_label(&mut self, prefix: &str) -> String {
        let l = format!("%{}_{}", prefix, self.label_counter);
        self.label_counter += 1;
        l
    }

    fn writeln(&mut self, s: String) {
        for _ in 0..self.indent {
            self.output.push_str("  ");
        }
        self.output.push_str(&s);
        self.output.push('\n');
    }
}

impl Default for CodeGenerator {
    fn default() -> Self {
        Self::new("unnamed")
    }
}
