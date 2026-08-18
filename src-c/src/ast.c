/* UltraCPP C compiler - AST construction, deep free, debug dump
 *
 * C99 port of src/frontend/ast.rs.
 *
 * Memory model:
 *   - All *_new functions allocate with malloc.  On allocation failure they
 *     abort; this is acceptable because the compiler is a short-lived
 *     batch program and we want a single, loud failure mode.
 *   - All *_free functions perform a deep release and are no-ops on NULL.
 *   - The *_new functions take ownership of every heap-allocated pointer
 *     passed to them; callers must not free those pointers themselves.
 */
#include "uc_ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Extern function signatures shared by parser and code generator. */
static char* ast_strdup(const char* s) {
    size_t n = strlen(s) + 1;
    char* p = (char*)malloc(n);
    if (!p) abort();
    memcpy(p, s, n);
    return p;
}


extern_func_sig_t* g_extern_funcs = NULL;
int g_extern_func_count = 0;
int g_extern_func_capacity = 0;

void extern_func_table_add(const char* name, const char* ret_type, char** param_types, int param_count) {
    if (g_extern_func_count >= g_extern_func_capacity) {
        int cap = g_extern_func_capacity ? g_extern_func_capacity * 2 : 16;
        g_extern_funcs = (extern_func_sig_t*)realloc(g_extern_funcs, cap * sizeof(*g_extern_funcs));
        g_extern_func_capacity = cap;
    }
    extern_func_sig_t* s = &g_extern_funcs[g_extern_func_count++];
    s->name = ast_strdup(name ? name : "");
    s->ret_type = ast_strdup(ret_type ? ret_type : "i32");
    s->param_types = param_types;
    s->param_count = param_count;
}

const extern_func_sig_t* lookup_extern_func(const char* name) {
    for (int i = 0; name && i < g_extern_func_count; i++)
        if (strcmp(g_extern_funcs[i].name, name) == 0) return &g_extern_funcs[i];
    return NULL;
}



static void* xmalloc(size_t n) {
    void* p = malloc(n);
    if (!p) {
        fprintf(stderr, "uc_ast: out of memory (malloc %zu bytes)\n", n);
        abort();
    }
    return p;
}

static void* xcalloc(size_t n, size_t size) {
    void* p = calloc(n, size);
    if (!p) {
        fprintf(stderr, "uc_ast: out of memory (calloc %zu * %zu bytes)\n", n, size);
        abort();
    }
    return p;
}

/* ------------------------------------------------------------------------- */
/* UCString                                                                  */
/* ------------------------------------------------------------------------- */

UCString uc_string_new(const char* s, size_t len) {
    UCString out = {NULL, 0};
    if (!s && len > 0) return out;
    out.data = (char*)xmalloc(len + 1);
    if (len > 0) memcpy(out.data, s, len);
    out.data[len] = '\0';
    out.len = len;
    return out;
}

UCString uc_string_new_cstr(const char* s) {
    if (!s) {
        return uc_string_empty();
    }
    return uc_string_new(s, strlen(s));
}

UCString uc_string_empty(void) {
    UCString out = {NULL, 0};
    return out;
}

void uc_string_free(UCString* s) {
    if (!s) return;
    free(s->data);
    s->data = NULL;
    s->len = 0;
}

UCString uc_string_move(UCString* s) {
    UCString out = {NULL, 0};
    if (!s) return out;
    out = *s;
    s->data = NULL;
    s->len = 0;
    return out;
}

/* ------------------------------------------------------------------------- */
/* UCVec                                                                     */
/* ------------------------------------------------------------------------- */

UCVec* uc_vec_new(void) {
    UCVec* v = (UCVec*)xcalloc(1, sizeof(UCVec));
    return v;
}

UCVec* uc_vec_push(UCVec* v, void* item) {
    if (!v) return NULL;
    if (v->len >= v->cap) {
        size_t new_cap = v->cap == 0 ? 4 : v->cap * 2;
        void** new_data = (void**)xmalloc(new_cap * sizeof(void*));
        if (v->data && v->len > 0) {
            memcpy(new_data, v->data, v->len * sizeof(void*));
        }
        free(v->data);
        v->data = new_data;
        v->cap = new_cap;
    }
    v->data[v->len++] = item;
    return v;
}

size_t uc_vec_len(const UCVec* v) {
    return v ? v->len : 0;
}

void* uc_vec_at(const UCVec* v, size_t i) {
    if (!v || i >= v->len) return NULL;
    return v->data[i];
}

void uc_vec_free(UCVec* v, void (*item_free)(void*)) {
    if (!v) return;
    if (item_free) {
        for (size_t i = 0; i < v->len; i++) {
            item_free(v->data[i]);
        }
    }
    free(v->data);
    free(v);
}

/* ------------------------------------------------------------------------- */
/* Forward decls for recursive free helpers                                  */
/* ------------------------------------------------------------------------- */

static void tl_free(void* p);   /* UCTopLevel*  */
static void stmt_free(void* p); /* UCStmt*      */
static void expr_free(void* p); /* UCExpr*      */
static void type_free(void* p) { uc_type_free((UCType*)p); }

static void param_free(void* p) {
    UCParam* x = (UCParam*)p;
    if (!x) return;
    uc_string_free(&x->name);
    type_free(x->ty);
    free(x);
}

static void struct_field_free(void* p) {
    UCStructField* x = (UCStructField*)p;
    if (!x) return;
    uc_string_free(&x->name);
    type_free(x->ty);
    free(x);
}

static void param_vec_free(UCVec* v) { uc_vec_free(v, param_free); }
static void field_vec_free(UCVec* v) { uc_vec_free(v, struct_field_free); }
static void stmt_vec_free(UCVec* v)  { uc_vec_free(v, stmt_free); }
static void expr_vec_free(UCVec* v)  { uc_vec_free(v, expr_free); }
static void tl_vec_free(UCVec* v)    { uc_vec_free(v, tl_free); }
static void type_vec_free(UCVec* v)  { uc_vec_free(v, type_free); }

