/* UltraCPP C compiler - token types */
#ifndef UC_TOKEN_H
#define UC_TOKEN_H

#include <stddef.h>

typedef enum {
    /* literals */
    UC_TOK_INT,
    UC_TOK_FLOAT,
    UC_TOK_CHAR,
    UC_TOK_STRING,
    UC_TOK_IDENT,

    /* keywords */
    UC_TOK_KW_IF,
    UC_TOK_KW_ELSE,
    UC_TOK_KW_WHILE,
    UC_TOK_KW_FOR,
    UC_TOK_KW_RETURN,
    UC_TOK_KW_STRUCT,
    UC_TOK_KW_EXPORT,
    UC_TOK_KW_IMPORT,
    UC_TOK_KW_CONST,
    UC_TOK_KW_UNIQUE,
    UC_TOK_KW_MOVE,
    UC_TOK_KW_FREE,
    UC_TOK_KW_ALLOC,
    UC_TOK_KW_NULL,
    UC_TOK_KW_TRUE,
    UC_TOK_KW_FALSE,
    UC_TOK_KW_VOID,
    UC_TOK_KW_EXTERN,
    UC_TOK_KW_UNSAFE,
    UC_TOK_KW_AS,
    UC_TOK_KW_STATIC,
    UC_TOK_KW_CLONE,
    UC_TOK_KW_ASM,
    UC_TOK_KW_BREAK,
    UC_TOK_KW_CONTINUE,
    UC_TOK_KW_IS_NULL,     /* is_null(x) intrinsic — 0.3.3 §11.0.1 (commit 3 codegen handler) */
    UC_TOK_KW_SIZEOF,      /* sizeof(T) intrinsic — 0.3.3 §11.0.1 */
    UC_TOK_KW_ALIGNOF,     /* alignof(T) intrinsic — 0.3.3 §11.0.1 */
    UC_TOK_KW_VOLATILE,    /* 'volatile' qualifier for asm {} block — 0.3.3 §10.3 */
    UC_TOK_KW_MOD,         /* mod(ref) builtin — 0.3.3 §4.9 + §11.0.1; emits @uc_borrow_mod_enter */
    UC_TOK_KW_UNMOD,       /* unmod(ref) builtin — 0.3.3 §4.10 + §11.0.1; emits @uc_borrow_mod_exit */

    /* [0.3.5 commit 14e] M1 borrow-check 前置 keywords (per milestones §3.1).
     * mod/unmod 已在 0.3.3 commit 4-5 实施 lexer；本批补 shared / __thread /
     * move_to_thread / typedef 4 个 M1 词法扩展。Lexer 阶段识别，parser / codegen
     * 暂不消费（M2-M5 实施时按需加；0.3.5 仅 lexer/parse 层闭环）。 */
    UC_TOK_KW_SHARED,           /* shared ownership qualifier — M3-M4 borrow check 前置 */
    UC_TOK_KW___THREAD,         /* __thread thread-local storage — 0.3.0 spec §3.x */
    UC_TOK_KW_MOVE_TO_THREAD,   /* move_to_thread ownership transfer across threads */
    UC_TOK_KW_TYPEDEF,          /* typedef type aliasing — m0_38 deferred → M1 */

    /* [0.3.3 commit 8a] @-prefixed preprocessor directive tokens.
     * Distinct from UC_TOK_KW_IF / UC_TOK_KW_ELSE (which represent the
     * language-level if/else statements used in parse_if_stmt) and from
     * the UC_TOK_PP_* family (which represents '#'-prefixed directives).
     * Spec: .dev/drafts/0.3.3-implementation-process.md §10.2; design:
     * runtime-architecture §7 (per-platform macro injection +
     * @ifdef/@end conditional compilation). */
    UC_TOK_KW_AT_IFDEF,    /* @ifdef(NAME)  — conditional compilation */
    UC_TOK_KW_AT_IF,       /* @if defined(NAME) — alias for @ifdef */
    UC_TOK_KW_AT_ELSE,     /* @else — alternative branch */
    UC_TOK_KW_AT_ELIF,     /* @elif defined(NAME) — chained condition */
    UC_TOK_KW_AT_END,      /* @end — close conditional block */

    /* operators */
    UC_TOK_OP_PLUS,
    UC_TOK_OP_MINUS,
    UC_TOK_OP_STAR,
    UC_TOK_OP_SLASH,
    UC_TOK_OP_PERCENT,
    UC_TOK_OP_ASSIGN,
    UC_TOK_OP_EQ,
    UC_TOK_OP_NE,
    UC_TOK_OP_LT,
    UC_TOK_OP_GT,
    UC_TOK_OP_LE,
    UC_TOK_OP_GE,
    UC_TOK_OP_AND,
    UC_TOK_OP_OR,
    UC_TOK_OP_NOT,
    UC_TOK_OP_BIT_AND,
    UC_TOK_OP_BIT_OR,
    UC_TOK_OP_BIT_XOR,
    UC_TOK_OP_BIT_NOT,
    UC_TOK_OP_SHL,
    UC_TOK_OP_SHR,
    UC_TOK_OP_INC,
    UC_TOK_OP_DEC,
    /* [0.3.5 commit 14f] Compound assignment operators (m0_07 → PASS).
     * Previously these were KNOWN BUG fall-throughs that emitted the
     * bare operator token (UC_TOK_OP_PLUS for `+=` etc.); the lexer
     * consumed both chars but downstream parser/codegen had no way to
     * tell `+` from `+=`. With these 5 dedicated tokens, parse_assignment
     * desugars `a += b` to `a = a + b` and emits the standard load+binop
     * +store pattern via UC_EXPR_ASSIGN. Same scheme for MINUS/MUL/DIV/
     * MOD. Spec: .dev/drafts/0.3.5-implementation-plan.md §3.1 commit 14f. */
    UC_TOK_OP_PLUS_ASSIGN,    /* += */
    UC_TOK_OP_MINUS_ASSIGN,   /* -= */
    UC_TOK_OP_MUL_ASSIGN,     /* *= */
    UC_TOK_OP_DIV_ASSIGN,     /* /= */
    UC_TOK_OP_MOD_ASSIGN,     /* %= */
    UC_TOK_OP_ARROW,
    UC_TOK_OP_SCOPE,
    UC_TOK_OP_QUESTION,

    /* delimiters */
    UC_TOK_LPAREN,
    UC_TOK_RPAREN,
    UC_TOK_LBRACE,
    UC_TOK_RBRACE,
    UC_TOK_LBRACKET,
    UC_TOK_RBRACKET,
    UC_TOK_COMMA,
    UC_TOK_SEMICOLON,
    UC_TOK_COLON,
    UC_TOK_DOT,
    UC_TOK_POUND,

    /* preprocessor */
    UC_TOK_PP_IMPORT,
    UC_TOK_PP_INCLUDE,
    UC_TOK_PP_DEFINE,
    UC_TOK_PP_IFDEF,
    UC_TOK_PP_IFNDEF,
    UC_TOK_PP_ENDIF,

    UC_TOK_EOF,
    UC_TOK_ERROR
} UCTokenKind;

/* Note: as_value union is stored in dynamic memory; freed by uc_token_free. */
typedef struct {
    UCTokenKind kind;
    char* lexeme;        /* owned, NUL-terminated, may be "" */
    size_t lexeme_len;
    int line;
    int column;

    union {
        long long int_val;
        double float_val;
        char char_val;
        char* string_val;  /* owned, NUL-terminated */
    } as;
} UCToken;

void uc_token_init(UCToken* tok);
void uc_token_free(UCToken* tok);
void uc_token_reset(UCToken* tok);
const char* uc_token_kind_name(UCTokenKind kind);
UCTokenKind uc_keyword_lookup(const char* ident, size_t len);
/* [0.3.3 commit 8a] Lookup for @-prefixed preprocessor directive names.
 * The lexer strips the leading '@' before calling this; ident is the
 * tail (e.g. "ifdef", "if", "else", "elif", "end"). Returns one of
 * UC_TOK_KW_AT_IFDEF / _IF / _ELSE / _ELIF / _END on match, or
 * UC_TOK_IDENT (NOT UC_TOK_ERROR) so the lexer can emit a contextual
 * error message including the lexeme. */
UCTokenKind uc_keyword_at_lookup(const char* ident, size_t len);

#endif /* UC_TOKEN_H */