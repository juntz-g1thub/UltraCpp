pub mod ast;
pub mod dump_ast;
pub mod lexer;
pub mod parser;
pub mod token;

pub use lexer::Lexer;
pub use parser::Parser;

use crate::preprocessor::{PreprocessedModule, Preprocessor};
use crate::frontend::token::{Token, TokenKind};
use std::path::Path;

pub struct CompilationResult {
    pub ast: Box<ast::Module>,
    pub imports: Vec<ImportInfo>,
}

#[derive(Debug, Clone)]
pub struct ImportInfo {
    pub module_path: String,
    pub alias: Option<String>,
}

pub fn compile(
    source: &str,
    file_path: Option<&Path>,
) -> Result<CompilationResult, super::error::CompileError> {
    let mut preprocessor = Preprocessor::new();
    let processed = preprocessor.preprocess(source, file_path);

    let imports: Vec<ImportInfo> = processed
        .imports
        .into_iter()
        .map(|i| ImportInfo {
            module_path: i.module_path,
            alias: i.alias,
        })
        .collect();

    let mut lexer = Lexer::new(&processed.source);
    let mut parser = Parser::new(&mut lexer);
    let ast = parser.parse()?;

    Ok(CompilationResult { ast, imports })
}

/// Run only the lexer and print one line per token. The output format
/// mirrors `uc_lexer` in the C port (src-c/src/main.c) so the two
/// implementations can be diffed directly.
///
/// The preprocessor is intentionally skipped so the C and Rust versions
/// see identical input.
pub fn dump_tokens(source: &str) {
    let mut lexer = Lexer::new(source);
    loop {
        let token: Token = lexer.next_token();
        let extras = match &token.kind {
            TokenKind::Int(n) => format!("  [int={}]", n),
            TokenKind::Float(f) => format!("  [float={}]", f),
            TokenKind::Char(c) => format!("  [char='{}']", c),
            TokenKind::String(s) => format!("  [str=\"{}\"]", s),
            _ => String::new(),
        };
        println!(
            "TOKEN {:12} {:4}:{:<3}  {}{}",
            token.kind.name(),
            token.line,
            token.column,
            token.lexeme,
            extras
        );
        if matches!(token.kind, TokenKind::Eof) {
            break;
        }
    }
}

/// Parse `source` and dump the resulting AST.  Output format mirrors
/// `uc_ast_dump` in src-c/src/ast.c (and is byte-level comparable via
/// `tools/ast_test.sh`).  The preprocessor is intentionally skipped so
/// the C and Rust versions see identical input.
pub fn dump_ast(source: &str) {
    use std::io::{self, Write};
    let mut stdout = io::stdout();
    let processed = crate::preprocessor::Preprocessor::new().preprocess(source, None);
    let mut lexer = Lexer::new(&processed.source);
    let mut parser = Parser::new(&mut lexer);
    match parser.parse() {
        Ok(ast) => {
            let _ = dump_ast::dump_module(&ast, &mut stdout);
        }
        Err(e) => {
            let _ = writeln!(stdout, "Error: {}", e);
            std::process::exit(1);
        }
    }
}
