/* UltraCPP C compiler - lexer implementation
 *
 * Faithful C99 port of src/frontend/lexer.rs (v0.1.0).
 * Behavior is preserved including known quirks (see KNOWN_ISSUES below).
 *
 * KNOWN_ISSUES (preserved from Rust source):
 *   - Compound-assignment operators (+=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=)
 *     are NOT produced by the lexer; it returns the bare operator instead.
 *     The parser does not consume compound-assign either.
 */
#include "uc_lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* helpers                                                                   */
/* ------------------------------------------------------------------------- */

static char* strndup_safe(const char* s, size_t n) {
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    memcpy(out, s, n);
    out[n] = '\0';
    return out;
}

static int is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

static int is_ident_cont(char c) {
    return is_ident_start(c) ||
           (c >= '0' && c <= '9') ||
           c == '$';
}

/* if next char matches expected, advance and return 1; else 0 */
static int match_char(UCLexer* l, char expected) {
    if (l->pos >= l->source_len) return 0;
    if (l->source[l->pos] != expected) return 0;
    l->pos++;
    l->column++;
    return 1;
}

/* if next char matches expected, advance and return 1; else 0 */
static void advance_one(UCLexer* l) {
    if (l->pos < l->source_len) {
        char c = l->source[l->pos];
        l->pos++;
        if (c == '\n') {
            l->line++;
            l->column = 1;
        } else {
            l->column++;
        }
    }
}

static UCToken make_simple(UCLexer* l, UCTokenKind kind,
                           const char* lexeme_start, size_t lexeme_len) {
    UCToken tok;
    uc_token_init(&tok);
    tok.kind = kind;
    tok.line = l->line;
    tok.column = l->column - (int)lexeme_len;
    if (lexeme_len > 0) {
        tok.lexeme = strndup_safe(lexeme_start, lexeme_len);
        tok.lexeme_len = lexeme_len;
    }
    return tok;
}

static UCToken make_error(UCLexer* l, const char* lexeme_start,
                          size_t lexeme_len, const char* msg) {
    UCToken tok = make_simple(l, UC_TOK_ERROR, lexeme_start, lexeme_len);
    if (l->error) {
        uc_error_set(l->error, UC_ERR_LEXER, l->line, l->column,
                     l->filename, "%s", msg);
    }
    return tok;
}

static UCToken make_eof(UCLexer* l) {
    UCToken tok;
    uc_token_init(&tok);
    tok.kind = UC_TOK_EOF;
    tok.line = l->line;
    tok.column = l->column;
    return tok;
}

/* ------------------------------------------------------------------------- */
/* init / reset                                                               */
/* ------------------------------------------------------------------------- */

void uc_lexer_init(UCLexer* l, const char* source, size_t source_len,
                   const char* filename, UCError* error) {
    if (!l) return;
    l->source = source;
    l->source_len = source_len;
    l->filename = filename;
    l->pos = 0;
    l->line = 1;
    l->column = 1;
    l->error = error;
}

void uc_lexer_reset(UCLexer* l) {
    if (!l) return;
    l->pos = 0;
    l->line = 1;
    l->column = 1;
}

/* ------------------------------------------------------------------------- */
/* whitespace / comment skipping                                              */
/* ------------------------------------------------------------------------- */

static void skip_whitespace_and_comments(UCLexer* l) {
    while (l->pos < l->source_len) {
        char c = l->source[l->pos];

        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            advance_one(l);
            continue;
        }

        /* line comment: // ... \n */
        if (c == '/' && (size_t)(l->pos + 1) < l->source_len &&
            l->source[l->pos + 1] == '/') {
            while (l->pos < l->source_len && l->source[l->pos] != '\n') {
                advance_one(l);
            }
            continue;
        }

        /* block comment: forward-slash-star ... star-forward-slash */
        if (c == '/' && (size_t)(l->pos + 1) < l->source_len &&
            l->source[l->pos + 1] == '*') {
            advance_one(l); /* / */
            advance_one(l); /* * */
            while ((size_t)(l->pos + 1) < l->source_len) {
                if (l->source[l->pos] == '*' &&
                    l->source[l->pos + 1] == '/') {
                    advance_one(l); /* * */
                    advance_one(l); /* / */
                    break;
                }
                advance_one(l);
            }
            continue;
        }

        break;
    }
}

