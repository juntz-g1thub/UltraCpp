// m0_44: Multi-file program — main in this file calls into a helper
// that lives in helper.uc. Together they exercise the
// "2+ .uc files 互调" scenario from §3.14.
//
// The build runner is expected to compile both files in this directory
// and link the resulting objects.
#import "helper.uc";
int main() {
    int a = 6;
    int b = 7;
    return helper.helper_mul(a, b);   // 42
}
