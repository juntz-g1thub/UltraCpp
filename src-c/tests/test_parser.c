/* UltraCPP C compiler - parser unit tests (Phase 2.2)
 *
 * Covers top-level declarations:
 *   - empty source
 *   - import with / without alias
 *   - struct def (empty and with fields)
 *   - extern declaration
 *   - var decl without / with init
 *   - func def (empty body, single return, return without value)
 *   - func decl (forward declaration)
 *   - export wrapping another decl
 *   - error: missing semicolon
 *   - error: unexpected token at top level
 */
#include "uc_parser.h"
#include "uc_lexer.h"
#include "uc_error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;
static const char* current_test = NULL;

#define ASSERT_TRUE(cond) do { \
    tests_run++; \
    if (cond) { tests_passed++; } \
    else { fprintf(stderr, "FAIL %s: %s:%d: %s\n", \
                   current_test ? current_test : "?", \
                   __FILE__, __LINE__, #cond); } \
} while (0)

#define ASSERT_EQ_INT(a, b) do { \
    tests_run++; \
    long long _a = (long long)(a); \
    long long _b = (long long)(b); \
    if (_a == _b) { tests_passed++; } \
    else { fprintf(stderr, "FAIL %s: %s:%d: expected %lld got %lld\n", \
                   current_test ? current_test : "?", \
                   __FILE__, __LINE__, _b, _a); } \
} while (0)

#define ASSERT_EQ_STR(a, b) do { \
    tests_run++; \
    const char* _a = (a); \
    const char* _b = (b); \
    if ((_a == NULL && _b == NULL) || (_a && _b && strcmp(_a, _b) == 0)) { \
        tests_passed++; \
    } else { \
        fprintf(stderr, "FAIL %s: %s:%d: expected \"%s\" got \"%s\"\n", \
                current_test ? current_test : "?", \
                __FILE__, __LINE__, _b ? _b : "(null)", _a ? _a : "(null)"); \
    } \
} while (0)

#define RUN(test_fn) do { current_test = #test_fn; test_fn(); } while (0)

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */

/* Parse `src` and return the resulting UCModule*. On error, returns NULL
 * and writes diagnostic to stderr. The error union is logged for
 * inspection in failure paths. */
static UCModule* parse_source(const char* src) {
    UCError err;
    uc_error_init(&err);
    UCLexer lex;
    uc_lexer_init(&lex, src, strlen(src), "<test>", &err);
    if (err.kind != UC_ERR_NONE) {
        fprintf(stderr, "lexer init error in test setup\n");
        return NULL;
    }
    UCParser p;
    uc_parser_init(&p, &lex, &err);
    UCModule* m = uc_parser_parse(&p);
    if (err.kind != UC_ERR_NONE) {
        uc_error_report(&err, stderr);
        uc_module_free(m);
        uc_parser_reset(&p);
        uc_lexer_reset(&lex);
        return NULL;
    }
    uc_parser_reset(&p);
    uc_lexer_reset(&lex);
    return m;
}

/* Assert that parse_source fails and reports an error. */
static int parse_must_fail(const char* src, const char* test_name) {
    UCError err;
    uc_error_init(&err);
    UCLexer lex;
    uc_lexer_init(&lex, src, strlen(src), "<test>", &err);
    UCParser p;
    uc_parser_init(&p, &lex, &err);
    UCModule* m = uc_parser_parse(&p);
    int failed = (err.kind != UC_ERR_NONE);
    if (!failed) {
        fprintf(stderr, "FAIL %s: expected parse error but got success\n",
                test_name);
        uc_module_free(m);
    }
    uc_parser_reset(&p);
    uc_lexer_reset(&lex);
    return failed;
}

#define ASSERT_PARSE_FAIL(src) do { \
    tests_run++; \
    if (parse_must_fail(src, current_test)) tests_passed++; \
} while (0)

/* ------------------------------------------------------------------------- */
/* Tests                                                                     */
/* ------------------------------------------------------------------------- */

