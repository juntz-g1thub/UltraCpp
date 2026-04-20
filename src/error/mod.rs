#[derive(Debug)]
pub enum CompileError {
    Lexer(String),
    Parser(String),
    Semantic(String),
    Codegen(String),
}

impl std::fmt::Display for CompileError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            CompileError::Lexer(s) => write!(f, "Lexer error: {}", s),
            CompileError::Parser(s) => write!(f, "Parser error: {}", s),
            CompileError::Semantic(s) => write!(f, "Semantic error: {}", s),
            CompileError::Codegen(s) => write!(f, "Codegen error: {}", s),
        }
    }
}

impl std::error::Error for CompileError {}
