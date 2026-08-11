// m0_29: Pointer declaration — non-owning pointer to a stack local
int main() {
    int x = 42;
    int* p = &x;
    return *p;            // 42
}
