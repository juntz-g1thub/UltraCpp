/* UltraCPP C compiler - parser
 *
 * C99 port of src/frontend/parser.rs.
 *
 * Phase 2.2 scope: skeleton + top-level declarations.
 *   - Handles: Import, StructDef, Extern, VarDecl, FuncDecl, FuncDef,
 *     and Export (wrapping any of the above).
 *   - Function bodies support an empty block {} and a single
 *     `return [literal];` form. All other statement and expression forms
 *     are deferred to Phase 2.3+.
 *
 * Memory model:
 *   - The parser holds two UCTokens (current, peek) that it owns and frees
 *     in uc_parser_reset.
 *   - uc_parser_parse returns a heap-allocated UCModule on success
 *     (caller frees with uc_module_free) or NULL on error (details in
 *     the UCError* passed to init).
 *   - The parser borrows the UCLexer and the UCError; it does not own
 *     them.
 */
#ifndef UC_PARSER_H
#define UC_PARSER_H

#include "uc_ast.h"
#include "uc_error.h"
#include "uc_lexer.h"

/* [0.3.3 commit 8a] Single entry in the preprocessor macro table.
 * Each @ifdef(NAME) / @if defined(NAME) / @elif defined(NAME) check is
 * resolved by linear search through UCParser.macros (the volume is
 * tiny — only the predefined set; user-defined macros via @define are
 * deferred to a later commit). */
typedef struct UCMacro {
    char* name;            /* owned, NUL-terminated */
    char* value;           /* owned, NUL-terminated; presence implies
                            * "defined" — value is metadata only for
                            * future @if EXPR support. */
} UCMacro;

typedef struct UCParser {
    UCLexer* lexer;        /* borrowed */
    UCToken current;       /* owned, freed by uc_parser_reset */
    UCToken peek;          /* owned, freed by uc_parser_reset */
    UCError* error;        /* borrowed; populated on parse error */
    UCVec* macros;         /* owned; each item is UCMacro*; NULL until
                            * init_predefined_macros() runs in
                            * uc_parser_init. Freed by uc_parser_reset. */
    UCVec* declarations;   /* [0.3.6 commit 16-7-pre fix] top-level
                            * declarations sink; NULL until
                            * uc_parser_parse() allocates it. Items are
                            * UCTopLevel* and are freed (via
                            * uc_top_level_free) by uc_parser_reset
                            * only if parse did NOT hand ownership to
                            * a UCModule (i.e. error path). On success
                            * uc_parser_parse transfers ownership to
                            * the returned UCModule and resets this to
                            * NULL. Allows recursive parse_top_level()
                            * calls inside #ifdef/@ifdef truthy
                            * branches to push their TopLevel results
                            * here even though they have no access to
                            * the local `decls` variable in
                            * uc_parser_parse. */
} UCParser;

/* Initialize parser `p` to consume tokens from `lexer`.  On error,
 * `p->error->kind` will be set to UC_ERR_PARSER with line/column/message
 * and uc_parser_parse will return NULL.
 *
 * After init, callers should immediately invoke uc_parser_parse; the
 * parser pre-loads two tokens of lookahead before parsing begins. */
void uc_parser_init(UCParser* p, UCLexer* lexer, UCError* error);

/* Releases the current/peek tokens owned by the parser. Does NOT free
 * the lexer or error; the parser may be re-used after another init. */
void uc_parser_reset(UCParser* p);

/* Parse the full token stream and return a UCModule. Returns NULL on
 * error (details in p->error). On success the caller owns the returned
 * module. */
UCModule* uc_parser_parse(UCParser* p);

#endif /* UC_PARSER_H */