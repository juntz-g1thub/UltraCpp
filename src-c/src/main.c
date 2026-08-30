/* UltraCPP C compiler - CLI entry (Phase 1+: lexer/parser)
 *
 * Phase 1 only supported --tokens. Phase 2.6 adds --ast so that we can
 * byte-level-diff the C parser's AST against the Rust compiler's
 * `--dump-ast` output (see tools/ast_test.sh).
 */
#define _POSIX_C_SOURCE 200809L  /* for getcwd() in path_absolute() */

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
#include <unistd.h>  /* getcwd for path_absolute() */

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

/* Extract module name (= file_stem of input path) into out buffer.
 * out_cap includes the trailing NUL. */
static void extract_module_name(const char* path, char* out, size_t out_cap) {
    const char* base = path;
    for (const char* p = path; *p; p++) {
        if (*p == '/' || *p == '\\') base = p + 1;
    }
    size_t blen = strlen(base);
    if (blen >= out_cap) blen = out_cap - 1;
    memcpy(out, base, blen);
    out[blen] = '\0';
    for (int i = (int)blen - 1; i >= 0; i--) {
        if (out[i] == '.') { out[i] = '\0'; break; }
    }
}

/* Read file into a heap-allocated buffer.  Returns NULL on error. */
static char* slurp_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", path);
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);

    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);
    if (out_len) *out_len = n;
    return buf;
}

/* ------------------------------------------------------------------------- */
/* [0.3.5 commit 14c] Source-copy `#include` preprocessor.                   */
/*                                                                           */
/* Scans a buffer for `#include "path"` lines, reads each referenced file, */
/* and inlines its contents into a new buffer that replaces the original.  */
/* Recursively expands nested includes with a "seen" set for cycle         */
/* protection. Runs BEFORE lexing (the parser-side `parse_pound_include`  */
/* just consumes the directive line; the real work is the file copy).    */
/*                                                                           */
/* Path resolution: include paths are relative to the directory of the    */
/* file that wrote the #include line (NOT cwd). This matches the C        */
/* preprocessor convention and matches UltraCPP's intended semantics     */
/* (the baseline m0_40 file lives in test/programs/baseline/ and its     */
/* relative include `../../lib/math.uc` resolves there).                  */
/* ------------------------------------------------------------------------- */

typedef struct {
    char** paths;   /* owned heap array of normalized absolute paths */
    size_t len;
    size_t cap;
} IncludeSet;

static int includeset_contains(const IncludeSet* s, const char* path) {
    if (!s || !path) return 0;
    for (size_t i = 0; i < s->len; i++) {
        if (s->paths[i] && strcmp(s->paths[i], path) == 0) return 1;
    }
    return 0;
}

static void includeset_add(IncludeSet* s, char* path) {
    if (!s || !path) return;
    if (s->len >= s->cap) {
        size_t new_cap = s->cap == 0 ? 8 : s->cap * 2;
        char** new_data = (char**)realloc(s->paths, new_cap * sizeof(char*));
        if (!new_data) { free(path); return; }  /* best-effort: leak */
        s->paths = new_data;
        s->cap = new_cap;
    }
    s->paths[s->len++] = path;
}

static void includeset_free(IncludeSet* s) {
    if (!s) return;
    for (size_t i = 0; i < s->len; i++) free(s->paths[i]);
    free(s->paths);
    s->paths = NULL;
    s->len = 0;
    s->cap = 0;
}

/* Append `len` bytes from `src` to a NUL-terminated heap buffer `*buf`,
 * growing as needed. `*cap` is the current allocated capacity (>= strlen
 * + 1); updated in place. Returns 1 on success, 0 on OOM. */
static int strbuf_append(char** buf, size_t* len, size_t* cap,
                         const char* src, size_t n) {
    if (!buf || !len || !cap || (!src && n > 0)) return 0;
    size_t need = *len + n + 1;
    if (need > *cap) {
        size_t new_cap = *cap == 0 ? 256 : *cap;
        while (new_cap < need) new_cap *= 2;
        char* nb = (char*)realloc(*buf, new_cap);
        if (!nb) return 0;
        *buf = nb;
        *cap = new_cap;
    }
    if (n > 0) memcpy(*buf + *len, src, n);
    *len += n;
    (*buf)[*len] = '\0';
    return 1;
}