/* ------------------------------------------------------------------------- */
/* UCType                                                                    */
/* ------------------------------------------------------------------------- */

static UCType* type_new(UCTypeKind kind) {
    UCType* t = (UCType*)xcalloc(1, sizeof(UCType));
    t->kind = kind;
    return t;
}

UCType* uc_type_void(void)          { return type_new(UC_TYPE_VOID); }
UCType* uc_type_bool(void)          { return type_new(UC_TYPE_BOOL); }
UCType* uc_type_char(void)          { return type_new(UC_TYPE_CHAR); }
UCType* uc_type_int(void)           { return type_new(UC_TYPE_INT); }
UCType* uc_type_i8(void)            { return type_new(UC_TYPE_I8); }
UCType* uc_type_i16(void)           { return type_new(UC_TYPE_I16); }
UCType* uc_type_i32(void)           { return type_new(UC_TYPE_I32); }
UCType* uc_type_i64(void)           { return type_new(UC_TYPE_I64); }
UCType* uc_type_uint(void)          { return type_new(UC_TYPE_UINT); }
UCType* uc_type_u8(void)            { return type_new(UC_TYPE_U8); }
UCType* uc_type_u16(void)           { return type_new(UC_TYPE_U16); }
UCType* uc_type_u32(void)           { return type_new(UC_TYPE_U32); }
UCType* uc_type_u64(void)           { return type_new(UC_TYPE_U64); }
UCType* uc_type_f32(void)           { return type_new(UC_TYPE_F32); }
UCType* uc_type_f64(void)           { return type_new(UC_TYPE_F64); }
UCType* uc_type_usize(void)         { return type_new(UC_TYPE_USIZE); }
UCType* uc_type_isize(void)         { return type_new(UC_TYPE_ISIZE); }

UCType* uc_type_pointer(UCType* inner) {
    UCType* t = type_new(UC_TYPE_POINTER);
    t->as.inner = inner;
    return t;
}

UCType* uc_type_mutable_pointer(UCType* inner) {
    UCType* t = type_new(UC_TYPE_MUTABLE_POINTER);
    t->as.inner = inner;
    return t;
}

UCType* uc_type_ref(UCType* inner) {
    UCType* t = type_new(UC_TYPE_REF);
    t->as.inner = inner;
    return t;
}

UCType* uc_type_array(UCType* element, size_t length) {
    UCType* t = type_new(UC_TYPE_ARRAY);
    t->as.array.element = element;
    t->as.array.length = length;
    return t;
}

UCType* uc_type_function(UCType* ret, UCVec* params) {
    UCType* t = type_new(UC_TYPE_FUNCTION);
    t->as.function.ret = ret;
    t->as.function.params = params;
    return t;
}

UCType* uc_type_named(const char* name, size_t len) {
    UCType* t = type_new(UC_TYPE_NAMED);
    t->as.named = uc_string_new(name, len);
    return t;
}

void uc_type_free(UCType* t) {
    if (!t) return;
    switch (t->kind) {
        case UC_TYPE_VOID:
        case UC_TYPE_BOOL:
        case UC_TYPE_CHAR:
        case UC_TYPE_INT:
        case UC_TYPE_I8:
        case UC_TYPE_I16:
        case UC_TYPE_I32:
        case UC_TYPE_I64:
        case UC_TYPE_UINT:
        case UC_TYPE_U8:
        case UC_TYPE_U16:
        case UC_TYPE_U32:
        case UC_TYPE_U64:
        case UC_TYPE_F32:
        case UC_TYPE_F64:
        case UC_TYPE_USIZE:
        case UC_TYPE_ISIZE:
            break;
        case UC_TYPE_POINTER:
        case UC_TYPE_MUTABLE_POINTER:
        case UC_TYPE_REF:
            type_free(t->as.inner);
            break;
        case UC_TYPE_ARRAY:
            type_free(t->as.array.element);
            break;
        case UC_TYPE_FUNCTION:
            type_free(t->as.function.ret);
            type_vec_free(t->as.function.params);
            break;
        case UC_TYPE_NAMED:
            uc_string_free(&t->as.named);
            break;
    }
    free(t);
}

/* ------------------------------------------------------------------------- */
/* Param / Func / Struct / Import / Var / Const                              */
/* ------------------------------------------------------------------------- */

UCParam* uc_param_new(UCString name, UCType* ty) {
    UCParam* p = (UCParam*)xcalloc(1, sizeof(UCParam));
    p->name = name;
    p->ty = ty;
    return p;
}

void uc_param_free(UCParam* p) { param_free(p); }

UCFuncDef* uc_func_def_new(UCString name, UCVec* params,
                           UCType* return_ty, UCStmt* body) {
    UCFuncDef* f = (UCFuncDef*)xcalloc(1, sizeof(UCFuncDef));
    f->name = name;
    f->params = params;
    f->return_ty = return_ty;
    f->body = body;
    return f;
}

void uc_func_def_free(UCFuncDef* f) {
    if (!f) return;
    uc_string_free(&f->name);
    param_vec_free(f->params);
    type_free(f->return_ty);
    stmt_free(f->body);
    free(f);
}

UCFuncDecl* uc_func_decl_new(UCString name, UCVec* params, UCType* return_ty) {
    UCFuncDecl* f = (UCFuncDecl*)xcalloc(1, sizeof(UCFuncDecl));
    f->name = name;
    f->params = params;
    f->return_ty = return_ty;
    return f;
}

void uc_func_decl_free(UCFuncDecl* f) {
    if (!f) return;
    uc_string_free(&f->name);
    param_vec_free(f->params);
    type_free(f->return_ty);
    free(f);
}

UCStructField* uc_struct_field_new(UCString name, UCType* ty) {
    UCStructField* f = (UCStructField*)xcalloc(1, sizeof(UCStructField));
    f->name = name;
    f->ty = ty;
    return f;
}

