/* UltraCPP C compiler - error handling */
#ifndef UC_ERROR_H
#define UC_ERROR_H

#include <stdio.h>

typedef enum {
    UC_ERR_NONE = 0,
    UC_ERR_LEXER,
    UC_ERR_PARSER,
    UC_ERR_SEMANTIC,
    UC_ERR_CODEGEN,
    UC_ERR_IO,
    UC_ERR_INTERNAL
} UCErrorKind;

typedef struct {
    UCErrorKind kind;
    char message[512];
    int line;
    int column;
    const char* filename;
} UCError;

void uc_error_init(UCError* err);
void uc_error_set(UCError* err, UCErrorKind kind, int line, int column,
                  const char* filename, const char* fmt, ...);
void uc_error_report(const UCError* err, FILE* out);
const char* uc_error_kind_name(UCErrorKind kind);

#endif /* UC_ERROR_H */