/* Extract the directory portion of a file path into `out` (NUL-termed).
 * Examples:
 *   "foo.uc"            -> "."
 *   "sub/foo.uc"        -> "sub"
 *   "/a/b/foo.uc"       -> "/a/b"
 * out_cap is the capacity of out (including NUL). */
static void path_dirname(const char* path, char* out, size_t out_cap) {
    if (!path || out_cap == 0) { if (out_cap) out[0] = '\0'; return; }
    size_t plen = strlen(path);
    /* Strip any trailing slashes. */
    while (plen > 0 && (path[plen - 1] == '/' || path[plen - 1] == '\\')) {
        plen--;
    }
    /* Find the last separator. */
    long last = -1;  /* signed so we can use -1 sentinel */
    for (long i = (long)plen - 1; i >= 0; i--) {
        if (path[i] == '/' || path[i] == '\\') { last = i; break; }
    }
    if (last < 0) {
        /* No separator: dir is ".". */
        if (out_cap >= 2) { out[0] = '.'; out[1] = '\0'; }
        else if (out_cap > 0) { out[0] = '\0'; }
        return;
    }
    if (last == 0) {
        /* Root: "/". */
        if (out_cap >= 2) { out[0] = '/'; out[1] = '\0'; }
        else if (out_cap > 0) { out[0] = '\0'; }
        return;
    }
    /* Strip trailing slashes between 0 and last. */
    while (last > 0 && (path[last - 1] == '/' || path[last - 1] == '\\')) {
        last--;
    }
    size_t dlen = (size_t)last;
    if (dlen >= out_cap) dlen = out_cap - 1;
    memcpy(out, path, dlen);
    out[dlen] = '\0';
}

/* Forward decl: defined later in this file. */
static char* local_strdup(const char* s);

/* Resolve a relative include path against a base directory using simple
 * lexical `..` and `.` resolution. `out` receives a heap-allocated
 * NUL-terminated path the caller frees. Returns NULL on error. */
static char* path_join(const char* base_dir, const char* rel) {
    if (!base_dir || !rel) return NULL;
    /* If `rel` is absolute, use it directly. */
    if (rel[0] == '/' || rel[0] == '\\') {
        return local_strdup(rel);
    }
    size_t base_len = strlen(base_dir);
    size_t rel_len  = strlen(rel);
    size_t total = base_len + 1 + rel_len + 1;
    char* tmp = (char*)malloc(total);
    if (!tmp) return NULL;
    if (base_len == 0 || (base_len == 1 && base_dir[0] == '.')) {
        tmp[0] = '\0';
    } else {
        /* No trailing '/': a leading `..` segment must pop a real path
         * component, not just the separator. */
        memcpy(tmp, base_dir, base_len);
        tmp[base_len] = '\0';
    }
    /* Append rel, splitting by '/' or '\'. */
    size_t out_len = strlen(tmp);
    const char* p = rel;
    while (*p) {
        /* Read one segment. */
        const char* seg = p;
        while (*p && *p != '/' && *p != '\\') p++;
        size_t seg_len = (size_t)(p - seg);
        /* Skip empty segments (e.g. "//") and "/./". */
        if (seg_len == 0
            || (seg_len == 1 && seg[0] == '.')) {
            /* skip */
        } else if (seg_len == 2 && seg[0] == '.' && seg[1] == '.') {
            /* Pop last segment from tmp, if any. */
            if (out_len == 0) { /* nothing to pop: error */
                free(tmp); return NULL;
            }
            /* Find last '/' in tmp. */
            size_t i = out_len;
            while (i > 0 && tmp[i - 1] != '/') i--;
            if (i == 0) {
                tmp[0] = '\0';
                out_len = 0;
            } else {
                tmp[i - 1] = '\0';
                out_len = i - 1;
            }
        } else {
            /* Append segment (adding a separator unless one is implied). */
            size_t sep = (out_len > 0 && tmp[out_len - 1] != '/') ? 1 : 0;
            if (out_len + sep + seg_len + 1 > total) {
                total = (out_len + sep + seg_len + 1) * 2;
                char* nb = (char*)realloc(tmp, total);
                if (!nb) { free(tmp); return NULL; }
                tmp = nb;
            }
            if (sep) tmp[out_len] = '/';
            memcpy(tmp + out_len + sep, seg, seg_len);
            out_len += sep + seg_len;
            tmp[out_len] = '\0';
        }
        if (*p == '/' || *p == '\\') p++;
    }
    if (out_len == 0) {
        /* Result is empty; fall back to ".". */
        free(tmp);
        return local_strdup(".");
    }
    return tmp;
}

