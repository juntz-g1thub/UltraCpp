// m0_43: main with argc / argv — the full ANSI-C entry point
int main(int argc, char** argv) {
    // Without a runtime argv inspection we just sanity-check the count:
    // argc is at least 1 (the program name).
    if (argc >= 1) {
        return 0;
    }
    return 1;
}
