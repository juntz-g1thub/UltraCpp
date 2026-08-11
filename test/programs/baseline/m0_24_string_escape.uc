// m0_24: String escape sequences (\n, \t, \\)
int main() {
    i8* s1 = "a\nb";      // 3 chars: 'a', '\n', 'b'
    i8* s2 = "x\t\ty";    // 5 chars: 'x', '\t', '\', 't', 'y'  (raw \t escape)
    int len1 = sys$strlen(s1);
    int len2 = sys$strlen(s2);
    return len1 + len2;   // 3 + 5 = 8
}
