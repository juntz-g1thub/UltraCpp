/* UltraCPP C compiler - LLVM IR code generator implementation
 *
 * C99 port of src/codegen/generator.rs.
 *
 * Output format must be byte-identical to the Rust compiler for the 5
 * test programs (see tools/codegen_test.sh). If you change anything in
 * this file, re-run that script before committing.
 */
#include "uc_codegen.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/* Growable string buffer                                                    */
/* ------------------------------------------------------------------------- */

typedef struct Buf {
    char* data;
    size_t len;
    size_t cap;
} Buf;

static void buf_reserve(Buf* b, size_t extra) {
    size_t need = b->len + extra + 1;  /* +1 for NUL */
    if (need <= b->cap) return;
    size_t new_cap = b->cap ? b->cap * 2 : 256;
    while (new_cap < need) new_cap *= 2;
    b->data = (char*)realloc(b->data, new_cap);
    if (!b->data) { fprintf(stderr, "uc_codegen: OOM\n"); abort(); }
    b->cap = new_cap;
}

static void buf_append(Buf* b, const char* s, size_t n) {
    buf_reserve(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void buf_appends(Buf* b, const char* s) {
    buf_append(b, s, strlen(s));
}

static void buf_printf(Buf* b, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (needed < 0) { va_end(ap2); return; }
    buf_reserve(b, (size_t)needed);
    vsnprintf(b->data + b->len, b->cap - b->len, fmt, ap2);
    va_end(ap2);
    b->len += (size_t)needed;
}

static void buf_free(Buf* b) {
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

/* ------------------------------------------------------------------------- */
/* Tiny key-value map (string -> string); linear-search. Test programs      */
/* have at most a handful of entries so this is fine.                       */
/* ------------------------------------------------------------------------- */

static char* cgen_strdup(const char* s);

typedef struct MapEntry {
    char* key;
    char* value;
} MapEntry;

typedef struct Map {
    MapEntry* entries;
    size_t len;
    size_t cap;
} Map;

static void map_init(Map* m) {
    m->entries = NULL;
    m->len = m->cap = 0;
}

static void map_free(Map* m) {
    for (size_t i = 0; i < m->len; i++) {
        free(m->entries[i].key);
        free(m->entries[i].value);
    }
    free(m->entries);
    m->entries = NULL;
    m->len = m->cap = 0;
}

static const char* map_get(const Map* m, const char* key) {
    for (size_t i = 0; i < m->len; i++) {
        if (strcmp(m->entries[i].key, key) == 0) {
            return m->entries[i].value;
        }
    }
    return NULL;
}

static int map_contains(const Map* m, const char* key) {
    return map_get(m, key) != NULL;
}

static void map_put(Map* m, const char* key, const char* value) {
    for (size_t i = 0; i < m->len; i++) {
        if (strcmp(m->entries[i].key, key) == 0) {
            free(m->entries[i].value);
            m->entries[i].value = cgen_strdup(value);
            return;
        }
    }
    if (m->len == m->cap) {
        m->cap = m->cap ? m->cap * 2 : 8;
        m->entries = (MapEntry*)realloc(m->entries, m->cap * sizeof(MapEntry));
    }
    m->entries[m->len].key = cgen_strdup(key);
    m->entries[m->len].value = cgen_strdup(value);
    m->len++;
}

/* ------------------------------------------------------------------------- */
/* UCCodeGenerator                                                           */
/* ------------------------------------------------------------------------- */

struct UCCodeGenerator {
    Buf output;
    Buf extern_decls;
    Buf global_strings;

    int indent;
    int temp_counter;
    int label_counter;
    int string_counter;

    char* module_name;

    Map local_types;   /* param name -> LLVM type */
    Map local_vars;    /* var name   -> LLVM type */
    Map local_funcs;   /* func name  -> module name (for mangling) */
    Map imported_modules; /* module name -> module name */
    Map declared_externals; /* symbol -> "1" (set membership) */

    char* last_expr_type;
};

static char* cgen_strdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char* r = (char*)malloc(n + 1);
    if (!r) { fprintf(stderr, "uc_codegen: OOM\n"); abort(); }
    memcpy(r, s, n + 1);
    return r;
}

UCCodeGenerator* uc_codegen_new(const char* module_name) {
    UCCodeGenerator* g = (UCCodeGenerator*)calloc(1, sizeof(UCCodeGenerator));
    if (!g) { fprintf(stderr, "uc_codegen: OOM\n"); abort(); }
    g->module_name = cgen_strdup(module_name ? module_name : "unnamed");
    g->indent = 0;
    g->temp_counter = 0;
    g->label_counter = 0;
    g->string_counter = 0;
    g->last_expr_type = cgen_strdup("i32");
    map_init(&g->local_types);
    map_init(&g->local_vars);
    map_init(&g->local_funcs);
    map_init(&g->imported_modules);
    map_init(&g->declared_externals);
    return g;
}

void uc_codegen_free(UCCodeGenerator* g) {
    if (!g) return;
    buf_free(&g->output);
    buf_free(&g->extern_decls);
    buf_free(&g->global_strings);
    free(g->module_name);
    free(g->last_expr_type);
    map_free(&g->local_types);
    map_free(&g->local_vars);
    map_free(&g->local_funcs);
    map_free(&g->imported_modules);
    map_free(&g->declared_externals);
    free(g);
}

void uc_codegen_add_imported_module(UCCodeGenerator* g, const char* module_path) {
    /* Strip trailing .uc / .upp / slashes to get the module name. */
    char* m = cgen_strdup(module_path);
    size_t n = strlen(m);
    /* Strip suffixes */
    static const char* suffixes[] = {".uc", ".upp", NULL};
    for (int s = 0; suffixes[s]; s++) {
        size_t sl = strlen(suffixes[s]);
        if (n >= sl && strcmp(m + n - sl, suffixes[s]) == 0) {
            m[n - sl] = '\0';
            n -= sl;
        }
    }
    /* Strip trailing slashes */
    while (n > 0 && (m[n-1] == '/' || m[n-1] == '\\')) {
        m[--n] = '\0';
    }
    /* Use only the file_stem portion */
    const char* slash = strrchr(m, '/');
    const char* bslash = strrchr(m, '\\');
    const char* last = slash;
    if (bslash && (!last || bslash > last)) last = bslash;
    const char* name = last ? last + 1 : m;
    map_put(&g->imported_modules, name, name);
    free(m);
}

/* ------------------------------------------------------------------------- */
/* Name mangling                                                             */
/* ------------------------------------------------------------------------- */

static char* mangle_name(UCCodeGenerator* g, const char* name, int local) {
    if (strncmp(name, "sys$", 4) == 0) {
        size_t n = strlen(name) + 2;
        char* r = (char*)malloc(n);
        snprintf(r, n, "@%s", name);
        return r;
    }
    if (strcmp(name, "main") == 0) {
        return cgen_strdup("@main");
    }
    if (local) {
        size_t n = strlen(g->module_name) + strlen(name) + 4;
        char* r = (char*)malloc(n);
        snprintf(r, n, "@%s$%s", g->module_name, name);
        return r;
    }
    size_t n = strlen(name) + 2;
    char* r = (char*)malloc(n);
    snprintf(r, n, "@%s", name);
    return r;
}

/* ------------------------------------------------------------------------- */
/* Builtin declarations                                                      */
/* ------------------------------------------------------------------------- */

static const char* get_builtins_decls(void) {
    return
        "\n"
        "declare i64 @strlen(i8*)\n"
        "declare i64 @write(i32, i8*, i64)\n"
        "declare i64 @read(i32, i8*, i64)\n"
        "declare i8* @malloc(i64)\n"
        "declare void @free(i8*)\n";
}

/* ------------------------------------------------------------------------- */
/* Type -> LLVM type string                                                  */
/* ------------------------------------------------------------------------- */

static const char* llvm_type(UCTypeKind k) {
    switch (k) {
        case UC_TYPE_VOID: return "void";
        case UC_TYPE_BOOL:
        case UC_TYPE_CHAR:
        case UC_TYPE_INT:
        case UC_TYPE_I32:
        case UC_TYPE_UINT:
        case UC_TYPE_U32: return "i32";
        case UC_TYPE_I64:
        case UC_TYPE_U64:
        case UC_TYPE_ISIZE:
        case UC_TYPE_USIZE: return "i64";
        case UC_TYPE_F32: return "float";
        case UC_TYPE_F64: return "double";
        case UC_TYPE_POINTER:
        case UC_TYPE_MUTABLE_POINTER:
        case UC_TYPE_REF:
        case UC_TYPE_NAMED: return "i8*";
        /* I8/I16/U8/U16 collapse to i32 in this Rust port. */
        case UC_TYPE_I8:
        case UC_TYPE_I16:
        case UC_TYPE_U8:
        case UC_TYPE_U16:
            return "i32";
        /* Array/Function unsupported in this minimal port. */
        case UC_TYPE_ARRAY:
        case UC_TYPE_FUNCTION:
            return "i32";
    }
    return "i32";
}

static char* llvm_type_of(UCCodeGenerator* g, const UCType* ty) {
    (void)g;
    return cgen_strdup(llvm_type(ty->kind));
}

/* ------------------------------------------------------------------------- */
/* emit helpers                                                              */
/* ------------------------------------------------------------------------- */

static void emit_writeln(UCCodeGenerator* g, const char* s) {
    for (int i = 0; i < g->indent; i++) buf_append(&g->output, "  ", 2);
    buf_appends(&g->output, s);
    buf_append(&g->output, "\n", 1);
}

static void emit_fmt_writeln(UCCodeGenerator* g, const char* fmt, ...) {
    /* First format into a temp buffer */
    char tmp[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);
    emit_writeln(g, tmp);
}

static char* mk_temp(UCCodeGenerator* g) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%%t%d", g->temp_counter++);
    return cgen_strdup(buf);
}

static char* mk_label(UCCodeGenerator* g, const char* prefix) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%%%s_%d", prefix, g->label_counter++);
    return cgen_strdup(buf);
}

