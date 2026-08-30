// m0_40: #include preprocessor directive — source-file copy form
// Pulls lib/math.uc into this translation unit so its exported `add`
// resolves as a local symbol.
#include "../../../lib/math.uc"

int main() {
    int r = add(5, 3);
    return r;    // 8
}