void uc_struct_field_free(UCStructField* f) { struct_field_free(f); }

UCStructDef* uc_struct_def_new(UCString name, UCVec* fields) {
    UCStructDef* s = (UCStructDef*)xcalloc(1, sizeof(UCStructDef));
    s->name = name;
    s->fields = fields;
    return s;
}

void uc_struct_def_free(UCStructDef* s) {
    if (!s) return;
    uc_string_free(&s->name);
    field_vec_free(s->fields);
    free(s);
}

UCImport* uc_import_new(UCString path, UCString alias) {
    UCImport* i = (UCImport*)xcalloc(1, sizeof(UCImport));
    i->path = path;
    i->alias = alias;
    return i;
}

void uc_import_free(UCImport* i) {
    if (!i) return;
    uc_string_free(&i->path);
    uc_string_free(&i->alias);
    free(i);
}

UCVarDecl* uc_var_decl_new(UCString name, UCType* ty, UCExpr* init) {
    UCVarDecl* v = (UCVarDecl*)xcalloc(1, sizeof(UCVarDecl));
    v->name = name;
    v->ty = ty;
    v->init = init;
    return v;
}

void uc_var_decl_free(UCVarDecl* v) {
    if (!v) return;
    uc_string_free(&v->name);
    type_free(v->ty);
    expr_free(v->init);
    free(v);
}

UCConstDecl* uc_const_decl_new(UCString name, UCType* ty, UCExpr* value) {
    UCConstDecl* c = (UCConstDecl*)xcalloc(1, sizeof(UCConstDecl));
    c->name = name;
    c->ty = ty;
    c->value = value;
    return c;
}

void uc_const_decl_free(UCConstDecl* c) {
    if (!c) return;
    uc_string_free(&c->name);
    type_free(c->ty);
    expr_free(c->value);
    free(c);
}

/* ------------------------------------------------------------------------- */
/* UCTopLevel                                                                */
/* ------------------------------------------------------------------------- */

UCTopLevel* uc_tl_func_def(UCFuncDef* f) {
    UCTopLevel* t = (UCTopLevel*)xcalloc(1, sizeof(UCTopLevel));
    t->kind = UC_TL_FUNC_DEF;
    t->as.func_def = f;
    return t;
}

UCTopLevel* uc_tl_func_decl(UCFuncDecl* f) {
    UCTopLevel* t = (UCTopLevel*)xcalloc(1, sizeof(UCTopLevel));
    t->kind = UC_TL_FUNC_DECL;
    t->as.func_decl = f;
    return t;
}

UCTopLevel* uc_tl_struct_def(UCStructDef* s) {
    UCTopLevel* t = (UCTopLevel*)xcalloc(1, sizeof(UCTopLevel));
    t->kind = UC_TL_STRUCT_DEF;
    t->as.struct_def = s;
    return t;
}

UCTopLevel* uc_tl_var_decl(UCVarDecl* v) {
    UCTopLevel* t = (UCTopLevel*)xcalloc(1, sizeof(UCTopLevel));
    t->kind = UC_TL_VAR_DECL;
    t->as.var_decl = v;
    return t;
}

UCTopLevel* uc_tl_const_decl(UCConstDecl* c) {
    UCTopLevel* t = (UCTopLevel*)xcalloc(1, sizeof(UCTopLevel));
    t->kind = UC_TL_CONST_DECL;
    t->as.const_decl = c;
    return t;
}

UCTopLevel* uc_tl_import(UCImport* i) {
    UCTopLevel* t = (UCTopLevel*)xcalloc(1, sizeof(UCTopLevel));
    t->kind = UC_TL_IMPORT;
    t->as.import = i;
    return t;
}

UCTopLevel* uc_tl_export(UCTopLevel* inner) {
    UCTopLevel* t = (UCTopLevel*)xcalloc(1, sizeof(UCTopLevel));
    t->kind = UC_TL_EXPORT;
    t->as.export_ = inner;
    return t;
}

UCTopLevel* uc_tl_extern(UCType* ty, UCString name, UCVec* params) {
    UCTopLevel* t = (UCTopLevel*)xcalloc(1, sizeof(UCTopLevel));
    t->kind = UC_TL_EXTERN;
    t->as.extern_.ty = ty;
    t->as.extern_.name = name;
    t->as.extern_.params = params;
    return t;
}

static void tl_free(void* p) {
    UCTopLevel* t = (UCTopLevel*)p;
    if (!t) return;
    switch (t->kind) {
        case UC_TL_FUNC_DEF:    uc_func_def_free(t->as.func_def); break;
        case UC_TL_FUNC_DECL:   uc_func_decl_free(t->as.func_decl); break;
        case UC_TL_STRUCT_DEF:  uc_struct_def_free(t->as.struct_def); break;
        case UC_TL_VAR_DECL:    uc_var_decl_free(t->as.var_decl); break;
        case UC_TL_CONST_DECL:  uc_const_decl_free(t->as.const_decl); break;
        case UC_TL_IMPORT:      uc_import_free(t->as.import); break;
        case UC_TL_EXPORT:      tl_free(t->as.export_); break;
        case UC_TL_EXTERN:
            type_free(t->as.extern_.ty);
            uc_string_free(&t->as.extern_.name);
            param_vec_free(t->as.extern_.params);
            break;
    }
    free(t);
}

void uc_top_level_free(UCTopLevel* t) { tl_free(t); }

/* ------------------------------------------------------------------------- */
/* UCStmt                                                                    */
/* ------------------------------------------------------------------------- */

UCStmt* uc_stmt_block(UCVec* stmts) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_BLOCK;
    s->as.block = stmts;
    return s;
}

UCStmt* uc_stmt_if(UCExpr* cond, UCStmt* then_b, UCStmt* else_b) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_IF;
    s->as.if_stmt.cond = cond;
    s->as.if_stmt.then_branch = then_b;
    s->as.if_stmt.else_branch = else_b;
    return s;
}