/* Forward decl; defined below. */
static char* expand_includes(char* buf, const char* base_dir,
                             IncludeSet* seen);

/* Local strdup wrapper: POSIX `strdup` requires _POSIX_C_SOURCE on
 * strict C99 compilers; rolling our own keeps the file platform-neutral. */
static char* local_strdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char* p = (char*)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

/* [block moved below: read_text_file follows after local_strdup.] */

/* Read a file from `path` and return its content as a heap-allocated
 * NUL-terminated string (caller frees). *out_len gets the content length
 * excluding the NUL. Returns NULL on error. */static char* read_text_file(const char* path, size_t* out_len) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return NULL; }
    rewind(f);
    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);
    if (out_len) *out_len = n;
    return buf;
}

/* Scan `buf` for `#include "path"` (and the rare `#include <path>`
 * form, which we currently emit as an error since UltraCPP has no
 * system include roots). Inlines source for double-quoted includes.
 * Returns a NEW NUL-terminated buffer the caller frees; the original
 * `buf` is NOT freed by this function. */
static char* expand_includes(char* buf, const char* base_dir,
                             IncludeSet* seen) {
    if (!buf) return NULL;
    char* out = NULL;
    size_t out_len = 0;
    size_t out_cap = 0;

    const char* p = buf;
    const char* end = buf + strlen(buf);
    while (p < end) {
        /* Find next '#' at start of line (skipping whitespace). */
        /* Skip leading whitespace on this line. */
        while (p < end && (*p == ' ' || *p == '\t')) p++;
        int has_include = 0;
        if (p < end && *p == '#') {
            const char* hash = p;
            const char* after_hash = p + 1;
            /* Optional whitespace between # and name. */
            while (after_hash < end
                   && (*after_hash == ' ' || *after_hash == '\t')) {
                after_hash++;
            }
            static const char incl_str[] = "include";
            if ((size_t)(end - after_hash) >= sizeof(incl_str) - 1
                && memcmp(after_hash, incl_str, sizeof(incl_str) - 1) == 0) {
                const char* after_name = after_hash + (sizeof(incl_str) - 1);
                /* Must be followed by whitespace, EOF, or punctuation. */
                int boundary = (after_name >= end)
                    || *after_name == ' '
                    || *after_name == '\t'
                    || *after_name == '\n'
                    || *after_name == '\r'
                    || *after_name == '"'
                    || *after_name == '<'
                    || *after_name == ';';
                if (boundary) {
                    has_include = 1;
                    p = after_name;
                }
            }
            (void)hash;  /* suppress unused warning */
        }

        if (has_include) {
            /* Skip whitespace. */
            while (p < end && (*p == ' ' || *p == '\t')) p++;
            char quote = 0;
            if (p < end && (*p == '"' || *p == '<')) {
                quote = *p;
                p++;
            }
            /* Read the path until matching close. */
            char close_char = (quote == '"') ? '"' : '>';
            const char* path_start = p;
            while (p < end && *p != close_char && *p != '\n' && *p != '\r') p++;
            if (p >= end || *p != close_char) {
                fprintf(stderr,
                        "uc_lexer: unterminated #include path\n");
                /* Best-effort: drop the rest of the line. */
                while (p < end && *p != '\n') p++;
                if (p < end) p++;  /* consume '\n' */
                continue;
            }
            size_t plen = (size_t)(p - path_start);
            char* inc_path = (char*)malloc(plen + 1);
            if (!inc_path) { /* OOM: drop */ plen = 0; inc_path = (char*)""; }
            if (plen > 0) memcpy(inc_path, path_start, plen);
            inc_path[plen] = '\0';
            p++;  /* consume close char */

            /* Optional 'as IDENT' (we ignore the alias). */
            while (p < end && (*p == ' ' || *p == '\t')) p++;
            if ((size_t)(end - p) >= 3
                && p[0] == 'a' && p[1] == 's'
                && (p[2] == ' ' || p[2] == '\t')) {
                p += 3;
                while (p < end && (*p == ' ' || *p == '\t')) p++;
                while (p < end
                       && (*p != ' ' && *p != '\t'
                           && *p != '\n' && *p != '\r'
                           && *p != ';')) {
                    p++;
                }
            }
            /* Optional trailing ';'. */
            while (p < end && (*p == ' ' || *p == '\t')) p++;
            if (p < end && *p == ';') p++;
            /* Consume rest of line. */
            while (p < end && *p != '\n') p++;
            if (p < end) p++;  /* consume '\n' */

            if (quote == '"') {
                /* Resolve relative to base_dir and inline contents. */
                char* resolved = path_join(base_dir, inc_path);
                if (!resolved) {
                    fprintf(stderr,
                            "uc_lexer: cannot resolve #include path '%s'\n",
                            inc_path);
                    free(inc_path);
                    continue;
                }
                /* Cycle protection. */
                if (includeset_contains(seen, resolved)) {
                    /* Already inlined: emit nothing. */
                    free(inc_path);
                    free(resolved);
                    continue;
                }
                size_t sub_len = 0;
                char* sub = read_text_file(resolved, &sub_len);
                if (!sub) {
                    fprintf(stderr,
                            "uc_lexer: cannot read #include file '%s'\n",
                            resolved);
                    free(inc_path);
                    free(resolved);
                    continue;
                }
                /* Track normalized path before recursing so transitive
                 * cycles are blocked. */
                char* normalized = local_strdup(resolved);
                includeset_add(seen, normalized);
                /* Recursive expansion against the included file's dir. */
                char sub_dir[1024];
                path_dirname(resolved, sub_dir, sizeof(sub_dir));
                char* exp_sub = expand_includes(sub, sub_dir, seen);
                if (exp_sub) {
                    /* Best-effort ownership hand-off: expand_includes
                     * always returns a NEW buffer (never `sub`). */
                    size_t exp_len = strlen(exp_sub);
                    if (!strbuf_append(&out, &out_len, &out_cap,
                                       exp_sub, exp_len)) {
                        fprintf(stderr, "uc_lexer: OOM in include expansion\n");
                    }
                    free(exp_sub);
                }
                free(sub);
                free(inc_path);
                free(resolved);
            } else if (quote == '<') {
                fprintf(stderr,
                        "uc_lexer: angle-bracket #include <%s> "
                        "is not supported\n", inc_path);
                free(inc_path);
                continue;
            } else {
                /* No quoting: bare `#include` with no path — skip line. */
                free(inc_path);
                continue;
            }
            continue;  /* included line replaced; don't fall through to copy */
        }

        /* Not an include line: copy up to (and including) the next '\n'. */
        const char* nl = (const char*)memchr(p, '\n',
                                             (size_t)(end - p));
        size_t copy_len;
        if (nl) {
            copy_len = (size_t)(nl - p) + 1;
        } else {
            copy_len = (size_t)(end - p);
        }
        if (!strbuf_append(&out, &out_len, &out_cap, p, copy_len)) {
            fprintf(stderr, "uc_lexer: OOM in include expansion\n");
            free(out);
            return NULL;
        }
        p += copy_len;
    }

    if (!out) {
        /* buf had no includes at all — return a fresh copy so the
         * caller's free(out) / free(buf) protocol is uniform. */
        size_t in_len = strlen(buf);
        out = (char*)malloc(in_len + 1);
        if (!out) return NULL;
        memcpy(out, buf, in_len + 1);
        return out;
    }
    return out;
}

