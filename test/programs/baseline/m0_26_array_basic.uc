// m0_26: Array declaration without initializer
int main() {
    int arr[10];
    arr[0] = 1;
    arr[1] = 2;
    arr[9] = 99;
    return arr[0] + arr[1] + arr[9];     // 102
}
