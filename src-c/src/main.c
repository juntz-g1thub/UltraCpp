/* UltraCPP C compiler - CLI entry (Phase 1+: lexer/parser)
 *
 * Phase 1 only supported --tokens. Phase 2.6 adds --ast so that we can
 * byte-level-diff the C parser's AST against the Rust compiler's
 * `--dump-ast` output (see tools/ast_test.sh).
 */
#include "uc_ast.h"
#include "uc_codegen.h"
#include "uc_error.h"
#include "uc_lexer.h"
#include "uc_parser.h"
#include "uc_token.h"
#include "uc_version.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_token(const UCToken* tok, FILE* out) {
    fprintf(out, "TOKEN %-12s %4d:%-3d  %s",
            uc_token_kind_name(tok->kind), tok->line, tok->column,
            tok->lexeme ? tok->lexeme : "");
    switch (tok->kind) {
        case UC_TOK_INT:    fprintf(out, "  [int=%lld]",    tok->as.int_val);    break;
        case UC_TOK_FLOAT:  fprintf(out, "  [float=%g]",    tok->as.float_val);  break;
        case UC_TOK_CHAR:   fprintf(out, "  [char='%c']",   tok->as.char_val);   break;
        case UC_TOK_STRING: fprintf(out, "  [str=\"%s\"]",  tok->as.string_val ? tok->as.string_val : ""); break;
        default: break;
    }
    fputc('\n', out);
}

static int run_dump_tokens(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", path);
        return 2;
    }

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 2; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 2; }
    rewind(f);

    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return 2; }

    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);

    UCError err;
    uc_error_init(&err);

    UCLexer lex;
    uc_lexer_init(&lex, buf, n, path, &err);

    for (;;) {
        UCToken tok = uc_lexer_next(&lex);
        print_token(&tok, stdout);
        if (tok.kind == UC_TOK_EOF) {
            uc_token_free(&tok);
            break;
        }
        if (tok.kind == UC_TOK_ERROR) {
            uc_token_free(&tok);
            uc_error_report(&err, stderr);
            free(buf);
            return 1;
        }
        uc_token_free(&tok);
    }

    free(buf);
    return 0;
}

static int run_dump_ast(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", path);
        return 2;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 2; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 2; }
    rewind(f);

    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return 2; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);

    UCError err; uc_error_init(&err);
    UCLexer lex;
    uc_lexer_init(&lex, buf, n, path, &err);
    UCParser p;
    uc_parser_init(&p, &lex, &err);
    UCModule* m = uc_parser_parse(&p);
    int rc = 0;
    if (err.kind != UC_ERR_NONE) {
        uc_error_report(&err, stderr);
        rc = 1;
    } else {
        uc_ast_dump(m, stdout);
    }
    uc_module_free(m);
    uc_parser_reset(&p);
    uc_lexer_reset(&lex);
    free(buf);
    return rc;
}

static int run_emit_ll(const char* path, const char* output_path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", path);
        return 2;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 2; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 2; }
    rewind(f);

    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return 2; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);

    /* Module name = file_stem of input. */
    char module_name[256];
    const char* base = path;
    for (const char* p = path; *p; p++) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    size_t blen = strlen(base);
    if (blen >= sizeof(module_name)) blen = sizeof(module_name) - 1;
    memcpy(module_name, base, blen);
    module_name[blen] = '\0';
    /* Strip extensions */
    for (int i = (int)blen - 1; i >= 0; i--) {
        if (module_name[i] == '.') { module_name[i] = '\0'; break; }
    }

    UCError err; uc_error_init(&err);
    UCLexer lex;
    uc_lexer_init(&lex, buf, n, path, &err);
    UCParser p;
    uc_parser_init(&p, &lex, &err);
    UCModule* m = uc_parser_parse(&p);
    int rc = 0;
    char* ir = NULL;
    if (err.kind != UC_ERR_NONE) {
        uc_error_report(&err, stderr);
        rc = 1;
    } else {
        UCCodeGenerator* g = uc_codegen_new(module_name);

        /* Scan source for `#import "path"` or `#import <path>` so the
         * codegen knows which dotted-call symbols need `declare` lines,
         * matching the Rust preprocessor's import extraction. */
        const char* p_scan = buf;
        while ((p_scan = strstr(p_scan, "#import")) != NULL) {
            const char* line_start = p_scan;
            const char* eol = strchr(p_scan, '\n');
            size_t llen = eol ? (size_t)(eol - line_start) : strlen(line_start);
            /* Find the path string between matching delimiters. */
            const char* q1 = (const char*)memchr(line_start, '"', llen);
            const char* lt1 = (const char*)memchr(line_start, '<', llen);
            if (q1 && q1 < line_start + llen) {
                const char* q2 = (const char*)memchr(q1 + 1, '"', llen - (size_t)(q1 - line_start) - 1);
                if (q2) {
                    size_t plen = (size_t)(q2 - q1 - 1);
                    char* ipath = (char*)malloc(plen + 1);
                    memcpy(ipath, q1 + 1, plen);
                    ipath[plen] = '\0';
                    uc_codegen_add_imported_module(g, ipath);
                    free(ipath);
                }
            } else if (lt1 && lt1 < line_start + llen) {
                const char* gt1 = (const char*)memchr(lt1 + 1, '>', llen - (size_t)(lt1 - line_start) - 1);
                if (gt1) {
                    size_t plen = (size_t)(gt1 - lt1 - 1);
                    char* ipath = (char*)malloc(plen + 1);
                    memcpy(ipath, lt1 + 1, plen);
                    ipath[plen] = '\0';
                    uc_codegen_add_imported_module(g, ipath);
                    free(ipath);
                }
            }
            p_scan = line_start + (eol ? (size_t)(eol - line_start) + 1 : llen);
        }

        ir = uc_codegen_generate(g, m, &err);
        if (err.kind != UC_ERR_NONE) {
            uc_error_report(&err, stderr);
            rc = 1;
        } else if (ir) {
            if (output_path) {
                FILE* out = fopen(output_path, "wb");
                if (!out) {
                    fprintf(stderr, "Error: cannot open output '%s'\n", output_path);
                    rc = 2;
                } else {
                    fputs(ir, out);
                    fclose(out);
                    printf("Wrote IR to %s\n", output_path);
                }
            } else {
                fputs(ir, stdout);
            }
        }
        uc_codegen_free(g);
    }
    free(ir);
    uc_module_free(m);
    uc_parser_reset(&p);
    uc_lexer_reset(&lex);
    free(buf);
    return rc;
}

