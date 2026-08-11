// m0_50: Non-ASCII (Chinese) identifiers
// The lexer treats any non-control Unicode letter as a valid identifier
// start. This file uses CJK ideographs in identifier names.
int 计数() {
    int 总和 = 0;
    for (int 索引 = 1; 索引 <= 5; 索引++) {
        总和 = 总和 + 索引;
    }
    return 总和;                 // 15
}

int main() {
    return 计数();                // 15
}