/* ------------------------------------------------------------------------- */
/* Forward decls                                                             */
/* ------------------------------------------------------------------------- */

static void gen_toplevel(UCCodeGenerator* g, const UCTopLevel* decl);
static void gen_func(UCCodeGenerator* g, const UCFuncDef* func, int exported);
static void gen_stmt(UCCodeGenerator* g, const UCStmt* stmt);
static char* gen_expr(UCCodeGenerator* g, const UCExpr* expr, UCError* err);
static void gen_syscall_call(UCCodeGenerator* g, const char* symbol,
                             const UCVec* args, const char* res, UCError* err);

/* ------------------------------------------------------------------------- */
/* Top level dispatch                                                        */
/* ------------------------------------------------------------------------- */

static void gen_toplevel(UCCodeGenerator* g, const UCTopLevel* decl) {
    switch (decl->kind) {
        case UC_TL_FUNC_DEF:
            gen_func(g, decl->as.func_def, 0);
            break;
        case UC_TL_FUNC_DECL: {
            char* mangled = mangle_name(g, decl->as.func_decl->name.data, 0);
            char* parts = cgen_strdup("");
            for (size_t i = 0; i < uc_vec_len(decl->as.func_decl->params); i++) {
                UCParam* p = (UCParam*)uc_vec_at(decl->as.func_decl->params, i);
                char* lt = llvm_type_of(g, p->ty);
                size_t cur_len = strlen(parts);
                size_t add_len = strlen(lt) + strlen(p->name.data) + 4;
                parts = (char*)realloc(parts, cur_len + add_len + 1);
                snprintf(parts + cur_len, add_len + 1,
                         "%s%s%s%s%%%s%s",
                         i == 0 ? "" : ", ",
                         lt,
                         i == 0 ? "" : " ",
                         "",  /* placeholder for `, ` */
                         p->name.data,
                         "");
                /* Use cleaner rebuild below */
                free(lt);
            }
            /* Rebuild properly to handle the comma correctly. */
            free(parts);
            parts = cgen_strdup("");
            for (size_t i = 0; i < uc_vec_len(decl->as.func_decl->params); i++) {
                UCParam* p = (UCParam*)uc_vec_at(decl->as.func_decl->params, i);
                char* lt = llvm_type_of(g, p->ty);
                size_t cur_len = strlen(parts);
                size_t add_len = strlen(lt) + strlen(p->name.data) + 5;
                parts = (char*)realloc(parts, cur_len + add_len + 1);
                snprintf(parts + cur_len, add_len + 1, "%s%s %%%s",
                         i == 0 ? "" : ", ",
                         lt, p->name.data);
                free(lt);
            }
            emit_fmt_writeln(g, "declare %s %s(%s)",
                             llvm_type(decl->as.func_decl->return_ty->kind),
                             mangled, parts);
            free(mangled);
            free(parts);
            break;
        }
        case UC_TL_EXPORT:
            if (decl->as.export_->kind == UC_TL_FUNC_DEF) {
                gen_func(g, decl->as.export_->as.func_def, 1);
            }
            break;
        case UC_TL_EXTERN: {
            char* mangled = mangle_name(g, decl->as.extern_.name.data, 0);
            char* parts = cgen_strdup("");
            for (size_t i = 0; i < uc_vec_len(decl->as.extern_.params); i++) {
                UCParam* p = (UCParam*)uc_vec_at(decl->as.extern_.params, i);
                char* lt = llvm_type_of(g, p->ty);
                size_t cur_len = strlen(parts);
                size_t add_len = strlen(lt) + strlen(p->name.data) + 5;
                parts = (char*)realloc(parts, cur_len + add_len + 1);
                snprintf(parts + cur_len, add_len + 1, "%s%s %%%s",
                         i == 0 ? "" : ", ", lt, p->name.data);
                free(lt);
            }
            emit_fmt_writeln(g, "declare %s %s(%s)",
                             llvm_type(decl->as.extern_.ty->kind),
                             mangled, parts);
            free(mangled);
            free(parts);
            break;
        }
        default:
            /* Imports, var/const decls, struct defs are not emitted
             * into IR in this minimal Phase 3 port. */
            break;
    }
}

