// m0_25: String I/O via the compiler-builtin sys$write
int main() {
    i8* msg = "hi\n";
    int len = sys$strlen(msg);           // 3
    int n   = sys$write(1, msg, len);    // bytes written (>= 0)
    return len + n;                      // depends on runtime
}
