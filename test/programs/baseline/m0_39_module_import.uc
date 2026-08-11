// m0_39: #import preprocessor directive
// References lib/io.uc which provides sys$* builtins.
#import "../../lib/io.uc"

int main() {
    i8* s = "imported\n";
    int len = sys$strlen(s);
    sys$write(1, s, len);
    return len;          // bytes after \n = 9
}
