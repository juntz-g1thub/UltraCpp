use super::ast::*;
use super::lexer::Lexer;
use super::token::{Token, TokenKind};
use crate::error::CompileError;

pub struct Parser<'a> {
    lexer: &'a mut Lexer<'a>,
    current_token: Token,
    peek_token: Token,
}

impl<'a> Parser<'a> {
    pub fn new(lexer: &'a mut Lexer<'a>) -> Self {
        let mut p = Parser {
            lexer,
            current_token: Token {
                kind: TokenKind::Eof,
                lexeme: String::new(),
                line: 0,
                column: 0,
            },
            peek_token: Token {
                kind: TokenKind::Eof,
                lexeme: String::new(),
                line: 0,
                column: 0,
            },
        };
        p.advance();
        p.advance();
        p
    }

    fn advance(&mut self) {
        self.current_token = self.peek_token.clone();
        self.peek_token = self.lexer.next_token();
    }

    fn check(&self, kind: &TokenKind) -> bool {
        std::mem::discriminant(&self.current_token.kind) == std::mem::discriminant(kind)
    }

    fn match_token(&mut self, kind: &TokenKind) -> bool {
        if self.check(kind) {
            self.advance();
            true
        } else {
            false
        }
    }

    fn expect(&mut self, kind: &TokenKind, message: &str) -> Result<(), CompileError> {
        if self.check(kind) {
            self.advance();
            Ok(())
        } else {
            Err(CompileError::Parser(format!(
                "{} at line {}, column {}: expected {:?}, found {:?}",
                message,
                self.current_token.line,
                self.current_token.column,
                kind,
                self.current_token.kind
            )))
        }
    }

    pub fn parse(&mut self) -> Result<Box<Module>, CompileError> {
        let mut declarations = Vec::new();

        while !self.check(&TokenKind::Eof) {
            if let Some(decl) = self.parse_top_level()? {
                declarations.push(decl);
            }
        }

        Ok(Box::new(Module { declarations }))
    }

    fn parse_top_level(&mut self) -> Result<Option<TopLevel>, CompileError> {
        if self.match_token(&TokenKind::KwExport) {
            if let Some(inner) = self.parse_top_level()? {
                return Ok(Some(TopLevel::Export(Box::new(inner))));
            }
        }

        if self.match_token(&TokenKind::KwImport) {
            return self.parse_import();
        }

        if self.match_token(&TokenKind::KwStruct) {
            return self.parse_struct_def();
        }

        if self.match_token(&TokenKind::KwExtern) {
            return self.parse_extern();
        }

        if let Some(ty) = self.parse_type()? {
            return self.parse_var_or_func(ty);
        }

        if self.check(&TokenKind::LBrace) {
            return Err(CompileError::Parser("unexpected token".to_string()));
        }

        self.advance();
        Ok(None)
    }

    fn parse_import(&mut self) -> Result<Option<TopLevel>, CompileError> {
        let path = if let TokenKind::String(s) = &self.peek_token.kind {
            let path = s.clone();
            self.advance();
            self.advance();
            path
        } else {
            return Err(CompileError::Parser("expected string literal".to_string()));
        };

        let alias = if self.match_token(&TokenKind::KwAs) {
            if let TokenKind::Ident(s) = &self.current_token.kind {
                let alias = s.clone();
                self.advance();
                Some(alias)
            } else {
                return Err(CompileError::Parser("expected identifier".to_string()));
            }
        } else {
            None
        };

        self.expect(&TokenKind::Semicolon, "expected ';'")?;
        Ok(Some(TopLevel::Import(Import { path, alias })))
    }

    fn parse_struct_def(&mut self) -> Result<Option<TopLevel>, CompileError> {
        let name = if let TokenKind::Ident(s) = &self.current_token.kind {
            s.clone()
        } else {
            return Err(CompileError::Parser("expected identifier".to_string()));
        };
        self.advance();

        self.expect(&TokenKind::LBrace, "expected '{'")?;

        let mut fields = Vec::new();
        while !self.check(&TokenKind::RBrace) {
            let field_name = if let TokenKind::Ident(s) = &self.current_token.kind {
                s.clone()
            } else {
                return Err(CompileError::Parser("expected identifier".to_string()));
            };
            self.advance();

            self.expect(&TokenKind::Colon, "expected ':'")?;
            let field_ty = self
                .parse_type()?
                .ok_or_else(|| CompileError::Parser("expected type".to_string()))?;

            self.expect(&TokenKind::Semicolon, "expected ';'")?;

            fields.push(StructField {
                name: field_name,
                ty: field_ty,
            });
        }

        self.expect(&TokenKind::RBrace, "expected '}'")?;
        self.expect(&TokenKind::Semicolon, "expected ';'")?;

        Ok(Some(TopLevel::StructDef(StructDef { name, fields })))
    }

