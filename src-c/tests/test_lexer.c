/* UltraCPP C compiler - lexer unit tests (Phase 1)
 *
 * Minimal smoke tests. Real verification is via tools/tokenize_test.sh
 * comparing against the Rust compiler's output.
 */
#include "uc_lexer.h"
#include "uc_token.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_EQ(a, b) do { \
    tests_run++; \
    long long _a = (long long)(a); \
    long long _b = (long long)(b); \
    if (_a == _b) { tests_passed++; } \
    else { fprintf(stderr, "FAIL %s:%d: expected %lld got %lld\n", \
                   __FILE__, __LINE__, _b, _a); } \
} while (0)

static void tokenize_and_assert(const char* src, const char* expected_kinds[]) {
    UCError err;
    uc_error_init(&err);
    UCLexer lex;
    uc_lexer_init(&lex, src, strlen(src), "<test>", &err);

    for (int i = 0; expected_kinds[i] != NULL; i++) {
        UCToken tok = uc_lexer_next(&lex);
        if (strcmp(uc_token_kind_name(tok.kind), expected_kinds[i]) != 0) {
            fprintf(stderr, "FAIL: at index %d expected %s got %s (lexeme='%s')\n",
                    i, expected_kinds[i], uc_token_kind_name(tok.kind),
                    tok.lexeme ? tok.lexeme : "");
            tests_run++;
        } else {
            tests_passed++;
            tests_run++;
        }
        uc_token_free(&tok);
    }
}

static void test_keywords(void) {
    const char* src = "if else while for return struct const void true false null";
    const char* expected[] = {
        "KwIf", "KwElse", "KwWhile", "KwFor", "KwReturn",
        "KwStruct", "KwConst", "KwVoid", "KwTrue", "KwFalse",
        "KwNull", "Eof", NULL
    };
    tokenize_and_assert(src, expected);
}

static void test_operators(void) {
    const char* src = "+ - * / % = == != < > <= >= && || ! & | ^ ~ << >> -> ?";
    const char* expected[] = {
        "OpPlus", "OpMinus", "OpStar", "OpSlash", "OpPercent",
        "OpAssign", "OpEq", "OpNe", "OpLt", "OpGt", "OpLe", "OpGe",
        "OpAnd", "OpOr", "OpNot", "OpBitAnd", "OpBitOr", "OpBitXor",
        "OpBitNot", "OpShl", "OpShr", "OpArrow", "OpQuestion",
        "Eof", NULL
    };
    tokenize_and_assert(src, expected);
}

static void test_delimiters(void) {
    const char* src = "( ) { } [ ] , ; : . #";
    const char* expected[] = {
        "LParen", "RParen", "LBrace", "RBrace",
        "LBracket", "RBracket", "Comma", "Semicolon", "Colon", "Dot",
        "Pound", "Eof", NULL
    };
    tokenize_and_assert(src, expected);
}

static void test_numbers(void) {
    const char* src = "42 0x1F 0b1010 3.14";
    const char* expected[] = {
        "Int", "Int", "Int", "Float", "Eof", NULL
    };
    tokenize_and_assert(src, expected);
}

static void test_identifiers(void) {
    const char* src = "foo _bar my$var Hello";
    const char* expected[] = {
        "Ident", "Ident", "Ident", "Ident", "Eof", NULL
    };
    tokenize_and_assert(src, expected);
}

static void test_string_literal(void) {
    UCError err;
    uc_error_init(&err);
    UCLexer lex;
    const char* src = "\"hello\\nworld\"";
    uc_lexer_init(&lex, src, strlen(src), "<test>", &err);

    UCToken tok = uc_lexer_next(&lex);
    tests_run++;
    if (tok.kind == UC_TOK_STRING &&
        tok.as.string_val &&
        strcmp(tok.as.string_val, "hello\nworld") == 0) {
        tests_passed++;
    } else {
        fprintf(stderr, "FAIL: string literal mismatch (got '%s')\n",
                tok.as.string_val ? tok.as.string_val : "");
    }
    uc_token_free(&tok);
}

static void test_comments(void) {
    const char* src = "/* block */ foo // line\n bar";
    UCError err;
    uc_error_init(&err);
    UCLexer lex;
    uc_lexer_init(&lex, src, strlen(src), "<test>", &err);

    UCToken t1 = uc_lexer_next(&lex);  /* foo */
    UCToken t2 = uc_lexer_next(&lex);  /* bar */
    UCToken t3 = uc_lexer_next(&lex);  /* EOF */
    tests_run++;
    if (t1.kind == UC_TOK_IDENT && strcmp(t1.lexeme, "foo") == 0) tests_passed++;
    else fprintf(stderr, "FAIL: t1 expected foo, got %s\n", t1.lexeme ? t1.lexeme : "");
    tests_run++;
    if (t2.kind == UC_TOK_IDENT && strcmp(t2.lexeme, "bar") == 0) tests_passed++;
    else fprintf(stderr, "FAIL: t2 expected bar, got %s\n", t2.lexeme ? t2.lexeme : "");
    tests_run++;
    if (t3.kind == UC_TOK_EOF) tests_passed++;
    else fprintf(stderr, "FAIL: t3 expected EOF\n");
    uc_token_free(&t1);
    uc_token_free(&t2);
    uc_token_free(&t3);
}

