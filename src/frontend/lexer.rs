use super::token::{Token, TokenKind};

pub struct Lexer<'a> {
    source: &'a str,
    chars: Vec<char>,
    pos: usize,
    line: usize,
    column: usize,
    last_token_end: usize,
}

impl<'a> Lexer<'a> {
    pub fn new(source: &'a str) -> Self {
        Lexer {
            source,
            chars: source.chars().collect(),
            pos: 0,
            line: 1,
            column: 1,
            last_token_end: 0,
        }
    }

    pub fn next_token(&mut self) -> Token {
        self.skip_whitespace_and_comments();

        if self.pos >= self.chars.len() {
            return self.make_token(TokenKind::Eof, "".to_string());
        }

        let ch = self.chars[self.pos];

        let start_pos = self.pos;

        let token = match ch {
            '+' => {
                self.pos += 1;
                if self.match_char('+') {
                    TokenKind::OpInc
                } else if self.match_char('=') {
                    TokenKind::OpPlus
                } else {
                    TokenKind::OpPlus
                }
            }
            '-' => {
                self.pos += 1;
                if self.match_char('-') {
                    TokenKind::OpDec
                } else if self.match_char('=') {
                    TokenKind::OpMinus
                } else if self.match_char('>') {
                    self.pos += 1;
                    TokenKind::OpArrow
                } else {
                    TokenKind::OpMinus
                }
            }
            '*' => {
                self.pos += 1;
                if self.match_char('=') {
                    TokenKind::OpStar
                } else {
                    TokenKind::OpStar
                }
            }
            '/' => {
                self.pos += 1;
                if self.match_char('=') {
                    TokenKind::OpSlash
                } else {
                    TokenKind::OpSlash
                }
            }
            '%' => {
                self.pos += 1;
                if self.match_char('=') {
                    TokenKind::OpPercent
                } else {
                    TokenKind::OpPercent
                }
            }
            '=' => {
                self.pos += 1;
                if self.match_char('=') {
                    TokenKind::OpEq
                } else {
                    TokenKind::OpAssign
                }
            }
            '!' => {
                self.pos += 1;
                if self.match_char('=') {
                    TokenKind::OpNe
                } else {
                    TokenKind::OpNot
                }
            }
            '<' => {
                self.pos += 1;
                if self.match_char('=') {
                    TokenKind::OpLe
                } else if self.match_char('<') {
                    self.pos += 1;
                    if self.match_char('=') {
                        TokenKind::OpShl
                    } else {
                        TokenKind::OpShl
                    }
                } else {
                    TokenKind::OpLt
                }
            }
            '>' => {
                self.pos += 1;
                if self.match_char('=') {
                    TokenKind::OpGe
                } else if self.match_char('>') {
                    self.pos += 1;
                    if self.match_char('=') {
                        TokenKind::OpShr
                    } else {
                        TokenKind::OpShr
                    }
                } else {
                    TokenKind::OpGt
                }
            }
            '&' => {
                self.pos += 1;
                if self.match_char('&') {
                    TokenKind::OpAnd
                } else if self.match_char('=') {
                    TokenKind::OpBitAnd
                } else {
                    TokenKind::OpBitAnd
                }
            }
            '|' => {
                self.pos += 1;
                if self.match_char('|') {
                    TokenKind::OpOr
                } else if self.match_char('=') {
                    TokenKind::OpBitOr
                } else {
                    TokenKind::OpBitOr
                }
            }
            '^' => {
                self.pos += 1;
                if self.match_char('=') {
                    TokenKind::OpBitXor
                } else {
                    TokenKind::OpBitXor
                }
            }
            '~' => {
                self.pos += 1;
                TokenKind::OpBitNot
            }
            '(' => {
                self.pos += 1;
                TokenKind::LParen
            }
            ')' => {
                self.pos += 1;
                TokenKind::RParen
            }
            '{' => {
                self.pos += 1;
                TokenKind::LBrace
            }
            '}' => {
                self.pos += 1;
                TokenKind::RBrace
            }
            '[' => {
                self.pos += 1;
                TokenKind::LBracket
            }
            ']' => {
                self.pos += 1;
                TokenKind::RBracket
            }
            ',' => {
                self.pos += 1;
                TokenKind::Comma
            }
            ';' => {
                self.pos += 1;
                TokenKind::Semicolon
            }
            ':' => {
                self.pos += 1;
                TokenKind::Colon
            }
            '.' => {
                self.pos += 1;
                TokenKind::Dot
            }
            '#' => {
                self.pos += 1;
                return self.scan_preprocessor();
            }
            '"' => return self.scan_string(),
            '\'' => return self.scan_char(),
            _ if ch.is_ascii_digit() => return self.scan_number(),
            _ if ch.is_alphabetic() || ch == '_' => return self.scan_identifier(),
            _ => {
                self.pos += 1;
                return self.make_token(TokenKind::Eof, format!("unknown: {}", ch));
            }
        };

        self.make_token(token, self.source[start_pos..self.pos].to_string())
    }