    fn parse_extern(&mut self) -> Result<Option<TopLevel>, CompileError> {
        let ty = self
            .parse_type()?
            .ok_or_else(|| CompileError::Parser("expected type after extern".to_string()))?;

        let name = if let TokenKind::Ident(s) = &self.current_token.kind {
            s.clone()
        } else {
            return Err(CompileError::Parser(
                "expected identifier after extern type".to_string(),
            ));
        };
        self.advance();

        self.expect(&TokenKind::LParen, "expected '('")?;

        let mut params = Vec::new();
        if !self.check(&TokenKind::RParen) {
            loop {
                let param_ty = self
                    .parse_type()?
                    .ok_or_else(|| CompileError::Parser("expected parameter type".to_string()))?;
                params.push(Param {
                    name: String::new(),
                    ty: param_ty,
                });
                if !self.match_token(&TokenKind::Comma) {
                    break;
                }
            }
        }

        self.expect(&TokenKind::RParen, "expected ')'")?;
        self.expect(&TokenKind::Semicolon, "expected ';'")?;

        Ok(Some(TopLevel::Extern(ty, name, params)))
    }

    fn parse_var_or_func(&mut self, ret_ty: Type) -> Result<Option<TopLevel>, CompileError> {
        let name = if let TokenKind::Ident(s) = &self.current_token.kind {
            s.clone()
        } else {
            return Err(CompileError::Parser("expected identifier".to_string()));
        };
        self.advance();

        if self.check(&TokenKind::LParen) {
            return self.parse_func_def(ret_ty, name);
        }

        if self.match_token(&TokenKind::Semicolon) {
            return Ok(Some(TopLevel::VarDecl(VarDecl {
                name,
                ty: ret_ty,
                init: None,
            })));
        }

        if self.match_token(&TokenKind::OpAssign) {
            let init = self.parse_expression()?;
            self.expect(&TokenKind::Semicolon, "expected ';'")?;
            return Ok(Some(TopLevel::VarDecl(VarDecl {
                name,
                ty: ret_ty,
                init: Some(init),
            })));
        }

        self.expect(&TokenKind::Semicolon, "expected ';'")?;
        Ok(Some(TopLevel::VarDecl(VarDecl {
            name,
            ty: ret_ty,
            init: None,
        })))
    }

    fn parse_func_def(
        &mut self,
        ret_ty: Type,
        name: String,
    ) -> Result<Option<TopLevel>, CompileError> {
        self.expect(&TokenKind::LParen, "expected '('")?;

        let mut params = Vec::new();
        if !self.check(&TokenKind::RParen) {
            loop {
                let mut param_ty = match self.parse_type()? {
                    Some(ty) => ty,
                    None => {
                        return Err(CompileError::Parser("expected type".to_string()));
                    }
                };

                while self.check(&TokenKind::OpStar) {
                    let next_kind = &self.peek_token.kind;
                    let is_type_or_ident = matches!(
                        next_kind,
                        TokenKind::Ident(_) | TokenKind::KwVoid | TokenKind::KwUnique
                    );
                    if !is_type_or_ident {
                        self.advance();
                        break;
                    }
                    self.advance();
                    if let Some(base) = self.parse_type()? {
                        param_ty = Type::Pointer(Box::new(base));
                    } else {
                        param_ty = Type::Pointer(Box::new(param_ty));
                        break;
                    }
                }

                let param_name = if let TokenKind::Ident(s) = &self.current_token.kind {
                    s.clone()
                } else {
                    return Err(CompileError::Parser("expected identifier".to_string()));
                };
                self.advance();

                params.push(Param {
                    name: param_name,
                    ty: param_ty,
                });

                if !self.match_token(&TokenKind::Comma) {
                    break;
                }
            }
        }

        self.expect(&TokenKind::RParen, "expected ')'")?;

        // Check if this is a forward declaration or definition
        if self.check(&TokenKind::Semicolon) {
            // Forward declaration - no body
            self.advance();
            return Ok(Some(TopLevel::FuncDecl(FuncDecl {
                name,
                params,
                return_ty: ret_ty,
            })));
        }

        let body = self.parse_block()?;

        Ok(Some(TopLevel::FuncDef(FuncDef {
            name,
            params,
            return_ty: ret_ty,
            body,
        })))
    }