/* ------------------------------------------------------------------------- */
/* Function definition                                                       */
/* ------------------------------------------------------------------------- */

static void gen_func(UCCodeGenerator* g, const UCFuncDef* func, int exported) {
    (void)exported;
    /* Reset per-function state. */
    map_free(&g->local_types);
    map_free(&g->local_vars);
    map_init(&g->local_types);
    map_init(&g->local_vars);

    for (size_t i = 0; i < uc_vec_len(func->params); i++) {
        UCParam* p = (UCParam*)uc_vec_at(func->params, i);
        char* lt = llvm_type_of(g, p->ty);
        map_put(&g->local_types, p->name.data, lt);
        free(lt);
    }

    int is_local = map_contains(&g->local_funcs, func->name.data);
    char* mangled_name = mangle_name(g, func->name.data, is_local);

    /* Build parameter list. */
    char* params = cgen_strdup("");
    for (size_t i = 0; i < uc_vec_len(func->params); i++) {
        UCParam* p = (UCParam*)uc_vec_at(func->params, i);
        char* lt = llvm_type_of(g, p->ty);
        size_t cur = strlen(params);
        size_t add = strlen(lt) + strlen(p->name.data) + 5;
        params = (char*)realloc(params, cur + add + 1);
        snprintf(params + cur, add + 1, "%s%s %%%s",
                 i == 0 ? "" : ", ", lt, p->name.data);
        free(lt);
    }

    emit_fmt_writeln(g, "define %s %s(%s) {",
                     llvm_type(func->return_ty->kind),
                     mangled_name,
                     params);
    emit_writeln(g, "entry:");
    g->indent += 1;
    gen_stmt(g, func->body);
    g->indent -= 1;
    emit_writeln(g, "}");
    emit_writeln(g, "");

    free(mangled_name);
    free(params);
}