static void print_version(void) {
    printf("uc_lexer (UltraCPP C compiler) version %s\n", UC_VERSION_STRING);
    printf("Phase 3 build: lexer + parser + LLVM IR codegen\n");
}

static void print_usage(const char* argv0) {
    fprintf(stderr,
        "Usage: %s [--tokens|--ast] [--version] [--help] <input>\n"
        "\n"
        "Phase 2+ build: lexer + parser.\n"
        "\n"
        "Options:\n"
        "  --tokens    Tokenize the input and print one line per token (default)\n"
        "  --ast       Parse and dump the AST (byte-level comparable with Rust's\n"
        "              --dump-ast; see tools/ast_test.sh)\n"
        "  --emit-ll   Parse, generate LLVM IR, write to stdout (or to -o FILE).\n"
        "              Output is byte-level comparable with Rust's main.ll output\n"
        "              (see tools/codegen_test.sh).\n"
        "  -o FILE     Output file (for --emit-ll)\n"
        "  --version   Print version and exit\n"
        "  --help      Print this help and exit\n",
        argv0 ? argv0 : "uc_lexer");
}

int main(int argc, char** argv) {
    int argi = 1;
    int want_version = 0;
    int want_help = 0;
    int want_tokens = 0;
    int want_ast = 0;
    int want_emit_ll = 0;
    const char* input = NULL;
    const char* output = NULL;

    while (argi < argc) {
        if (strcmp(argv[argi], "--tokens") == 0) {
            want_tokens = 1;
            argi++;
        } else if (strcmp(argv[argi], "--ast") == 0 || strcmp(argv[argi], "-a") == 0) {
            want_ast = 1;
            argi++;
        } else if (strcmp(argv[argi], "--emit-ll") == 0 || strcmp(argv[argi], "-S") == 0) {
            want_emit_ll = 1;
            argi++;
        } else if (strcmp(argv[argi], "-o") == 0) {
            if (argi + 1 >= argc) {
                fprintf(stderr, "Error: -o requires an argument\n");
                print_usage(argv[0]);
                return 2;
            }
            output = argv[argi + 1];
            argi += 2;
        } else if (strcmp(argv[argi], "--version") == 0 || strcmp(argv[argi], "-V") == 0) {
            want_version = 1;
            argi++;
        } else if (strcmp(argv[argi], "--help") == 0 || strcmp(argv[argi], "-h") == 0) {
            want_help = 1;
            argi++;
        } else if (argv[argi][0] != '-' && !input) {
            input = argv[argi];
            argi++;
        } else {
            fprintf(stderr, "Error: unknown argument '%s'\n", argv[argi]);
            print_usage(argv[0]);
            return 2;
        }
    }

    if (want_version) { print_version(); return 0; }
    if (want_help)    { print_usage(argv[0]); return 0; }

    if (!input) {
        fprintf(stderr, "Error: no input file specified\n");
        print_usage(argv[0]);
        return 2;
    }

    if (want_ast) return run_dump_ast(input);
    if (want_emit_ll) return run_emit_ll(input, output);
    (void)want_tokens;
    return run_dump_tokens(input);
}