    fn parse_block(&mut self) -> Result<Box<Stmt>, CompileError> {
        self.expect(&TokenKind::LBrace, "expected '{'")?;

        let mut stmts = Vec::new();
        while !self.check(&TokenKind::RBrace) {
            if self.check(&TokenKind::Eof) {
                return Err(CompileError::Parser("unexpected end of file".to_string()));
            }
            stmts.push(self.parse_statement()?);
        }

        self.expect(&TokenKind::RBrace, "expected '}'")?;
        Ok(Box::new(Stmt::Block(stmts)))
    }

    fn parse_statement(&mut self) -> Result<Stmt, CompileError> {
        match &self.current_token.kind {
            TokenKind::LBrace => Ok(*self.parse_block()?),
            TokenKind::KwIf => self.parse_if(),
            TokenKind::KwWhile => self.parse_while(),
            TokenKind::KwFor => self.parse_for(),
            TokenKind::KwReturn => self.parse_return(),
            TokenKind::KwBreak => {
                self.advance();
                self.expect(&TokenKind::Semicolon, "expected ';'")?;
                Ok(Stmt::Break)
            }
            TokenKind::KwContinue => {
                self.advance();
                self.expect(&TokenKind::Semicolon, "expected ';'")?;
                Ok(Stmt::Continue)
            }
            TokenKind::KwFree => self.parse_free(),
            TokenKind::KwUnsafe => self.parse_unsafe(),
            TokenKind::KwStruct
            | TokenKind::KwExport
            | TokenKind::KwImport
            | TokenKind::KwExtern => Err(CompileError::Parser(
                "unexpected token at statement level".to_string(),
            )),
            _ => {
                // Check if this could be a declaration: type name identifier
                if let Some(ty) = self.parse_type()? {
                    if let TokenKind::Ident(_) = &self.current_token.kind {
                        // This looks like a declaration: type identifier
                        // Save current position in case this is actually an expression
                        let saved_current = self.current_token.clone();
                        let saved_peek = self.peek_token.clone();
                        match self.parse_var_or_func_decl(ty) {
                            Ok(stmt) => return Ok(stmt),
                            Err(_) => {
                                // Not a declaration, restore position and try as expression
                                self.current_token = saved_current;
                                self.peek_token = saved_peek;
                            }
                        }
                    }
                }
                self.parse_expression_statement()
            }
        }
    }

    fn parse_var_or_func_decl(&mut self, ret_ty: Type) -> Result<Stmt, CompileError> {
        let name = if let TokenKind::Ident(s) = &self.current_token.kind {
            s.clone()
        } else {
            return Err(CompileError::Parser("expected identifier".to_string()));
        };
        self.advance();

        // Check if this is a function call (followed by '(') or a variable declaration
        if self.check(&TokenKind::LParen) {
            // This is a function, but we're in a statement context - shouldn't happen
            return Err(CompileError::Parser(
                "unexpected function in statement context".to_string(),
            ));
        }

        // Variable declaration: identifier [= expression] ;
        let init = if self.match_token(&TokenKind::OpAssign) {
            Some(self.parse_expression()?)
        } else {
            None
        };
        self.expect(&TokenKind::Semicolon, "expected ';'")?;
        Ok(Stmt::Decl(VarDecl {
            name,
            ty: ret_ty,
            init,
        }))
    }

    fn parse_if(&mut self) -> Result<Stmt, CompileError> {
        self.advance();
        self.expect(&TokenKind::LParen, "expected '('")?;
        let condition = self.parse_expression()?;
        self.expect(&TokenKind::RParen, "expected ')'")?;
        let then_branch = self.parse_block()?;

        let else_branch = if self.match_token(&TokenKind::KwElse) {
            if self.check(&TokenKind::KwIf) {
                Some(Box::new(self.parse_if()?))
            } else {
                Some(self.parse_block()?)
            }
        } else {
            None
        };

        Ok(Stmt::If(condition, then_branch, else_branch))
    }

