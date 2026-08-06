
declare i64 @strlen(i8*)
declare i64 @write(i32, i8*, i64)
declare i64 @read(i32, i8*, i64)
declare i8* @malloc(i64)
declare void @free(i8*)
define i32 @main() {
entry:
  %t0 = add i32 0, 0
  ret i32 %t0
}

