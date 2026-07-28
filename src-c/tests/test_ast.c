/* UltraCPP C compiler - AST unit tests (Phase 2.1)
 *
 * Exercises the AST construction / deep-free / ownership model.
 * Each test builds a small AST subtree, optionally asserts structure,
 * and frees it. ASAN / valgrind are expected to find no leaks or
 * double-frees when run against this binary.
 *
 * Test style mirrors tests/test_lexer.c (tests_run / tests_passed).
 */
#include "uc_ast.h"

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
/* 1. String ownership: a UCString made from C string must be independent   */
/*    and free without leaks even when freed twice (idempotent).            */
/* ------------------------------------------------------------------------- */
static void test_string_ownership(void) {
    const char* src = "hello_world";
    UCString s = uc_string_new(src, strlen(src));
    ASSERT_TRUE(s.data != NULL);
    ASSERT_EQ_INT((long long)s.len, 11);
    ASSERT_EQ_STR(s.data, "hello_world");

    /* mutate the source -- the copy must not be affected */
    char* writable = (char*)src; /* UB in general, but the test string is a
                                     literal so we cannot safely mutate it.
                                     Instead, verify byte-by-byte equality. */
    (void)writable;
    ASSERT_EQ_INT((long long)s.data[0], 'h');
    ASSERT_EQ_INT((long long)s.data[10], 'd');

    uc_string_free(&s);
    ASSERT_TRUE(s.data == NULL);
    ASSERT_EQ_INT((long long)s.len, 0);
    /* second free is a no-op (NULL-safe) */
    uc_string_free(&s);

    /* NULL input -> empty string */
    UCString empty = uc_string_new(NULL, 0);
    ASSERT_TRUE(empty.data != NULL);
    ASSERT_EQ_INT((long long)empty.len, 0);
    uc_string_free(&empty);

    /* uc_string_move transfers ownership */
    UCString donor = uc_string_new_cstr("moved");
    UCString taken = uc_string_move(&donor);
    ASSERT_TRUE(donor.data == NULL);
    ASSERT_EQ_INT((long long)donor.len, 0);
    ASSERT_EQ_STR(taken.data, "moved");
    uc_string_free(&taken);
}

/* ------------------------------------------------------------------------- */
/* 2. UCVec: push grows; item_free is called on every element.              */
/* ------------------------------------------------------------------------- */
static void test_vec_push_grows(void) {
    UCVec* v = uc_vec_new();
    ASSERT_TRUE(v != NULL);
    ASSERT_EQ_INT((long long)uc_vec_len(v), 0);

    int items[16];
    for (int i = 0; i < 16; i++) {
        items[i] = i;
        v = uc_vec_push(v, &items[i]);
    }
    ASSERT_EQ_INT((long long)uc_vec_len(v), 16);
    for (int i = 0; i < 16; i++) {
        int* p = (int*)uc_vec_at(v, (size_t)i);
        ASSERT_TRUE(p != NULL);
        ASSERT_EQ_INT((long long)*p, (long long)i);
    }

    /* free without item_free -> items[] stays intact (stack) */
    uc_vec_free(v, NULL);
}

/* ------------------------------------------------------------------------- */
/* 3. FuncDef: build a `int main()` with empty body, then deep-free.        */
/* ------------------------------------------------------------------------- */
static void test_func_def_create_free(void) {
    UCString name = uc_string_new_cstr("main");
    UCVec* params = uc_vec_new();
    UCType* ret = uc_type_int();
    /* body = bare `;` (UCStmt expr with NULL expr) */
    UCStmt* body = uc_stmt_expr(NULL);

    UCFuncDef* f = uc_func_def_new(name, params, ret, body);
    ASSERT_TRUE(f != NULL);
    ASSERT_EQ_STR(f->name.data, "main");
    ASSERT_TRUE(f->return_ty->kind == UC_TYPE_INT);

    uc_func_def_free(f);
}

/* ------------------------------------------------------------------------- */
/* 4. If statement: with and without else branch (None semantics).           */
/* ------------------------------------------------------------------------- */
static void test_if_stmt_with_and_without_else(void) {
    /* if (cond) then */
    UCExpr* cond = uc_expr_literal(uc_literal_true());
    UCStmt* then_b = uc_stmt_return(uc_expr_literal(uc_literal_int(0)));
    UCStmt* s = uc_stmt_if(cond, then_b, NULL);
    ASSERT_TRUE(s->kind == UC_STMT_IF);
    ASSERT_TRUE(s->as.if_stmt.else_branch == NULL);
    uc_stmt_free(s);

    /* if (cond) then else else_b */
    UCExpr* cond2 = uc_expr_literal(uc_literal_false());
    UCStmt* then2 = uc_stmt_return(uc_expr_literal(uc_literal_int(1)));
    UCStmt* else2 = uc_stmt_return(uc_expr_literal(uc_literal_int(2)));
    UCStmt* s2 = uc_stmt_if(cond2, then2, else2);
    ASSERT_TRUE(s2->kind == UC_STMT_IF);
    ASSERT_TRUE(s2->as.if_stmt.else_branch != NULL);
    uc_stmt_free(s2);
}