    fn parse_while(&mut self) -> Result<Stmt, CompileError> {
        self.advance();
        self.expect(&TokenKind::LParen, "expected '('")?;
        let condition = self.parse_expression()?;
        self.expect(&TokenKind::RParen, "expected ')'")?;
        let body = self.parse_block()?;

        Ok(Stmt::While(condition, body))
    }

    fn parse_for(&mut self) -> Result<Stmt, CompileError> {
        self.advance();
        self.expect(&TokenKind::LParen, "expected '('")?;

        let init = if !self.check(&TokenKind::Semicolon) {
            Some(Box::new(self.parse_for_init()?))
        } else {
            self.advance();
            None
        };

        let condition = if !self.check(&TokenKind::Semicolon) {
            Some(self.parse_expression()?)
        } else {
            None
        };
        self.expect(&TokenKind::Semicolon, "expected ';'")?;

        let update = if !self.check(&TokenKind::RParen) {
            Some(self.parse_expression()?)
        } else {
            None
        };
        self.expect(&TokenKind::RParen, "expected ')'")?;

        let body = self.parse_block()?;

        Ok(Stmt::For(init, condition, update, body))
    }

    fn parse_for_init(&mut self) -> Result<Stmt, CompileError> {
        if let Some(ty) = self.parse_type()? {
            let name = if let TokenKind::Ident(s) = &self.current_token.kind {
                s.clone()
            } else {
                return Err(CompileError::Parser("expected identifier".to_string()));
            };
            self.advance();

            let init = if self.match_token(&TokenKind::OpAssign) {
                Some(self.parse_expression()?)
            } else {
                None
            };

            self.expect(&TokenKind::Semicolon, "expected ';'")?;

            Ok(Stmt::Decl(VarDecl { name, ty, init }))
        } else {
            let expr = self.parse_expression()?;
            self.expect(&TokenKind::Semicolon, "expected ';'")?;
            Ok(Stmt::Expr(Some(expr)))
        }
    }

    fn parse_return(&mut self) -> Result<Stmt, CompileError> {
        self.advance();

        if self.check(&TokenKind::Semicolon) {
            self.advance();
            return Ok(Stmt::Return(None));
        }

        let value = self.parse_expression()?;
        self.expect(&TokenKind::Semicolon, "expected ';'")?;
        Ok(Stmt::Return(Some(value)))
    }

    fn parse_free(&mut self) -> Result<Stmt, CompileError> {
        self.advance();
        self.expect(&TokenKind::LParen, "expected '('")?;
        let expr = self.parse_expression()?;
        self.expect(&TokenKind::RParen, "expected ')'")?;
        self.expect(&TokenKind::Semicolon, "expected ';'")?;

        Ok(Stmt::Free(expr))
    }

    fn parse_unsafe(&mut self) -> Result<Stmt, CompileError> {
        self.advance();
        self.parse_block().map(|b| *b)
    }

    fn parse_expression_statement(&mut self) -> Result<Stmt, CompileError> {
        let expr = self.parse_expression()?;

        if self.match_token(&TokenKind::OpAssign) || self.is_assignment_op() {
            let rhs = self.parse_expression()?;
            self.expect(&TokenKind::Semicolon, "expected ';'")?;
            return Ok(Stmt::Expr(Some(Expr::Assign(
                Box::new(expr),
                Box::new(rhs),
            ))));
        }

        self.expect(&TokenKind::Semicolon, "expected ';'")?;
        Ok(Stmt::Expr(Some(expr)))
    }

    fn is_assignment_op(&self) -> bool {
        matches!(
            &self.current_token.kind,
            TokenKind::OpPlus
                | TokenKind::OpMinus
                | TokenKind::OpStar
                | TokenKind::OpSlash
                | TokenKind::OpPercent
                | TokenKind::OpBitAnd
                | TokenKind::OpBitOr
                | TokenKind::OpBitXor
                | TokenKind::OpShl
                | TokenKind::OpShr
        )
    }

    fn parse_expression(&mut self) -> Result<Expr, CompileError> {
        self.parse_assignment()
    }

