// io.uc - UltraCPP I/O Library
// sys$* functions are compiler builtins, auto-declared

int print_str(i8* s) {
    int len = sys$strlen(s);
    int result = sys$write(1, s, len);
    return 0;
}

int getValue() {
    return 42;
}
