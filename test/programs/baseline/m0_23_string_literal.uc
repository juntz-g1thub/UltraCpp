// m0_23: String literal as rvalue
int main() {
    i8* s = "hello";
    int len = sys$strlen(s);
    return len;          // 5
}
