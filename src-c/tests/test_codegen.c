/* UltraCPP C compiler - codegen unit tests (Phase 3)
 *
 * Smoke tests for the LLVM IR emitter.  Full byte-level equivalence with
 * the Rust compiler is covered by tools/codegen_test.sh.
 */
#include "uc_codegen.h"
#include "uc_error.h"
#include "uc_lexer.h"
#include "uc_parser.h"

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

#define ASSERT_CONTAINS(haystack, needle) do { \
    tests_run++; \
    if (strstr(haystack, needle) != NULL) { tests_passed++; } \
    else { fprintf(stderr, "FAIL %s: %s:%d: missing '%s' in:\n%s\n", \
                   current_test ? current_test : "?", \
                   __FILE__, __LINE__, needle, haystack); } \
} while (0)

#define RUN(test_fn) do { current_test = #test_fn; test_fn(); } while (0)

/* Helper: parse + codegen a source string, return IR (or NULL on error). */
static char* compile_src(const char* src) {
    UCError err; uc_error_init(&err);
    UCLexer lex;
    uc_lexer_init(&lex, src, strlen(src), "<test>", &err);
    UCParser p;
    uc_parser_init(&p, &lex, &err);
    UCModule* m = uc_parser_parse(&p);
    if (err.kind != UC_ERR_NONE) {
        fprintf(stderr, "%s: parse error\n", current_test);
        uc_error_report(&err, stderr);
        uc_module_free(m);
        uc_parser_reset(&p);
        uc_lexer_reset(&lex);
        return NULL;
    }
    UCCodeGenerator* g = uc_codegen_new("test");

    /* Mimic the import-scanning done in src-c/src/main.c::run_emit_ll. */
    const char* scan = src;
    while ((scan = strstr(scan, "#import")) != NULL) {
        const char* eol = strchr(scan, '\n');
        size_t llen = eol ? (size_t)(eol - scan) : strlen(scan);
        const char* q1 = (const char*)memchr(scan, '"', llen);
        const char* lt1 = (const char*)memchr(scan, '<', llen);
        if (q1 && q1 < scan + llen) {
            const char* q2 = (const char*)memchr(q1 + 1, '"',
                llen - (size_t)(q1 - scan) - 1);
            if (q2) {
                size_t plen = (size_t)(q2 - q1 - 1);
                char* ipath = (char*)malloc(plen + 1);
                memcpy(ipath, q1 + 1, plen);
                ipath[plen] = '\0';
                uc_codegen_add_imported_module(g, ipath);
                free(ipath);
            }
        } else if (lt1 && lt1 < scan + llen) {
            const char* gt1 = (const char*)memchr(lt1 + 1, '>',
                llen - (size_t)(lt1 - scan) - 1);
            if (gt1) {
                size_t plen = (size_t)(gt1 - lt1 - 1);
                char* ipath = (char*)malloc(plen + 1);
                memcpy(ipath, lt1 + 1, plen);
                ipath[plen] = '\0';
                uc_codegen_add_imported_module(g, ipath);
                free(ipath);
            }
        }
        scan = scan + (eol ? (size_t)(eol - scan) + 1 : llen);
    }

    char* ir = uc_codegen_generate(g, m, &err);
    if (err.kind != UC_ERR_NONE) {
        fprintf(stderr, "%s: codegen error\n", current_test);
        uc_error_report(&err, stderr);
        free(ir);
        ir = NULL;
    }
    uc_codegen_free(g);
    uc_module_free(m);
    uc_parser_reset(&p);
    uc_lexer_reset(&lex);
    return ir;
}