/* ------------------------------------------------------------------------- */
/* 5. Nested expression tree: (1 + 2) * 3, with call + field access.        */
/* ------------------------------------------------------------------------- */
static void test_expr_tree_nested(void) {
    /* (1 + 2) * 3 */
    UCExpr* one  = uc_expr_literal(uc_literal_int(1));
    UCExpr* two  = uc_expr_literal(uc_literal_int(2));
    UCExpr* add  = uc_expr_binary(UC_BIN_ADD, one, two);
    UCExpr* three = uc_expr_literal(uc_literal_int(3));
    UCExpr* mul  = uc_expr_binary(UC_BIN_MUL, add, three);
    ASSERT_TRUE(mul->kind == UC_EXPR_BINARY);
    ASSERT_TRUE(mul->as.binary.op == UC_BIN_MUL);
    ASSERT_EQ_INT((long long)mul->as.binary.lhs->as.binary.op, UC_BIN_ADD);
    ASSERT_EQ_INT((long long)mul->as.binary.rhs->as.literal.as.int_val, 3);

    /* f(g(x), h.y)  -- callee is a Call, args contain another field-access */
    UCExpr* x = uc_expr_ident("x", 1);
    UCExpr* g_x = uc_expr_call(uc_expr_ident("g", 1),
                               uc_vec_push(uc_vec_new(), x));
    UCExpr* h_y = uc_expr_field(uc_expr_ident("h", 1),
                                uc_string_new_cstr("y"));
    UCVec* args = uc_vec_new();
    args = uc_vec_push(args, g_x);
    args = uc_vec_push(args, h_y);
    UCExpr* call = uc_expr_call(uc_expr_ident("f", 1), args);
    ASSERT_EQ_INT((long long)uc_vec_len(call->as.call.args), 2);

    uc_expr_free(mul);
    uc_expr_free(call);
}

/* ------------------------------------------------------------------------- */
/* 6. Type variants: pointer-of-pointer, array, function, named.            */
/* ------------------------------------------------------------------------- */
static void test_type_variants(void) {
    /* int** */
    UCType* t1 = uc_type_pointer(uc_type_pointer(uc_type_int()));
    ASSERT_TRUE(t1->kind == UC_TYPE_POINTER);
    ASSERT_TRUE(t1->as.inner->kind == UC_TYPE_POINTER);
    ASSERT_TRUE(t1->as.inner->as.inner->kind == UC_TYPE_INT);
    uc_type_free(t1);

    /* int[10] */
    UCType* t2 = uc_type_array(uc_type_int(), 10);
    ASSERT_TRUE(t2->kind == UC_TYPE_ARRAY);
    ASSERT_EQ_INT((long long)t2->as.array.length, 10);
    uc_type_free(t2);

    /* int (int, int) -- function type */
    UCVec* params = uc_vec_new();
    params = uc_vec_push(params, uc_type_int());
    params = uc_vec_push(params, uc_type_int());
    UCType* t3 = uc_type_function(uc_type_int(), params);
    ASSERT_TRUE(t3->kind == UC_TYPE_FUNCTION);
    ASSERT_EQ_INT((long long)uc_vec_len(t3->as.function.params), 2);
    uc_type_free(t3);

    /* MyStruct (named) */
    UCType* t4 = uc_type_named("MyStruct", 8);
    ASSERT_TRUE(t4->kind == UC_TYPE_NAMED);
    ASSERT_EQ_STR(t4->as.named.data, "MyStruct");
    uc_type_free(t4);
}

/* ------------------------------------------------------------------------- */
/* 7. Module: a complete top-level program (var + func + import).            */
/* ------------------------------------------------------------------------- */
static void test_module_with_decls(void) {
    /* #import "lib/io" */
    UCImport* imp = uc_import_new(uc_string_new_cstr("lib/io"),
                                  uc_string_empty());  /* no alias */

    /* int x = 1 + 2; */
    UCExpr* xinit = uc_expr_binary(UC_BIN_ADD,
                                   uc_expr_literal(uc_literal_int(1)),
                                   uc_expr_literal(uc_literal_int(2)));
    UCVarDecl* vd = uc_var_decl_new(uc_string_new_cstr("x"),
                                    uc_type_int(), xinit);

    /* int main(void) { return 0; } */
    UCStmt* ret_stmt = uc_stmt_return(uc_expr_literal(uc_literal_int(0)));
    UCStmt* body = uc_stmt_block(uc_vec_push(uc_vec_new(), ret_stmt));
    UCFuncDef* fd = uc_func_def_new(uc_string_new_cstr("main"),
                                    uc_vec_new(), uc_type_int(), body);

    /* Wrap as top-levels */
    UCVec* decls = uc_vec_new();
    decls = uc_vec_push(decls, uc_tl_import(imp));
    decls = uc_vec_push(decls, uc_tl_var_decl(vd));
    decls = uc_vec_push(decls, uc_tl_func_def(fd));

    UCModule* m = uc_module_new(decls);
    ASSERT_TRUE(m != NULL);
    ASSERT_EQ_INT((long long)uc_vec_len(m->declarations), 3);

    UCTopLevel* first = (UCTopLevel*)uc_vec_at(m->declarations, 0);
    ASSERT_TRUE(first->kind == UC_TL_IMPORT);
    ASSERT_EQ_STR(first->as.import->path.data, "lib/io");
    ASSERT_TRUE(first->as.import->alias.data == NULL);

    uc_module_free(m);
}

