pub mod ast;
pub mod lexer;
pub mod parser;
pub mod token;

pub use lexer::Lexer;
pub use parser::Parser;

use crate::preprocessor::{PreprocessedModule, Preprocessor};
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
