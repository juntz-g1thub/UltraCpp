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
    Map global_vars;   /* global var/const name -> LLVM type (M0 P0-1) */
    Map const_init_values; /* const-global name -> folded long long (as string) */
    Map local_funcs;   /* func name  -> module name (for mangling) */
    Map imported_modules; /* module name -> module name */
    Map declared_externals; /* symbol -> "1" (set membership) */

    char* last_expr_type;

    UCVec* loop_scopes;  /* stack of LoopScope* for break/continue targets */
};

static char* cgen_strdup(const char* s) {
    if (!s) return NULL;
    size_t n = strlen(s);
    char* r = (char*)malloc(n + 1);
    if (!r) { fprintf(stderr, "uc_codegen: OOM\n"); abort(); }
    memcpy(r, s, n + 1);
    return r;
}

/* LoopScope is referenced from uc_codegen_free above (line ~200); the full
 * typedef + helpers live near gen_stmt below. */
typedef struct LoopScope LoopScope;
static void free_loop_scope(LoopScope* ls);

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
    map_init(&g->global_vars);
    map_init(&g->const_init_values);
    map_init(&g->local_funcs);
    map_init(&g->imported_modules);
    map_init(&g->declared_externals);
    g->loop_scopes = uc_vec_new();
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
    map_free(&g->global_vars);
    map_free(&g->const_init_values);
    map_free(&g->local_funcs);
    map_free(&g->imported_modules);
    map_free(&g->declared_externals);
    uc_vec_free(g->loop_scopes, (void (*)(void*))free_loop_scope);
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
/* Constant folding for global initializers (M0 P0-1)                        */
/*                                                                           */
/* The 3 failing tests (m0_22/m0_45/m0_46) all use literal/identifier/binary */
/* expressions to initialize globals. LLVM requires global initializers to  */
/* be constant IR expressions, so we evaluate them at codegen time and emit  */
/* a literal `i32 <value>`. The function is recursive: identifiers resolve  */
/* against the previously-emitted const_global_values map (so `Y = X * 4`   */
/* in m0_45 can fold after `X = 1 + 2` was processed first).                */
/*                                                                           */
/* Returns 1 on success (sets *out_value); 0 on any non-foldable shape.    */
/* ------------------------------------------------------------------------- */