/* ------------------------------------------------------------------------- */
/* preprocessor directive (#import, #include, etc.)                           */
/* ------------------------------------------------------------------------- */

static UCToken scan_preprocessor(UCLexer* l) {
    /* caller already consumed '#'; capture lexeme start including '#' */
    const char* lexeme_start = l->source + l->pos - 1;
    size_t name_start = l->pos;
    size_t name_end = name_start;

    while (l->pos < l->source_len) {
        char c = l->source[l->pos];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            l->pos++;
            l->column++;
            name_end = l->pos;
        } else {
            break;
        }
    }

    size_t name_len = name_end - name_start;
    UCTokenKind kind = UC_TOK_POUND;
    if (name_len == 6 && memcmp(l->source + name_start, "import", 6) == 0) {
        kind = UC_TOK_PP_IMPORT;
    } else if (name_len == 7 && memcmp(l->source + name_start, "include", 7) == 0) {
        kind = UC_TOK_PP_INCLUDE;
    } else if (name_len == 6 && memcmp(l->source + name_start, "define", 6) == 0) {
        kind = UC_TOK_PP_DEFINE;
    } else if (name_len == 5 && memcmp(l->source + name_start, "ifdef", 5) == 0) {
        kind = UC_TOK_PP_IFDEF;
    } else if (name_len == 6 && memcmp(l->source + name_start, "ifndef", 6) == 0) {
        kind = UC_TOK_PP_IFNDEF;
    } else if (name_len == 5 && memcmp(l->source + name_start, "endif", 5) == 0) {
        kind = UC_TOK_PP_ENDIF;
    }

    /* lexeme includes the leading '#' + the name */
    size_t lex_len = 1 + name_len;
    UCToken tok = make_simple(l, kind, lexeme_start, lex_len);
    return tok;
}

/* ------------------------------------------------------------------------- */
/* string literal                                                             */
/* ------------------------------------------------------------------------- */

