// m0_28: Array indexing with a computed index
int main() {
    int arr[5];
    for (int i = 0; i < 5; i++) {
        arr[i] = i * i;
    }
    int idx = 4;
    return arr[idx] + arr[0];            // 16 + 0 = 16
}
