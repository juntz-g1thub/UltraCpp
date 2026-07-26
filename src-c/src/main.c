/* UltraCPP C compiler - CLI entry (Phase 1: lexer-only) */
#include "uc_error.h"
#include "uc_lexer.h"
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

static void print_version(void) {
    printf("uc_lexer (UltraCPP C compiler) version %s\n", UC_VERSION_STRING);
    printf("Phase 1 - lexer-only build\n");
}

static void print_usage(const char* argv0) {
    fprintf(stderr,
        "Usage: %s [--tokens] [--version] [--help] <input>\n"
        "\n"
        "Phase 1 build: lexer-only.\n"
        "\n"
        "Options:\n"
        "  --tokens    Tokenize the input and print one line per token (default)\n"
        "  --version   Print version and exit\n"
        "  --help      Print this help and exit\n",
        argv0 ? argv0 : "uc_lexer");
}

int main(int argc, char** argv) {
    int argi = 1;
    int want_version = 0;
    int want_help = 0;
    int want_tokens = 0;
    const char* input = NULL;

    while (argi < argc) {
        if (strcmp(argv[argi], "--tokens") == 0) {
            want_tokens = 1;
            argi++;
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

    (void)want_tokens;
    return run_dump_tokens(input);
}