UCStmt* uc_stmt_while(UCExpr* cond, UCStmt* body) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_WHILE;
    s->as.while_stmt.cond = cond;
    s->as.while_stmt.body = body;
    return s;
}

UCStmt* uc_stmt_for(UCStmt* init, UCExpr* cond, UCExpr* step, UCStmt* body) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_FOR;
    s->as.for_stmt.init = init;
    s->as.for_stmt.cond = cond;
    s->as.for_stmt.step = step;
    s->as.for_stmt.body = body;
    return s;
}

UCStmt* uc_stmt_return(UCExpr* value) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_RETURN;
    s->as.ret = value;
    return s;
}

UCStmt* uc_stmt_break(void) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_BREAK;
    return s;
}

UCStmt* uc_stmt_continue(void) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_CONTINUE;
    return s;
}

UCStmt* uc_stmt_expr(UCExpr* e) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_EXPR;
    s->as.expr = e;
    return s;
}

UCStmt* uc_stmt_kw_free(UCExpr* e) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_FREE;
    s->as.expr = e;
    return s;
}

UCStmt* uc_stmt_decl(UCVarDecl* d) {
    UCStmt* s = (UCStmt*)xcalloc(1, sizeof(UCStmt));
    s->kind = UC_STMT_DECL;
    s->as.decl = d;
    return s;
}

static void stmt_free(void* p) {
    UCStmt* s = (UCStmt*)p;
    if (!s) return;
    switch (s->kind) {
        case UC_STMT_BLOCK:    stmt_vec_free(s->as.block); break;
        case UC_STMT_IF:
            expr_free(s->as.if_stmt.cond);
            stmt_free(s->as.if_stmt.then_branch);
            stmt_free(s->as.if_stmt.else_branch);
            break;
        case UC_STMT_WHILE:
            expr_free(s->as.while_stmt.cond);
            stmt_free(s->as.while_stmt.body);
            break;
        case UC_STMT_FOR:
            stmt_free(s->as.for_stmt.init);
            expr_free(s->as.for_stmt.cond);
            expr_free(s->as.for_stmt.step);
            stmt_free(s->as.for_stmt.body);
            break;
        case UC_STMT_RETURN:   expr_free(s->as.ret); break;
        case UC_STMT_BREAK:
        case UC_STMT_CONTINUE: break;
        case UC_STMT_EXPR:     expr_free(s->as.expr); break;
        case UC_STMT_FREE:     expr_free(s->as.expr); break;
        case UC_STMT_DECL:     uc_var_decl_free(s->as.decl); break;
    }
    free(s);
}

void uc_stmt_free(UCStmt* s) { stmt_free(s); }

/* ------------------------------------------------------------------------- */
/* Literal                                                                   */
/* ------------------------------------------------------------------------- */

UCLiteral uc_literal_int(long long v) {
    UCLiteral lit;
    memset(&lit, 0, sizeof(lit));
    lit.kind = UC_LIT_INT;
    lit.as.int_val = v;
    return lit;
}

UCLiteral uc_literal_float(double v) {
    UCLiteral lit;
    memset(&lit, 0, sizeof(lit));
    lit.kind = UC_LIT_FLOAT;
    lit.as.float_val = v;
    return lit;
}

UCLiteral uc_literal_char(char c) {
    UCLiteral lit;
    memset(&lit, 0, sizeof(lit));
    lit.kind = UC_LIT_CHAR;
    lit.as.char_val = c;
    return lit;
}

UCLiteral uc_literal_string(const char* s, size_t len) {
    UCLiteral lit;
    memset(&lit, 0, sizeof(lit));
    lit.kind = UC_LIT_STRING;
    lit.as.string_val = uc_string_new(s, len);
    return lit;
}

UCLiteral uc_literal_string_cstr(const char* s) {
    return uc_literal_string(s, s ? strlen(s) : 0);
}

UCLiteral uc_literal_true(void) {
    UCLiteral lit;
    memset(&lit, 0, sizeof(lit));
    lit.kind = UC_LIT_TRUE;
    return lit;
}

UCLiteral uc_literal_false(void) {
    UCLiteral lit;
    memset(&lit, 0, sizeof(lit));
    lit.kind = UC_LIT_FALSE;
    return lit;
}

void uc_literal_free(UCLiteral* lit) {
    if (!lit) return;
    if (lit->kind == UC_LIT_STRING) {
        uc_string_free(&lit->as.string_val);
    }
    memset(lit, 0, sizeof(*lit));
    lit->kind = UC_LIT_FALSE;  /* anything non-string so a second free is safe */
}

/* ------------------------------------------------------------------------- */
/* UCExpr                                                                    */
/* ------------------------------------------------------------------------- */

UCExpr* uc_expr_binary(UCBinaryOp op, UCExpr* lhs, UCExpr* rhs) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_BINARY;
    e->as.binary.op = op;
    e->as.binary.lhs = lhs;
    e->as.binary.rhs = rhs;
    return e;
}

UCExpr* uc_expr_unary(UCUnaryOp op, UCExpr* operand) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_UNARY;
    e->as.unary.op = op;
    e->as.unary.operand = operand;
    return e;
}

UCExpr* uc_expr_ternary(UCExpr* cond, UCExpr* then_e, UCExpr* else_e) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_TERNARY;
    e->as.ternary.cond = cond;
    e->as.ternary.then_e = then_e;
    e->as.ternary.else_e = else_e;
    return e;
}

UCExpr* uc_expr_call(UCExpr* callee, UCVec* args) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_CALL;
    e->as.call.callee = callee;
    e->as.call.args = args;
    return e;
}

UCExpr* uc_expr_index(UCExpr* target, UCExpr* index) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_INDEX;
    e->as.index.target = target;
    e->as.index.index = index;
    return e;
}

