/* UltraCPP C compiler - lexer */
#ifndef UC_LEXER_H
#define UC_LEXER_H

#include "uc_error.h"
#include "uc_token.h"

typedef struct UCLexer {
    const char* source;       /* borrowed, NUL-terminated */
    size_t source_len;
    const char* filename;     /* borrowed, for error reporting */

    size_t pos;               /* byte offset into source */
    int line;
    int column;

    UCError* error;           /* borrowed; populated on UC_TOK_ERROR */
} UCLexer;

void uc_lexer_init(UCLexer* l, const char* source, size_t source_len,
                   const char* filename, UCError* error);
void uc_lexer_reset(UCLexer* l);

UCToken uc_lexer_next(UCLexer* l);

#endif /* UC_LEXER_H */