/* Convert `src_path` to an absolute path. If it is already absolute,
 * returns local_strdup(src_path). If relative, prepends the current
 * working directory. Caller frees the result. Returns NULL on OOM. */
static char* path_absolute(const char* src_path) {
    if (!src_path) return NULL;
    if (src_path[0] == '/' || src_path[0] == '\\') {
        return local_strdup(src_path);
    }
    char cwd[1024];
    if (!getcwd(cwd, sizeof(cwd))) return local_strdup(src_path);
    size_t cwd_len = strlen(cwd);
    /* Drop a leading "./" on src_path to avoid cwd/./foo. */
    const char* rel = src_path;
    if (rel[0] == '.' && (rel[1] == '/' || rel[1] == '\\')) rel += 2;
    size_t rel_len = strlen(rel);
    size_t total = cwd_len + 1 + rel_len + 1;
    char* out = (char*)malloc(total);
    if (!out) return NULL;
    memcpy(out, cwd, cwd_len);
    out[cwd_len] = '/';
    memcpy(out + cwd_len + 1, rel, rel_len);
    out[cwd_len + 1 + rel_len] = '\0';
    return out;
}

/* Public entry point: walk `buf` (which is the source for `src_path`)
 * expanding any `#include` directives. Returns a NEW NUL-terminated
 * buffer the caller frees, or a fresh copy of `buf` if no includes
 * were found. `*inout_len` is updated to the new length. */