    fn scan_preprocessor(&mut self) -> Token {
        let start = self.pos;
        let mut name = String::new();

        while self.pos < self.chars.len() && self.chars[self.pos].is_alphabetic() {
            name.push(self.chars[self.pos]);
            self.pos += 1;
        }

        let kind = match name.as_str() {
            "import" => TokenKind::PpImport,
            "include" => TokenKind::PpInclude,
            "define" => TokenKind::PpDefine,
            "ifdef" => TokenKind::PpIfdef,
            "ifndef" => TokenKind::PpIfndef,
            "endif" => TokenKind::PpEndif,
            _ => TokenKind::Pound,
        };

        self.make_token(kind, format!("#{}", name))
    }

    fn scan_string(&mut self) -> Token {
        self.pos += 1;
        let mut value = String::new();

        while self.pos < self.chars.len() && self.chars[self.pos] != '"' {
            let ch = self.chars[self.pos];
            if ch == '\\' && self.pos + 1 < self.chars.len() {
                self.pos += 1;
                match self.chars[self.pos] {
                    'n' => value.push('\n'),
                    't' => value.push('\t'),
                    'r' => value.push('\r'),
                    '\\' => value.push('\\'),
                    '\'' => value.push('\''),
                    '"' => value.push('"'),
                    'x' => {
                        if self.pos + 2 < self.chars.len() {
                            let hex = &self.source[self.pos + 1..self.pos + 3];
                            if let Ok(byte) = u8::from_str_radix(hex, 16) {
                                value.push(byte as char);
                                self.pos += 2;
                            }
                        }
                    }
                    _ => {}
                }
            } else {
                value.push(ch);
            }
            self.pos += 1;
        }

        if self.pos < self.chars.len() {
            self.pos += 1;
        }

        self.make_token(TokenKind::String(value.clone()), format!("\"{}\"", value))
    }

    fn scan_char(&mut self) -> Token {
        self.pos += 1;
        let mut value = '\0';

        if self.pos < self.chars.len() && self.chars[self.pos] != '\'' {
            let ch = self.chars[self.pos];
            if ch == '\\' && self.pos + 1 < self.chars.len() {
                self.pos += 1;
                value = match self.chars[self.pos] {
                    'n' => '\n',
                    't' => '\t',
                    'r' => '\r',
                    '\\' => '\\',
                    '\'' => '\'',
                    _ => self.chars[self.pos],
                };
            } else {
                value = ch;
            }
        }

        if self.pos < self.chars.len() && self.chars[self.pos] == '\'' {
            self.pos += 1;
        }

        self.make_token(TokenKind::Char(value), format!("'{}'", value))
    }

