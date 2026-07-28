/* UltraCPP C compiler - parser implementation
 *
 * C99 port of src/frontend/parser.rs (Phase 2.2: skeleton + top-level).
 *
 * Error handling model:
 *   - parse_*() helpers return NULL on error (with p->error set) or on
 *     a logical "not present" outcome (with p->error UNSET).
 *   - Callers distinguish the two cases by checking p->error->kind.
 *   - The main loop in uc_parser_parse checks p->error after every
 *     sub-parse and propagates by returning NULL.
 *
 * Memory model:
 *   - All parse_*() helpers return heap-allocated nodes that the caller
 *     takes ownership of.  When a helper aborts partway, it frees the
 *     partial nodes it already produced before returning NULL.
 */
#include "uc_parser.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Item free helpers (mirrors of the static ones in ast.c; kept local so    */
/* parser.c does not depend on ast.c internal symbols).                     */
/* ------------------------------------------------------------------------- */

static void param_free_local(void* p) {
    UCParam* x = (UCParam*)p;
    if (!x) return;
    uc_string_free(&x->name);
    uc_type_free(x->ty);
    free(x);
}

static void struct_field_free_local(void* p) {
    UCStructField* f = (UCStructField*)p;
    if (!f) return;
    uc_string_free(&f->name);
    uc_type_free(f->ty);
    free(f);
}

/* ------------------------------------------------------------------------- */
/* Token helpers                                                             */
/* ------------------------------------------------------------------------- */

static int is_err(const UCParser* p) {
    return p->error && p->error->kind != UC_ERR_NONE;
}

