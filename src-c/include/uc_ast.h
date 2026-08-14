/* UltraCPP C compiler - AST type definitions
 *
 * C99 port of src/frontend/ast.rs.
 *
 * Memory ownership:
 *   - Every AST node is heap-allocated and returned by uc_*_new().
 *   - uc_*_free() releases the node AND all owned children (deep free).
 *   - Caller may pass NULL to any *_free(); it is a no-op.
 *   - After *_free(), the pointer is INVALID. Callers that retain a
 *     reference should set it to NULL themselves.
 *   - String fields are owned (UCString).  Copy-on-construct, free-on-release.
 */
#ifndef UC_AST_H
#define UC_AST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* ------------------------------------------------------------------------- */
/* Common owned-string and growable-array helpers                            */
/* ------------------------------------------------------------------------- */

typedef struct UCString {
    char* data;     /* owned, NUL-terminated; data[0..len] is meaningful */
    size_t len;     /* bytes excluding trailing NUL */
} UCString;

/* Owned growable array of void* (typeless by design; access via UC_VEC_AT). */
typedef struct UCVec {
    void** data;    /* owned; NULL only when len == 0 and never grown */
    size_t len;
    size_t cap;
} UCVec;

UCString uc_string_new(const char* s, size_t len);
UCString uc_string_new_cstr(const char* s);
/* Returns {NULL, 0}; used to represent Option::None for optional strings. */
UCString uc_string_empty(void);
void uc_string_free(UCString* s);
/* uc_string_move yields ownership of `s` to the returned value and zeroes
 * the source. The caller must NOT subsequently uc_string_free the source. */
UCString uc_string_move(UCString* s);

UCVec* uc_vec_new(void);
UCVec* uc_vec_push(UCVec* v, void* item);  /* may grow; returns possibly-new vec */
size_t uc_vec_len(const UCVec* v);
void* uc_vec_at(const UCVec* v, size_t i);   /* bounds-checked */
void uc_vec_free(UCVec* v, void (*item_free)(void*));

#define UC_VEC_AT(v, i, T) (((T*)(v)->data)[(i)])

/* ------------------------------------------------------------------------- */
/* Forward declarations                                                      */
/* ------------------------------------------------------------------------- */

typedef struct UCModule    UCModule;
typedef struct UCTopLevel  UCTopLevel;
typedef struct UCFuncDef   UCFuncDef;
typedef struct UCFuncDecl  UCFuncDecl;
typedef struct UCParam     UCParam;
typedef struct UCStructDef UCStructDef;
typedef struct UCStructField UCStructField;
typedef struct UCVarDecl   UCVarDecl;
typedef struct UCConstDecl UCConstDecl;
typedef struct UCImport    UCImport;
typedef struct UCType      UCType;
typedef struct UCStmt      UCStmt;
typedef struct UCExpr      UCExpr;

/* ------------------------------------------------------------------------- */
/* Type                                                                      */
/* ------------------------------------------------------------------------- */

typedef enum UCTypeKind {
    UC_TYPE_VOID,
    UC_TYPE_BOOL,
    UC_TYPE_CHAR,

    UC_TYPE_INT,
    UC_TYPE_I8,
    UC_TYPE_I16,
    UC_TYPE_I32,
    UC_TYPE_I64,

    UC_TYPE_UINT,
    UC_TYPE_U8,
    UC_TYPE_U16,
    UC_TYPE_U32,
    UC_TYPE_U64,

    UC_TYPE_F32,
    UC_TYPE_F64,

    UC_TYPE_USIZE,
    UC_TYPE_ISIZE,

    UC_TYPE_POINTER,
    UC_TYPE_MUTABLE_POINTER,
    UC_TYPE_REF,
    UC_TYPE_ARRAY,
    UC_TYPE_FUNCTION,
    UC_TYPE_NAMED
} UCTypeKind;

struct UCType {
    UCTypeKind kind;
    union {
        UCType* inner;                  /* Pointer / MutablePointer / Ref */
        struct {
            UCType* element;            /* owned */
            size_t length;              /* usize */
        } array;
        struct {
            UCType* ret;                /* owned */
            UCVec* params;              /* owned; each item is UCType* */
        } function;
        UCString named;                 /* struct/typedef name */
    } as;
};

/* Constructors for primitive type variants. */
UCType* uc_type_void(void);
UCType* uc_type_bool(void);
UCType* uc_type_char(void);
UCType* uc_type_int(void);
UCType* uc_type_i8(void);
UCType* uc_type_i16(void);
UCType* uc_type_i32(void);
UCType* uc_type_i64(void);
UCType* uc_type_uint(void);
UCType* uc_type_u8(void);
UCType* uc_type_u16(void);
UCType* uc_type_u32(void);
UCType* uc_type_u64(void);
UCType* uc_type_f32(void);
UCType* uc_type_f64(void);
UCType* uc_type_usize(void);
UCType* uc_type_isize(void);