static void test_empty_source(void) {
    UCModule* m = parse_source("");
    ASSERT_TRUE(m != NULL);
    ASSERT_EQ_INT((long long)uc_vec_len(m->declarations), 0);
    uc_module_free(m);
}

static void test_pound_import_no_decl(void) {
    /* '#import' is a directive (mirrors the Rust preprocessor): it is
     * consumed but produces no TopLevel. Semicolon is optional. */
    UCModule* m = parse_source("#import \"lib/io\";");
    ASSERT_TRUE(m != NULL); if (!m) return;
    ASSERT_EQ_INT((long long)uc_vec_len(m->declarations), 0);
    uc_module_free(m);
}

static void test_pound_import_no_semicolon(void) {
    /* Existing test programs use '#import "path"' with no semicolon. */
    UCModule* m = parse_source("#import \"lib/io\"");
    ASSERT_TRUE(m != NULL); if (!m) return;
    ASSERT_EQ_INT((long long)uc_vec_len(m->declarations), 0);
    uc_module_free(m);
}

static void test_pound_import_angle_path(void) {
    UCModule* m = parse_source("#import <io>");
    ASSERT_TRUE(m != NULL); if (!m) return;
    ASSERT_EQ_INT((long long)uc_vec_len(m->declarations), 0);
    uc_module_free(m);
}

static void test_pound_import_with_alias(void) {
    UCModule* m = parse_source("#import \"lib/io\" as io");
    ASSERT_TRUE(m != NULL); if (!m) return;
    ASSERT_EQ_INT((long long)uc_vec_len(m->declarations), 0);
    uc_module_free(m);
}

static void test_bare_import_no_alias(void) {
    /* Bare 'import' (UC_TOK_KW_IMPORT) is rare; it produces a UC_TL_IMPORT. */
    UCModule* m = parse_source("import \"lib/io\";");
    ASSERT_TRUE(m != NULL); if (!m) return;
    ASSERT_EQ_INT((long long)uc_vec_len(m->declarations), 1);
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_IMPORT);
    ASSERT_EQ_STR(tl->as.import->path.data, "lib/io");
    ASSERT_TRUE(tl->as.import->alias.data == NULL);  /* None */
    uc_module_free(m);
}

static void test_bare_import_with_alias(void) {
    UCModule* m = parse_source("import \"lib/io\" as io;");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_IMPORT);
    ASSERT_EQ_STR(tl->as.import->path.data, "lib/io");
    ASSERT_EQ_STR(tl->as.import->alias.data, "io");
    uc_module_free(m);
}

static void test_struct_empty(void) {
    UCModule* m = parse_source("struct Point {};");
    ASSERT_TRUE(m != NULL);
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_STRUCT_DEF);
    ASSERT_EQ_STR(tl->as.struct_def->name.data, "Point");
    ASSERT_EQ_INT((long long)uc_vec_len(tl->as.struct_def->fields), 0);
    uc_module_free(m);
}

static void test_struct_with_fields(void) {
    /* UltraCPP struct fields use `name: type;` syntax. */
    UCModule* m = parse_source(
        "struct Point { x: int; y: int; z: f64; };");
    ASSERT_TRUE(m != NULL);
    if (!m) return;
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_STRUCT_DEF);
    ASSERT_EQ_STR(tl->as.struct_def->name.data, "Point");
    ASSERT_EQ_INT((long long)uc_vec_len(tl->as.struct_def->fields), 3);
    UCStructField* f0 = (UCStructField*)uc_vec_at(
        tl->as.struct_def->fields, 0);
    ASSERT_EQ_STR(f0->name.data, "x");
    ASSERT_TRUE(f0->ty->kind == UC_TYPE_INT);
    UCStructField* f2 = (UCStructField*)uc_vec_at(
        tl->as.struct_def->fields, 2);
    ASSERT_TRUE(f2->ty->kind == UC_TYPE_F64);
    uc_module_free(m);
}