UCExpr* uc_expr_field(UCExpr* target, UCString field) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_FIELD;
    e->as.field.target = target;
    e->as.field.field = field;
    return e;
}

UCExpr* uc_expr_assign(UCExpr* target, UCExpr* value) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_ASSIGN;
    e->as.assign.target = target;
    e->as.assign.value = value;
    return e;
}

UCExpr* uc_expr_move(UCExpr* inner) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_MOVE;
    e->as.move_expr = inner;
    return e;
}

UCExpr* uc_expr_clone(UCExpr* inner) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_CLONE;
    e->as.clone_expr = inner;
    return e;
}

UCExpr* uc_expr_cast(UCType* ty, UCExpr* operand) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_CAST;
    e->as.cast.ty = ty;
    e->as.cast.operand = operand;
    return e;
}

UCExpr* uc_expr_alloc(UCType* alloc_type) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_ALLOC;
    e->as.alloc_type = alloc_type;
    return e;
}

UCExpr* uc_expr_sizeof(UCType* ty) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_SIZEOF;
    e->as.sizeof_ty = ty;
    return e;
}

UCExpr* uc_expr_ident(const char* name, size_t len) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_IDENT;
    e->as.ident = uc_string_new(name, len);
    return e;
}

UCExpr* uc_expr_literal(UCLiteral lit) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_LITERAL;
    e->as.literal = lit;
    return e;
}

UCExpr* uc_expr_block(UCVec* stmts, UCExpr* trailing) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_BLOCK;
    e->as.block.stmts = stmts;
    e->as.block.trailing = trailing;
    return e;
}

UCExpr* uc_expr_null(void) {
    UCExpr* e = (UCExpr*)xcalloc(1, sizeof(UCExpr));
    e->kind = UC_EXPR_NULL;
    return e;
}

static void expr_free(void* p) {
    UCExpr* e = (UCExpr*)p;
    if (!e) return;
    switch (e->kind) {
        case UC_EXPR_BINARY:
            expr_free(e->as.binary.lhs);
            expr_free(e->as.binary.rhs);
            break;
        case UC_EXPR_UNARY:
            expr_free(e->as.unary.operand);
            break;
        case UC_EXPR_TERNARY:
            expr_free(e->as.ternary.cond);
            expr_free(e->as.ternary.then_e);
            expr_free(e->as.ternary.else_e);
            break;
        case UC_EXPR_CALL:
            expr_free(e->as.call.callee);
            expr_vec_free(e->as.call.args);
            free(e->as.call.return_type);
            break;
        case UC_EXPR_INDEX:
            expr_free(e->as.index.target);
            expr_free(e->as.index.index);
            break;
        case UC_EXPR_FIELD:
            expr_free(e->as.field.target);
            uc_string_free(&e->as.field.field);
            break;
        case UC_EXPR_ASSIGN:
            expr_free(e->as.assign.target);
            expr_free(e->as.assign.value);
            break;
        case UC_EXPR_MOVE:    expr_free(e->as.move_expr); break;
        case UC_EXPR_CLONE:   expr_free(e->as.clone_expr); break;
        case UC_EXPR_CAST:    type_free(e->as.cast.ty);
                              expr_free(e->as.cast.operand); break;
        case UC_EXPR_ALLOC:   type_free(e->as.alloc_type); break;
        case UC_EXPR_SIZEOF:  type_free(e->as.sizeof_ty); break;
        case UC_EXPR_IDENT:   uc_string_free(&e->as.ident); break;
        case UC_EXPR_LITERAL: uc_literal_free(&e->as.literal); break;
        case UC_EXPR_BLOCK:
            stmt_vec_free(e->as.block.stmts);
            expr_free(e->as.block.trailing);
            break;
        case UC_EXPR_NULL:    break;
    }
    free(e);
}

void uc_expr_free(UCExpr* e) { expr_free(e); }

/* ------------------------------------------------------------------------- */
/* Module                                                                    */
/* ------------------------------------------------------------------------- */

UCModule* uc_module_new(UCVec* declarations) {
    UCModule* m = (UCModule*)xcalloc(1, sizeof(UCModule));
    m->declarations = declarations;
    return m;
}

void uc_module_free(UCModule* m) {
    if (!m) return;
    tl_vec_free(m->declarations);
    free(m);
}

/* ------------------------------------------------------------------------- */
/* Pretty-printing                                                           */
/* ------------------------------------------------------------------------- */

static void dump_indent(FILE* out, int n) {
    for (int i = 0; i < n; i++) fputc(' ', out);
}

static const char* bool_str(bool b) { return b ? "true" : "false"; }

static void type_dump(const UCType* t, FILE* out, int indent);
static void expr_dump(const UCExpr* e, FILE* out, int indent);
static void stmt_dump(const UCStmt* s, FILE* out, int indent);
static void tl_dump(const UCTopLevel* t, FILE* out, int indent);

static void type_dump(const UCType* t, FILE* out, int indent) {
    if (!t) { dump_indent(out, indent); fputs("Type(null)\n", out); return; }
    dump_indent(out, indent);
    fprintf(out, "Type %s", uc_type_kind_name(t->kind));
    switch (t->kind) {
        case UC_TYPE_POINTER:
        case UC_TYPE_MUTABLE_POINTER:
        case UC_TYPE_REF:
            fputc('\n', out);
            type_dump(t->as.inner, out, indent + 2);
            break;
        case UC_TYPE_ARRAY:
            fprintf(out, " length=%zu\n", t->as.array.length);
            type_dump(t->as.array.element, out, indent + 2);
            break;
        case UC_TYPE_FUNCTION:
            fputs("\n", out);
            dump_indent(out, indent + 2); fprintf(out, "Return:\n");
            type_dump(t->as.function.ret, out, indent + 4);
            dump_indent(out, indent + 2);
            fprintf(out, "Params (%zu):\n", uc_vec_len(t->as.function.params));
            for (size_t i = 0; i < uc_vec_len(t->as.function.params); i++) {
                type_dump((const UCType*)uc_vec_at(t->as.function.params, i),
                          out, indent + 4);
            }
            break;
        case UC_TYPE_NAMED:
            fprintf(out, " name=\"%s\"\n",
                    t->as.named.data ? t->as.named.data : "");
            break;
        default:
            fputc('\n', out);
            break;
    }
}