/* Constructors for composite type variants (take ownership of inputs). */
UCType* uc_type_pointer(UCType* inner);                /* takes ownership */
UCType* uc_type_mutable_pointer(UCType* inner);
UCType* uc_type_ref(UCType* inner);
UCType* uc_type_array(UCType* element, size_t length);
UCType* uc_type_function(UCType* ret, UCVec* params);  /* takes ownership of ret & params */
UCType* uc_type_named(const char* name, size_t len);   /* copies name */

void uc_type_free(UCType* t);

/* ------------------------------------------------------------------------- */
/* Function / declaration / parameter                                        */
/* ------------------------------------------------------------------------- */

struct UCParam {
    UCString name;
    UCType* ty;          /* owned */
};

UCParam* uc_param_new(UCString name, UCType* ty);  /* takes ownership */
void uc_param_free(UCParam* p);

struct UCFuncDef {
    UCString name;
    UCVec* params;       /* owned; each item is UCParam* */
    UCType* return_ty;   /* owned */
    UCStmt* body;        /* owned */
};

UCFuncDef* uc_func_def_new(UCString name, UCVec* params,
                          UCType* return_ty, UCStmt* body);
void uc_func_def_free(UCFuncDef* f);

struct UCFuncDecl {
    UCString name;
    UCVec* params;       /* owned; each item is UCParam* */
    UCType* return_ty;   /* owned */
};

UCFuncDecl* uc_func_decl_new(UCString name, UCVec* params, UCType* return_ty);
void uc_func_decl_free(UCFuncDecl* f);

/* ------------------------------------------------------------------------- */
/* Struct / field / import / const / var                                     */
/* ------------------------------------------------------------------------- */

struct UCStructField {
    UCString name;
    UCType* ty;          /* owned */
};

UCStructField* uc_struct_field_new(UCString name, UCType* ty);
void uc_struct_field_free(UCStructField* f);

struct UCStructDef {
    UCString name;
    UCVec* fields;       /* owned; each item is UCStructField* */
};

UCStructDef* uc_struct_def_new(UCString name, UCVec* fields);
void uc_struct_def_free(UCStructDef* s);

struct UCImport {
    UCString path;
    UCString alias;      /* alias.data == NULL means None */
};

UCImport* uc_import_new(UCString path, UCString alias);
void uc_import_free(UCImport* i);

struct UCVarDecl {
    UCString name;
    UCType* ty;              /* owned */
    UCExpr* init;            /* NULL = None */
};

UCVarDecl* uc_var_decl_new(UCString name, UCType* ty, UCExpr* init);
void uc_var_decl_free(UCVarDecl* v);

struct UCConstDecl {
    UCString name;
    UCType* ty;              /* owned */
    UCExpr* value;           /* owned (always present) */
};

UCConstDecl* uc_const_decl_new(UCString name, UCType* ty, UCExpr* value);
void uc_const_decl_free(UCConstDecl* c);

/* ------------------------------------------------------------------------- */
/* Top-level                                                                 */
/* ------------------------------------------------------------------------- */

typedef enum UCTopLevelKind {
    UC_TL_FUNC_DEF,
    UC_TL_FUNC_DECL,
    UC_TL_STRUCT_DEF,
    UC_TL_VAR_DECL,
    UC_TL_CONST_DECL,
    UC_TL_IMPORT,
    UC_TL_EXPORT,
    UC_TL_EXTERN
} UCTopLevelKind;

struct UCTopLevel {
    UCTopLevelKind kind;
    union {
        UCFuncDef* func_def;
        UCFuncDecl* func_decl;
        UCStructDef* struct_def;
        UCVarDecl* var_decl;
        UCConstDecl* const_decl;
        UCImport* import;
        UCTopLevel* export_;          /* Box<TopLevel>: recursive */
        struct {
            UCType* ty;                /* owned */
            UCString name;
            UCVec* params;             /* owned; each item is UCParam* */
        } extern_;
    } as;
};

/* Constructors; each takes ownership of the heap allocations passed in. */
UCTopLevel* uc_tl_func_def(UCFuncDef* f);
UCTopLevel* uc_tl_func_decl(UCFuncDecl* f);
UCTopLevel* uc_tl_struct_def(UCStructDef* s);
UCTopLevel* uc_tl_var_decl(UCVarDecl* v);
UCTopLevel* uc_tl_const_decl(UCConstDecl* c);
UCTopLevel* uc_tl_import(UCImport* i);
UCTopLevel* uc_tl_export(UCTopLevel* inner);
UCTopLevel* uc_tl_extern(UCType* ty, UCString name, UCVec* params);