static void test_extern_decl(void) {
    UCModule* m = parse_source("extern int foo(int, *int);");
    ASSERT_TRUE(m != NULL);
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_EXTERN);
    ASSERT_EQ_STR(tl->as.extern_.name.data, "foo");
    ASSERT_TRUE(tl->as.extern_.ty->kind == UC_TYPE_INT);
    ASSERT_EQ_INT((long long)uc_vec_len(tl->as.extern_.params), 2);
    /* Second param has pointer type */
    UCParam* p1 = (UCParam*)uc_vec_at(tl->as.extern_.params, 1);
    ASSERT_TRUE(p1->ty->kind == UC_TYPE_POINTER);
    ASSERT_TRUE(p1->ty->as.inner->kind == UC_TYPE_INT);
    uc_module_free(m);
}

static void test_var_decl_no_init(void) {
    UCModule* m = parse_source("int x;");
    ASSERT_TRUE(m != NULL);
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_VAR_DECL);
    ASSERT_EQ_STR(tl->as.var_decl->name.data, "x");
    ASSERT_TRUE(tl->as.var_decl->ty->kind == UC_TYPE_INT);
    ASSERT_TRUE(tl->as.var_decl->init == NULL);
    uc_module_free(m);
}

static void test_var_decl_with_int_init(void) {
    UCModule* m = parse_source("int x = 42;");
    ASSERT_TRUE(m != NULL);
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_VAR_DECL);
    ASSERT_TRUE(tl->as.var_decl->init != NULL);
    ASSERT_TRUE(tl->as.var_decl->init->kind == UC_EXPR_LITERAL);
    ASSERT_TRUE(tl->as.var_decl->init->as.literal.kind == UC_LIT_INT);
    ASSERT_EQ_INT(tl->as.var_decl->init->as.literal.as.int_val, 42);
    uc_module_free(m);
}

static void test_var_decl_with_ident_init(void) {
    UCModule* m = parse_source("int x = y;");
    ASSERT_TRUE(m != NULL);
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_VAR_DECL);
    ASSERT_TRUE(tl->as.var_decl->init->kind == UC_EXPR_IDENT);
    ASSERT_EQ_STR(tl->as.var_decl->init->as.ident.data, "y");
    uc_module_free(m);
}

static void test_func_def_empty_body(void) {
    UCModule* m = parse_source("int main() {}");
    ASSERT_TRUE(m != NULL);
    if (!m) return;
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_FUNC_DEF);
    ASSERT_EQ_STR(tl->as.func_def->name.data, "main");
    ASSERT_TRUE(tl->as.func_def->return_ty->kind == UC_TYPE_INT);
    ASSERT_TRUE(tl->as.func_def->body->kind == UC_STMT_BLOCK);
    ASSERT_EQ_INT(
        (long long)uc_vec_len(tl->as.func_def->body->as.block), 0);
    uc_module_free(m);
}

static void test_func_def_with_return_zero(void) {
    /* The showcase test from HANDOFF.md */
    UCModule* m = parse_source("int main() { return 0; }");
    ASSERT_TRUE(m != NULL);
    if (!m) return;
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_FUNC_DEF);
    ASSERT_EQ_STR(tl->as.func_def->name.data, "main");
    UCStmt* body = tl->as.func_def->body;
    ASSERT_TRUE(body->kind == UC_STMT_BLOCK);
    ASSERT_EQ_INT((long long)uc_vec_len(body->as.block), 1);
    UCStmt* ret = (UCStmt*)uc_vec_at(body->as.block, 0);
    ASSERT_TRUE(ret->kind == UC_STMT_RETURN);
    ASSERT_TRUE(ret->as.ret->kind == UC_EXPR_LITERAL);
    ASSERT_TRUE(ret->as.ret->as.literal.kind == UC_LIT_INT);
    ASSERT_EQ_INT(ret->as.ret->as.literal.as.int_val, 0);
    uc_module_free(m);
}

static void test_func_def_with_return_string(void) {
    UCModule* m = parse_source("char *hello() { return \"hi\"; }");
    ASSERT_TRUE(m != NULL);
    if (!m) return;
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_FUNC_DEF);
    UCStmt* body = tl->as.func_def->body;
    UCStmt* ret = (UCStmt*)uc_vec_at(body->as.block, 0);
    ASSERT_TRUE(ret->as.ret->as.literal.kind == UC_LIT_STRING);
    ASSERT_EQ_STR(ret->as.ret->as.literal.as.string_val.data, "hi");
    uc_module_free(m);
}

