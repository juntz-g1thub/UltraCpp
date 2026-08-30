/* UltraCPP C compiler - token operations */
#include "uc_token.h"

#include <stdlib.h>
#include <string.h>

void uc_token_init(UCToken* tok) {
    if (!tok) return;
    memset(tok, 0, sizeof(*tok));
    tok->kind = UC_TOK_EOF;
}

void uc_token_reset(UCToken* tok) {
    if (!tok) return;
    uc_token_free(tok);
    memset(tok, 0, sizeof(*tok));
    tok->kind = UC_TOK_EOF;
}

void uc_token_free(UCToken* tok) {
    if (!tok) return;
    free(tok->lexeme);
    tok->lexeme = NULL;
    tok->lexeme_len = 0;
    if (tok->kind == UC_TOK_STRING) {
        free(tok->as.string_val);
        tok->as.string_val = NULL;
    }
}

const char* uc_token_kind_name(UCTokenKind kind) {
    switch (kind) {
        case UC_TOK_INT:    return "Int";
        case UC_TOK_FLOAT:  return "Float";
        case UC_TOK_CHAR:   return "Char";
        case UC_TOK_STRING: return "String";
        case UC_TOK_IDENT:  return "Ident";

        case UC_TOK_KW_IF:       return "KwIf";
        case UC_TOK_KW_ELSE:     return "KwElse";
        case UC_TOK_KW_WHILE:    return "KwWhile";
        case UC_TOK_KW_FOR:      return "KwFor";
        case UC_TOK_KW_RETURN:   return "KwReturn";
        case UC_TOK_KW_STRUCT:   return "KwStruct";
        case UC_TOK_KW_EXPORT:   return "KwExport";
        case UC_TOK_KW_IMPORT:   return "KwImport";
        case UC_TOK_KW_CONST:    return "KwConst";
        case UC_TOK_KW_UNIQUE:   return "KwUnique";
        case UC_TOK_KW_MOVE:     return "KwMove";
        case UC_TOK_KW_FREE:     return "KwFree";
        case UC_TOK_KW_ALLOC:    return "KwAlloc";
        case UC_TOK_KW_NULL:     return "KwNull";
        case UC_TOK_KW_TRUE:     return "KwTrue";
        case UC_TOK_KW_FALSE:    return "KwFalse";
        case UC_TOK_KW_VOID:     return "KwVoid";
        case UC_TOK_KW_EXTERN:   return "KwExtern";
        case UC_TOK_KW_UNSAFE:   return "KwUnsafe";
        case UC_TOK_KW_AS:       return "KwAs";
        case UC_TOK_KW_STATIC:   return "KwStatic";
        case UC_TOK_KW_CLONE:    return "KwClone";
        case UC_TOK_KW_ASM:      return "KwAsm";
        case UC_TOK_KW_BREAK:    return "KwBreak";
        case UC_TOK_KW_CONTINUE: return "KwContinue";
        case UC_TOK_KW_IS_NULL:  return "KwIsNull";   /* 0.3.3 §11.0.1 (commit 4) */
        case UC_TOK_KW_SIZEOF:   return "KwSizeof";   /* 0.3.3 §11.0.1 (commit 4) */
        case UC_TOK_KW_ALIGNOF:  return "KwAlignof";  /* 0.3.3 §11.0.1 (commit 4) */
        case UC_TOK_KW_VOLATILE: return "KwVolatile"; /* 0.3.3 §10.3  (commit 4) */
        case UC_TOK_KW_MOD:      return "KwMod";      /* 0.3.3 §4.9 + §11.0.1 (commit 5) — emits @uc_borrow_mod_enter  */
        case UC_TOK_KW_UNMOD:    return "KwUnmod";    /* 0.3.3 §4.10 + §11.0.1 (commit 5) — emits @uc_borrow_mod_exit */

        /* [0.3.5 commit 14e] M1 keywords (lexer 阶段识别；parser/codegen 暂不消费)。 */
        case UC_TOK_KW_SHARED:         return "KwShared";         /* M3-M4 borrow check 前置 */
        case UC_TOK_KW___THREAD:       return "KwThread";         /* __thread  → 避免标识符冲突用三下划线命名 */
        case UC_TOK_KW_MOVE_TO_THREAD: return "KwMoveToThread";   /* cross-thread ownership transfer */
        case UC_TOK_KW_TYPEDEF:        return "KwTypedef";        /* m0_38 deferred → M1 */

        /* [0.3.3 commit 8a] @-prefixed preprocessor directive tokens. */
        case UC_TOK_KW_AT_IFDEF: return "KwAtIfdef";
        case UC_TOK_KW_AT_IF:    return "KwAtIf";
        case UC_TOK_KW_AT_ELSE:  return "KwAtElse";
        case UC_TOK_KW_AT_ELIF:  return "KwAtElif";
        case UC_TOK_KW_AT_END:   return "KwAtEnd";

        case UC_TOK_OP_PLUS:     return "OpPlus";
        case UC_TOK_OP_MINUS:    return "OpMinus";
        case UC_TOK_OP_STAR:     return "OpStar";
        case UC_TOK_OP_SLASH:    return "OpSlash";
        case UC_TOK_OP_PERCENT:  return "OpPercent";
        case UC_TOK_OP_ASSIGN:   return "OpAssign";
        case UC_TOK_OP_EQ:       return "OpEq";
        case UC_TOK_OP_NE:       return "OpNe";
        case UC_TOK_OP_LT:       return "OpLt";
        case UC_TOK_OP_GT:       return "OpGt";
        case UC_TOK_OP_LE:       return "OpLe";
        case UC_TOK_OP_GE:       return "OpGe";
        case UC_TOK_OP_AND:      return "OpAnd";
        case UC_TOK_OP_OR:       return "OpOr";
        case UC_TOK_OP_NOT:      return "OpNot";
        case UC_TOK_OP_BIT_AND:  return "OpBitAnd";
        case UC_TOK_OP_BIT_OR:   return "OpBitOr";
        case UC_TOK_OP_BIT_XOR:  return "OpBitXor";
        case UC_TOK_OP_BIT_NOT:  return "OpBitNot";
        case UC_TOK_OP_SHL:      return "OpShl";
        case UC_TOK_OP_SHR:      return "OpShr";
        case UC_TOK_OP_INC:      return "OpInc";
        case UC_TOK_OP_DEC:      return "OpDec";
        case UC_TOK_OP_ARROW:    return "OpArrow";
        case UC_TOK_OP_SCOPE:    return "OpScope";
        case UC_TOK_OP_QUESTION: return "OpQuestion";

        case UC_TOK_LPAREN:     return "LParen";
        case UC_TOK_RPAREN:     return "RParen";
        case UC_TOK_LBRACE:     return "LBrace";
        case UC_TOK_RBRACE:     return "RBrace";
        case UC_TOK_LBRACKET:   return "LBracket";
        case UC_TOK_RBRACKET:   return "RBracket";
        case UC_TOK_COMMA:      return "Comma";
        case UC_TOK_SEMICOLON:  return "Semicolon";
        case UC_TOK_COLON:      return "Colon";
        case UC_TOK_DOT:        return "Dot";
        case UC_TOK_POUND:      return "Pound";

        case UC_TOK_PP_IMPORT:  return "PpImport";
        case UC_TOK_PP_INCLUDE: return "PpInclude";
        case UC_TOK_PP_DEFINE:  return "PpDefine";
        case UC_TOK_PP_IFDEF:   return "PpIfdef";
        case UC_TOK_PP_IFNDEF:  return "PpIfndef";
        case UC_TOK_PP_ENDIF:   return "PpEndif";

        case UC_TOK_EOF:        return "Eof";
        case UC_TOK_ERROR:      return "Error";
    }
    return "Unknown";
}