/* ------------------------------------------------------------------------- */
/* Statements                                                                */
/* ------------------------------------------------------------------------- */

static void gen_stmt(UCCodeGenerator* g, const UCStmt* stmt) {
    switch (stmt->kind) {
        case UC_STMT_BLOCK: {
            for (size_t i = 0; i < uc_vec_len(stmt->as.block); i++) {
                UCStmt* s = (UCStmt*)uc_vec_at(stmt->as.block, i);
                gen_stmt(g, s);
            }
            break;
        }
        case UC_STMT_IF: {
            UCError err; uc_error_init(&err);
            char* cv = gen_expr(g, stmt->as.if_stmt.cond, &err);
            if (!cv || err.kind != UC_ERR_NONE) { free(cv); return; }
            char* else_lbl = mk_label(g, "else");
            char* end_lbl  = mk_label(g, "if_end");
            emit_fmt_writeln(g, "br i1 %s, label %%%s, label %%%s",
                             cv, else_lbl, end_lbl);
            emit_fmt_writeln(g, "%s:", else_lbl);
            g->indent += 1;
            gen_stmt(g, stmt->as.if_stmt.then_branch);
            g->indent -= 1;
            if (stmt->as.if_stmt.else_branch) {
                emit_fmt_writeln(g, "br label %%%s", end_lbl);
                emit_fmt_writeln(g, "%s:", end_lbl);
                g->indent += 1;
                gen_stmt(g, stmt->as.if_stmt.else_branch);
                g->indent -= 1;
            } else {
                emit_fmt_writeln(g, "br label %%%s", end_lbl);
                emit_fmt_writeln(g, "%s:", end_lbl);
            }
            free(cv); free(else_lbl); free(end_lbl);
            break;
        }
        case UC_STMT_WHILE: {
            char* c_lbl = mk_label(g, "while_cond");
            char* b_lbl = mk_label(g, "while_body");
            char* e_lbl = mk_label(g, "while_end");
            emit_fmt_writeln(g, "br label %%%s", c_lbl);
            emit_fmt_writeln(g, "%s:", c_lbl);
            UCError err; uc_error_init(&err);
            char* cv = gen_expr(g, stmt->as.while_stmt.cond, &err);
            if (!cv || err.kind != UC_ERR_NONE) { free(cv); return; }
            emit_fmt_writeln(g, "br i1 %s, label %%%s, label %%%s",
                             cv, b_lbl, e_lbl);
            emit_fmt_writeln(g, "%s:", b_lbl);
            g->indent += 1;
            gen_stmt(g, stmt->as.while_stmt.body);
            g->indent -= 1;
            emit_fmt_writeln(g, "br label %%%s", c_lbl);
            emit_fmt_writeln(g, "%s:", e_lbl);
            free(cv); free(c_lbl); free(b_lbl); free(e_lbl);
            break;
        }
        case UC_STMT_RETURN: {
            if (stmt->as.ret) {
                UCError err; uc_error_init(&err);
                char* v = gen_expr(g, stmt->as.ret, &err);
                if (!v || err.kind != UC_ERR_NONE) { free(v); return; }
                emit_fmt_writeln(g, "ret i32 %s", v);
                free(v);
            } else {
                emit_writeln(g, "ret void");
            }
            break;
        }
        case UC_STMT_EXPR: {
            if (stmt->as.expr) {
                UCError err; uc_error_init(&err);
                char* v = gen_expr(g, stmt->as.expr, &err);
                free(v);
            }
            break;
        }
        case UC_STMT_FREE: {
            UCError err; uc_error_init(&err);
            char* p = gen_expr(g, stmt->as.expr, &err);
            if (!p || err.kind != UC_ERR_NONE) { free(p); return; }
            emit_fmt_writeln(g, "call void @free(i8* %s)", p);
            free(p);
            break;
        }
        case UC_STMT_DECL: {
            UCVarDecl* d = stmt->as.decl;
            char* alloc = (char*)malloc(strlen(d->name.data) + 3);
            sprintf(alloc, "%%%s", d->name.data);
            char* ll_type = llvm_type_of(g, d->ty);
            map_put(&g->local_vars, d->name.data, ll_type);
            emit_fmt_writeln(g, "%s = alloca %s", alloc, ll_type);
            if (d->init) {
                UCError err; uc_error_init(&err);
                char* v = gen_expr(g, d->init, &err);
                if (v && err.kind == UC_ERR_NONE) {
                    const char* expr_type = g->last_expr_type
                                            ? g->last_expr_type : "i32";
                    if (strcmp(expr_type, ll_type) != 0) {
                        char* conv = mk_temp(g);
                        emit_fmt_writeln(g, "%s = trunc i64 %s to i32", conv, v);
                        emit_fmt_writeln(g, "store i32 %s, i32* %s", conv, alloc);
                        free(conv);
                    } else {
                        emit_fmt_writeln(g, "store %s %s, %s* %s",
                                         expr_type, v, ll_type, alloc);
                    }
                }
                free(v);
            }
            free(alloc);
            free(ll_type);
            break;
        }
        case UC_STMT_BREAK:
        case UC_STMT_CONTINUE:
        case UC_STMT_FOR:
            /* Phase 3.1 minimal: not yet emitted. The Rust compiler also
             * has these as known gaps (HANDOFF §6.4). */
            break;
    }
}