static void test_func_def_with_return_ident(void) {
    UCModule* m = parse_source("int f() { return x; }");
    ASSERT_TRUE(m != NULL);
    if (!m) return;
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    UCStmt* ret = (UCStmt*)uc_vec_at(tl->as.func_def->body->as.block, 0);
    ASSERT_TRUE(ret->as.ret->kind == UC_EXPR_IDENT);
    ASSERT_EQ_STR(ret->as.ret->as.ident.data, "x");
    uc_module_free(m);
}

static void test_func_def_with_return_null(void) {
    UCModule* m = parse_source("int *f() { return null; }");
    ASSERT_TRUE(m != NULL);
    if (!m) return;
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    UCStmt* ret = (UCStmt*)uc_vec_at(tl->as.func_def->body->as.block, 0);
    ASSERT_TRUE(ret->as.ret->kind == UC_EXPR_NULL);
    uc_module_free(m);
}

static void test_func_def_with_params(void) {
    UCModule* m = parse_source(
        "int add(int a, int b) { return a; }");
    ASSERT_TRUE(m != NULL);
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_FUNC_DEF);
    ASSERT_EQ_INT((long long)uc_vec_len(tl->as.func_def->params), 2);
    UCParam* pa = (UCParam*)uc_vec_at(tl->as.func_def->params, 0);
    ASSERT_EQ_STR(pa->name.data, "a");
    ASSERT_TRUE(pa->ty->kind == UC_TYPE_INT);
    uc_module_free(m);
}

static void test_func_decl_forward(void) {
    UCModule* m = parse_source("int add(int a, int b);");
    ASSERT_TRUE(m != NULL);
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_FUNC_DECL);
    ASSERT_EQ_STR(tl->as.func_decl->name.data, "add");
    ASSERT_EQ_INT((long long)uc_vec_len(tl->as.func_decl->params), 2);
    uc_module_free(m);
}

static void test_export_wraps_var(void) {
    UCModule* m = parse_source("export int counter = 0;");
    ASSERT_TRUE(m != NULL);
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(tl->kind == UC_TL_EXPORT);
    ASSERT_TRUE(tl->as.export_->kind == UC_TL_VAR_DECL);
    ASSERT_EQ_STR(tl->as.export_->as.var_decl->name.data, "counter");
    uc_module_free(m);
}

static void test_multiple_decls(void) {
    /* '#import' is a directive: 0 TopLevel entries; the source otherwise
     * produces 3 TopLevels (var, func, struct). */
    UCModule* m = parse_source(
        "#import \"lib/io\";\n"
        "int x = 1;\n"
        "int main() { return 0; }\n"
        "struct S { a: int; };");
    ASSERT_TRUE(m != NULL); if (!m) return;
    ASSERT_EQ_INT((long long)uc_vec_len(m->declarations), 3);
    UCTopLevel* t0 = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    UCTopLevel* t1 = (UCTopLevel*)uc_vec_at(m->declarations, 1);
    UCTopLevel* t2 = (UCTopLevel*)uc_vec_at(m->declarations, 2);
    ASSERT_TRUE(t0->kind == UC_TL_VAR_DECL);
    ASSERT_TRUE(t1->kind == UC_TL_FUNC_DEF);
    ASSERT_TRUE(t2->kind == UC_TL_STRUCT_DEF);
    uc_module_free(m);
}

static void test_pointer_types(void) {
    UCModule* m = parse_source("int *p; char **pp;");
    ASSERT_TRUE(m != NULL);
    UCTopLevel* t0 = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    UCTopLevel* t1 = (UCTopLevel*)uc_vec_at(m->declarations, 1);
    ASSERT_TRUE(t0->as.var_decl->ty->kind == UC_TYPE_POINTER);
    ASSERT_TRUE(t0->as.var_decl->ty->as.inner->kind == UC_TYPE_INT);
    ASSERT_TRUE(t1->as.var_decl->ty->kind == UC_TYPE_POINTER);
    ASSERT_TRUE(t1->as.var_decl->ty->as.inner->kind == UC_TYPE_POINTER);
    ASSERT_TRUE(t1->as.var_decl->ty->as.inner->as.inner->kind == UC_TYPE_CHAR);
    uc_module_free(m);
}