static UCToken scan_string(UCLexer* l) {
    const char* lexeme_start = l->source + l->pos;  /* include opening " */
    int start_line = l->line;
    int start_col = l->column;

    advance_one(l); /* opening " */

    /* we keep two parallel accumulators: lexeme (raw text, used as token
     * lexeme) and value (decoded, allocated, used as as.string_val). */
    size_t raw_cap = 32, raw_len = 0;
    char* raw = (char*)malloc(raw_cap);
    if (!raw) return make_error(l, lexeme_start, 1, "out of memory");

    size_t val_cap = 32, val_len = 0;
    char* val = (char*)malloc(val_cap);
    if (!val) { free(raw); return make_error(l, lexeme_start, 1, "out of memory"); }

    int closed = 0;
    while (l->pos < l->source_len) {
        char c = l->source[l->pos];

        /* grow raw buffer to include this char (or its escape form) */
        if (raw_len + 2 > raw_cap) {
            raw_cap *= 2;
            char* p = (char*)realloc(raw, raw_cap);
            if (!p) { free(raw); free(val); return make_error(l, lexeme_start, 1, "oom"); }
            raw = p;
        }

        if (c == '"') {
            raw[raw_len++] = '"';
            advance_one(l);
            closed = 1;
            break;
        }

        if (c == '\\') {
            raw[raw_len++] = '\\';
            advance_one(l);
            if (l->pos >= l->source_len) break;
            char esc = l->source[l->pos];
            int consumed = 1;
            char decoded = esc;
            switch (esc) {
                case 'n':  decoded = '\n'; break;
                case 't':  decoded = '\t'; break;
                case 'r':  decoded = '\r'; break;
                case '\\': decoded = '\\'; break;
                case '\'': decoded = '\''; break;
                case '"':  decoded = '"';  break;
                case 'x': {
                    if ((size_t)(l->pos + 2) < l->source_len) {
                        int hi = l->source[l->pos + 1];
                        int lo = l->source[l->pos + 2];
                        int hv = (hi >= '0' && hi <= '9') ? hi - '0'
                              : (hi >= 'a' && hi <= 'f') ? hi - 'a' + 10
                              : (hi >= 'A' && hi <= 'F') ? hi - 'A' + 10
                              : -1;
                        int lv = (lo >= '0' && lo <= '9') ? lo - '0'
                              : (lo >= 'a' && lo <= 'f') ? lo - 'a' + 10
                              : (lo >= 'A' && lo <= 'F') ? lo - 'A' + 10
                              : -1;
                        if (hv >= 0 && lv >= 0) {
                            decoded = (char)((hv << 4) | lv);
                            /* record 3 chars into raw */
                            if (raw_len + 4 > raw_cap) {
                                raw_cap *= 2;
                                char* p = (char*)realloc(raw, raw_cap);
                                if (!p) { free(raw); free(val); return make_error(l, lexeme_start, 1, "oom"); }
                                raw = p;
                            }
                            raw[raw_len++] = esc;
                            raw[raw_len++] = l->source[l->pos + 1];
                            raw[raw_len++] = l->source[l->pos + 2];
                            advance_one(l); /* x */
                            advance_one(l); /* hi */
                            advance_one(l); /* lo */
                            consumed = 0;
                        }
                    }
                    break;
                }
                default:
                    /* keep backslash + char as-is in decoded (matches Rust) */
                    break;
            }
            if (consumed) {
                raw[raw_len++] = esc;
                advance_one(l);
            }

            /* append decoded to val */
            if (val_len + 1 >= val_cap) {
                val_cap *= 2;
                char* p = (char*)realloc(val, val_cap);
                if (!p) { free(raw); free(val); return make_error(l, lexeme_start, 1, "oom"); }
                val = p;
            }
            val[val_len++] = decoded;
        } else {
            raw[raw_len++] = c;
            val[val_len++] = c;
            advance_one(l);
        }
    }

    if (!closed) {
        free(raw);
        free(val);
        return make_error(l, lexeme_start, raw_len, "unterminated string literal");
    }

    /* NUL-terminate val */
    if (val_len >= val_cap) {
        val_cap = val_len + 1;
        char* p = (char*)realloc(val, val_cap);
        if (!p) { free(raw); free(val); return make_error(l, lexeme_start, 1, "oom"); }
        val = p;
    }
    val[val_len] = '\0';

    UCToken tok;
    uc_token_init(&tok);
    tok.kind = UC_TOK_STRING;
    tok.line = start_line;
    tok.column = start_col;
    tok.lexeme = raw;             /* transfer ownership */
    tok.lexeme_len = raw_len;
    tok.as.string_val = val;      /* transfer ownership */
    return tok;
}

/* ------------------------------------------------------------------------- */
/* char literal                                                               */
/* ------------------------------------------------------------------------- */

static UCToken scan_char(UCLexer* l) {
    const char* lexeme_start = l->source + l->pos;
    int start_line = l->line;
    int start_col = l->column;

    advance_one(l); /* opening ' */

    char value = '\0';
    size_t raw_len = 1;  /* at least the opening ' */

    if (l->pos < l->source_len && l->source[l->pos] != '\'') {
        char c = l->source[l->pos];
        if (c == '\\') {
            raw_len++; /* backslash */
            advance_one(l);
            if (l->pos < l->source_len) {
                char esc = l->source[l->pos];
                raw_len++;
                switch (esc) {
                    case 'n':  value = '\n'; break;
                    case 't':  value = '\t'; break;
                    case 'r':  value = '\r'; break;
                    case '\\': value = '\\'; break;
                    case '\'': value = '\''; break;
                    default:   value = esc; break;
                }
                advance_one(l);
            }
        } else {
            value = c;
            raw_len++;
            advance_one(l);
        }
    }

    if (l->pos < l->source_len && l->source[l->pos] == '\'') {
        raw_len++; /* closing ' */
        advance_one(l);
    }

    UCToken tok;
    uc_token_init(&tok);
    tok.kind = UC_TOK_CHAR;
    tok.line = start_line;
    tok.column = start_col;
    tok.lexeme = strndup_safe(lexeme_start, raw_len);
    tok.lexeme_len = raw_len;
    tok.as.char_val = value;
    return tok;
}