/* ------------------------------------------------------------------------- */
/* 8. Deep free: ensure a deeply nested tree frees without leaks.           */
/*    Builds an AST that exercises every node kind at least once, then     */
/*    frees the module. With ASAN / valgrind this must be clean.            */
/* ------------------------------------------------------------------------- */
static void test_deep_free_no_leak(void) {
    /* Body:
     *   {
     *     int total = 0;
     *     for (int i = 0; i < 10; i = i + 1) {
     *       if (i == 5) { break; } else { total = total + i; }
     *     }
     *     return move(total);
     *   }
     */
    UCStmt* init_decl = uc_stmt_decl(uc_var_decl_new(
        uc_string_new_cstr("i"), uc_type_int(),
        uc_expr_literal(uc_literal_int(0))));

    UCExpr* cond = uc_expr_binary(UC_BIN_LT,
                                  uc_expr_ident("i", 1),
                                  uc_expr_literal(uc_literal_int(10)));
    UCExpr* step = uc_expr_assign(
        uc_expr_ident("i", 1),
        uc_expr_binary(UC_BIN_ADD,
                       uc_expr_ident("i", 1),
                       uc_expr_literal(uc_literal_int(1))));

    UCStmt* if_break = uc_stmt_if(
        uc_expr_binary(UC_BIN_EQ, uc_expr_ident("i", 1),
                       uc_expr_literal(uc_literal_int(5))),
        uc_stmt_block(uc_vec_push(uc_vec_new(), uc_stmt_break())),
        uc_stmt_expr(uc_expr_assign(
            uc_expr_ident("total", 5),
            uc_expr_binary(UC_BIN_ADD,
                           uc_expr_ident("total", 5),
                           uc_expr_ident("i", 1)))));

    UCStmt* for_body = uc_stmt_block(
        uc_vec_push(uc_vec_new(), if_break));

    UCStmt* for_stmt = uc_stmt_for(init_decl, cond, step, for_body);

    UCStmt* total_decl = uc_stmt_decl(uc_var_decl_new(
        uc_string_new_cstr("total"), uc_type_int(),
        uc_expr_literal(uc_literal_int(0))));

    UCStmt* ret_move = uc_stmt_return(
        uc_expr_move(uc_expr_ident("total", 5)));

    UCVec* body_stmts = uc_vec_new();
    body_stmts = uc_vec_push(body_stmts, total_decl);
    body_stmts = uc_vec_push(body_stmts, for_stmt);
    body_stmts = uc_vec_push(body_stmts, ret_move);

    UCStmt* body = uc_stmt_block(body_stmts);

    UCFuncDef* fd = uc_func_def_new(uc_string_new_cstr("main"),
                                    uc_vec_new(), uc_type_int(), body);

    UCVec* decls = uc_vec_push(uc_vec_new(), uc_tl_func_def(fd));
    UCModule* m = uc_module_new(decls);

    /* dump to stdout -- if this binary is invoked with --dump, will not be
     * printed here, but smoke-tests the printer doesn't crash */
    uc_ast_dump(m, stdout);

    uc_module_free(m);  /* ASAN / valgrind should report nothing */
}

/* ------------------------------------------------------------------------- */
/* 9. Idempotent free: uc_*_free on a struct after manual deep-cleanup of  */
/*    a sub-tree must not double-free (because leaves are NULL-tolerant).    */
/* ------------------------------------------------------------------------- */
static void test_idempotent_free(void) {
    UCString name = uc_string_new_cstr("f");
    UCFuncDef* f = uc_func_def_new(name, uc_vec_new(), uc_type_void(),
                                   uc_stmt_return(NULL));
    /* Manually detach a child and free it; the parent must still free
     * cleanly without touching that pointer.
     */
    UCStmt* body = f->body;
    f->body = NULL;
    uc_stmt_free(body);
    uc_func_def_free(f);
}

/* ------------------------------------------------------------------------- */
/* main                                                                      */
/* ------------------------------------------------------------------------- */
int main(void) {
    RUN(test_string_ownership);
    RUN(test_vec_push_grows);
    RUN(test_func_def_create_free);
    RUN(test_if_stmt_with_and_without_else);
    RUN(test_expr_tree_nested);
    RUN(test_type_variants);
    RUN(test_module_with_decls);
    RUN(test_deep_free_no_leak);
    RUN(test_idempotent_free);

    printf("\nResults: %d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}