/* ------------------------------------------------------------------------- */
/* Error cases                                                               */
/* ------------------------------------------------------------------------- */

static void test_error_missing_semicolon_var(void) {
    ASSERT_PARSE_FAIL("int x");  /* no semicolon */
}

static void test_error_missing_semicolon_import(void) {
    /* Bare 'import' (UC_TOK_KW_IMPORT) requires a semicolon. '#import'
     * is a directive that doesn't. */
    ASSERT_PARSE_FAIL("import \"lib/io\"");  /* no semicolon */
}

static void test_error_unexpected_token(void) {
    ASSERT_PARSE_FAIL("+");  /* '+' cannot start a top-level */
}

static void test_error_unmatched_brace(void) {
    ASSERT_PARSE_FAIL("int main() { ");  /* EOF in body */
}

static void test_error_return_outside_block(void) {
    /* 'return' at top level is not allowed. */
    ASSERT_PARSE_FAIL("return 0;");
}

/* ------------------------------------------------------------------------- */
/* Statement tests (Phase 2.3)                                              */
/* ------------------------------------------------------------------------- */

/* Helper: extract the single FuncDef's body block. */
static UCStmt* func_body(const UCModule* m) {
    UCTopLevel* tl = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    return tl->as.func_def->body;
}

static UCStmt* block_stmt_at(const UCStmt* blk, size_t i) {
    return (UCStmt*)uc_vec_at(blk->as.block, i);
}

