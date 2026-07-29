/* UltraCPP Rust compiler -- dump AST
 *
 * Phase 2.6 (byte-level AST alignment with the C port).
 *
 * The output format MUST match src-c/src/ast.c::uc_ast_dump() exactly
 * (after normalising line/column-free parts). tools/ast_test.sh diffs
 * the two outputs and any divergence is a real parser bug in either
 * the Rust or the C implementation.
 *
 * If you change this file, also change uc_ast_dump in src-c/src/ast.c
 * in lock-step, and re-run `bash tools/ast_test.sh` to confirm.
 */

use super::ast::*;
use std::io::{self, Write};

pub fn dump_module(m: &Module, out: &mut dyn Write) -> io::Result<()> {
    writeln!(out, "Module declarations={}", m.declarations.len())?;
    for tl in &m.declarations {
        dump_top_level(tl, out, 1)?;
    }
    Ok(())
}

fn indent(out: &mut dyn Write, n: usize) -> io::Result<()> {
    for _ in 0..n {
        out.write_all(b" ")?;
    }
    Ok(())
}

fn dump_top_level(t: &TopLevel, out: &mut dyn Write, ind: usize) -> io::Result<()> {
    indent(out, ind)?;
    match t {
        TopLevel::FuncDef(f) => {
            writeln!(out, "FuncDef name=\"{}\" params={}", f.name, f.params.len())?;
            for p in &f.params {
                dump_param(p, out, ind + 2)?;
            }
            indent(out, ind + 2)?;
            writeln!(out, "Return:")?;
            dump_type(&f.return_ty, out, ind + 4)?;
            indent(out, ind + 2)?;
            writeln!(out, "Body:")?;
            dump_stmt(&f.body, out, ind + 4)?;
        }
        TopLevel::FuncDecl(f) => {
            writeln!(out, "FuncDecl name=\"{}\" params={}", f.name, f.params.len())?;
            for p in &f.params {
                dump_param(p, out, ind + 2)?;
            }
            indent(out, ind + 2)?;
            writeln!(out, "Return:")?;
            dump_type(&f.return_ty, out, ind + 4)?;
        }
        TopLevel::StructDef(s) => {
            writeln!(out, "StructDef name=\"{}\" fields={}", s.name, s.fields.len())?;
            for f in &s.fields {
                dump_struct_field(f, out, ind + 2)?;
            }
        }
        TopLevel::VarDecl(v) => {
            writeln!(out, "VarDecl name=\"{}\"", v.name)?;
            dump_type(&v.ty, out, ind + 2)?;
            if let Some(init) = &v.init {
                dump_expr(init, out, ind + 2)?;
            }
        }
        TopLevel::ConstDecl(c) => {
            writeln!(out, "ConstDecl name=\"{}\"", c.name)?;
            dump_type(&c.ty, out, ind + 2)?;
            dump_expr(&c.value, out, ind + 2)?;
        }
        TopLevel::Import(i) => {
            writeln!(
                out,
                "Import path=\"{}\" alias=\"{}\"",
                i.path,
                i.alias.as_deref().unwrap_or("")
            )?;
        }
        TopLevel::Export(inner) => {
            writeln!(out, "Export")?;
            dump_top_level(inner, out, ind + 2)?;
        }
        TopLevel::Extern(ty, name, params) => {
            writeln!(out, "Extern name=\"{}\" params={}", name, params.len())?;
            dump_type(ty, out, ind + 2)?;
            for p in params {
                dump_param(p, out, ind + 2)?;
            }
        }
    }
    Ok(())
}

fn dump_param(p: &Param, out: &mut dyn Write, ind: usize) -> io::Result<()> {
    indent(out, ind)?;
    writeln!(out, "Param name=\"{}\"", p.name)?;
    dump_type(&p.ty, out, ind + 2)?;
    Ok(())
}

fn dump_struct_field(f: &StructField, out: &mut dyn Write, ind: usize) -> io::Result<()> {
    indent(out, ind)?;
    writeln!(out, "Field name=\"{}\"", f.name)?;
    dump_type(&f.ty, out, ind + 2)?;
    Ok(())
}

fn dump_type(t: &Type, out: &mut dyn Write, ind: usize) -> io::Result<()> {
    indent(out, ind)?;
    match t {
        Type::Void
        | Type::Bool
        | Type::Char
        | Type::Int
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
        | Type::USize
        | Type::ISize => {
            writeln!(out, "Type {}", type_kind_name(t))?;
        }
        Type::Pointer(inner) | Type::MutablePointer(inner) | Type::Ref(inner) => {
            writeln!(out, "Type {}", type_kind_name(t))?;
            dump_type(inner, out, ind + 2)?;
        }
        Type::Array(elem, len) => {
            writeln!(out, "Type {} length={}", type_kind_name(t), len)?;
            dump_type(elem, out, ind + 2)?;
        }
        Type::Function(ret, params) => {
            writeln!(out, "Type {}", type_kind_name(t))?;
            indent(out, ind + 2)?;
            writeln!(out, "Return:")?;
            dump_type(ret, out, ind + 4)?;
            indent(out, ind + 2)?;
            writeln!(out, "Params ({}):", params.len())?;
            for p in params {
                dump_type(p, out, ind + 4)?;
            }
        }
        Type::Named(name) => {
            writeln!(out, "Type {} name=\"{}\"", type_kind_name(t), name)?;
        }
    }
    Ok(())
}

