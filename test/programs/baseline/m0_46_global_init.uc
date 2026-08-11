// m0_46: Global with an initializing expression
int g = 42;

int read_g() {
    return g;
}

int main() {
    int copy_g = g;             // 42
    copy_g = copy_g + read_g(); // 84
    return copy_g - g;          // 42
}