static void test_stmt_if_without_else(void) {
    UCModule* m = parse_source("int f() { if (true) { return 0; } return 1; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* blk = func_body(m);
    ASSERT_TRUE(blk->kind == UC_STMT_BLOCK);
    UCStmt* s0 = block_stmt_at(blk, 0);
    ASSERT_TRUE(s0->kind == UC_STMT_IF);
    ASSERT_TRUE(s0->as.if_stmt.cond->kind == UC_EXPR_LITERAL);
    ASSERT_TRUE(s0->as.if_stmt.cond->as.literal.kind == UC_LIT_TRUE);
    ASSERT_TRUE(s0->as.if_stmt.then_branch->kind == UC_STMT_BLOCK);
    ASSERT_TRUE(s0->as.if_stmt.else_branch == NULL);
    uc_module_free(m);
}

static void test_stmt_if_with_else(void) {
    UCModule* m = parse_source(
        "int f() { if (x) { return 1; } else { return 2; } return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_IF);
    ASSERT_TRUE(s0->as.if_stmt.cond->kind == UC_EXPR_IDENT);
    ASSERT_TRUE(s0->as.if_stmt.else_branch != NULL);
    ASSERT_TRUE(s0->as.if_stmt.else_branch->kind == UC_STMT_BLOCK);
    uc_module_free(m);
}

static void test_stmt_if_else_if(void) {
    UCModule* m = parse_source(
        "int f() { if (x) { return 1; } else if (y) { return 2; } "
        "else { return 3; } return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_IF);
    ASSERT_TRUE(s0->as.if_stmt.else_branch->kind == UC_STMT_IF);
    UCStmt* inner = s0->as.if_stmt.else_branch;
    ASSERT_TRUE(inner->as.if_stmt.cond->as.ident.data
                && strcmp(inner->as.if_stmt.cond->as.ident.data, "y") == 0);
    ASSERT_TRUE(inner->as.if_stmt.else_branch != NULL);
    uc_module_free(m);
}

static void test_stmt_while(void) {
    UCModule* m = parse_source(
        "int f() { while (true) { break; } return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_WHILE);
    ASSERT_TRUE(s0->as.while_stmt.cond->as.literal.kind == UC_LIT_TRUE);
    ASSERT_TRUE(s0->as.while_stmt.body->kind == UC_STMT_BLOCK);
    uc_module_free(m);
}

static void test_stmt_for_all_parts(void) {
    UCModule* m = parse_source(
        "int f() { for (int i = 0; x; i) { break; } return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_FOR);
    /* init: DeclStmt */
    ASSERT_TRUE(s0->as.for_stmt.init != NULL);
    ASSERT_TRUE(s0->as.for_stmt.init->kind == UC_STMT_DECL);
    ASSERT_EQ_STR(s0->as.for_stmt.init->as.decl->name.data, "i");
    /* cond: ident */
    ASSERT_TRUE(s0->as.for_stmt.cond != NULL);
    ASSERT_TRUE(s0->as.for_stmt.cond->kind == UC_EXPR_IDENT);
    ASSERT_EQ_STR(s0->as.for_stmt.cond->as.ident.data, "x");
    /* step: ident */
    ASSERT_TRUE(s0->as.for_stmt.step != NULL);
    ASSERT_TRUE(s0->as.for_stmt.step->kind == UC_EXPR_IDENT);
    ASSERT_EQ_STR(s0->as.for_stmt.step->as.ident.data, "i");
    uc_module_free(m);
}

static void test_stmt_for_no_init(void) {
    UCModule* m = parse_source(
        "int f() { for (; x; ) { break; } return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_FOR);
    ASSERT_TRUE(s0->as.for_stmt.init == NULL);
    ASSERT_TRUE(s0->as.for_stmt.cond != NULL);
    ASSERT_TRUE(s0->as.for_stmt.step == NULL);
    uc_module_free(m);
}

static void test_stmt_for_infinite(void) {
    UCModule* m = parse_source(
        "int f() { for (;;) { break; } return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_FOR);
    ASSERT_TRUE(s0->as.for_stmt.init == NULL);
    ASSERT_TRUE(s0->as.for_stmt.cond == NULL);
    ASSERT_TRUE(s0->as.for_stmt.step == NULL);
    uc_module_free(m);
}

static void test_stmt_break_continue(void) {
    UCModule* m = parse_source(
        "int f() { while (true) { break; } while (true) { continue; } return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* blk = func_body(m);
    UCStmt* w0 = block_stmt_at(blk, 0);
    UCStmt* w1 = block_stmt_at(blk, 1);
    ASSERT_TRUE(block_stmt_at(w0->as.while_stmt.body, 0)->kind == UC_STMT_BREAK);
    ASSERT_TRUE(block_stmt_at(w1->as.while_stmt.body, 0)->kind
                == UC_STMT_CONTINUE);
    uc_module_free(m);
}

static void test_stmt_decl_no_init(void) {
    UCModule* m = parse_source("int f() { int x; return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_DECL);
    ASSERT_EQ_STR(s0->as.decl->name.data, "x");
    ASSERT_TRUE(s0->as.decl->init == NULL);
    uc_module_free(m);
}

static void test_stmt_decl_with_init(void) {
    UCModule* m = parse_source("int f() { int x = 42; return x; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_DECL);
    ASSERT_TRUE(s0->as.decl->init != NULL);
    ASSERT_TRUE(s0->as.decl->init->kind == UC_EXPR_LITERAL);
    ASSERT_EQ_INT(s0->as.decl->init->as.literal.as.int_val, 42);
    uc_module_free(m);
}

static void test_stmt_expr_statement(void) {
    UCModule* m = parse_source("int f() { x; return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_EXPR);
    ASSERT_TRUE(s0->as.expr->kind == UC_EXPR_IDENT);
    ASSERT_EQ_STR(s0->as.expr->as.ident.data, "x");
    uc_module_free(m);
}

static void test_stmt_free(void) {
    UCModule* m = parse_source("int f() { free(x); return 0; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_FREE);
    ASSERT_TRUE(s0->as.expr->kind == UC_EXPR_IDENT);
    ASSERT_EQ_STR(s0->as.expr->as.ident.data, "x");
    uc_module_free(m);
}

static void test_stmt_unsafe_block(void) {
    UCModule* m = parse_source(
        "int f() { unsafe { return 0; } return 1; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* s0 = block_stmt_at(func_body(m), 0);
    ASSERT_TRUE(s0->kind == UC_STMT_BLOCK);
    uc_module_free(m);
}

static void test_stmt_nested_blocks(void) {
    UCModule* m = parse_source(
        "int f() { { { return 0; } } return 1; }");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* blk = func_body(m);
    UCStmt* outer = block_stmt_at(blk, 0);
    ASSERT_TRUE(outer->kind == UC_STMT_BLOCK);
    UCStmt* inner = block_stmt_at(outer, 0);
    ASSERT_TRUE(inner->kind == UC_STMT_BLOCK);
    uc_module_free(m);
}

static void test_stmt_complex_nesting(void) {
    /* If inside while inside for inside main. */
    UCModule* m = parse_source(
        "int main() {\n"
        "  int n = 5;\n"
        "  for (int i = 0; x; i) {\n"
        "    if (i) { return 0; }\n"
        "    while (y) { break; }\n"
        "  }\n"
        "  return 1;\n"
        "}\n");
    ASSERT_TRUE(m != NULL); if (!m) return;
    UCStmt* blk = func_body(m);
    /* [0] decl, [1] for, [2] return */
    ASSERT_EQ_INT((long long)uc_vec_len(blk->as.block), 3);
    ASSERT_TRUE(block_stmt_at(blk, 0)->kind == UC_STMT_DECL);
    ASSERT_TRUE(block_stmt_at(blk, 1)->kind == UC_STMT_FOR);
    UCStmt* for_body = block_stmt_at(blk, 1)->as.for_stmt.body;
    ASSERT_TRUE(for_body->kind == UC_STMT_BLOCK);
    UCStmt* if_s = block_stmt_at(for_body, 0);
    ASSERT_TRUE(if_s->kind == UC_STMT_IF);
    ASSERT_EQ_STR(if_s->as.if_stmt.cond->as.ident.data, "i");
    uc_module_free(m);
}

/* ------------------------------------------------------------------------- */
/* main                                                                      */
/* ------------------------------------------------------------------------- */
int main(void) {
    RUN(test_empty_source);
    RUN(test_pound_import_no_decl);
    RUN(test_pound_import_no_semicolon);
    RUN(test_pound_import_angle_path);
    RUN(test_pound_import_with_alias);
    RUN(test_bare_import_no_alias);
    RUN(test_bare_import_with_alias);
    RUN(test_struct_empty);
    RUN(test_struct_with_fields);
    RUN(test_extern_decl);
    RUN(test_var_decl_no_init);
    RUN(test_var_decl_with_int_init);
    RUN(test_var_decl_with_ident_init);
    RUN(test_func_def_empty_body);
    RUN(test_func_def_with_return_zero);
    RUN(test_func_def_with_return_string);
    RUN(test_func_def_with_return_ident);
    RUN(test_func_def_with_return_null);
    RUN(test_func_def_with_params);
    RUN(test_func_decl_forward);
    RUN(test_export_wraps_var);
    RUN(test_multiple_decls);
    RUN(test_pointer_types);

    RUN(test_stmt_if_without_else);
    RUN(test_stmt_if_with_else);
    RUN(test_stmt_if_else_if);
    RUN(test_stmt_while);
    RUN(test_stmt_for_all_parts);
    RUN(test_stmt_for_no_init);
    RUN(test_stmt_for_infinite);
    RUN(test_stmt_break_continue);
    RUN(test_stmt_decl_no_init);
    RUN(test_stmt_decl_with_init);
    RUN(test_stmt_expr_statement);
    RUN(test_stmt_free);
    RUN(test_stmt_unsafe_block);
    RUN(test_stmt_nested_blocks);
    RUN(test_stmt_complex_nesting);

    RUN(test_error_missing_semicolon_var);
    RUN(test_error_missing_semicolon_import);
    RUN(test_error_unexpected_token);
    RUN(test_error_unmatched_brace);
    RUN(test_error_return_outside_block);

    printf("\nResults: %d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}