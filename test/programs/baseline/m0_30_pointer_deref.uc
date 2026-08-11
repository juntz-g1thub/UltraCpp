// m0_30: Write through a dereferenced pointer
int main() {
    int x = 5;
    int* p = &x;
    *p = 100;            // write
    return x;            // 100
}