fn type_kind_name(t: &Type) -> &'static str {
    match t {
        Type::Void => "Void",
        Type::Bool => "Bool",
        Type::Char => "Char",
        Type::Int => "Int",
        Type::I8 => "I8",
        Type::I16 => "I16",
        Type::I32 => "I32",
        Type::I64 => "I64",
        Type::UInt => "UInt",
        Type::U8 => "U8",
        Type::U16 => "U16",
        Type::U32 => "U32",
        Type::U64 => "U64",
        Type::F32 => "F32",
        Type::F64 => "F64",
        Type::USize => "USize",
        Type::ISize => "ISize",
        Type::Pointer(_) => "Pointer",
        Type::MutablePointer(_) => "MutablePointer",
        Type::Ref(_) => "Ref",
        Type::Array(_, _) => "Array",
        Type::Function(_, _) => "Function",
        Type::Named(_) => "Named",
    }
}

fn dump_stmt(s: &Stmt, out: &mut dyn Write, ind: usize) -> io::Result<()> {
    indent(out, ind)?;
    match s {
        Stmt::Block(stmts) => {
            writeln!(out, "Block stmts={}", stmts.len())?;
            for s in stmts {
                dump_stmt(s, out, ind + 2)?;
            }
        }
        Stmt::If(cond, then_b, else_b) => {
            writeln!(out, "If")?;
            indent(out, ind + 2)?;
            writeln!(out, "Cond:")?;
            dump_expr(cond, out, ind + 4)?;
            indent(out, ind + 2)?;
            writeln!(out, "Then:")?;
            dump_stmt(then_b, out, ind + 4)?;
            if let Some(e) = else_b {
                indent(out, ind + 2)?;
                writeln!(out, "Else:")?;
                dump_stmt(e, out, ind + 4)?;
            }
        }
        Stmt::While(cond, body) => {
            writeln!(out, "While")?;
            indent(out, ind + 2)?;
            writeln!(out, "Cond:")?;
            dump_expr(cond, out, ind + 4)?;
            indent(out, ind + 2)?;
            writeln!(out, "Body:")?;
            dump_stmt(body, out, ind + 4)?;
        }
        Stmt::For(init, cond, step, body) => {
            writeln!(out, "For")?;
            indent(out, ind + 2)?;
            writeln!(out, "Init:")?;
            match init {
                Some(i) => dump_stmt(i, out, ind + 4)?,
                None => {
                    indent(out, ind + 4)?;
                    writeln!(out, "(none)")?;
                }
            }
            indent(out, ind + 2)?;
            writeln!(out, "Cond:")?;
            match cond {
                Some(c) => dump_expr(c, out, ind + 4)?,
                None => {
                    indent(out, ind + 4)?;
                    writeln!(out, "(none)")?;
                }
            }
            indent(out, ind + 2)?;
            writeln!(out, "Step:")?;
            match step {
                Some(s) => dump_expr(s, out, ind + 4)?,
                None => {
                    indent(out, ind + 4)?;
                    writeln!(out, "(none)")?;
                }
            }
            indent(out, ind + 2)?;
            writeln!(out, "Body:")?;
            dump_stmt(body, out, ind + 4)?;
        }
        Stmt::Return(v) => {
            writeln!(out, "Return")?;
            if let Some(v) = v {
                dump_expr(v, out, ind + 2)?;
            }
        }
        Stmt::Break => writeln!(out, "Break")?,
        Stmt::Continue => writeln!(out, "Continue")?,
        Stmt::Expr(e) => {
            writeln!(out, "ExprStmt")?;
            if let Some(e) = e {
                dump_expr(e, out, ind + 2)?;
            }
        }
        Stmt::Free(e) => {
            writeln!(out, "Free")?;
            dump_expr(e, out, ind + 2)?;
        }
        Stmt::Decl(v) => {
            writeln!(out, "DeclStmt")?;
            indent(out, ind + 2)?;
            writeln!(out, "VarDecl name=\"{}\"", v.name)?;
            dump_type(&v.ty, out, ind + 4)?;
            if let Some(init) = &v.init {
                dump_expr(init, out, ind + 4)?;
            }
        }
    }
    Ok(())
}