/* ------------------------------------------------------------------------- */
/* Syscall wrappers                                                          */
/* ------------------------------------------------------------------------- */

static void gen_syscall_call(UCCodeGenerator* g, const char* symbol,
                             const UCVec* args, const char* res, UCError* err) {
    const char* name = symbol + 1;  /* strip leading '@' */
    if (strcmp(name, "sys$strlen") == 0) {
        if (uc_vec_len(args) != 1) {
            uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                         "sys$strlen takes 1 arg");
            return;
        }
        UCError e2; uc_error_init(&e2);
        char* a0 = gen_expr(g, (const UCExpr*)uc_vec_at(args, 0), &e2);
        if (!a0) { free(a0); return; }
        emit_fmt_writeln(g, "%s = call i64 @strlen(i8* %s)", res, a0);
        free(a0);
        free(g->last_expr_type);
        g->last_expr_type = cgen_strdup("i64");
    } else if (strcmp(name, "sys$write") == 0) {
        if (uc_vec_len(args) != 3) {
            uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                         "sys$write takes 3 args");
            return;
        }
        UCError e2; uc_error_init(&e2);
        char* a0 = gen_expr(g, (const UCExpr*)uc_vec_at(args, 0), &e2);
        char* a1 = gen_expr(g, (const UCExpr*)uc_vec_at(args, 1), &e2);
        char* a2 = gen_expr(g, (const UCExpr*)uc_vec_at(args, 2), &e2);
        char* a2_final;
        if (g->last_expr_type && strcmp(g->last_expr_type, "i32") == 0) {
            char* ext = mk_temp(g);
            emit_fmt_writeln(g, "%s = sext i32 %s to i64", ext, a2);
            a2_final = ext;
        } else {
            a2_final = cgen_strdup(a2);
        }
        emit_fmt_writeln(g, "%s = call i64 @write(i32 %s, i8* %s, i64 %s)",
                         res, a0, a1, a2_final);
        free(a0); free(a1); free(a2); free(a2_final);
        free(g->last_expr_type);
        g->last_expr_type = cgen_strdup("i64");
    } else if (strcmp(name, "sys$read") == 0) {
        if (uc_vec_len(args) != 3) {
            uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                         "sys$read takes 3 args");
            return;
        }
        UCError e2; uc_error_init(&e2);
        char* a0 = gen_expr(g, (const UCExpr*)uc_vec_at(args, 0), &e2);
        char* a1 = gen_expr(g, (const UCExpr*)uc_vec_at(args, 1), &e2);
        char* a2 = gen_expr(g, (const UCExpr*)uc_vec_at(args, 2), &e2);
        emit_fmt_writeln(g, "%s = call i64 @read(i32 %s, i8* %s, i64 %s)",
                         res, a0, a1, a2);
        free(a0); free(a1); free(a2);
        free(g->last_expr_type);
        g->last_expr_type = cgen_strdup("i64");
    } else {
        uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                     "unknown syscall: %s", name);
    }
}

