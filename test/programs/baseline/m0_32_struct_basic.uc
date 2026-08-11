// m0_32: Struct definition with fields
// Note: in src-c/ the struct field syntax is `name: type;` (Rust-like),
// while function parameters stay `type name` (C-style). This is the
// current accepted form per src-c/tests/test_parser.c.
struct Point {
    x: int;
    y: int;
}

int main() {
    Point p;
    p.x = 3;
    p.y = 4;
    return p.x + p.y;     // 7
}
