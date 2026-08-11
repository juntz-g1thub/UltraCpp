// m0_33: Struct field access through a variable
struct Rect {
    x: int;
    y: int;
    w: int;
    h: int;
}

int area(Rect r) {
    return r.w * r.h;
}

int main() {
    Rect r;
    r.x = 1;
    r.y = 2;
    r.w = 5;
    r.h = 7;
    int a = area(r);            // 35
    return a + r.x + r.y;       // 35 + 1 + 2 = 38
}