/* ------------------------------------------------------------------------- */
/* Expressions                                                               */
/* ------------------------------------------------------------------------- */

static char* gen_expr(UCCodeGenerator* g, const UCExpr* expr, UCError* err) {
    switch (expr->kind) {
        case UC_EXPR_BINARY: {
            char* lv = gen_expr(g, expr->as.binary.lhs, err);
            if (!lv || err->kind != UC_ERR_NONE) return lv;
            char* rv = gen_expr(g, expr->as.binary.rhs, err);
            if (!rv || err->kind != UC_ERR_NONE) { free(lv); return rv; }
            char* res = mk_temp(g);
            const char* irop = NULL;
            switch (expr->as.binary.op) {
                case UC_BIN_ADD: irop = "add"; break;
                case UC_BIN_SUB: irop = "sub"; break;
                case UC_BIN_MUL: irop = "mul"; break;
                case UC_BIN_DIV: irop = "sdiv"; break;
                case UC_BIN_LT:  irop = "icmp slt"; break;
                case UC_BIN_GT:  irop = "icmp sgt"; break;
                case UC_BIN_LE:  irop = "icmp sle"; break;
                case UC_BIN_GE:  irop = "icmp sge"; break;
                case UC_BIN_EQ:  irop = "icmp eq"; break;
                case UC_BIN_NE:  irop = "icmp ne"; break;
                case UC_BIN_AND: irop = "and"; break;
                case UC_BIN_OR:  irop = "or"; break;
                default:
                    uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                                 "unsupported binary op: %d",
                                 (int)expr->as.binary.op);
                    free(lv); free(rv);
                    return NULL;
            }
            emit_fmt_writeln(g, "%s = %s i32 %s, %s", res, irop, lv, rv);
            free(lv); free(rv);
            return res;
        }
        case UC_EXPR_UNARY: {
            char* v = gen_expr(g, expr->as.unary.operand, err);
            if (!v || err->kind != UC_ERR_NONE) return v;
            char* res = mk_temp(g);
            switch (expr->as.unary.op) {
                case UC_UN_NEG:
                    emit_fmt_writeln(g, "%s = sub i32 0, %s", res, v);
                    break;
                case UC_UN_NOT:
                    emit_fmt_writeln(g, "%s = xor i32 %s, 1", res, v);
                    break;
                case UC_UN_DEREF:
                    emit_fmt_writeln(g, "%s = load i32, i32* %s", res, v);
                    break;
                default:
                    uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                                 "unsupported unary op: %d",
                                 (int)expr->as.unary.op);
                    free(v); free(res);
                    return NULL;
            }
            free(v);
            return res;
        }
        case UC_EXPR_CALL: {
            UCExpr* callee = expr->as.call.callee;
            char* symbol = NULL;
            if (callee->kind == UC_EXPR_IDENT) {
                const char* name = callee->as.ident.data;
                int is_local = map_contains(&g->local_funcs, name);
                symbol = mangle_name(g, name, is_local);
            } else if (callee->kind == UC_EXPR_FIELD) {
                UCExpr* mod = callee->as.field.target;
                if (mod->kind != UC_EXPR_IDENT) {
                    uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                                 "invalid module expression");
                    return NULL;
                }
                const char* module_name = mod->as.ident.data;
                const char* fn = callee->as.field.field.data;
                size_t n = strlen(module_name) + strlen(fn) + 3;
                symbol = (char*)malloc(n);
                snprintf(symbol, n, "@%s$%s", module_name, fn);
                if (map_contains(&g->imported_modules, module_name)) {
                    if (!map_contains(&g->declared_externals, symbol)) {
                        map_put(&g->declared_externals, symbol, "1");
                        buf_printf(&g->extern_decls,
                                   "declare i32 %s()\n", symbol);
                    }
                }
            } else {
                uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                             "indirect calls not supported");
                return NULL;
            }

            char* res = mk_temp(g);
            if (strncmp(symbol, "@sys$", 5) == 0) {
                gen_syscall_call(g, symbol, expr->as.call.args, res, err);
            } else {
                /* Build arg list with per-arg types */
                Buf argbuf; argbuf.data = NULL; argbuf.len = 0; argbuf.cap = 0;
                for (size_t i = 0; i < uc_vec_len(expr->as.call.args); i++) {
                    UCExpr* a = (UCExpr*)uc_vec_at(expr->as.call.args, i);
                    char* av = gen_expr(g, a, err);
                    if (!av || err->kind != UC_ERR_NONE) {
                        free(symbol); buf_free(&argbuf);
                        return av;
                    }
                    const char* at = g->last_expr_type
                                     ? g->last_expr_type : "i32";
                    if (argbuf.len > 0) buf_append(&argbuf, ", ", 2);
                    buf_append(&argbuf, at, strlen(at));
                    buf_append(&argbuf, " ", 1);
                    buf_append(&argbuf, av, strlen(av));
                    free(av);
                }
                if (argbuf.data == NULL) buf_appends(&argbuf, "");
                emit_fmt_writeln(g, "%s = call i32 %s(%s)",
                                 res, symbol, argbuf.data);
                buf_free(&argbuf);
            }
            free(symbol);
            return res;
        }
        case UC_EXPR_IDENT: {
            const char* n = expr->as.ident.data;
            const char* ll_type = map_get(&g->local_vars, n);
            if (ll_type) {
                char* loaded = mk_temp(g);
                emit_fmt_writeln(g, "%s = load %s, %s* %%%s",
                                 loaded, ll_type, ll_type, n);
                free(g->last_expr_type);
                g->last_expr_type = cgen_strdup(ll_type);
                return loaded;
            } else {
                size_t l = strlen(n) + 3;
                char* r = (char*)malloc(l);
                snprintf(r, l, "%%%s", n);
                return r;
            }
        }
        case UC_EXPR_LITERAL: {
            const UCLiteral* lit = &expr->as.literal;
            char* res = mk_temp(g);
            switch (lit->kind) {
                case UC_LIT_INT:
                    emit_fmt_writeln(g, "%s = add i32 0, %lld",
                                     res, lit->as.int_val);
                    break;
                case UC_LIT_TRUE:
                    emit_writeln(g, "  ");
                    buf_printf(&g->output, "%s = add i32 0, 1\n", res);
                    break;
                case UC_LIT_FALSE:
                    emit_writeln(g, "  ");
                    buf_printf(&g->output, "%s = add i32 0, 0\n", res);
                    break;
                case UC_LIT_CHAR:
                    emit_fmt_writeln(g, "%s = add i32 0, %d",
                                     res, (int)(unsigned char)lit->as.char_val);
                    break;
                case UC_LIT_FLOAT:
                    emit_fmt_writeln(g, "%s = fadd double 0.0, %g",
                                     res, lit->as.float_val);
                    break;
                case UC_LIT_STRING: {
                    int id = g->string_counter++;
                    char gname[64];
                    snprintf(gname, sizeof(gname), "@.str.%d", id);
                    const char* s = lit->as.string_val.data
                                     ? lit->as.string_val.data : "";
                    buf_printf(&g->global_strings,
                               "%s = private constant [%zu x i8] c\"%s\\00\"\n",
                               gname, strlen(s) + 1, s);
                    emit_fmt_writeln(g, "%s = getelementptr [%zu x i8], "
                                     "[%zu x i8]* %s, i64 0, i64 0",
                                     res, strlen(s) + 1, strlen(s) + 1, gname);
                    free(g->last_expr_type);
                    g->last_expr_type = cgen_strdup("i8*");
                    break;
                }
            }
            return res;
        }
        case UC_EXPR_NULL:
            return cgen_strdup("null");
        default:
            uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                         "unsupported expression kind: %d", (int)expr->kind);
            return NULL;
    }
}