static void param_dump(const UCParam* p, FILE* out, int indent) {
    if (!p) return;
    dump_indent(out, indent);
    fprintf(out, "Param name=\"%s\"\n", p->name.data ? p->name.data : "");
    type_dump(p->ty, out, indent + 2);
}

static void struct_field_dump(const UCStructField* f, FILE* out, int indent) {
    if (!f) return;
    dump_indent(out, indent);
    fprintf(out, "Field name=\"%s\"\n", f->name.data ? f->name.data : "");
    type_dump(f->ty, out, indent + 2);
}

static void expr_dump(const UCExpr* e, FILE* out, int indent) {
    if (!e) { dump_indent(out, indent); fputs("Expr(null)\n", out); return; }
    dump_indent(out, indent);
    switch (e->kind) {
        case UC_EXPR_BINARY:
            fprintf(out, "Binary op=%s\n", uc_binary_op_name(e->as.binary.op));
            expr_dump(e->as.binary.lhs, out, indent + 2);
            expr_dump(e->as.binary.rhs, out, indent + 2);
            break;
        case UC_EXPR_UNARY:
            fprintf(out, "Unary op=%s\n", uc_unary_op_name(e->as.unary.op));
            expr_dump(e->as.unary.operand, out, indent + 2);
            break;
        case UC_EXPR_TERNARY:
            fputs("Ternary\n", out);
            fputs("  Cond:\n", out);
            expr_dump(e->as.ternary.cond, out, indent + 2);
            fputs("  Then:\n", out);
            expr_dump(e->as.ternary.then_e, out, indent + 2);
            fputs("  Else:\n", out);
            expr_dump(e->as.ternary.else_e, out, indent + 2);
            break;
        case UC_EXPR_CALL:
            fprintf(out, "Call args=%zu\n", uc_vec_len(e->as.call.args));
            expr_dump(e->as.call.callee, out, indent + 2);
            for (size_t i = 0; i < uc_vec_len(e->as.call.args); i++) {
                expr_dump((const UCExpr*)uc_vec_at(e->as.call.args, i),
                          out, indent + 2);
            }
            break;
        case UC_EXPR_INDEX:
            fputs("Index\n", out);
            expr_dump(e->as.index.target, out, indent + 2);
            expr_dump(e->as.index.index, out, indent + 2);
            break;
        case UC_EXPR_FIELD:
            fprintf(out, "Field field=\"%s\"\n",
                    e->as.field.field.data ? e->as.field.field.data : "");
            expr_dump(e->as.field.target, out, indent + 2);
            break;
        case UC_EXPR_ASSIGN:
            fputs("Assign\n", out);
            expr_dump(e->as.assign.target, out, indent + 2);
            expr_dump(e->as.assign.value, out, indent + 2);
            break;
        case UC_EXPR_MOVE:
            fputs("Move\n", out);
            expr_dump(e->as.move_expr, out, indent + 2);
            break;
        case UC_EXPR_CLONE:
            fputs("Clone\n", out);
            expr_dump(e->as.clone_expr, out, indent + 2);
            break;
        case UC_EXPR_CAST:
            fputs("Cast\n", out);
            type_dump(e->as.cast.ty, out, indent + 2);
            expr_dump(e->as.cast.operand, out, indent + 2);
            break;
        case UC_EXPR_ALLOC:
            fputs("Alloc\n", out);
            type_dump(e->as.alloc_type, out, indent + 2);
            break;
        case UC_EXPR_SIZEOF:
            fputs("Sizeof\n", out);
            type_dump(e->as.sizeof_ty, out, indent + 2);
            break;
        case UC_EXPR_IDENT:
            fprintf(out, "Ident name=\"%s\"\n",
                    e->as.ident.data ? e->as.ident.data : "");
            break;
        case UC_EXPR_LITERAL:
            fprintf(out, "Literal kind=%s value=", uc_literal_kind_name(e->as.literal.kind));
            switch (e->as.literal.kind) {
                case UC_LIT_INT:    fprintf(out, "%lld\n", e->as.literal.as.int_val); break;
                case UC_LIT_FLOAT:  fprintf(out, "%g\n",  e->as.literal.as.float_val); break;
                case UC_LIT_CHAR:   fprintf(out, "'%c'\n", e->as.literal.as.char_val); break;
                case UC_LIT_STRING: fprintf(out, "\"%s\"\n",
                                            e->as.literal.as.string_val.data
                                              ? e->as.literal.as.string_val.data : ""); break;
                case UC_LIT_TRUE:   fputs("true\n", out); break;
                case UC_LIT_FALSE:  fputs("false\n", out); break;
            }
            break;
        case UC_EXPR_BLOCK:
            fprintf(out, "ExprBlock stmts=%zu trailing=%s\n",
                    uc_vec_len(e->as.block.stmts),
                    bool_str(e->as.block.trailing != NULL));
            for (size_t i = 0; i < uc_vec_len(e->as.block.stmts); i++) {
                stmt_dump((const UCStmt*)uc_vec_at(e->as.block.stmts, i),
                          out, indent + 2);
            }
            if (e->as.block.trailing) {
                expr_dump(e->as.block.trailing, out, indent + 2);
            }
            break;
        case UC_EXPR_NULL:
            fputs("Null\n", out);
            break;
    }
}