    fn parse_assignment(&mut self) -> Result<Expr, CompileError> {
        let expr = self.parse_ternary()?;

        if self.match_token(&TokenKind::OpAssign) {
            let rhs = self.parse_assignment()?;
            return Ok(Expr::Assign(Box::new(expr), Box::new(rhs)));
        }

        Ok(expr)
    }

    fn parse_ternary(&mut self) -> Result<Expr, CompileError> {
        let expr = self.parse_or()?;

        if self.match_token(&TokenKind::OpQuestion) {
            let then_expr = self.parse_ternary()?;
            self.expect(&TokenKind::Colon, "expected ':'")?;
            let else_expr = self.parse_ternary()?;
            Ok(Expr::BinaryOp(
                BinaryOp::Add,
                Box::new(expr),
                Box::new(Expr::BinaryOp(
                    BinaryOp::Add,
                    Box::new(then_expr),
                    Box::new(else_expr),
                )),
            ))
        } else {
            Ok(expr)
        }
    }

    fn parse_or(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_and()?;

        while self.match_token(&TokenKind::OpOr) {
            let rhs = self.parse_and()?;
            expr = Expr::BinaryOp(BinaryOp::Or, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_and(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_bitwise_or()?;

        while self.match_token(&TokenKind::OpAnd) {
            let rhs = self.parse_bitwise_or()?;
            expr = Expr::BinaryOp(BinaryOp::And, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_bitwise_or(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_bitwise_xor()?;

        while self.match_token(&TokenKind::OpBitOr) {
            let rhs = self.parse_bitwise_xor()?;
            expr = Expr::BinaryOp(BinaryOp::BitOr, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_bitwise_xor(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_bitwise_and()?;

        while self.match_token(&TokenKind::OpBitXor) {
            let rhs = self.parse_bitwise_and()?;
            expr = Expr::BinaryOp(BinaryOp::BitXor, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_bitwise_and(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_equality()?;

        while self.match_token(&TokenKind::OpBitAnd) {
            let rhs = self.parse_equality()?;
            expr = Expr::BinaryOp(BinaryOp::BitAnd, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_equality(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_comparison()?;

        while self.match_token(&TokenKind::OpEq) {
            let rhs = self.parse_comparison()?;
            expr = Expr::BinaryOp(BinaryOp::Eq, Box::new(expr), Box::new(rhs));
        }

        while self.match_token(&TokenKind::OpNe) {
            let rhs = self.parse_comparison()?;
            expr = Expr::BinaryOp(BinaryOp::Ne, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_comparison(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_shift()?;

        loop {
            let op = if self.check(&TokenKind::OpLt) {
                BinaryOp::Lt
            } else if self.check(&TokenKind::OpGt) {
                BinaryOp::Gt
            } else if self.check(&TokenKind::OpLe) {
                BinaryOp::Le
            } else if self.check(&TokenKind::OpGe) {
                BinaryOp::Ge
            } else {
                break;
            };
            self.advance();
            let rhs = self.parse_shift()?;
            expr = Expr::BinaryOp(op, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_shift(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_additive()?;

        while self.match_token(&TokenKind::OpShl) {
            let rhs = self.parse_additive()?;
            expr = Expr::BinaryOp(BinaryOp::Shl, Box::new(expr), Box::new(rhs));
        }

        while self.match_token(&TokenKind::OpShr) {
            let rhs = self.parse_additive()?;
            expr = Expr::BinaryOp(BinaryOp::Shr, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_additive(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_multiplicative()?;

        while self.check(&TokenKind::OpPlus) || self.check(&TokenKind::OpMinus) {
            let is_add = self.check(&TokenKind::OpPlus);
            if is_add {
                self.advance();
            } else {
                self.advance();
            }
            let op = if is_add { BinaryOp::Add } else { BinaryOp::Sub };
            let rhs = self.parse_multiplicative()?;
            expr = Expr::BinaryOp(op, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_multiplicative(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_unary()?;

        while self.check(&TokenKind::OpStar)
            || self.check(&TokenKind::OpSlash)
            || self.check(&TokenKind::OpPercent)
        {
            let op = if self.check(&TokenKind::OpStar) {
                BinaryOp::Mul
            } else if self.check(&TokenKind::OpSlash) {
                BinaryOp::Div
            } else {
                BinaryOp::Mod
            };
            self.advance();
            let rhs = self.parse_unary()?;
            expr = Expr::BinaryOp(op, Box::new(expr), Box::new(rhs));
        }

        Ok(expr)
    }

    fn parse_unary(&mut self) -> Result<Expr, CompileError> {
        if self.match_token(&TokenKind::OpPlus) {
            return self.parse_unary();
        }

        if self.match_token(&TokenKind::OpMinus) {
            let expr = self.parse_unary()?;
            return Ok(Expr::UnaryOp(UnaryOp::Neg, Box::new(expr)));
        }

        if self.match_token(&TokenKind::OpNot) {
            let expr = self.parse_unary()?;
            return Ok(Expr::UnaryOp(UnaryOp::Not, Box::new(expr)));
        }

        if self.match_token(&TokenKind::OpBitNot) {
            let expr = self.parse_unary()?;
            return Ok(Expr::UnaryOp(UnaryOp::BitNot, Box::new(expr)));
        }

        if self.match_token(&TokenKind::OpBitAnd) {
            let expr = self.parse_unary()?;
            return Ok(Expr::UnaryOp(UnaryOp::AddrOf, Box::new(expr)));
        }

        if self.match_token(&TokenKind::OpStar) {
            let expr = self.parse_unary()?;
            return Ok(Expr::UnaryOp(UnaryOp::Deref, Box::new(expr)));
        }

        self.parse_postfix()
    }

    fn parse_postfix(&mut self) -> Result<Expr, CompileError> {
        let mut expr = self.parse_primary()?;

        loop {
            if self.match_token(&TokenKind::LBracket) {
                let index = self.parse_expression()?;
                self.expect(&TokenKind::RBracket, "expected ']'")?;
                expr = Expr::Index(Box::new(expr), Box::new(index));
            } else if self.match_token(&TokenKind::LParen) {
                let mut args = Vec::new();
                if !self.check(&TokenKind::RParen) {
                    loop {
                        args.push(self.parse_expression()?);
                        if !self.match_token(&TokenKind::Comma) {
                            break;
                        }
                    }
                }
                self.expect(&TokenKind::RParen, "expected ')'")?;
                expr = Expr::Call(Box::new(expr), args);
            } else if self.match_token(&TokenKind::Dot) {
                if let TokenKind::Ident(name) = &self.current_token.kind {
                    let field = name.clone();
                    self.advance();
                    expr = Expr::FieldAccess(Box::new(expr), field);
                } else {
                    return Err(CompileError::Parser("expected identifier".to_string()));
                }
            } else if self.match_token(&TokenKind::OpArrow) {
                if let TokenKind::Ident(name) = &self.current_token.kind {
                    let field = name.clone();
                    self.advance();
                    expr = Expr::FieldAccess(
                        Box::new(Expr::UnaryOp(UnaryOp::Deref, Box::new(expr))),
                        field,
                    );
                } else {
                    return Err(CompileError::Parser("expected identifier".to_string()));
                }
            } else if self.match_token(&TokenKind::OpInc) {
                expr = Expr::BinaryOp(
                    BinaryOp::AddAssign,
                    Box::new(expr),
                    Box::new(Expr::Literal(Literal::Int(1))),
                );
            } else if self.match_token(&TokenKind::OpDec) {
                expr = Expr::BinaryOp(
                    BinaryOp::SubAssign,
                    Box::new(expr),
                    Box::new(Expr::Literal(Literal::Int(1))),
                );
            } else {
                break;
            }
        }

        Ok(expr)
    }

    fn parse_primary(&mut self) -> Result<Expr, CompileError> {
        match &self.current_token.kind {
            TokenKind::Int(n) => {
                let n = *n;
                self.advance();
                Ok(Expr::Literal(Literal::Int(n)))
            }
            TokenKind::Float(f) => {
                let f = *f;
                self.advance();
                Ok(Expr::Literal(Literal::Float(f)))
            }
            TokenKind::Char(c) => {
                let c = *c;
                self.advance();
                Ok(Expr::Literal(Literal::Char(c)))
            }
            TokenKind::String(s) => {
                let s = s.clone();
                self.advance();
                Ok(Expr::Literal(Literal::String(s)))
            }
            TokenKind::KwTrue => {
                self.advance();
                Ok(Expr::Literal(Literal::True))
            }
            TokenKind::KwFalse => {
                self.advance();
                Ok(Expr::Literal(Literal::False))
            }
            TokenKind::KwNull => {
                self.advance();
                Ok(Expr::Null)
            }
            TokenKind::Ident(name) => {
                let name = name.clone();
                self.advance();
                Ok(Expr::Ident(name))
            }
            TokenKind::LParen => {
                self.advance();
                let expr = self.parse_expression()?;
                self.expect(&TokenKind::RParen, "expected ')'")?;
                Ok(expr)
            }
            TokenKind::LBrace => {
                let block = *self.parse_block()?;
                match block {
                    Stmt::Block(stmts) => Ok(Expr::Block(stmts, None)),
                    _ => Err(CompileError::Parser("expected block statement".to_string())),
                }
            }
            TokenKind::KwMove => {
                self.advance();
                self.expect(&TokenKind::LParen, "expected '('")?;
                let expr = self.parse_expression()?;
                self.expect(&TokenKind::RParen, "expected ')'")?;
                Ok(Expr::Move(Box::new(expr)))
            }
            TokenKind::KwClone => {
                self.advance();
                self.expect(&TokenKind::LParen, "expected '('")?;
                let expr = self.parse_expression()?;
                self.expect(&TokenKind::RParen, "expected ')'")?;
                Ok(Expr::Clone(Box::new(expr)))
            }
            _ => Err(CompileError::Parser(format!(
                "unexpected token: {:?}",
                self.current_token.kind
            ))),
        }
    }

    fn parse_type(&mut self) -> Result<Option<Type>, CompileError> {
        match &self.current_token.kind {
            TokenKind::KwVoid => {
                self.advance();
                Ok(Some(Type::Void))
            }
            TokenKind::KwUnique => {
                self.advance();
                Ok(Some(Type::Pointer(Box::new(Type::Int))))
            }
            TokenKind::OpStar => {
                let next_kind = &self.peek_token.kind;
                let is_type_keyword = matches!(next_kind,
                    TokenKind::Ident(name) if Self::is_type_keyword(name)
                );
                if is_type_keyword {
                    self.advance();
                    match self.parse_type()? {
                        Some(base) => Ok(Some(Type::Pointer(Box::new(base)))),
                        None => Ok(None),
                    }
                } else {
                    Ok(None)
                }
            }
            TokenKind::Ident(name) => {
                let name = name.clone();
                match name.as_str() {
                    "int" => {
                        self.advance();
                        Ok(Some(Type::Int))
                    }
                    "bool" => {
                        self.advance();
                        Ok(Some(Type::Bool))
                    }
                    "char" => {
                        self.advance();
                        Ok(Some(Type::Char))
                    }
                    "i8" => {
                        self.advance();
                        Ok(Some(Type::I8))
                    }
                    "i16" => {
                        self.advance();
                        Ok(Some(Type::I16))
                    }
                    "i32" => {
                        self.advance();
                        Ok(Some(Type::I32))
                    }
                    "i64" => {
                        self.advance();
                        Ok(Some(Type::I64))
                    }
                    "uint" => {
                        self.advance();
                        Ok(Some(Type::UInt))
                    }
                    "u8" => {
                        self.advance();
                        Ok(Some(Type::U8))
                    }
                    "u16" => {
                        self.advance();
                        Ok(Some(Type::U16))
                    }
                    "u32" => {
                        self.advance();
                        Ok(Some(Type::U32))
                    }
                    "u64" => {
                        self.advance();
                        Ok(Some(Type::U64))
                    }
                    "f32" => {
                        self.advance();
                        Ok(Some(Type::F32))
                    }
                    "f64" => {
                        self.advance();
                        Ok(Some(Type::F64))
                    }
                    "usize" => {
                        self.advance();
                        Ok(Some(Type::USize))
                    }
                    "isize" => {
                        self.advance();
                        Ok(Some(Type::ISize))
                    }
                    _ => Ok(None),
                }
            }
            _ => Ok(None),
        }
    }

    fn parse_pointer_base(&mut self) -> Result<Option<Type>, CompileError> {
        self.parse_type()
    }

    fn is_type_keyword(name: &str) -> bool {
        matches!(
            name,
            "int"
                | "bool"
                | "char"
                | "i8"
                | "i16"
                | "i32"
                | "i64"
                | "uint"
                | "u8"
                | "u16"
                | "u32"
                | "u64"
                | "f32"
                | "f64"
                | "usize"
                | "isize"
                | "void"
                | "unique"
        )
    }
}