/* ------------------------------------------------------------------------- */
/* number literal                                                             */
/* ------------------------------------------------------------------------- */

static UCToken scan_number(UCLexer* l) {
    const char* lexeme_start = l->source + l->pos;
    int start_line = l->line;
    int start_col = l->column;
    int has_dot = 0;
    int has_exp = 0;

    /* hex */
    if ((size_t)(l->pos + 2) <= l->source_len &&
        l->source[l->pos] == '0' &&
        (l->source[l->pos + 1] == 'x' || l->source[l->pos + 1] == 'X')) {
        advance_one(l); /* 0 */
        advance_one(l); /* x */
        while (l->pos < l->source_len) {
            char c = l->source[l->pos];
            if ((c >= '0' && c <= '9') ||
                (c >= 'a' && c <= 'f') ||
                (c >= 'A' && c <= 'F')) {
                advance_one(l);
            } else {
                break;
            }
        }
        size_t len = (size_t)(l->source + l->pos - lexeme_start);
        long long value = 0;
        for (size_t i = 2; i < len; i++) {
            char c = lexeme_start[i];
            int d = (c >= '0' && c <= '9') ? c - '0'
                  : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                  : c - 'A' + 10;
            value = (value << 4) | d;
        }
        UCToken tok;
        uc_token_init(&tok);
        tok.kind = UC_TOK_INT;
        tok.line = start_line;
        tok.column = start_col;
        tok.lexeme = strndup_safe(lexeme_start, len);
        tok.lexeme_len = len;
        tok.as.int_val = value;
        return tok;
    }

    /* binary */
    if ((size_t)(l->pos + 2) <= l->source_len &&
        l->source[l->pos] == '0' &&
        (l->source[l->pos + 1] == 'b' || l->source[l->pos + 1] == 'B')) {
        advance_one(l); /* 0 */
        advance_one(l); /* b */
        while (l->pos < l->source_len) {
            char c = l->source[l->pos];
            if (c == '0' || c == '1') {
                advance_one(l);
            } else {
                break;
            }
        }
        size_t len = (size_t)(l->source + l->pos - lexeme_start);
        long long value = 0;
        for (size_t i = 2; i < len; i++) {
            value = (value << 1) | (lexeme_start[i] - '0');
        }
        UCToken tok;
        uc_token_init(&tok);
        tok.kind = UC_TOK_INT;
        tok.line = start_line;
        tok.column = start_col;
        tok.lexeme = strndup_safe(lexeme_start, len);
        tok.lexeme_len = len;
        tok.as.int_val = value;
        return tok;
    }

    /* decimal or float */
    while (l->pos < l->source_len) {
        char c = l->source[l->pos];
        if (c >= '0' && c <= '9') {
            advance_one(l);
        } else if (c == '.' && !has_dot && !has_exp) {
            has_dot = 1;
            advance_one(l);
        } else if ((c == 'e' || c == 'E') && !has_exp) {
            has_exp = 1;
            advance_one(l);
        } else {
            break;
        }
    }

    size_t len = (size_t)(l->source + l->pos - lexeme_start);
    char* lex = strndup_safe(lexeme_start, len);

    UCToken tok;
    uc_token_init(&tok);
    tok.line = start_line;
    tok.column = start_col;
    tok.lexeme = lex;
    tok.lexeme_len = len;

    if (has_dot || has_exp) {
        tok.kind = UC_TOK_FLOAT;
        tok.as.float_val = lex ? strtod(lex, NULL) : 0.0;
    } else {
        tok.kind = UC_TOK_INT;
        tok.as.int_val = lex ? strtoll(lex, NULL, 10) : 0;
    }
    return tok;
}