static void stmt_dump(const UCStmt* s, FILE* out, int indent) {
    if (!s) { dump_indent(out, indent); fputs("Stmt(null)\n", out); return; }
    dump_indent(out, indent);
    switch (s->kind) {
        case UC_STMT_BLOCK:
            fprintf(out, "Block stmts=%zu\n", uc_vec_len(s->as.block));
            for (size_t i = 0; i < uc_vec_len(s->as.block); i++) {
                stmt_dump((const UCStmt*)uc_vec_at(s->as.block, i), out, indent + 2);
            }
            break;
        case UC_STMT_IF:
            fputs("If\n", out);
            dump_indent(out, indent + 2); fputs("Cond:\n", out);
            expr_dump(s->as.if_stmt.cond, out, indent + 4);
            dump_indent(out, indent + 2); fputs("Then:\n", out);
            stmt_dump(s->as.if_stmt.then_branch, out, indent + 4);
            if (s->as.if_stmt.else_branch) {
                dump_indent(out, indent + 2); fputs("Else:\n", out);
                stmt_dump(s->as.if_stmt.else_branch, out, indent + 4);
            }
            break;
        case UC_STMT_WHILE:
            fputs("While\n", out);
            dump_indent(out, indent + 2); fputs("Cond:\n", out);
            expr_dump(s->as.while_stmt.cond, out, indent + 4);
            dump_indent(out, indent + 2); fputs("Body:\n", out);
            stmt_dump(s->as.while_stmt.body, out, indent + 4);
            break;
        case UC_STMT_FOR:
            fputs("For\n", out);
            dump_indent(out, indent + 2); fputs("Init:\n", out);
            if (s->as.for_stmt.init) stmt_dump(s->as.for_stmt.init, out, indent + 4);
            else { dump_indent(out, indent + 4); fputs("(none)\n", out); }
            dump_indent(out, indent + 2); fputs("Cond:\n", out);
            if (s->as.for_stmt.cond) expr_dump(s->as.for_stmt.cond, out, indent + 4);
            else { dump_indent(out, indent + 4); fputs("(none)\n", out); }
            dump_indent(out, indent + 2); fputs("Step:\n", out);
            if (s->as.for_stmt.step) expr_dump(s->as.for_stmt.step, out, indent + 4);
            else { dump_indent(out, indent + 4); fputs("(none)\n", out); }
            dump_indent(out, indent + 2); fputs("Body:\n", out);
            stmt_dump(s->as.for_stmt.body, out, indent + 4);
            break;
        case UC_STMT_RETURN:
            fputs("Return\n", out);
            if (s->as.ret) expr_dump(s->as.ret, out, indent + 2);
            break;
        case UC_STMT_BREAK:    fputs("Break\n", out); break;
        case UC_STMT_CONTINUE: fputs("Continue\n", out); break;
        case UC_STMT_EXPR:
            fputs("ExprStmt\n", out);
            if (s->as.expr) expr_dump(s->as.expr, out, indent + 2);
            break;
        case UC_STMT_FREE:
            fputs("Free\n", out);
            expr_dump(s->as.expr, out, indent + 2);
            break;
        case UC_STMT_DECL:
            fputs("DeclStmt\n", out);
            if (s->as.decl) {
                dump_indent(out, indent + 2);
                fprintf(out, "VarDecl name=\"%s\"\n",
                        s->as.decl->name.data ? s->as.decl->name.data : "");
                type_dump(s->as.decl->ty, out, indent + 4);
                if (s->as.decl->init) expr_dump(s->as.decl->init, out, indent + 4);
            }
            break;
    }
}

static void tl_dump(const UCTopLevel* t, FILE* out, int indent) {
    if (!t) return;
    dump_indent(out, indent);
    switch (t->kind) {
        case UC_TL_FUNC_DEF:
            fprintf(out, "FuncDef name=\"%s\" params=%zu\n",
                    t->as.func_def->name.data ? t->as.func_def->name.data : "",
                    uc_vec_len(t->as.func_def->params));
            for (size_t i = 0; i < uc_vec_len(t->as.func_def->params); i++) {
                param_dump((const UCParam*)uc_vec_at(t->as.func_def->params, i),
                           out, indent + 2);
            }
            dump_indent(out, indent + 2); fputs("Return:\n", out);
            type_dump(t->as.func_def->return_ty, out, indent + 4);
            dump_indent(out, indent + 2); fputs("Body:\n", out);
            stmt_dump(t->as.func_def->body, out, indent + 4);
            break;
        case UC_TL_FUNC_DECL:
            fprintf(out, "FuncDecl name=\"%s\" params=%zu\n",
                    t->as.func_decl->name.data ? t->as.func_decl->name.data : "",
                    uc_vec_len(t->as.func_decl->params));
            for (size_t i = 0; i < uc_vec_len(t->as.func_decl->params); i++) {
                param_dump((const UCParam*)uc_vec_at(t->as.func_decl->params, i),
                           out, indent + 2);
            }
            dump_indent(out, indent + 2); fputs("Return:\n", out);
            type_dump(t->as.func_decl->return_ty, out, indent + 4);
            break;
        case UC_TL_STRUCT_DEF:
            fprintf(out, "StructDef name=\"%s\" fields=%zu\n",
                    t->as.struct_def->name.data ? t->as.struct_def->name.data : "",
                    uc_vec_len(t->as.struct_def->fields));
            for (size_t i = 0; i < uc_vec_len(t->as.struct_def->fields); i++) {
                struct_field_dump(
                    (const UCStructField*)uc_vec_at(t->as.struct_def->fields, i),
                    out, indent + 2);
            }
            break;
        case UC_TL_VAR_DECL:
            fprintf(out, "VarDecl name=\"%s\"\n",
                    t->as.var_decl->name.data ? t->as.var_decl->name.data : "");
            type_dump(t->as.var_decl->ty, out, indent + 2);
            if (t->as.var_decl->init) expr_dump(t->as.var_decl->init, out, indent + 2);
            break;
        case UC_TL_CONST_DECL:
            fprintf(out, "ConstDecl name=\"%s\"\n",
                    t->as.const_decl->name.data ? t->as.const_decl->name.data : "");
            type_dump(t->as.const_decl->ty, out, indent + 2);
            expr_dump(t->as.const_decl->value, out, indent + 2);
            break;
        case UC_TL_IMPORT:
            fprintf(out, "Import path=\"%s\" alias=\"%s\"\n",
                    t->as.import->path.data ? t->as.import->path.data : "",
                    t->as.import->alias.data ? t->as.import->alias.data : "");
            break;
        case UC_TL_EXPORT:
            fputs("Export\n", out);
            tl_dump(t->as.export_, out, indent + 2);
            break;
        case UC_TL_EXTERN:
            fprintf(out, "Extern name=\"%s\" params=%zu\n",
                    t->as.extern_.name.data ? t->as.extern_.name.data : "",
                    uc_vec_len(t->as.extern_.params));
            type_dump(t->as.extern_.ty, out, indent + 2);
            for (size_t i = 0; i < uc_vec_len(t->as.extern_.params); i++) {
                param_dump((const UCParam*)uc_vec_at(t->as.extern_.params, i),
                           out, indent + 2);
            }
            break;
    }
}

