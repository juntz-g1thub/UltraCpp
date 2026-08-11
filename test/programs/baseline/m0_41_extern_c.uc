// m0_41: extern "C" { ... } FFI block
// Per spec §10.1, an extern "C" block declares C-linkage functions so
// that they can be linked against existing C libraries without name
// mangling.
extern "C" {
    int abs_int(int x);
}

int main() {
    int a = abs_int(-7);   // 7
    int b = abs_int(0);    // 0
    int c = abs_int(3);    // 3
    return a + b + c;      // 10
}