static int fold_const_init(UCCodeGenerator* g, const UCExpr* e,
                           long long* out_value) {
    if (!e) return 0;
    switch (e->kind) {
        case UC_EXPR_LITERAL:
            if (e->as.literal.kind == UC_LIT_INT) {
                *out_value = e->as.literal.as.int_val;
                return 1;
            }
            if (e->as.literal.kind == UC_LIT_TRUE) {
                *out_value = 1;
                return 1;
            }
            if (e->as.literal.kind == UC_LIT_FALSE) {
                *out_value = 0;
                return 1;
            }
            return 0;
        case UC_EXPR_IDENT: {
            const char* cached = map_get(&g->const_init_values,
                                         e->as.ident.data);
            if (cached) {
                *out_value = strtoll(cached, NULL, 10);
                return 1;
            }
            return 0;
        }
        case UC_EXPR_BINARY: {
            long long lv = 0, rv = 0;
            if (!fold_const_init(g, e->as.binary.lhs, &lv)) return 0;
            if (!fold_const_init(g, e->as.binary.rhs, &rv)) return 0;
            switch (e->as.binary.op) {
                case UC_BIN_ADD: *out_value = lv + rv; return 1;
                case UC_BIN_SUB: *out_value = lv - rv; return 1;
                case UC_BIN_MUL: *out_value = lv * rv; return 1;
                case UC_BIN_DIV:
                    if (rv == 0) return 0;
                    *out_value = lv / rv; return 1;
                case UC_BIN_MOD:
                    if (rv == 0) return 0;
                    *out_value = lv % rv; return 1;
                default: return 0;
            }
        }
        case UC_EXPR_UNARY: {
            long long v = 0;
            if (!fold_const_init(g, e->as.unary.operand, &v)) return 0;
            switch (e->as.unary.op) {
                case UC_UN_NEG:     *out_value = -v; return 1;
                case UC_UN_BIT_NOT: *out_value = ~v; return 1;
                case UC_UN_NOT:     *out_value = !v; return 1;
                default: return 0;
            }
        }
        default:
            return 0;
    }
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

/* Emit an LLVM IR basic-block label definition at column 1 (no indent).
 * LLVM IR requires block labels at column 1 (LangRef: "Each basic block
 * ... is required to start with a label"). emit_fmt_writeln prepends
 * g->indent pairs of spaces, which would place '%name:' at column 3
 * (or deeper), confusing llc / the IR parser. Use this for every
 * label-definition site; use emit_fmt_writeln("br label %s", lbl) for
 * label *references* where the '%' is part of the literal format.
 *
 * LLVM 18 llc (and clang -x ir) requires label defs in 'name:' form
 * (no leading '%') — '%name:' is parsed as an SSA value reference
 * expecting '=' after, then the ':' yields 'expected = after
 * instruction name'. mk_label returns '%prefix_N' (refs need the %);
 * we strip it here. All 6 emit_label call sites benefit.
 */
static void emit_label(UCCodeGenerator* g, const char* lbl) {
    const char* p = (lbl && lbl[0] == '%') ? lbl + 1 : lbl;
    buf_append(&g->output, p, strlen(p));
    buf_append(&g->output, ":\n", 2);
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
        case UC_TL_VAR_DECL: {
            /* M0 P0-1: emit top-level mutable global variables so that
             * functions can read them via a `load` in UC_EXPR_IDENT. */
            UCVarDecl* d = decl->as.var_decl;
            if (!d || !d->name.data) break;
            char* ll_type = llvm_type_of(g, d->ty);
            long long folded = 0;
            if (d->init && fold_const_init(g, d->init, &folded)) {
                emit_fmt_writeln(g, "@%s = global %s %lld",
                                 d->name.data, ll_type, folded);
                /* Cache so subsequent const-fold references resolve. */
                char buf[32];
                snprintf(buf, sizeof(buf), "%lld", folded);
                map_put(&g->const_init_values, d->name.data, buf);
            } else {
                /* No init or un-foldable init: zero-init the storage.
                 * (Future P-level work may add a __uc_init_globals
                 * constructor for un-foldable cases.) */
                emit_fmt_writeln(g, "@%s = global %s 0",
                                 d->name.data, ll_type);
            }
            map_put(&g->global_vars, d->name.data, ll_type);
            free(ll_type);
            break;
        }
        case UC_TL_CONST_DECL: {
            /* M0 P0-1: emit `const` top-level globals as LLVM `constant`s.
             * The init expression must evaluate to an integer literal at
             * codegen time so we can emit `private constant i32 <N>`. */
            UCConstDecl* c = decl->as.const_decl;
            if (!c || !c->name.data) break;
            char* ll_type = llvm_type_of(g, c->ty);
            long long folded = 0;
            if (c->value && fold_const_init(g, c->value, &folded)) {
                emit_fmt_writeln(g, "@%s = private constant %s %lld",
                                 c->name.data, ll_type, folded);
                char buf[32];
                snprintf(buf, sizeof(buf), "%lld", folded);
                map_put(&g->const_init_values, c->name.data, buf);
            } else {
                /* Fallback: zero-init constant. */
                emit_fmt_writeln(g, "@%s = private constant %s 0",
                                 c->name.data, ll_type);
            }
            map_put(&g->global_vars, c->name.data, ll_type);
            free(ll_type);
            break;
        }
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
            /* Imports and struct defs remain not-emitted in this minimal
             * Phase 3 port (struct/array types are P2 work). */
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
    /* Void-returning functions with an empty body (e.g. `void f() {}`)
     * produce no terminator instruction, which makes llc reject the IR
     * with "expected instruction opcode }". Emit `ret void` to close
     * the entry block. Returns inside the body are already emitted by
     * UC_STMT_RETURN, so we only fall through here when the body had
     * no statements. */
    if (func->return_ty->kind == UC_TYPE_VOID) {
        int body_empty = (func->body == NULL) ||
                         (func->body->kind == UC_STMT_BLOCK &&
                          (func->body->as.block == NULL ||
                           uc_vec_len(func->body->as.block) == 0));
        if (body_empty) {
            emit_writeln(g, "ret void");
        }
    }
    emit_writeln(g, "}");
    emit_writeln(g, "");

    free(mangled_name);
    free(params);
}

/* ------------------------------------------------------------------------- */
/* Statements                                                                */
/* ------------------------------------------------------------------------- */

/* Per-loop frame: the labels break/continue should jump to inside the
 * nearest enclosing loop. break_lbl is emitted after the loop body;
 * continue_lbl is emitted at the step (for) / cond (while) point. The
 * labels are owned by this frame and freed on pop. */
struct LoopScope {
    char* break_lbl;    /* owned */
    char* continue_lbl; /* owned */
};

static void free_loop_scope(LoopScope* ls) {
    if (!ls) return;
    free(ls->break_lbl);
    free(ls->continue_lbl);
    free(ls);
}

static void push_loop_scope(UCCodeGenerator* g, char* break_lbl, char* continue_lbl) {
    LoopScope* s = (LoopScope*)malloc(sizeof(LoopScope));
    if (!s) { fprintf(stderr, "uc_codegen: OOM\n"); abort(); }
    s->break_lbl = break_lbl;
    s->continue_lbl = continue_lbl;
    uc_vec_push(g->loop_scopes, s);
}

static LoopScope* current_loop_scope(UCCodeGenerator* g) {
    if (!g->loop_scopes || g->loop_scopes->len == 0) return NULL;
    return (LoopScope*)uc_vec_at(g->loop_scopes, g->loop_scopes->len - 1);
}

static void pop_loop_scope(UCCodeGenerator* g) {
    if (!g->loop_scopes || g->loop_scopes->len == 0) return;
    LoopScope* top = (LoopScope*)uc_vec_at(g->loop_scopes, g->loop_scopes->len - 1);
    free_loop_scope(top);
    g->loop_scopes->len -= 1;
}

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
            char* then_lbl = mk_label(g, "if_then");
            char* else_lbl = stmt->as.if_stmt.else_branch
                                 ? mk_label(g, "if_else") : NULL;
            char* end_lbl  = mk_label(g, "if_end");
            emit_fmt_writeln(g, "br i1 %s, label %s, label %s",
                             cv, then_lbl, else_lbl ? else_lbl : end_lbl);
            emit_label(g, then_lbl);
            g->indent += 1;
            gen_stmt(g, stmt->as.if_stmt.then_branch);
            g->indent -= 1;
            emit_fmt_writeln(g, "br label %s", end_lbl);
            if (else_lbl) {
                emit_label(g, else_lbl);
                g->indent += 1;
                gen_stmt(g, stmt->as.if_stmt.else_branch);
                g->indent -= 1;
                emit_fmt_writeln(g, "br label %s", end_lbl);
            }
            emit_label(g, end_lbl);
            free(cv); free(then_lbl); free(else_lbl); free(end_lbl);
            break;
        }
        case UC_STMT_WHILE: {
            char* c_lbl = mk_label(g, "while_cond");
            char* b_lbl = mk_label(g, "while_body");
            char* e_lbl = mk_label(g, "while_end");
            /* push scope (e_lbl=break target, c_lbl=continue target) */
            push_loop_scope(g, e_lbl, c_lbl);
            emit_fmt_writeln(g, "br label %s", c_lbl);
            emit_label(g, c_lbl);
            UCError err; uc_error_init(&err);
            char* cv = gen_expr(g, stmt->as.while_stmt.cond, &err);
            if (!cv || err.kind != UC_ERR_NONE) {
                free(cv);
                pop_loop_scope(g);  /* frees e_lbl + c_lbl */
                free(b_lbl);
                return;
            }
            emit_fmt_writeln(g, "br i1 %s, label %s, label %s",
                             cv, b_lbl, e_lbl);
            emit_label(g, b_lbl);
            g->indent += 1;
            gen_stmt(g, stmt->as.while_stmt.body);
            g->indent -= 1;
            emit_fmt_writeln(g, "br label %s", c_lbl);
            emit_label(g, e_lbl);
            pop_loop_scope(g);  /* frees e_lbl + c_lbl */
            free(cv); free(b_lbl);
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
                        if (strcmp(expr_type, "i1") == 0
                            && strcmp(ll_type, "i32") == 0) {
                            emit_fmt_writeln(g, "%s = zext i1 %s to i32",
                                             conv, v);
                            emit_fmt_writeln(g, "store i32 %s, i32* %s",
                                             conv, alloc);
                        } else if (strcmp(expr_type, "i64") == 0
                                   && strcmp(ll_type, "i32") == 0) {
                            emit_fmt_writeln(g, "%s = trunc i64 %s to i32",
                                             conv, v);
                            emit_fmt_writeln(g, "store i32 %s, i32* %s",
                                             conv, alloc);
                        } else {
                            uc_error_set(&err, UC_ERR_CODEGEN, 0, 0, NULL,
                                         "type mismatch in decl: "
                                         "expr=%s target=%s",
                                         expr_type, ll_type);
                            free(conv);
                            free(v);
                            return;
                        }
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
        case UC_STMT_FOR: {
            /* for (init; cond; step) body — init/cond/step may be NULL. */
            UCError err; uc_error_init(&err);
            char* cond_lbl = mk_label(g, "for_cond");
            char* body_lbl = mk_label(g, "for_body");
            char* step_lbl = mk_label(g, "for_step");
            char* end_lbl  = mk_label(g, "for_end");

            /* init: emit in linear flow (before loop back-edges). */
            if (stmt->as.for_stmt.init) {
                gen_stmt(g, stmt->as.for_stmt.init);
            }
            /* push scope (end_lbl=break target, step_lbl=continue target) */
            push_loop_scope(g, end_lbl, step_lbl);
            /* jump to cond check */
            emit_fmt_writeln(g, "br label %s", cond_lbl);

            emit_label(g, cond_lbl);
            if (stmt->as.for_stmt.cond) {
                char* cv = gen_expr(g, stmt->as.for_stmt.cond, &err);
                if (!cv || err.kind != UC_ERR_NONE) {
                    free(cv);
                    pop_loop_scope(g);  /* frees end_lbl + step_lbl */
                    free(cond_lbl); free(body_lbl);
                    return;
                }
                emit_fmt_writeln(g, "br i1 %s, label %s, label %s",
                                 cv, body_lbl, end_lbl);
                free(cv);
            } else {
                /* missing cond == infinite loop; exit only via break */
                emit_fmt_writeln(g, "br i1 1, label %s, label %s",
                                 body_lbl, end_lbl);
            }

            emit_label(g, body_lbl);
            gen_stmt(g, stmt->as.for_stmt.body);
            /* after body, jump to step (continue target) */
            emit_fmt_writeln(g, "br label %s", step_lbl);

            emit_label(g, step_lbl);
            if (stmt->as.for_stmt.step) {
                char* sv = gen_expr(g, stmt->as.for_stmt.step, &err);
                if (err.kind != UC_ERR_NONE) {
                    free(sv);
                    pop_loop_scope(g);
                    free(cond_lbl); free(body_lbl);
                    return;
                }
                free(sv);
            }
            /* back-edge to cond check */
            emit_fmt_writeln(g, "br label %s", cond_lbl);

            emit_label(g, end_lbl);
            pop_loop_scope(g);  /* frees end_lbl + step_lbl */
            free(cond_lbl); free(body_lbl);
            break;
        }
        case UC_STMT_BREAK: {
            LoopScope* s = current_loop_scope(g);
            if (!s) {
                fprintf(stderr, "uc_codegen: 'break' outside loop\n");
                break;
            }
            emit_fmt_writeln(g, "br label %s", s->break_lbl);
            break;
        }
        case UC_STMT_CONTINUE: {
            LoopScope* s = current_loop_scope(g);
            if (!s) {
                fprintf(stderr, "uc_codegen: 'continue' outside loop\n");
                break;
            }
            emit_fmt_writeln(g, "br label %s", s->continue_lbl);
            break;
        }
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
                case UC_BIN_MOD: irop = "srem"; break;
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
            /* Comparison ops (icmp *) produce i1; arithmetic ops produce i32.
             * Without this, UC_STMT_DECL defaults to 'i32' and tries to
             * store the i1 result into an i32 slot, which llc rejects. */
            if (irop && strncmp(irop, "icmp", 4) == 0) {
                free(g->last_expr_type);
                g->last_expr_type = cgen_strdup("i1");
            } else if (g->last_expr_type
                       && strcmp(g->last_expr_type, "i8*") == 0) {
                /* Arithmetic ops always emit `i32` results; reset a stale
                 * pointer type left by a prior UC_UN_ADDR_OF so the
                 * subsequent decl sees the correct i32 type. m0_31 uses
                 * `int sum = (*p) + (*q);` after `int* p = &x;` and
                 * relied on this implicit reset. */
                free(g->last_expr_type);
                g->last_expr_type = cgen_strdup("i32");
            }
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
                case UC_UN_ADDR_OF: {
                    /* m0_31 (&x): the operand IDENT is a tracked local alloca.
                     * The IDENT branch loads its value into a fresh temp,
                     * which we discard; the alloca name itself is already a
                     * pointer. Return it directly.
                     *
                     * The `int*` decl side allocates as `i8*` (see
                     * llvm_type(UC_TYPE_POINTER)), and LLVM 18 uses opaque
                     * pointers, so emitting `store i8* %x, i8** %p` with
                     * %x = `i32*` (from `alloca i32`) is accepted.
                     * last_expr_type must match the decl slot type. */
                    UCExpr* opd = expr->as.unary.operand;
                    if (!opd || opd->kind != UC_EXPR_IDENT) {
                        uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                                     "address-of requires a local variable "
                                     "operand");
                        free(v); free(res);
                        return NULL;
                    }
                    const char* name = opd->as.ident.data;
                    const char* ll_ty = map_get(&g->local_vars, name);
                    if (!ll_ty) {
                        uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                                     "address-of: local variable '%s' not "
                                     "found", name);
                        free(v); free(res);
                        return NULL;
                    }
                    (void)ll_ty; /* opaque-ptr: type tag unused at IR level */
                    free(v); free(res);
                    size_t addr_len = strlen(name) + 3;
                    char* addr = (char*)malloc(addr_len);
                    snprintf(addr, addr_len, "%%%s", name);
                    free(g->last_expr_type);
                    g->last_expr_type = cgen_strdup("i8*");
                    return addr;
                }
                case UC_UN_PRE_INC:
                case UC_UN_PRE_DEC:
                case UC_UN_POST_INC:
                case UC_UN_POST_DEC: {
                    /* m0_47: ++/-- operand must be a tracked local
                     * (UC_EXPR_IDENT with an entry in g->local_vars).
                     * For m0_47 we only handle i32 locals. */
                    UCExpr* opd = expr->as.unary.operand;
                    if (!opd || opd->kind != UC_EXPR_IDENT) {
                        uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                                     "inc/dec requires a local variable operand");
                        free(v); free(res);
                        return NULL;
                    }
                    const char* name = opd->as.ident.data;
                    const char* ll_ty = map_get(&g->local_vars, name);
                    if (!ll_ty) {
                        uc_error_set(err, UC_ERR_CODEGEN, 0, 0, NULL,
                                     "inc/dec: local variable '%s' not found",
                                     name);
                        free(v); free(res);
                        return NULL;
                    }
                    UCUnaryOp op = expr->as.unary.op;
                    int is_inc = (op == UC_UN_PRE_INC || op == UC_UN_POST_INC);
                    int is_pre = (op == UC_UN_PRE_INC || op == UC_UN_PRE_DEC);
                    char* old_v = mk_temp(g);
                    char* new_v = mk_temp(g);
                    size_t addr_len = strlen(name) + 3;
                    char* addr = (char*)malloc(addr_len);
                    snprintf(addr, addr_len, "%%%s", name);
                    emit_fmt_writeln(g, "%s = load %s, %s* %s",
                                     old_v, ll_ty, ll_ty, addr);
                    emit_fmt_writeln(g, "%s = %s %s %s, 1",
                                     new_v, is_inc ? "add" : "sub",
                                     ll_ty, old_v);
                    emit_fmt_writeln(g, "store %s %s, %s* %s",
                                     ll_ty, new_v, ll_ty, addr);
                    free(addr);
                    emit_fmt_writeln(g, "%s = add %s 0, %s",
                                     res, ll_ty, is_pre ? new_v : old_v);
                    free(old_v); free(new_v);
                    free(g->last_expr_type);
                    g->last_expr_type = cgen_strdup(ll_ty);
                    free(v);
                    return res;
                }
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
        case UC_EXPR_ASSIGN: {
            /* M0 P0-1 follow-up: assignment expressions must be lowered to
             * an LLVM `store` so that m0_46_global_init's `copy_g = ...`
             * line updates copy_g's alloca. The lhs must be a pointer
             * (addressable lvalue); for plain local var identifiers we
             * can use the alloca name directly. The expression result is
             * a fresh unused SSA value (matching the loose behaviour the
             * other cases already provide when an expression result is
             * discarded). */
            char* target_addr = NULL;
            if (expr->as.assign.target
                && expr->as.assign.target->kind == UC_EXPR_IDENT) {
                const char* n = expr->as.assign.target->as.ident.data;
                const char* ll_type = map_get(&g->local_vars, n);
                if (ll_type) {
                    size_t l = strlen(n) + 3;
                    target_addr = (char*)malloc(l);
                    snprintf(target_addr, l, "%%%s", n);
                    free(g->last_expr_type);
                    g->last_expr_type = cgen_strdup(ll_type);
                }
            }
            if (!target_addr) {
                target_addr = gen_expr(g, expr->as.assign.target, err);
                if (!target_addr || err->kind != UC_ERR_NONE) return target_addr;
            }
            char* value = gen_expr(g, expr->as.assign.value, err);
            if (!value || err->kind != UC_ERR_NONE) { free(target_addr); return value; }
            const char* ty = g->last_expr_type
                             ? g->last_expr_type : "i32";
            emit_fmt_writeln(g, "store %s %s, %s* %s",
                             ty, value, ty, target_addr);
            free(target_addr);
            free(value);
            char* res = mk_temp(g);
            return res;
        }
        case UC_EXPR_TERNARY: {
            /* cond ? then_e : else_e — right-assoc. Generates:
             *   br i1 %cv, label %tern_t_N, label %tern_e_N
             * tern_t_N: %tv = <then_e>   br label %tern_j_N
             * tern_e_N: %ev = <else_e>   br label %tern_j_N
             * tern_j_N:
             *   %res = phi %ty [%tv, %tern_t_N], [%ev, %tern_e_N]
             * Single join label (tern_j) is fine: each predecessor has
             * exactly one successor (the join), so the merge is not on
             * a critical edge and the phi lives in the join block. */
            char* cv = gen_expr(g, expr->as.ternary.cond, err);
            if (!cv || err->kind != UC_ERR_NONE) { free(cv); return NULL; }

            char* then_lbl = mk_label(g, "tern_t");
            char* else_lbl = mk_label(g, "tern_e");
            char* join_lbl = mk_label(g, "tern_j");

            emit_fmt_writeln(g, "br i1 %s, label %s, label %s",
                             cv, then_lbl, else_lbl);

            emit_label(g, then_lbl);
            char* tv = gen_expr(g, expr->as.ternary.then_e, err);
            if (!tv || err->kind != UC_ERR_NONE) { free(cv); free(tv); return NULL; }
            emit_fmt_writeln(g, "br label %s", join_lbl);

            emit_label(g, else_lbl);
            char* ev = gen_expr(g, expr->as.ternary.else_e, err);
            if (!ev || err->kind != UC_ERR_NONE) { free(cv); free(tv); free(ev); return NULL; }
            emit_fmt_writeln(g, "br label %s", join_lbl);

            emit_label(g, join_lbl);
            char* res = mk_temp(g);
            const char* ty = g->last_expr_type ? g->last_expr_type : "i32";
            emit_fmt_writeln(g, "%s = phi %s [%s, %s], [%s, %s]",
                             res, ty, tv, then_lbl, ev, else_lbl);
            free(cv); free(tv); free(ev);
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
            } else if ((ll_type = map_get(&g->global_vars, n)) != NULL) {
                /* M0 P0-1: global var/const — emit a load from `@name`. */
                char* loaded = mk_temp(g);
                emit_fmt_writeln(g, "%s = load %s, %s* @%s",
                                 loaded, ll_type, ll_type, n);
                free(g->last_expr_type);
                g->last_expr_type = cgen_strdup(ll_type);
                return loaded;
            } else {
                /* Parameter or function-name SSA value: return `%name`
                 * directly. For params, the type was registered in
                 * local_types; we leave last_expr_type unchanged so
                 * downstream call-arg coercion uses its prior state. */
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
                    break;
                }
            }
            /* Map lit kind to the LLVM type actually emitted above; without
             * this, a stale last_expr_type (often i1 from a prior icmp)
             * leaks into a subsequent UC_STMT_DECL and triggers bogus
             * 'zext i1 %v to i32' against an i32 %v, failing llc. */
            const char* lit_ty;
            switch (lit->kind) {
                case UC_LIT_INT:    lit_ty = "i32";   break;
                case UC_LIT_TRUE:   lit_ty = "i32";   break;
                case UC_LIT_FALSE:  lit_ty = "i32";   break;
                case UC_LIT_CHAR:   lit_ty = "i32";   break;
                case UC_LIT_FLOAT:  lit_ty = "double"; break;
                case UC_LIT_STRING: lit_ty = "i8*";   break;
                default:            lit_ty = "i32";   break;
            }
            free(g->last_expr_type);
            g->last_expr_type = cgen_strdup(lit_ty);
            return res;
        }
        case UC_EXPR_NULL:
            return cgen_strdup("null");
        case UC_EXPR_MOVE:
            /* PARSE-ONLY: ownership transfer is M2. Pass through. */
            return gen_expr(g, expr->as.move_expr, err);
        case UC_EXPR_ALLOC: {
            /* PARSE-ONLY: minimal codegen — emit `call i8* @malloc(i64 4)`
             * and leave last_expr_type as i8*. Proper sized alloc +
             * ownership transfer is M2. */
            char* res = mk_temp(g);
            emit_fmt_writeln(g, "%s = call i8* @malloc(i64 4)", res);
            free(g->last_expr_type);
            g->last_expr_type = cgen_strdup("i8*");
            return res;
        }
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