    fn scan_number(&mut self) -> Token {
        let start = self.pos;
        let mut has_dot = false;
        let mut has_exponent = false;

        if self.match_str("0x") || self.match_str("0X") {
            while self.pos < self.chars.len() {
                let ch = self.chars[self.pos];
                if (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')
                {
                    self.pos += 1;
                } else {
                    break;
                }
            }
            let lexeme = &self.source[start..self.pos];
            let value = i64::from_str_radix(&lexeme[2..], 16).unwrap_or(0);
            return self.make_token(TokenKind::Int(value), lexeme.to_string());
        }

        if self.match_str("0b") || self.match_str("0B") {
            while self.pos < self.chars.len() {
                let ch = self.chars[self.pos];
                if ch == '0' || ch == '1' {
                    self.pos += 1;
                } else {
                    break;
                }
            }
            let lexeme = &self.source[start..self.pos];
            let value = i64::from_str_radix(&lexeme[2..], 2).unwrap_or(0);
            return self.make_token(TokenKind::Int(value), lexeme.to_string());
        }

        while self.pos < self.chars.len() {
            let ch = self.chars[self.pos];
            if ch.is_ascii_digit() {
                self.pos += 1;
            } else if ch == '.' && !has_dot && !has_exponent {
                has_dot = true;
                self.pos += 1;
            } else if (ch == 'e' || ch == 'E') && !has_exponent {
                has_exponent = true;
                self.pos += 1;
            } else {
                break;
            }
        }

        let lexeme = &self.source[start..self.pos];

        if has_dot || has_exponent {
            let value = lexeme.parse::<f64>().unwrap_or(0.0);
            self.make_token(TokenKind::Float(value), lexeme.to_string())
        } else {
            let value = lexeme.parse::<i64>().unwrap_or(0);
            self.make_token(TokenKind::Int(value), lexeme.to_string())
        }
    }

    fn scan_identifier(&mut self) -> Token {
        let start = self.pos;

        while self.pos < self.chars.len() {
            let ch = self.chars[self.pos];
            if ch.is_alphanumeric() || ch == '_' || ch == '$' {
                self.pos += 1;
            } else {
                break;
            }
        }

        let lexeme = &self.source[start..self.pos];

        if let Some(kind) = TokenKind::from_keyword(lexeme) {
            self.make_token(kind, lexeme.to_string())
        } else {
            self.make_token(TokenKind::Ident(lexeme.to_string()), lexeme.to_string())
        }
    }

    fn skip_whitespace_and_comments(&mut self) {
        while self.pos < self.chars.len() {
            let ch = self.chars[self.pos];
            if ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' {
                if ch == '\n' {
                    self.line += 1;
                    self.column = 1;
                } else {
                    self.column += 1;
                }
                self.pos += 1;
            } else if ch == '/'
                && self.pos + 1 < self.chars.len()
                && self.chars[self.pos + 1] == '/'
            {
                while self.pos < self.chars.len() && self.chars[self.pos] != '\n' {
                    self.pos += 1;
                }
            } else if ch == '/'
                && self.pos + 1 < self.chars.len()
                && self.chars[self.pos + 1] == '*'
            {
                self.pos += 2;
                while self.pos + 1 < self.chars.len() {
                    if self.chars[self.pos] == '*' && self.chars[self.pos + 1] == '/' {
                        self.pos += 2;
                        break;
                    }
                    if self.chars[self.pos] == '\n' {
                        self.line += 1;
                        self.column = 1;
                    }
                    self.pos += 1;
                }
            } else {
                break;
            }
        }
    }

    fn match_char(&mut self, expected: char) -> bool {
        if self.pos < self.chars.len() && self.chars[self.pos] == expected {
            self.pos += 1;
            self.column += 1;
            true
        } else {
            false
        }
    }

    fn match_str(&mut self, expected: &str) -> bool {
        let end = self.pos + expected.len();
        if end <= self.chars.len() {
            let slice: String = self.chars[self.pos..end].iter().collect();
            if slice == expected {
                self.pos = end;
                self.column += expected.len();
                return true;
            }
        }
        false
    }

    fn make_token(&self, kind: TokenKind, lexeme: String) -> Token {
        Token {
            kind,
            lexeme,
            line: self.line,
            column: self.column,
        }
    }
}