static void test_preprocessor(void) {
    const char* src = "#import #include #define #ifdef #ifndef #endif";
    const char* expected[] = {
        "PpImport", "PpInclude", "PpDefine", "PpIfdef", "PpIfndef", "PpEndif",
        "Eof", NULL
    };
    tokenize_and_assert(src, expected);
}

/* ------------------------------------------------------------------------- */
/* [0.3.5 commit 14e] M1 keywords (shared / __thread / move_to_thread /      */
/* typedef). Lexer 阶段识别，parser/codegen 暂不消费。每个 keyword 一个       */
/* 独立 test，确保 token enum + name lookup 双向一致；末尾加 1 个 combined   */
/* test 验证 4 个 keyword 同输入下顺序均产生对应 KW_* token（mirror 既有      */
/* test_keywords 风格）。 compound_assign 属于 14f，不在本 commit 范围。      */
/* ------------------------------------------------------------------------- */

static void assert_single_keyword(const char* src, UCTokenKind expected,
                                  const char* expected_kind_name) {
    UCError err;
    uc_error_init(&err);
    UCLexer lex;
    uc_lexer_init(&lex, src, strlen(src), "<test>", &err);
    UCToken tok = uc_lexer_next(&lex);
    UCToken eof = uc_lexer_next(&lex);
    tests_run++;
    if (tok.kind == expected &&
        tok.lexeme != NULL &&
        strcmp(tok.lexeme, src) == 0 &&
        eof.kind == UC_TOK_EOF) {
        tests_passed++;
    } else {
        fprintf(stderr,
                "FAIL: '%s' expected kind=%s got kind=%s lexeme='%s' eof=%s\n",
                src, expected_kind_name, uc_token_kind_name(tok.kind),
                tok.lexeme ? tok.lexeme : "",
                uc_token_kind_name(eof.kind));
    }
    uc_token_free(&tok);
    uc_token_free(&eof);
}

static void test_lexer_keyword_shared(void) {
    assert_single_keyword("shared", UC_TOK_KW_SHARED, "KwShared");
}

static void test_lexer_keyword___thread(void) {
    assert_single_keyword("__thread", UC_TOK_KW___THREAD, "KwThread");
}

static void test_lexer_keyword_move_to_thread(void) {
    assert_single_keyword("move_to_thread", UC_TOK_KW_MOVE_TO_THREAD,
                          "KwMoveToThread");
}

static void test_lexer_keyword_typedef(void) {
    assert_single_keyword("typedef", UC_TOK_KW_TYPEDEF, "KwTypedef");
}

/* 4 M1 keywords 同输入 — 验证 lexer 顺序识别 + 互不冲突（不命中已存
 * keywords 任何前缀）。例如 '__thread' 不被 'true' 等抢先识别。
 * 单次 ASSERT：循环收集所有 4 个 KW_* token 后一次断言总和 == 4。 */
static void test_lexer_keyword_m1_combined(void) {
    UCError err;
    uc_error_init(&err);
    UCLexer lex;
    const char* src = "shared __thread move_to_thread typedef";
    uc_lexer_init(&lex, src, strlen(src), "<test>", &err);

    UCTokenKind expected[] = {
        UC_TOK_KW_SHARED, UC_TOK_KW___THREAD,
        UC_TOK_KW_MOVE_TO_THREAD, UC_TOK_KW_TYPEDEF
    };
    int matches = 0;
    UCToken tok;
    for (int i = 0; i < 4; i++) {
        tok = uc_lexer_next(&lex);
        if (tok.kind == expected[i]) matches++;
        uc_token_free(&tok);
    }
    UCToken eof = uc_lexer_next(&lex);
    tests_run++;
    if (matches == 4 && eof.kind == UC_TOK_EOF) {
        tests_passed++;
    } else {
        fprintf(stderr,
                "FAIL: m1 combined expected 4 KW_* got %d, eof=%s\n",
                matches, uc_token_kind_name(eof.kind));
    }
    uc_token_free(&eof);
}

int main(void) {
    test_keywords();
    test_operators();
    test_delimiters();
    test_numbers();
    test_identifiers();
    test_string_literal();
    test_comments();
    test_preprocessor();
    test_lexer_keyword_shared();
    test_lexer_keyword___thread();
    test_lexer_keyword_move_to_thread();
    test_lexer_keyword_typedef();
    test_lexer_keyword_m1_combined();

    printf("\nResults: %d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}