UCTokenKind uc_keyword_lookup(const char* ident, size_t len) {
    if (!ident) return UC_TOK_IDENT;

    /* length-then-pointer compare to avoid string compare cost */
    #define KW(s, k) do { \
        static const char _kw[] = s; \
        if (len == (sizeof(_kw) - 1) && memcmp(ident, _kw, sizeof(_kw) - 1) == 0) \
            return k; \
    } while (0)

    KW("if",       UC_TOK_KW_IF);
    KW("else",     UC_TOK_KW_ELSE);
    KW("while",    UC_TOK_KW_WHILE);
    KW("for",      UC_TOK_KW_FOR);
    KW("return",   UC_TOK_KW_RETURN);
    KW("struct",   UC_TOK_KW_STRUCT);
    KW("export",   UC_TOK_KW_EXPORT);
    KW("import",   UC_TOK_KW_IMPORT);
    KW("const",    UC_TOK_KW_CONST);
    KW("unique",   UC_TOK_KW_UNIQUE);
    KW("move",     UC_TOK_KW_MOVE);
    KW("free",     UC_TOK_KW_FREE);
    KW("alloc",    UC_TOK_KW_ALLOC);
    KW("null",     UC_TOK_KW_NULL);
    KW("true",     UC_TOK_KW_TRUE);
    KW("false",    UC_TOK_KW_FALSE);
    KW("void",     UC_TOK_KW_VOID);
    KW("extern",   UC_TOK_KW_EXTERN);
    KW("unsafe",   UC_TOK_KW_UNSAFE);
    KW("as",       UC_TOK_KW_AS);
    KW("static",   UC_TOK_KW_STATIC);
    KW("clone",    UC_TOK_KW_CLONE);
    KW("asm",      UC_TOK_KW_ASM);
    KW("break",    UC_TOK_KW_BREAK);
    KW("continue", UC_TOK_KW_CONTINUE);
    KW("is_null",  UC_TOK_KW_IS_NULL);   /* 0.3.3 §11.0.1 (commit 4) — intrinsic name */
    KW("sizeof",   UC_TOK_KW_SIZEOF);    /* 0.3.3 §11.0.1 (commit 4) — intrinsic name */
    KW("alignof",  UC_TOK_KW_ALIGNOF);   /* 0.3.3 §11.0.1 (commit 4) — intrinsic name */
    KW("volatile", UC_TOK_KW_VOLATILE);  /* 0.3.3 §10.3  (commit 4) — asm {} qualifier; usage wired in commit 8 */
    KW("mod",      UC_TOK_KW_MOD);       /* 0.3.3 §4.9 + §11.0.1 (commit 5) — intrinsic, emits @uc_borrow_mod_enter  */
    KW("unmod",    UC_TOK_KW_UNMOD);     /* 0.3.3 §4.10 + §11.0.1 (commit 5) — intrinsic, emits @uc_borrow_mod_exit */

    /* [0.3.5 commit 14e] M1 borrow-check 前置 keywords (lex/parse 层)。
     * mod/unmod 已在 0.3.3 实施 lexer；本批补 4 个 M1 词法扩展。 */
    KW("shared",         UC_TOK_KW_SHARED);         /* M3-M4 borrow check 前置 — shared ownership qualifier */
    KW("__thread",       UC_TOK_KW___THREAD);       /* thread-local storage (double underscore for C-style TLS) */
    KW("move_to_thread", UC_TOK_KW_MOVE_TO_THREAD); /* cross-thread ownership transfer */
    KW("typedef",        UC_TOK_KW_TYPEDEF);        /* type aliasing — m0_38 deferred → M1 */

    #undef KW
    return UC_TOK_IDENT;
}

