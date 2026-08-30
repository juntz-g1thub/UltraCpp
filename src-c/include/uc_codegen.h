/* UltraCPP C compiler - LLVM IR code generator
 *
 * C99 port of src/codegen/generator.rs.
 *
 * Phase 3 scope: produce LLVM IR byte-equivalent to the Rust compiler's
 * output for the 5 existing test programs (see tools/codegen_test.sh).
 *
 * Memory model:
 *   - UCCodeGenerator owns its output buffers (output, extern_decls,
 *     global_strings); each is grown via a realloc loop and released by
 *     uc_codegen_free.
 *   - uc_codegen_generate takes ownership of nothing (the AST is
 *     borrowed) and returns a heap-allocated NUL-terminated string that
 *     the caller must free.
 */
#ifndef UC_CODEGEN_H
#define UC_CODEGEN_H

#include "uc_ast.h"
#include "uc_error.h"

typedef struct UCCodeGenerator UCCodeGenerator;

UCCodeGenerator* uc_codegen_new(const char* module_name);
void uc_codegen_free(UCCodeGenerator* g);

void uc_codegen_add_imported_module(UCCodeGenerator* g, const char* module_path);

/* [0.3.5 commit 14d] Multi-TU build: when set, the generated IR omits the
 * builtin decls/defs and treats all functions in this TU as non-local
 * (bare-name mangling). Used for imported TUs so that the main TU is the
 * sole provider of builtin shims and so that user-defined symbols match
 * the caller's `@fn` declare (rather than `@<module>$fn`). */
void uc_codegen_set_skip_builtins(UCCodeGenerator* g, int skip);

/* Generate LLVM IR for the module. Returns a heap-allocated NUL-
 * terminated string (caller frees) on success; on error returns NULL
 * and writes details into `err`. */
char* uc_codegen_generate(UCCodeGenerator* g, const UCModule* m, UCError* err);

#endif /* UC_CODEGEN_H */