fn dump_expr(e: &Expr, out: &mut dyn Write, ind: usize) -> io::Result<()> {
    indent(out, ind)?;
    match e {
        Expr::BinaryOp(op, lhs, rhs) => {
            writeln!(out, "Binary op={}", binop_name(op))?;
            dump_expr(lhs, out, ind + 2)?;
            dump_expr(rhs, out, ind + 2)?;
        }
        Expr::UnaryOp(op, operand) => {
            writeln!(out, "Unary op={}", unop_name(op))?;
            dump_expr(operand, out, ind + 2)?;
        }
        Expr::Call(callee, args) => {
            writeln!(out, "Call args={}", args.len())?;
            dump_expr(callee, out, ind + 2)?;
            for a in args {
                dump_expr(a, out, ind + 2)?;
            }
        }
        Expr::Index(target, idx) => {
            writeln!(out, "Index")?;
            dump_expr(target, out, ind + 2)?;
            dump_expr(idx, out, ind + 2)?;
        }
        Expr::FieldAccess(target, field) => {
            writeln!(out, "Field field=\"{}\"", field)?;
            dump_expr(target, out, ind + 2)?;
        }
        Expr::Assign(target, value) => {
            writeln!(out, "Assign")?;
            dump_expr(target, out, ind + 2)?;
            dump_expr(value, out, ind + 2)?;
        }
        Expr::Move(inner) => {
            writeln!(out, "Move")?;
            dump_expr(inner, out, ind + 2)?;
        }
        Expr::Clone(inner) => {
            writeln!(out, "Clone")?;
            dump_expr(inner, out, ind + 2)?;
        }
        Expr::Sizeof(ty) => {
            writeln!(out, "Sizeof")?;
            dump_type(ty, out, ind + 2)?;
        }
        Expr::Ident(name) => {
            writeln!(out, "Ident name=\"{}\"", name)?;
        }
        Expr::Literal(lit) => {
            write!(out, "Literal kind={} value=", lit_kind_name(lit))?;
            match lit {
                Literal::Int(n) => writeln!(out, "{}", n)?,
                Literal::Float(f) => writeln!(out, "{}", f)?,
                Literal::Char(c) => writeln!(out, "'{}'", c)?,
                Literal::String(s) => writeln!(out, "\"{}\"", s)?,
                Literal::True => writeln!(out, "true")?,
                Literal::False => writeln!(out, "false")?,
            }
        }
        Expr::Block(stmts, trailing) => {
            writeln!(
                out,
                "ExprBlock stmts={} trailing={}",
                stmts.len(),
                if trailing.is_some() { "true" } else { "false" }
            )?;
            for s in stmts {
                dump_stmt(s, out, ind + 2)?;
            }
            if let Some(t) = trailing {
                dump_expr(t, out, ind + 2)?;
            }
        }
        Expr::Null => writeln!(out, "Null")?,
    }
    Ok(())
}

fn binop_name(op: &BinaryOp) -> &'static str {
    match op {
        BinaryOp::Add => "Add",
        BinaryOp::Sub => "Sub",
        BinaryOp::Mul => "Mul",
        BinaryOp::Div => "Div",
        BinaryOp::Mod => "Mod",
        BinaryOp::Shl => "Shl",
        BinaryOp::Shr => "Shr",
        BinaryOp::Lt => "Lt",
        BinaryOp::Gt => "Gt",
        BinaryOp::Le => "Le",
        BinaryOp::Ge => "Ge",
        BinaryOp::Eq => "Eq",
        BinaryOp::Ne => "Ne",
        BinaryOp::BitAnd => "BitAnd",
        BinaryOp::BitOr => "BitOr",
        BinaryOp::BitXor => "BitXor",
        BinaryOp::And => "And",
        BinaryOp::Or => "Or",
        BinaryOp::Assign => "Assign",
        BinaryOp::AddAssign => "AddAssign",
        BinaryOp::SubAssign => "SubAssign",
        BinaryOp::MulAssign => "MulAssign",
        BinaryOp::DivAssign => "DivAssign",
        BinaryOp::ModAssign => "ModAssign",
        BinaryOp::BitAndAssign => "BitAndAssign",
        BinaryOp::BitOrAssign => "BitOrAssign",
        BinaryOp::BitXorAssign => "BitXorAssign",
        BinaryOp::ShlAssign => "ShlAssign",
        BinaryOp::ShrAssign => "ShrAssign",
    }
}

fn unop_name(op: &UnaryOp) -> &'static str {
    match op {
        UnaryOp::Neg => "Neg",
        UnaryOp::Not => "Not",
        UnaryOp::BitNot => "BitNot",
        UnaryOp::Deref => "Deref",
        UnaryOp::AddrOf => "AddrOf",
    }
}

fn lit_kind_name(lit: &Literal) -> &'static str {
    match lit {
        Literal::Int(_) => "Int",
        Literal::Float(_) => "Float",
        Literal::Char(_) => "Char",
        Literal::String(_) => "String",
        Literal::True => "True",
        Literal::False => "False",
    }
}