void uc_top_level_free(UCTopLevel* t);

/* ------------------------------------------------------------------------- */
/* Statements                                                                */
/* ------------------------------------------------------------------------- */

typedef enum UCStmtKind {
    UC_STMT_BLOCK,
    UC_STMT_IF,
    UC_STMT_WHILE,
    UC_STMT_FOR,
    UC_STMT_RETURN,
    UC_STMT_BREAK,
    UC_STMT_CONTINUE,
    UC_STMT_EXPR,
    UC_STMT_FREE,
    UC_STMT_DECL
} UCStmtKind;

struct UCStmt {
    UCStmtKind kind;
    union {
        UCVec* block;             /* owned; each item is UCStmt* */
        struct {
            UCExpr* cond;         /* owned */
            UCStmt* then_branch;  /* owned */
            UCStmt* else_branch;  /* NULL = None */
        } if_stmt;
        struct {
            UCExpr* cond;         /* owned */
            UCStmt* body;         /* owned */
        } while_stmt;
        struct {
            UCStmt* init;         /* NULL = None */
            UCExpr* cond;         /* NULL = None */
            UCExpr* step;         /* NULL = None */
            UCStmt* body;         /* owned */
        } for_stmt;
        UCExpr* ret;              /* NULL = None (bare `return;`) */
        UCExpr* expr;             /* expression-stmt or free-stmt */
        UCVarDecl* decl;
    } as;
};

UCStmt* uc_stmt_block(UCVec* stmts);                     /* takes ownership */
UCStmt* uc_stmt_if(UCExpr* cond, UCStmt* then_b, UCStmt* else_b);
UCStmt* uc_stmt_while(UCExpr* cond, UCStmt* body);
UCStmt* uc_stmt_for(UCStmt* init, UCExpr* cond, UCExpr* step, UCStmt* body);
UCStmt* uc_stmt_return(UCExpr* value);                  /* NULL = bare return */
UCStmt* uc_stmt_break(void);
UCStmt* uc_stmt_continue(void);
UCStmt* uc_stmt_expr(UCExpr* e);                        /* NULL = bare `;` */
UCStmt* uc_stmt_kw_free(UCExpr* e);  /* 'free' is a UC keyword */
UCStmt* uc_stmt_decl(UCVarDecl* d);

void uc_stmt_free(UCStmt* s);

/* ------------------------------------------------------------------------- */
/* Expressions                                                               */
/* ------------------------------------------------------------------------- */

typedef enum UCBinaryOp {
    UC_BIN_ADD, UC_BIN_SUB, UC_BIN_MUL, UC_BIN_DIV, UC_BIN_MOD,
    UC_BIN_SHL, UC_BIN_SHR,
    UC_BIN_LT, UC_BIN_GT, UC_BIN_LE, UC_BIN_GE,
    UC_BIN_EQ, UC_BIN_NE,
    UC_BIN_BIT_AND, UC_BIN_BIT_OR, UC_BIN_BIT_XOR,
    UC_BIN_AND, UC_BIN_OR,
    UC_BIN_ASSIGN,
    UC_BIN_ADD_ASSIGN, UC_BIN_SUB_ASSIGN, UC_BIN_MUL_ASSIGN, UC_BIN_DIV_ASSIGN,
    UC_BIN_MOD_ASSIGN,
    UC_BIN_BIT_AND_ASSIGN, UC_BIN_BIT_OR_ASSIGN, UC_BIN_BIT_XOR_ASSIGN,
    UC_BIN_SHL_ASSIGN, UC_BIN_SHR_ASSIGN
} UCBinaryOp;

typedef enum UCUnaryOp {
    UC_UN_NEG, UC_UN_NOT, UC_UN_BIT_NOT, UC_UN_DEREF, UC_UN_ADDR_OF,
    UC_UN_PRE_INC, UC_UN_PRE_DEC, UC_UN_POST_INC, UC_UN_POST_DEC
} UCUnaryOp;

typedef enum UCLiteralKind {
    UC_LIT_INT,
    UC_LIT_FLOAT,
    UC_LIT_CHAR,
    UC_LIT_STRING,
    UC_LIT_TRUE,
    UC_LIT_FALSE
} UCLiteralKind;

typedef struct UCLiteral {
    UCLiteralKind kind;
    union {
        long long int_val;
        double float_val;
        char char_val;          /* 0 allowed; kind == UC_LIT_CHAR means literal */
        UCString string_val;
    } as;
} UCLiteral;