void uc_ast_dump(const UCModule* m, FILE* out) {
    if (!m) { fputs("Module(null)\n", out); return; }
    fprintf(out, "Module declarations=%zu\n", uc_vec_len(m->declarations));
    for (size_t i = 0; i < uc_vec_len(m->declarations); i++) {
        tl_dump((const UCTopLevel*)uc_vec_at(m->declarations, i), out, 1);
    }
}

/* ------------------------------------------------------------------------- */
/* Enum -> name                                                              */
/* ------------------------------------------------------------------------- */

const char* uc_type_kind_name(UCTypeKind k) {
    switch (k) {
        case UC_TYPE_VOID:            return "Void";
        case UC_TYPE_BOOL:            return "Bool";
        case UC_TYPE_CHAR:            return "Char";
        case UC_TYPE_INT:             return "Int";
        case UC_TYPE_I8:              return "I8";
        case UC_TYPE_I16:             return "I16";
        case UC_TYPE_I32:             return "I32";
        case UC_TYPE_I64:             return "I64";
        case UC_TYPE_UINT:            return "UInt";
        case UC_TYPE_U8:              return "U8";
        case UC_TYPE_U16:             return "U16";
        case UC_TYPE_U32:             return "U32";
        case UC_TYPE_U64:             return "U64";
        case UC_TYPE_F32:             return "F32";
        case UC_TYPE_F64:             return "F64";
        case UC_TYPE_USIZE:           return "USize";
        case UC_TYPE_ISIZE:           return "ISize";
        case UC_TYPE_POINTER:         return "Pointer";
        case UC_TYPE_MUTABLE_POINTER: return "MutablePointer";
        case UC_TYPE_REF:             return "Ref";
        case UC_TYPE_ARRAY:           return "Array";
        case UC_TYPE_FUNCTION:        return "Function";
        case UC_TYPE_NAMED:           return "Named";
    }
    return "Unknown";
}

const char* uc_binary_op_name(UCBinaryOp op) {
    switch (op) {
        case UC_BIN_ADD:    return "Add";
        case UC_BIN_SUB:    return "Sub";
        case UC_BIN_MUL:    return "Mul";
        case UC_BIN_DIV:    return "Div";
        case UC_BIN_MOD:    return "Mod";
        case UC_BIN_SHL:    return "Shl";
        case UC_BIN_SHR:    return "Shr";
        case UC_BIN_LT:     return "Lt";
        case UC_BIN_GT:     return "Gt";
        case UC_BIN_LE:     return "Le";
        case UC_BIN_GE:     return "Ge";
        case UC_BIN_EQ:     return "Eq";
        case UC_BIN_NE:     return "Ne";
        case UC_BIN_BIT_AND: return "BitAnd";
        case UC_BIN_BIT_OR:  return "BitOr";
        case UC_BIN_BIT_XOR: return "BitXor";
        case UC_BIN_AND:    return "And";
        case UC_BIN_OR:     return "Or";
        case UC_BIN_ASSIGN: return "Assign";
        case UC_BIN_ADD_ASSIGN: return "AddAssign";
        case UC_BIN_SUB_ASSIGN: return "SubAssign";
        case UC_BIN_MUL_ASSIGN: return "MulAssign";
        case UC_BIN_DIV_ASSIGN: return "DivAssign";
        case UC_BIN_MOD_ASSIGN: return "ModAssign";
        case UC_BIN_BIT_AND_ASSIGN: return "BitAndAssign";
        case UC_BIN_BIT_OR_ASSIGN:  return "BitOrAssign";
        case UC_BIN_BIT_XOR_ASSIGN: return "BitXorAssign";
        case UC_BIN_SHL_ASSIGN: return "ShlAssign";
        case UC_BIN_SHR_ASSIGN: return "ShrAssign";
    }
    return "Unknown";
}

const char* uc_unary_op_name(UCUnaryOp op) {
    switch (op) {
        case UC_UN_NEG:     return "Neg";
        case UC_UN_NOT:     return "Not";
        case UC_UN_BIT_NOT: return "BitNot";
        case UC_UN_DEREF:   return "Deref";
        case UC_UN_ADDR_OF: return "AddrOf";
        case UC_UN_PRE_INC:  return "PreInc";
        case UC_UN_PRE_DEC:  return "PreDec";
        case UC_UN_POST_INC: return "PostInc";
        case UC_UN_POST_DEC: return "PostDec";
    }
    return "Unknown";
}

const char* uc_literal_kind_name(UCLiteralKind k) {
    switch (k) {
        case UC_LIT_INT:    return "Int";
        case UC_LIT_FLOAT:  return "Float";
        case UC_LIT_CHAR:   return "Char";
        case UC_LIT_STRING: return "String";
        case UC_LIT_TRUE:   return "True";
        case UC_LIT_FALSE:  return "False";
    }
    return "Unknown";
}