/* ------------------------------------------------------------------------- */
/* Entry point                                                               */
/* ------------------------------------------------------------------------- */

char* uc_codegen_generate(UCCodeGenerator* g, const UCModule* m, UCError* err) {
    /* Pre-populate local_funcs so mangle_name() knows which functions
     * are local. */
    for (size_t i = 0; i < uc_vec_len(m->declarations); i++) {
        UCTopLevel* decl = (UCTopLevel*)uc_vec_at(m->declarations, i);
        if (decl->kind == UC_TL_FUNC_DEF) {
            map_put(&g->local_funcs, decl->as.func_def->name.data,
                    g->module_name);
        }
    }
    for (size_t i = 0; i < uc_vec_len(m->declarations); i++) {
        UCTopLevel* decl = (UCTopLevel*)uc_vec_at(m->declarations, i);
        gen_toplevel(g, decl);
    }

    /* Concatenate: header + global_strings + extern_decls + output */
    Buf all; all.data = NULL; all.len = 0; all.cap = 0;
    buf_appends(&all, get_builtins_decls());
    if (g->global_strings.data) buf_append(&all, g->global_strings.data,
                                            g->global_strings.len);
    if (g->extern_decls.data) buf_append(&all, g->extern_decls.data,
                                          g->extern_decls.len);
    if (g->output.data) buf_append(&all, g->output.data, g->output.len);

    /* Detach */
    char* result = all.data ? all.data : cgen_strdup("");
    if (!result) result = cgen_strdup("");
    /* Ensure NUL-termination */
    if (all.data) result[all.len] = '\0';
    /* Reset accumulators for potential re-use */
    g->output.len = 0; if (g->output.data) g->output.data[0] = '\0';
    g->extern_decls.len = 0; if (g->extern_decls.data) g->extern_decls.data[0] = '\0';
    g->global_strings.len = 0; if (g->global_strings.data) g->global_strings.data[0] = '\0';
    (void)err;
    return result;
}