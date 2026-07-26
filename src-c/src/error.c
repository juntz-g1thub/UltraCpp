/* UltraCPP C compiler - error handling implementation */
#include "uc_error.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void uc_error_init(UCError* err) {
    if (!err) return;
    err->kind = UC_ERR_NONE;
    err->message[0] = '\0';
    err->line = 0;
    err->column = 0;
    err->filename = NULL;
}

void uc_error_set(UCError* err, UCErrorKind kind, int line, int column,
                  const char* filename, const char* fmt, ...) {
    if (!err) return;
    err->kind = kind;
    err->line = line;
    err->column = column;
    err->filename = filename;

    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, sizeof(err->message), fmt, args);
    va_end(args);
}

void uc_error_report(const UCError* err, FILE* out) {
    if (!err || !out || err->kind == UC_ERR_NONE) return;

    const char* fname = err->filename ? err->filename : "<input>";
    fprintf(out, "%s error at %s:%d:%d: %s\n",
            uc_error_kind_name(err->kind),
            fname, err->line, err->column, err->message);
}

const char* uc_error_kind_name(UCErrorKind kind) {
    switch (kind) {
        case UC_ERR_NONE:     return "none";
        case UC_ERR_LEXER:    return "Lexer";
        case UC_ERR_PARSER:   return "Parser";
        case UC_ERR_SEMANTIC: return "Semantic";
        case UC_ERR_CODEGEN:  return "Codegen";
        case UC_ERR_IO:       return "IO";
        case UC_ERR_INTERNAL: return "Internal";
    }
    return "Unknown";
}