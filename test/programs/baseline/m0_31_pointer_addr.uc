// m0_31: Address-of operator (&) producing a pointer
int main() {
    int x = 5;
    int* p = &x;
    int* q = &x;                    // two non-owning aliases
    int sum = (*p) + (*q);          // 10
    return sum / 2;                 // 5
}