static void err_here(const UCParser* p, const char* fmt, ...) {
    if (!p->error || p->error->kind != UC_ERR_NONE) return;
    va_list ap;
    va_start(ap, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    uc_error_set(p->error, UC_ERR_PARSER,
                 p->current.line, p->current.column, NULL, "%s", buf);
}

/* Advance: shift peek into current, read a new peek. */
static void advance(UCParser* p) {
    uc_token_free(&p->current);
    p->current = p->peek;
    p->peek = uc_lexer_next(p->lexer);
    /* Propagate lexer errors to the parser's error sink so that the
     * main parse loop sees them as a fatal parser error. */
    if (p->peek.kind == UC_TOK_ERROR
        && p->lexer->error
        && p->lexer->error->kind != UC_ERR_NONE
        && p->error
        && p->error->kind == UC_ERR_NONE) {
        *p->error = *p->lexer->error;
    }
}

static int check(const UCParser* p, UCTokenKind k) {
    return p->current.kind == k;
}

static int match(UCParser* p, UCTokenKind k) {
    if (check(p, k)) { advance(p); return 1; }
    return 0;
}

static int expect(UCParser* p, UCTokenKind k, const char* what) {
    if (check(p, k)) { advance(p); return 1; }
    err_here(p, "expected %s, got %s",
             what, uc_token_kind_name(p->current.kind));
    return 0;
}

static const char* cur_lex(const UCParser* p) {
    return p->current.lexeme ? p->current.lexeme : "";
}

/* ------------------------------------------------------------------------- */
/* Type parsing                                                              */
/* ------------------------------------------------------------------------- */

static int is_prim_type_name(const char* s) {
    static const char* prim[] = {
        "int", "bool", "char",
        "i8", "i16", "i32", "i64",
        "uint", "u8", "u16", "u32", "u64",
        "f32", "f64", "usize", "isize",
        NULL
    };
    for (int i = 0; prim[i]; i++) {
        if (strcmp(s, prim[i]) == 0) return 1;
    }
    return 0;
}

static UCType* lookup_prim(const char* s) {
    if (strcmp(s, "int") == 0)   return uc_type_int();
    if (strcmp(s, "bool") == 0)  return uc_type_bool();
    if (strcmp(s, "char") == 0)  return uc_type_char();
    if (strcmp(s, "i8") == 0)    return uc_type_i8();
    if (strcmp(s, "i16") == 0)   return uc_type_i16();
    if (strcmp(s, "i32") == 0)   return uc_type_i32();
    if (strcmp(s, "i64") == 0)   return uc_type_i64();
    if (strcmp(s, "uint") == 0)  return uc_type_uint();
    if (strcmp(s, "u8") == 0)    return uc_type_u8();
    if (strcmp(s, "u16") == 0)   return uc_type_u16();
    if (strcmp(s, "u32") == 0)   return uc_type_u32();
    if (strcmp(s, "u64") == 0)   return uc_type_u64();
    if (strcmp(s, "f32") == 0)   return uc_type_f32();
    if (strcmp(s, "f64") == 0)   return uc_type_f64();
    if (strcmp(s, "usize") == 0) return uc_type_usize();
    if (strcmp(s, "isize") == 0) return uc_type_isize();
    return NULL;
}

/* parse_type returns:
 *   - heap-allocated UCType* on success (caller owns)
 *   - NULL with p->error set on parse error
 *   - NULL with p->error unset when current token is not a type prefix
 *
 * Accepted type spellings (matches src/frontend/parser.rs plus the
 * postfix `*` form used throughout the language spec, e.g. `int* p`):
 *   - `void`
 *   - `unique`                  → Pointer(Int)
 *   - `*T`                      → Pointer(T)             (prefix)
 *   - `int`, `bool`, ...        → primitive
 *   - followed by zero or more `*` for `int*`, `int**`, etc. (postfix)
 */
static UCType* parse_type(UCParser* p) {
    UCType* base = NULL;
    if (check(p, UC_TOK_KW_VOID)) {
        advance(p);
        base = uc_type_void();
    } else if (check(p, UC_TOK_KW_UNIQUE)) {
        advance(p);
        base = uc_type_pointer(uc_type_int());
    } else if (check(p, UC_TOK_OP_STAR)
               && p->peek.kind == UC_TOK_IDENT
               && p->peek.lexeme
               && is_prim_type_name(p->peek.lexeme)) {
        advance(p);
        base = parse_type(p);
        if (!base) return NULL;
        base = uc_type_pointer(base);
    } else if (check(p, UC_TOK_IDENT)
               && p->current.lexeme
               && is_prim_type_name(p->current.lexeme)) {
        base = lookup_prim(p->current.lexeme);
        advance(p);
    } else {
        return NULL;
    }
    /* Postfix `*`: zero or more. */
    while (check(p, UC_TOK_OP_STAR)) {
        advance(p);
        base = uc_type_pointer(base);
    }
    return base;
}

/* ------------------------------------------------------------------------- */
/* Forward decls                                                              */
/* ------------------------------------------------------------------------- */

static UCTopLevel* parse_top_level(UCParser* p);
static UCTopLevel* parse_import(UCParser* p);
static UCTopLevel* parse_struct_def(UCParser* p);
static UCTopLevel* parse_extern_decl(UCParser* p);
static UCTopLevel* parse_var_or_func(UCParser* p, UCType* ret_ty);
static UCTopLevel* parse_func_def(UCParser* p, UCType* ret_ty,
                                  UCString name);
static UCStmt* parse_block(UCParser* p);
static UCStmt* parse_return_stmt(UCParser* p);
static UCExpr* parse_expr_minimal(UCParser* p);
static UCVec* parse_func_params(UCParser* p);
static UCParam* parse_one_param(UCParser* p);

/* ------------------------------------------------------------------------- */
/* Top-level dispatch                                                        */
/* ------------------------------------------------------------------------- */

static UCTopLevel* parse_top_level(UCParser* p) {
    if (match(p, UC_TOK_KW_EXPORT)) {
        UCTopLevel* inner = parse_top_level(p);
        if (is_err(p)) { uc_top_level_free(inner); return NULL; }
        if (!inner) {
            err_here(p, "'export' must be followed by a declaration");
            return NULL;
        }
        return uc_tl_export(inner);
    }

    /* Both '#import' (UC_TOK_PP_IMPORT) and bare 'import' (UC_TOK_KW_IMPORT)
     * enter parse_import; the Rust preprocessor normally strips the former
     * before parsing, but the C parser accepts it directly. */
    if (match(p, UC_TOK_PP_IMPORT) || match(p, UC_TOK_KW_IMPORT)) {
        return parse_import(p);
    }

    if (match(p, UC_TOK_KW_STRUCT)) {
        return parse_struct_def(p);
    }

    if (match(p, UC_TOK_KW_EXTERN)) {
        return parse_extern_decl(p);
    }

    if (check(p, UC_TOK_KW_VOID)
        || check(p, UC_TOK_KW_UNIQUE)
        || check(p, UC_TOK_OP_STAR)
        || (check(p, UC_TOK_IDENT) && p->current.lexeme
            && is_prim_type_name(p->current.lexeme))) {
        UCType* ty = parse_type(p);
        if (is_err(p)) { uc_type_free(ty); return NULL; }
        if (!ty) {
            err_here(p, "expected type at top level");
            return NULL;
        }
        return parse_var_or_func(p, ty);
    }

    if (check(p, UC_TOK_LBRACE)) {
        err_here(p, "unexpected '{' at top level");
        return NULL;
    }
    if (check(p, UC_TOK_EOF)) return NULL;
    err_here(p, "unexpected token at top level: %s '%s'",
             uc_token_kind_name(p->current.kind), cur_lex(p));
    return NULL;
}

/* ------------------------------------------------------------------------- */
/* #import "path" [as alias];                                                */
/* ------------------------------------------------------------------------- */

static UCTopLevel* parse_import(UCParser* p) {
    if (!check(p, UC_TOK_STRING)) {
        err_here(p, "expected string literal after 'import'");
        return NULL;
    }
    /* The lexer stores the decoded (unescaped, quote-stripped) string in
     * as.string_val; use that instead of the raw lexeme which still has
     * the surrounding double-quotes. */
    UCString path;
    if (p->current.as.string_val) {
        path = uc_string_new(p->current.as.string_val,
                             strlen(p->current.as.string_val));
    } else {
        path = uc_string_new(p->current.lexeme, p->current.lexeme_len);
    }
    advance(p);

    UCString alias = uc_string_empty();
    if (match(p, UC_TOK_KW_AS)) {
        if (!check(p, UC_TOK_IDENT)) {
            err_here(p, "expected identifier after 'as'");
            uc_string_free(&path);
            uc_string_free(&alias);
            return NULL;
        }
        alias = uc_string_new(p->current.lexeme, p->current.lexeme_len);
        advance(p);
    }

    if (!expect(p, UC_TOK_SEMICOLON, "';' after import")) {
        uc_string_free(&path);
        uc_string_free(&alias);
        return NULL;
    }

    return uc_tl_import(uc_import_new(path, alias));
}

/* ------------------------------------------------------------------------- */
/* struct Name { T1 f1; T2 f2; ... };                                        */
/* ------------------------------------------------------------------------- */

static UCTopLevel* parse_struct_def(UCParser* p) {
    if (!check(p, UC_TOK_IDENT)) {
        err_here(p, "expected identifier after 'struct'");
        return NULL;
    }
    UCString name = uc_string_new(p->current.lexeme, p->current.lexeme_len);
    advance(p);

    if (!expect(p, UC_TOK_LBRACE, "'{' after struct name")) {
        uc_string_free(&name);
        return NULL;
    }

    UCVec* fields = uc_vec_new();
    while (!check(p, UC_TOK_RBRACE)) {
        if (check(p, UC_TOK_EOF)) {
            err_here(p, "unexpected end of file in struct body");
            uc_string_free(&name);
            uc_vec_free(fields, NULL);
            return NULL;
        }
        if (!check(p, UC_TOK_IDENT)) {
            err_here(p, "expected field name in struct");
            uc_string_free(&name);
            uc_vec_free(fields, NULL);
            return NULL;
        }
        UCString fname = uc_string_new(p->current.lexeme,
                                       p->current.lexeme_len);
        advance(p);

        if (!expect(p, UC_TOK_COLON, "':' after field name")) {
            uc_string_free(&fname);
            uc_string_free(&name);
            uc_vec_free(fields, NULL);
            return NULL;
        }

        UCType* fty = parse_type(p);
        if (is_err(p)) {
            uc_string_free(&fname);
            uc_string_free(&name);
            uc_type_free(fty);
            uc_vec_free(fields, NULL);
            return NULL;
        }
        if (!fty) {
            err_here(p, "expected field type");
            uc_string_free(&fname);
            uc_string_free(&name);
            uc_vec_free(fields, NULL);
            return NULL;
        }

        if (!expect(p, UC_TOK_SEMICOLON, "';' after field")) {
            uc_string_free(&fname);
            uc_type_free(fty);
            uc_string_free(&name);
            uc_vec_free(fields, NULL);
            return NULL;
        }

        uc_vec_push(fields, uc_struct_field_new(fname, fty));
    }

    if (!expect(p, UC_TOK_RBRACE, "'}' to close struct")) {
        uc_string_free(&name);
        uc_vec_free(fields, struct_field_free_local);
        return NULL;
    }
    if (!expect(p, UC_TOK_SEMICOLON, "';' after struct")) {
        uc_string_free(&name);
        uc_vec_free(fields, struct_field_free_local);
        return NULL;
    }

    return uc_tl_struct_def(uc_struct_def_new(name, fields));
}

/* ------------------------------------------------------------------------- */
/* extern Type name(p1, p2, ...);                                           */
/* ------------------------------------------------------------------------- */

static UCTopLevel* parse_extern_decl(UCParser* p) {
    UCType* ty = parse_type(p);
    if (is_err(p)) { uc_type_free(ty); return NULL; }
    if (!ty) {
        err_here(p, "expected type after 'extern'");
        return NULL;
    }

    if (!check(p, UC_TOK_IDENT)) {
        err_here(p, "expected identifier after extern type");
        uc_type_free(ty);
        return NULL;
    }
    UCString name = uc_string_new(p->current.lexeme, p->current.lexeme_len);
    advance(p);

    if (!expect(p, UC_TOK_LPAREN, "'(' after extern name")) {
        uc_type_free(ty);
        uc_string_free(&name);
        return NULL;
    }

    UCVec* params = uc_vec_new();
    if (!check(p, UC_TOK_RPAREN)) {
        for (;;) {
            UCType* pty = parse_type(p);
            if (is_err(p)) {
                uc_type_free(pty);
                uc_type_free(ty);
                uc_string_free(&name);
                uc_vec_free(params, param_free_local);
                return NULL;
            }
            if (!pty) {
                err_here(p, "expected parameter type");
                uc_type_free(ty);
                uc_string_free(&name);
                uc_vec_free(params, param_free_local);
                return NULL;
            }
            uc_vec_push(params, uc_param_new(uc_string_empty(), pty));
            if (!match(p, UC_TOK_COMMA)) break;
        }
    }

    if (!expect(p, UC_TOK_RPAREN, "')' to close extern params")) {
        uc_type_free(ty);
        uc_string_free(&name);
        uc_vec_free(params, param_free_local);
        return NULL;
    }
    if (!expect(p, UC_TOK_SEMICOLON, "';' after extern")) {
        uc_type_free(ty);
        uc_string_free(&name);
        uc_vec_free(params, param_free_local);
        return NULL;
    }

    return uc_tl_extern(ty, name, params);
}

/* ------------------------------------------------------------------------- */
/* Parameter parsing (shared between func def and decl)                      */
/* ------------------------------------------------------------------------- */

static UCParam* parse_one_param(UCParser* p) {
    UCType* ty = parse_type(p);
    if (is_err(p)) { uc_type_free(ty); return NULL; }
    if (!ty) {
        err_here(p, "expected parameter type");
        return NULL;
    }

    if (!check(p, UC_TOK_IDENT)) {
        err_here(p, "expected parameter name");
        uc_type_free(ty);
        return NULL;
    }
    UCString pname = uc_string_new(p->current.lexeme, p->current.lexeme_len);
    advance(p);
    return uc_param_new(pname, ty);
}

static UCVec* parse_func_params(UCParser* p) {
    UCVec* params = uc_vec_new();
    if (check(p, UC_TOK_RPAREN)) return params;
    for (;;) {
        UCParam* prm = parse_one_param(p);
        if (is_err(p)) {
            uc_vec_free(params, param_free_local);
            return NULL;
        }
        if (!prm) {
            uc_vec_free(params, param_free_local);
            return NULL;
        }
        uc_vec_push(params, prm);
        if (!match(p, UC_TOK_COMMA)) break;
    }
    return params;
}

/* ------------------------------------------------------------------------- */
/* Type-name-driven: var decl, func def, func decl                            */
/* ------------------------------------------------------------------------- */

static UCTopLevel* parse_var_or_func(UCParser* p, UCType* ret_ty) {
    if (!check(p, UC_TOK_IDENT)) {
        err_here(p, "expected identifier after type");
        uc_type_free(ret_ty);
        return NULL;
    }
    UCString name = uc_string_new(p->current.lexeme, p->current.lexeme_len);
    advance(p);

    if (check(p, UC_TOK_LPAREN)) {
        return parse_func_def(p, ret_ty, name);
    }

    UCExpr* init = NULL;
    if (match(p, UC_TOK_OP_ASSIGN)) {
        init = parse_expr_minimal(p);
        if (is_err(p)) {
            uc_string_free(&name);
            uc_type_free(ret_ty);
            uc_expr_free(init);
            return NULL;
        }
        if (!init) {
            err_here(p, "expected initializer expression");
            uc_string_free(&name);
            uc_type_free(ret_ty);
            return NULL;
        }
    }

    if (!expect(p, UC_TOK_SEMICOLON, "';' after variable declaration")) {
        uc_string_free(&name);
        uc_type_free(ret_ty);
        uc_expr_free(init);
        return NULL;
    }

    return uc_tl_var_decl(uc_var_decl_new(name, ret_ty, init));
}

static UCTopLevel* parse_func_def(UCParser* p, UCType* ret_ty, UCString name) {
    if (!expect(p, UC_TOK_LPAREN, "'(' after function name")) {
        uc_string_free(&name);
        uc_type_free(ret_ty);
        return NULL;
    }

    UCVec* params = parse_func_params(p);
    if (is_err(p)) {
        uc_string_free(&name);
        uc_type_free(ret_ty);
        return NULL;
    }

    if (!expect(p, UC_TOK_RPAREN, "')' to close function parameters")) {
        uc_string_free(&name);
        uc_type_free(ret_ty);
        uc_vec_free(params, param_free_local);
        return NULL;
    }

    /* Forward declaration: ends with ';' */
    if (match(p, UC_TOK_SEMICOLON)) {
        return uc_tl_func_decl(uc_func_decl_new(name, params, ret_ty));
    }

    UCStmt* body = parse_block(p);
    if (is_err(p)) {
        uc_string_free(&name);
        uc_type_free(ret_ty);
        uc_vec_free(params, param_free_local);
        uc_stmt_free(body);
        return NULL;
    }
    if (!body) {
        err_here(p, "expected function body");
        uc_string_free(&name);
        uc_type_free(ret_ty);
        uc_vec_free(params, param_free_local);
        return NULL;
    }

    return uc_tl_func_def(uc_func_def_new(name, params, ret_ty, body));
}

/* ------------------------------------------------------------------------- */
/* Minimal statements: '{ ... }' (empty or single return), 'return [e];'    */
/* ------------------------------------------------------------------------- */

static UCStmt* parse_block(UCParser* p) {
    if (!expect(p, UC_TOK_LBRACE, "'{' to start block")) return NULL;
    UCVec* stmts = uc_vec_new();
    while (!check(p, UC_TOK_RBRACE)) {
        if (check(p, UC_TOK_EOF)) {
            err_here(p, "unexpected end of file in block");
            uc_vec_free(stmts, NULL);
            return NULL;
        }
        UCStmt* s = NULL;
        if (check(p, UC_TOK_KW_RETURN)) {
            s = parse_return_stmt(p);
        } else {
            err_here(p,
                     "only 'return ...;' supported in body (Phase 2.2); "
                     "got %s '%s'",
                     uc_token_kind_name(p->current.kind), cur_lex(p));
            uc_vec_free(stmts, NULL);
            return NULL;
        }
        if (is_err(p)) {
            uc_stmt_free(s);
            uc_vec_free(stmts, NULL);
            return NULL;
        }
        if (s) uc_vec_push(stmts, s);
    }
    if (!expect(p, UC_TOK_RBRACE, "'}' to close block")) {
        uc_vec_free(stmts, NULL);
        return NULL;
    }
    return uc_stmt_block(stmts);
}

static UCStmt* parse_return_stmt(UCParser* p) {
    advance(p);
    UCExpr* val = NULL;
    if (!check(p, UC_TOK_SEMICOLON)) {
        val = parse_expr_minimal(p);
        if (is_err(p)) { uc_expr_free(val); return NULL; }
        if (!val) {
            err_here(p, "expected expression after 'return'");
            return NULL;
        }
    }
    if (!expect(p, UC_TOK_SEMICOLON, "';' after return")) {
        uc_expr_free(val);
        return NULL;
    }
    return uc_stmt_return(val);
}

/* ------------------------------------------------------------------------- */
/* Minimal expressions: literals + identifier + null                        */
/* ------------------------------------------------------------------------- */

/* Build a literal-expr UCExpr from a fully-decoded literal value.
 * The literal is moved into the expression; the caller must not free it. */
static UCExpr* expr_literal(UCLiteral lit) {
    UCExpr* e = (UCExpr*)calloc(1, sizeof(UCExpr));
    if (!e) { fprintf(stderr, "uc_parser: out of memory\n"); abort(); }
    e->kind = UC_EXPR_LITERAL;
    e->as.literal = lit;
    return e;
}

/* Build an identifier-expr UCExpr from a copied name. The name is moved
 * into the expression; the caller must not free it. */
static UCExpr* expr_ident(UCString name) {
    UCExpr* e = (UCExpr*)calloc(1, sizeof(UCExpr));
    if (!e) { fprintf(stderr, "uc_parser: out of memory\n"); abort(); }
    e->kind = UC_EXPR_IDENT;
    e->as.ident = name;
    return e;
}

static UCExpr* parse_expr_minimal(UCParser* p) {
    UCTokenKind k = p->current.kind;
    switch (k) {
        case UC_TOK_INT: {
            long long v = p->current.as.int_val;
            advance(p);
            return expr_literal(uc_literal_int(v));
        }
        case UC_TOK_FLOAT: {
            double v = p->current.as.float_val;
            advance(p);
            return expr_literal(uc_literal_float(v));
        }
        case UC_TOK_CHAR: {
            char v = p->current.as.char_val;
            advance(p);
            return expr_literal(uc_literal_char(v));
        }
        case UC_TOK_STRING: {
            /* The lexer stores the decoded (unescaped, quote-stripped) value
             * in as.string_val; use it instead of the raw lexeme which
             * still has the surrounding double-quotes. */
            UCString s;
            if (p->current.as.string_val) {
                s = uc_string_new(p->current.as.string_val,
                                  strlen(p->current.as.string_val));
            } else {
                s = uc_string_new(p->current.lexeme,
                                  p->current.lexeme_len);
            }
            advance(p);
            UCLiteral lit;
            memset(&lit, 0, sizeof(lit));
            lit.kind = UC_LIT_STRING;
            lit.as.string_val = s;
            return expr_literal(lit);
        }
        case UC_TOK_KW_TRUE:
            advance(p);
            return expr_literal(uc_literal_true());
        case UC_TOK_KW_FALSE:
            advance(p);
            return expr_literal(uc_literal_false());
        case UC_TOK_KW_NULL:
            advance(p);
            return uc_expr_null();
        case UC_TOK_IDENT: {
            UCString s = uc_string_new(p->current.lexeme,
                                       p->current.lexeme_len);
            advance(p);
            return expr_ident(s);
        }
        default:
            err_here(p, "expected expression; got %s '%s'",
                     uc_token_kind_name(k), cur_lex(p));
            return NULL;
    }
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                */
/* ------------------------------------------------------------------------- */

void uc_parser_init(UCParser* p, UCLexer* lexer, UCError* error) {
    p->lexer = lexer;
    uc_token_init(&p->current);
    uc_token_init(&p->peek);
    p->current.kind = UC_TOK_EOF;
    p->peek.kind = UC_TOK_EOF;
    p->error = error;

    /* Prime two-token lookahead. */
    p->peek = uc_lexer_next(lexer);
    advance(p);
}

void uc_parser_reset(UCParser* p) {
    uc_token_free(&p->current);
    uc_token_free(&p->peek);
    uc_token_init(&p->current);
    uc_token_init(&p->peek);
    p->lexer = NULL;
    p->error = NULL;
}

UCModule* uc_parser_parse(UCParser* p) {
    UCVec* decls = uc_vec_new();

    while (!check(p, UC_TOK_EOF) && !check(p, UC_TOK_ERROR)) {
        UCTopLevel* tl = parse_top_level(p);
        if (is_err(p)) {
            uc_top_level_free(tl);
            uc_vec_free(decls, NULL);
            return NULL;
        }
        if (tl) {
            uc_vec_push(decls, tl);
        } else {
            /* 'None' branch; in practice parse_top_level returns NULL only
             * on error or at EOF, both handled above. */
            break;
        }
    }

    return uc_module_new(decls);
}