static void test_codegen_builtin_decls(void) {
    char* ir = compile_src("int main() { return 0; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "declare i64 @strlen");
    ASSERT_CONTAINS(ir, "declare i64 @write");
    ASSERT_CONTAINS(ir, "declare i8* @malloc");
    ASSERT_CONTAINS(ir, "declare void @free");
    free(ir);
}

static void test_codegen_return_int_literal(void) {
    char* ir = compile_src("int main() { return 0; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "define i32 @main()");
    ASSERT_CONTAINS(ir, "ret i32 %");
    free(ir);
}

static void test_codegen_var_decl(void) {
    char* ir = compile_src("int main() { int x = 5; return x; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "%x = alloca i32");
    ASSERT_CONTAINS(ir, "store i32 %");
    ASSERT_CONTAINS(ir, "load i32, i32* %x");
    free(ir);
}

static void test_codegen_arith(void) {
    char* ir = compile_src("int main() { return 1 + 2; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "= add i32");
    free(ir);
}

static void test_codegen_compare(void) {
    char* ir = compile_src("int main() { return 1 < 2; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "= icmp slt i32");
    free(ir);
}

static void test_codegen_string_literal(void) {
    char* ir = compile_src("int main() { return \"hi\"[0]; }");
    /* The above won't compile fully, but we just want to check the
     * string global is emitted. */
    /* Use a valid alternative: */
    free(ir);
    ir = compile_src("char *f() { return \"hi\"; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "@.str.");
    ASSERT_CONTAINS(ir, "private constant");
    free(ir);
}

static void test_codegen_field_call_no_decl(void) {
    /* Without #import, no `declare` line should appear for io.X. */
    char* ir = compile_src("int main() { return io.print(); }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    /* The call is emitted (bare `@fn` symbol per 0.3.5 commit 14d) but no
     * extern decl since no import. */
    ASSERT_CONTAINS(ir, "call i32 @print(");
    /* No declare line for the field call's symbol. */
    ASSERT_TRUE(strstr(ir, "declare i32 @print") == NULL);
    free(ir);
}

static void test_codegen_field_call_with_pound_import(void) {
    /* With #import, declare line should appear. */
    char* ir = compile_src(
        "#import \"lib/io\"\n"
        "int main() { return io.print(); }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    /* [0.3.5 commit 14d] Declare emit now uses bare `@fn` symbol (matching
     * export-side definition) with signature `(i32, i32)`. */
    ASSERT_CONTAINS(ir, "declare i32 @print(i32, i32)");
    free(ir);
}

/* [0.3.5 commit 14f] Compound assignment emit tests.
 * Parser desugars `a += b` → `a = a + b`, so the IR contains:
 *   - alloca for the lvalue (`%a`)
 *   - load (read current value)
 *   - the binop (`add`/`sub`/`mul`/`sdiv`/`srem`)
 *   - store back into the lvalue
 * The minimal ASSERT below verifies the binop landed and the alloca/
 * store pair is intact. */
static void test_codegen_compound_assign_plus(void) {
    char* ir = compile_src("int main() { int a = 5; a += 3; return a; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "%a = alloca i32");
    ASSERT_CONTAINS(ir, "= add i32");
    ASSERT_CONTAINS(ir, "store i32");
    free(ir);
}

static void test_codegen_compound_assign_minus(void) {
    char* ir = compile_src("int main() { int a = 5; a -= 2; return a; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "%a = alloca i32");
    ASSERT_CONTAINS(ir, "= sub i32");
    ASSERT_CONTAINS(ir, "store i32");
    free(ir);
}

static void test_codegen_compound_assign_mul(void) {
    char* ir = compile_src("int main() { int a = 5; a *= 4; return a; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "%a = alloca i32");
    ASSERT_CONTAINS(ir, "= mul i32");
    ASSERT_CONTAINS(ir, "store i32");
    free(ir);
}

static void test_codegen_compound_assign_div(void) {
    char* ir = compile_src("int main() { int a = 20; a /= 4; return a; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "%a = alloca i32");
    ASSERT_CONTAINS(ir, "= sdiv i32");
    ASSERT_CONTAINS(ir, "store i32");
    free(ir);
}

static void test_codegen_compound_assign_mod(void) {
    char* ir = compile_src("int main() { int a = 17; a %= 5; return a; }");
    ASSERT_TRUE(ir != NULL); if (!ir) return;
    ASSERT_CONTAINS(ir, "%a = alloca i32");
    ASSERT_CONTAINS(ir, "= srem i32");
    ASSERT_CONTAINS(ir, "store i32");
    free(ir);
}

int main(void) {
    RUN(test_codegen_builtin_decls);
    RUN(test_codegen_return_int_literal);
    RUN(test_codegen_var_decl);
    RUN(test_codegen_arith);
    RUN(test_codegen_compare);
    RUN(test_codegen_string_literal);
    RUN(test_codegen_field_call_no_decl);
    RUN(test_codegen_field_call_with_pound_import);
    RUN(test_codegen_compound_assign_plus);
    RUN(test_codegen_compound_assign_minus);
    RUN(test_codegen_compound_assign_mul);
    RUN(test_codegen_compound_assign_div);
    RUN(test_codegen_compound_assign_mod);

    printf("\nResults: %d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}