/* [0.3.3 commit 8a] Lookup for @-prefixed preprocessor directive names.
 *
 * Used by the lexer after consuming the '@' character and reading the
 * following identifier tail. The lexer reports a contextual error when
 * this function returns UC_TOK_IDENT (i.e. unknown directive), so we
 * never produce UC_TOK_ERROR here — that keeps the error message
 * generation in one place (the lexer).
 *
 * Spec: .dev/drafts/0.3.3-implementation-process.md §10.2 (preprocessor)
 * + runtime-architecture §7 (@ifdef/@end conditional compilation). */
UCTokenKind uc_keyword_at_lookup(const char* ident, size_t len) {
    if (!ident) return UC_TOK_IDENT;

    #define KW_AT(s, k) do { \
        static const char _kw[] = s; \
        if (len == (sizeof(_kw) - 1) && memcmp(ident, _kw, sizeof(_kw) - 1) == 0) \
            return k; \
    } while (0)

    KW_AT("ifdef", UC_TOK_KW_AT_IFDEF);
    KW_AT("if",    UC_TOK_KW_AT_IF);
    KW_AT("else",  UC_TOK_KW_AT_ELSE);
    KW_AT("elif",  UC_TOK_KW_AT_ELIF);
    KW_AT("end",   UC_TOK_KW_AT_END);

    #undef KW_AT
    return UC_TOK_IDENT;
}