/* ------------------------------------------------------------------------- */
/* identifier                                                                 */
/* ------------------------------------------------------------------------- */

static UCToken scan_identifier(UCLexer* l) {
    const char* lexeme_start = l->source + l->pos;
    int start_line = l->line;
    int start_col = l->column;

    while (l->pos < l->source_len) {
        char c = l->source[l->pos];
        if (is_ident_cont(c)) {
            advance_one(l);
        } else {
            break;
        }
    }

    size_t len = (size_t)(l->source + l->pos - lexeme_start);
    UCTokenKind kind = uc_keyword_lookup(lexeme_start, len);

    UCToken tok;
    uc_token_init(&tok);
    tok.kind = kind;
    tok.line = start_line;
    tok.column = start_col;
    tok.lexeme = strndup_safe(lexeme_start, len);
    tok.lexeme_len = len;
    return tok;
}

/* ------------------------------------------------------------------------- */
/* public entry                                                               */
/* ------------------------------------------------------------------------- */

UCToken uc_lexer_next(UCLexer* l) {
    if (!l) {
        UCToken tok;
        uc_token_init(&tok);
        tok.kind = UC_TOK_ERROR;
        return tok;
    }

    skip_whitespace_and_comments(l);

    if (l->pos >= l->source_len) {
        return make_eof(l);
    }

    char c = l->source[l->pos];

    /* '#' starts a preprocessor directive; we capture it (advance over #) then
       scan the name. */
    if (c == '#') {
        advance_one(l); /* # */
        return scan_preprocessor(l);
    }

    if (c == '"') return scan_string(l);
    if (c == '\'') return scan_char(l);

    if (c >= '0' && c <= '9') return scan_number(l);

    if (is_ident_start(c)) return scan_identifier(l);

    /* single & multi-char operators / delimiters. Preserve Rust quirks
       (compound-assign operators fall back to the bare operator kind). */
    const char* lexeme_start = l->source + l->pos;
    int start_line = l->line;
    int start_col = l->column;

    UCTokenKind kind;
    size_t len;

    switch (c) {
        case '+':
            advance_one(l);
            if (match_char(l, '+')) { kind = UC_TOK_OP_INC;   len = 2; }
            else if (match_char(l, '=')) { kind = UC_TOK_OP_PLUS;   len = 2; }  /* KNOWN BUG */
            else                    { kind = UC_TOK_OP_PLUS;   len = 1; }
            break;
        case '-':
            advance_one(l);
            if (match_char(l, '-')) { kind = UC_TOK_OP_DEC;     len = 2; }
            else if (match_char(l, '=')) { kind = UC_TOK_OP_MINUS;  len = 2; }  /* KNOWN BUG */
            else if (match_char(l, '>')) { kind = UC_TOK_OP_ARROW;  len = 2; }
            else                    { kind = UC_TOK_OP_MINUS;  len = 1; }
            break;
        case '*':
            advance_one(l);
            if (match_char(l, '=')) { kind = UC_TOK_OP_STAR;   len = 2; }  /* KNOWN BUG */
            else                    { kind = UC_TOK_OP_STAR;   len = 1; }
            break;
        case '/':
            advance_one(l);
            if (match_char(l, '=')) { kind = UC_TOK_OP_SLASH;  len = 2; }  /* KNOWN BUG */
            else                    { kind = UC_TOK_OP_SLASH;  len = 1; }
            break;
        case '%':
            advance_one(l);
            if (match_char(l, '=')) { kind = UC_TOK_OP_PERCENT; len = 2; }  /* KNOWN BUG */
            else                    { kind = UC_TOK_OP_PERCENT; len = 1; }
            break;
        case '=':
            advance_one(l);
            if (match_char(l, '=')) { kind = UC_TOK_OP_EQ;     len = 2; }
            else                    { kind = UC_TOK_OP_ASSIGN; len = 1; }
            break;
        case '!':
            advance_one(l);
            if (match_char(l, '=')) { kind = UC_TOK_OP_NE;     len = 2; }
            else                    { kind = UC_TOK_OP_NOT;    len = 1; }
            break;
        case '<':
            advance_one(l);
            if (match_char(l, '=')) { kind = UC_TOK_OP_LE;     len = 2; }
            else if (match_char(l, '<')) {
                if (match_char(l, '=')) { kind = UC_TOK_OP_SHL; len = 3; }  /* KNOWN BUG */
                else                    { kind = UC_TOK_OP_SHL; len = 2; }
            } else { kind = UC_TOK_OP_LT; len = 1; }
            break;
        case '>':
            advance_one(l);
            if (match_char(l, '=')) { kind = UC_TOK_OP_GE;     len = 2; }
            else if (match_char(l, '>')) {
                if (match_char(l, '=')) { kind = UC_TOK_OP_SHR; len = 3; }  /* KNOWN BUG */
                else                    { kind = UC_TOK_OP_SHR; len = 2; }
            } else { kind = UC_TOK_OP_GT; len = 1; }
            break;
        case '&':
            advance_one(l);
            if (match_char(l, '&'))      { kind = UC_TOK_OP_AND;     len = 2; }
            else if (match_char(l, '=')) { kind = UC_TOK_OP_BIT_AND; len = 2; }  /* KNOWN BUG */
            else                         { kind = UC_TOK_OP_BIT_AND; len = 1; }
            break;
        case '|':
            advance_one(l);
            if (match_char(l, '|'))      { kind = UC_TOK_OP_OR;      len = 2; }
            else if (match_char(l, '=')) { kind = UC_TOK_OP_BIT_OR;  len = 2; }  /* KNOWN BUG */
            else                         { kind = UC_TOK_OP_BIT_OR;  len = 1; }
            break;
        case '^':
            advance_one(l);
            if (match_char(l, '=')) { kind = UC_TOK_OP_BIT_XOR; len = 2; }  /* KNOWN BUG */
            else                    { kind = UC_TOK_OP_BIT_XOR; len = 1; }
            break;
        case '~':
            advance_one(l);
            kind = UC_TOK_OP_BIT_NOT;
            len = 1;
            break;
        case '(': advance_one(l); kind = UC_TOK_LPAREN;   len = 1; break;
        case ')': advance_one(l); kind = UC_TOK_RPAREN;   len = 1; break;
        case '{': advance_one(l); kind = UC_TOK_LBRACE;   len = 1; break;
        case '}': advance_one(l); kind = UC_TOK_RBRACE;   len = 1; break;
        case '[': advance_one(l); kind = UC_TOK_LBRACKET; len = 1; break;
        case ']': advance_one(l); kind = UC_TOK_RBRACKET; len = 1; break;
        case ',': advance_one(l); kind = UC_TOK_COMMA;    len = 1; break;
        case ';': advance_one(l); kind = UC_TOK_SEMICOLON;len = 1; break;
        case ':':
            advance_one(l);
            if (match_char(l, ':')) { kind = UC_TOK_OP_SCOPE; len = 2; }
            else                    { kind = UC_TOK_COLON;    len = 1; }
            break;
        case '.': advance_one(l); kind = UC_TOK_DOT;      len = 1; break;
        case '?': advance_one(l); kind = UC_TOK_OP_QUESTION; len = 1; break;
        default: {
            char msg[64];
            snprintf(msg, sizeof(msg), "unknown character: '%c'", c);
            UCToken err_tok = make_error(l, lexeme_start, 1, msg);
            advance_one(l);
            return err_tok;
        }
    }

    (void)start_line;
    UCToken tok;
    uc_token_init(&tok);
    tok.kind = kind;
    tok.line = start_line;
    tok.column = start_col;
    tok.lexeme = strndup_safe(lexeme_start, len);
    tok.lexeme_len = len;
    return tok;
}