static char* preprocess_includes(const char* src_path, char* buf,
                                 size_t* inout_len) {
    if (!buf) return NULL;
    char* abs_src = path_absolute(src_path);
    const char* dir_src = abs_src ? abs_src : src_path;
    char base_dir[1024];
    path_dirname(dir_src, base_dir, sizeof(base_dir));
    IncludeSet seen = {NULL, 0, 0};
    char* expanded = expand_includes(buf, base_dir, &seen);
    includeset_free(&seen);
    if (!expanded) {
        /* OOM: keep original buffer, leave length unchanged. */
        if (inout_len) *inout_len = strlen(buf);
        return buf;
    }
    if (inout_len) *inout_len = strlen(expanded);
    return expanded;
}

/* Walk source looking for `#import "..."` or `#import <...>` lines and
 * register each path with the codegen (replicates Rust's preprocessor
 * import extraction). */
static void scan_imports(const char* buf, UCCodeGenerator* g) {
    const char* p_scan = buf;
    while ((p_scan = strstr(p_scan, "#import")) != NULL) {
        const char* line_start = p_scan;
        const char* eol = strchr(p_scan, '\n');
        size_t llen = eol ? (size_t)(eol - line_start) : strlen(line_start);
        const char* q1 = (const char*)memchr(line_start, '"', llen);
        const char* lt1 = (const char*)memchr(line_start, '<', llen);
        if (q1 && q1 < line_start + llen) {
            const char* q2 = (const char*)memchr(q1 + 1, '"',
                llen - (size_t)(q1 - line_start) - 1);
            if (q2) {
                size_t plen = (size_t)(q2 - q1 - 1);
                char* ipath = (char*)malloc(plen + 1);
                memcpy(ipath, q1 + 1, plen);
                ipath[plen] = '\0';
                uc_codegen_add_imported_module(g, ipath);
                free(ipath);
            }
        } else if (lt1 && lt1 < line_start + llen) {
            const char* gt1 = (const char*)memchr(lt1 + 1, '>',
                llen - (size_t)(lt1 - line_start) - 1);
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
}

/* Run lex + parse + codegen on `src_path`.  Returns the IR as a
 * heap-allocated NUL-terminated string the caller frees, or NULL on
 * error.  On error, prints to stderr.  `keep_obj_out` is set to either
 * the module-name stem (for further assembly) or NULL if unknown.
 * [0.3.5 commit 14d] `skip_builtins` is 1 when compiling an imported TU
 * for a multi-TU build: the codegen omits the builtin decls/defs and
 * treats all functions in this TU as non-local (bare-name mangling). */
static char* generate_ir(const char* src_path, const char* module_name,
                         int skip_builtins,
                         char* err_buf, size_t err_buf_cap) {
    (void)err_buf; (void)err_buf_cap;
    size_t src_len = 0;
    char* buf = slurp_file(src_path, &src_len);
    if (!buf) return NULL;

    /* [0.3.5 commit 14c] Source-copy #include preprocessor. Runs BEFORE
     * lexing — the lexer is fed the expanded buffer; the parser side
     * never sees the #include directive (parse_pound_include() exists
     * for the rare at-test-time case where the test feeds a raw source
     * with #include directly into parse_source()). */
    char* expanded = preprocess_includes(src_path, buf, &src_len);
    if (expanded != buf) {
        free(buf);
        buf = expanded;
    }

    UCError err; uc_error_init(&err);
    UCLexer lex;
    uc_lexer_init(&lex, buf, src_len, src_path, &err);
    UCParser p;
    uc_parser_init(&p, &lex, &err);
    UCModule* m = uc_parser_parse(&p);
    char* ir = NULL;
    if (err.kind != UC_ERR_NONE) {
        uc_error_report(&err, stderr);
    } else {
        UCCodeGenerator* g = uc_codegen_new(module_name);
        if (skip_builtins) uc_codegen_set_skip_builtins(g, 1);
        scan_imports(buf, g);
        ir = uc_codegen_generate(g, m, &err);
        if (err.kind != UC_ERR_NONE) {
            uc_error_report(&err, stderr);
            free(ir);
            ir = NULL;
        }
        uc_codegen_free(g);
    }
    uc_module_free(m);
    uc_parser_reset(&p);
    uc_lexer_reset(&lex);
    free(buf);
    return ir;
}

static int run_emit_ll(const char* path, const char* output_path) {
    char module_name[256];
    extract_module_name(path, module_name, sizeof(module_name));

    char* ir = generate_ir(path, module_name, 0, NULL, 0);
    int rc = 0;
    if (!ir) { return 1; }

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
    free(ir);
    return rc;
}

/* Extract `#import "..."` / `#import <...>` path strings from a source
 * buffer. Returns a heap-allocated NULL-terminated array of
 * heap-allocated path strings (caller frees each entry + the array);
 * `*out_n` is the count. Returns NULL when the source contains no
 * `#import` directives. Paths retain their original relative form —
 * they have NOT been stripped to basenames (unlike
 * uc_codegen_add_imported_module, which is for codegen-side use). */
static char** extract_import_paths(const char* buf, size_t* out_n) {
    *out_n = 0;
    size_t cap = 0;
    char** arr = NULL;
    const char* p = buf;
    while ((p = strstr(p, "#import")) != NULL) {
        const char* eol = strchr(p, '\n');
        size_t llen = eol ? (size_t)(eol - p) : strlen(p);
        const char* q1  = (const char*)memchr(p, '"', llen);
        const char* lt1 = (const char*)memchr(p, '<', llen);
        const char* opener = NULL, *closer = NULL;
        if (q1) {
            const char* q2 = (const char*)memchr(q1 + 1, '"',
                llen - (size_t)(q1 - p) - 1);
            if (q2) { opener = q1; closer = q2; }
        } else if (lt1) {
            const char* gt1 = (const char*)memchr(lt1 + 1, '>',
                llen - (size_t)(lt1 - p) - 1);
            if (gt1) { opener = lt1; closer = gt1; }
        }
        if (opener && closer) {
            size_t plen = (size_t)(closer - opener - 1);
            if (*out_n >= cap) {
                cap = cap ? cap * 2 : 4;
                arr = (char**)realloc(arr, cap * sizeof(char*));
            }
            char* ip = (char*)malloc(plen + 1);
            if (!ip) { p = eol ? eol + 1 : p + llen; continue; }
            memcpy(ip, opener + 1, plen);
            ip[plen] = '\0';
            arr[(*out_n)++] = ip;
        }
        p = eol ? eol + 1 : p + llen;
    }
    return arr;
}

/* End-to-end build: generate IR, run llc, then gcc.  Temp files in
 * /tmp; cleaned up on success unless `keep` is true.
 * [0.3.5 commit 14d] Multi-TU: when the main source contains
 * `#import "path"` directives, each imported TU is compiled
 * separately (with `skip_builtins=1` to avoid duplicate @uc_abs
 * emits across the link unit) and the resulting .s files are linked
 * together with the main .s into the executable. */
#define UC_BUILD_MAX_IMPORTS 64
static int run_build(const char* path, const char* output_path, int keep) {
    char module_name[256];
    extract_module_name(path, module_name, sizeof(module_name));

    /* Default output = basename without extension, alongside input */
    char default_exe[512];
    if (!output_path) {
        snprintf(default_exe, sizeof(default_exe), "%s", module_name);
        output_path = default_exe;
    }

    /* Main source dir — used to resolve relative #imports. */
    char main_dir[1024];
    path_dirname(path, main_dir, sizeof(main_dir));

    /* Scan main source for #import paths (uses the un-preprocessed
     * source: #include is for source-copy inlining, not TU bring-up;
     * #import is the actual TU reference). */
    size_t src_len = 0;
    char* main_buf = slurp_file(path, &src_len);
    if (!main_buf) return 1;
    size_t import_n = 0;
    char** import_paths = extract_import_paths(main_buf, &import_n);
    free(main_buf);
    if (import_n > UC_BUILD_MAX_IMPORTS) {
        fprintf(stderr, "Error: too many #imports (%zu > %d)\n",
                import_n, UC_BUILD_MAX_IMPORTS);
        import_n = UC_BUILD_MAX_IMPORTS;
    }

    /* Resolve each import against main_dir. */
    char** resolved = NULL;
    if (import_n > 0) {
        resolved = (char**)calloc(import_n, sizeof(char*));
        for (size_t i = 0; i < import_n; i++) {
            resolved[i] = path_join(main_dir, import_paths[i]);
            free(import_paths[i]);
        }
        free(import_paths);
    }

    /* Temp file tracking for cleanup. imp_* are 2D char arrays so we
     * can reuse sizeof() for the per-row size. */
    char imp_ll[UC_BUILD_MAX_IMPORTS][1024];
    char imp_s[UC_BUILD_MAX_IMPORTS][1024];
    memset(imp_ll, 0, sizeof(imp_ll));
    memset(imp_s,  0, sizeof(imp_s));
    char main_ll[1024] = "";
    char main_s[1024]  = "";
    int rc = 0;
    char cmd[4096];

    /* 1) Compile each imported TU (skip_builtins=1) → .ll → llc → .s.
     * Imported TUs run with bare symbol names so the @fn declares
     * emitted by the main TU (via UC_EXPR_FIELD mod.fn) link cleanly
     * against the @fn definitions here. */
    for (size_t i = 0; i < import_n; i++) {
        char imp_name[256];
        extract_module_name(resolved[i], imp_name, sizeof(imp_name));
        snprintf(imp_ll[i], sizeof(imp_ll[i]),
                 "/tmp/uc_build_imp%zu_%s.ll", i, imp_name);
        snprintf(imp_s[i],  sizeof(imp_s[i]),
                 "/tmp/uc_build_imp%zu_%s.s",  i, imp_name);

        char* imp_ir = generate_ir(resolved[i], imp_name, 1, NULL, 0);
        if (!imp_ir) { rc = 1; goto cleanup; }
        FILE* f = fopen(imp_ll[i], "wb");
        if (!f) {
            fprintf(stderr, "Error: cannot write %s\n", imp_ll[i]);
            free(imp_ir);
            rc = 2;
            goto cleanup;
        }
        fputs(imp_ir, f);
        fclose(f);
        free(imp_ir);
        printf("Wrote IR to %s\n", imp_ll[i]);

        snprintf(cmd, sizeof(cmd), "llc %s -o %s", imp_ll[i], imp_s[i]);
        printf("Running: %s\n", cmd);
        int lrc = system(cmd);
        if (lrc != 0) {
            fprintf(stderr, "llc failed for %s (exit %d)\n",
                    resolved[i], lrc);
            rc = 1;
            goto cleanup;
        }
        printf("Compiled to %s\n", imp_s[i]);
    }

    /* 2) Compile main TU (skip_builtins=0) → .ll → llc → .s. */
    snprintf(main_ll, sizeof(main_ll), "/tmp/uc_build_%s.ll", module_name);
    snprintf(main_s,  sizeof(main_s),  "/tmp/uc_build_%s.s",  module_name);

    char* ir = generate_ir(path, module_name, 0, NULL, 0);
    if (!ir) { rc = 1; goto cleanup; }
    FILE* f = fopen(main_ll, "wb");
    if (!f) {
        fprintf(stderr, "Error: cannot write %s\n", main_ll);
        free(ir);
        rc = 2;
        goto cleanup;
    }
    fputs(ir, f);
    fclose(f);
    free(ir);
    printf("Wrote IR to %s\n", main_ll);

    snprintf(cmd, sizeof(cmd), "llc %s -o %s", main_ll, main_s);
    printf("Running: %s\n", cmd);
    int lrc = system(cmd);
    if (lrc != 0) {
        fprintf(stderr, "llc failed (exit %d)\n", lrc);
        rc = 1;
        goto cleanup;
    }
    printf("Compiled to %s\n", main_s);

    /* 3) Link: main.s + all imported .s. */
    {
        char link_cmd[8192];
        size_t off = (size_t)snprintf(link_cmd, sizeof(link_cmd),
                                      "gcc -no-pie %s", main_s);
        for (size_t i = 0; i < import_n; i++) {
            off += (size_t)snprintf(link_cmd + off, sizeof(link_cmd) - off,
                                    " %s", imp_s[i]);
        }
        off += (size_t)snprintf(link_cmd + off, sizeof(link_cmd) - off,
                                " -o %s", output_path);
        printf("Running: %s\n", link_cmd);
        int grc = system(link_cmd);
        if (grc != 0) {
            fprintf(stderr, "gcc linking failed (exit %d)\n", grc);
            rc = 1;
            goto cleanup;
        }
    }
    printf("Built executable %s\n", output_path);

cleanup:
    if (!keep) {
        for (size_t i = 0; i < import_n; i++) {
            if (imp_ll[i][0]) remove(imp_ll[i]);
            if (imp_s[i][0])  remove(imp_s[i]);
        }
        if (main_ll[0]) remove(main_ll);
        if (main_s[0])  remove(main_s);
    }
    if (resolved) {
        for (size_t i = 0; i < import_n; i++) free(resolved[i]);
        free(resolved);
    }
    return rc;
}

static void print_version(void) {
    printf("uc_lexer (UltraCPP C compiler) version %s\n", UC_VERSION_STRING);
    printf("Phase 4 build: lexer + parser + LLVM IR codegen + end-to-end build\n");
}

static void print_usage(const char* argv0) {
    fprintf(stderr,
        "Usage: %s [--tokens|--ast|--emit-ll|--build] [-o FILE] [--version] [--help] <input>\n"
        "\n"
        "Phase 4 build: lexer + parser + LLVM IR codegen + end-to-end build.\n"
        "\n"
        "Options:\n"
        "  --tokens    Tokenize the input and print one line per token (default)\n"
        "  --ast       Parse and dump the AST (byte-level comparable with Rust's\n"
        "              --dump-ast; see tools/ast_test.sh)\n"
        "  --emit-ll   Parse, generate LLVM IR, write to stdout (or to -o FILE).\n"
        "              Output is byte-level comparable with Rust's main.ll output\n"
        "              (see tools/codegen_test.sh).\n"
        "  --build     End-to-end: emit IR, run llc + gcc to produce an executable.\n"
        "              Default output is the input basename without extension; use\n"
        "              -o FILE to override. Temp .ll and .s files are created in /tmp\n"
        "              and removed on success.\n"
        "  --keep      With --build, keep the intermediate .ll and .s files.\n"
        "  -o FILE     Output file (for --emit-ll or --build)\n"
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
    int want_build = 0;
    int want_keep = 0;
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
        } else if (strcmp(argv[argi], "--build") == 0) {
            want_build = 1;
            argi++;
        } else if (strcmp(argv[argi], "--keep") == 0) {
            want_keep = 1;
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
    if (want_build) return run_build(input, output, want_keep);
    (void)want_tokens;
    return run_dump_tokens(input);
}