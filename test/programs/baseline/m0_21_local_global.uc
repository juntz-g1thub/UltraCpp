// m0_21: Local vs global scope
int g = 100;

int read_global() {
    return g;
}

int main() {
    int local_val = 5;
    int shadow    = g + local_val;          // 105
    int outside   = read_global();          // 100
    return shadow + outside;                // 205
}