UCLiteral uc_literal_int(long long v);
UCLiteral uc_literal_float(double v);
UCLiteral uc_literal_char(char c);
UCLiteral uc_literal_string(const char* s, size_t len);   /* copies s */
UCLiteral uc_literal_string_cstr(const char* s);
UCLiteral uc_literal_true(void);
UCLiteral uc_literal_false(void);
void uc_literal_free(UCLiteral* lit);

typedef enum UCExprKind {
    UC_EXPR_BINARY,
    UC_EXPR_TERNARY,
    UC_EXPR_UNARY,
    UC_EXPR_CALL,
    UC_EXPR_INDEX,
    UC_EXPR_FIELD,
    UC_EXPR_ASSIGN,
    UC_EXPR_MOVE,
    UC_EXPR_CLONE,
    UC_EXPR_CAST,
    UC_EXPR_ALLOC,
    UC_EXPR_SIZEOF,
    UC_EXPR_IDENT,
    UC_EXPR_LITERAL,
    UC_EXPR_BLOCK,
    UC_EXPR_NULL
} UCExprKind;

struct UCExpr {
    UCExprKind kind;
    union {
        struct {
            UCBinaryOp op;
            UCExpr* lhs;            /* owned */
            UCExpr* rhs;            /* owned */
        } binary;
        struct {
            UCExpr* cond;           /* owned */
            UCExpr* then_e;         /* owned */
            UCExpr* else_e;         /* owned */
        } ternary;
        struct {
            UCUnaryOp op;
            UCExpr* operand;        /* owned */
        } unary;
        struct {
            UCExpr* callee;         /* owned */
            UCVec* args;            /* owned; each item is UCExpr* */
        } call;
        struct {
            UCExpr* target;         /* owned */
            UCExpr* index;          /* owned */
        } index;
        struct {
            UCExpr* target;         /* owned */
            UCString field;         /* owned */
        } field;
        struct {
            UCExpr* target;         /* owned */
            UCExpr* value;          /* owned */
        } assign;
        UCExpr* move_expr;          /* owned */
        UCExpr* clone_expr;         /* owned */
        struct {
            UCType* ty;             /* owned */
            UCExpr* operand;        /* owned */
        } cast;
        UCType* alloc_type;         /* owned */
        UCType* sizeof_ty;          /* owned */
        UCString ident;
        UCLiteral literal;
        struct {
            UCVec* stmts;           /* owned; each item is UCStmt* */
            UCExpr* trailing;       /* NULL = no trailing expression */
        } block;
    } as;
};

/* Constructors take ownership of all heap-allocated inputs. */
UCExpr* uc_expr_binary(UCBinaryOp op, UCExpr* lhs, UCExpr* rhs);
UCExpr* uc_expr_ternary(UCExpr* cond, UCExpr* then_e, UCExpr* else_e);
UCExpr* uc_expr_unary(UCUnaryOp op, UCExpr* operand);
UCExpr* uc_expr_call(UCExpr* callee, UCVec* args);
UCExpr* uc_expr_index(UCExpr* target, UCExpr* index);
UCExpr* uc_expr_field(UCExpr* target, UCString field);
UCExpr* uc_expr_assign(UCExpr* target, UCExpr* value);
UCExpr* uc_expr_move(UCExpr* inner);
UCExpr* uc_expr_clone(UCExpr* inner);
UCExpr* uc_expr_cast(UCType* ty, UCExpr* operand);
UCExpr* uc_expr_alloc(UCType* alloc_type);
UCExpr* uc_expr_sizeof(UCType* ty);
UCExpr* uc_expr_ident(const char* name, size_t len);     /* copies */
UCExpr* uc_expr_literal(UCLiteral lit);                  /* moves string if any */
UCExpr* uc_expr_block(UCVec* stmts, UCExpr* trailing);
UCExpr* uc_expr_null(void);

void uc_expr_free(UCExpr* e);

/* ------------------------------------------------------------------------- */
/* Module                                                                    */
/* ------------------------------------------------------------------------- */

struct UCModule {
    UCVec* declarations;    /* owned; each item is UCTopLevel* */
};

UCModule* uc_module_new(UCVec* declarations);
void uc_module_free(UCModule* m);

/* ------------------------------------------------------------------------- */
/* Pretty-printing for debugging (--dump-ast) and tests                      */
/* ------------------------------------------------------------------------- */

/* Writes a human-readable, indented dump of the module to `out`.
 * Output is meant for diff-based verification against the Rust --dump-ast. */
void uc_ast_dump(const UCModule* m, FILE* out);

/* Returns a stable string for each enum kind (never NULL). Useful in tests
 * and for error messages. */
const char* uc_type_kind_name(UCTypeKind k);
const char* uc_binary_op_name(UCBinaryOp op);
const char* uc_unary_op_name(UCUnaryOp op);
const char* uc_literal_kind_name(UCLiteralKind k);

#endif /* UC_AST_H */