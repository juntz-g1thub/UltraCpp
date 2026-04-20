// Builtin function declarations for code generation
// This module provides declarations for standard library functions

pub fn get_builtins_decls() -> String {
    r#"
declare i64 @strlen(i8*)
declare i64 @write(i32, i8*, i64)
declare i64 @read(i32, i8*, i64)
declare i8* @malloc(i64)
declare void @free(i8*)
"#
    .to_string()
}
