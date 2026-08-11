// m0_42: unsafe { ... } block — FFI / raw-pointer escape hatch
// Per spec §10.3, `unsafe` suppresses borrow / ownership checks inside
// the block. The body itself is still ordinary UltraCPP code.
extern "C" {
    void* malloc(int size);
    void  free(void* ptr);
}

int main() {
    void* mem;
    int payload = 42;
    int* view;
    unsafe {
        mem  = malloc(8);
        view = (int*)mem;
        *view = payload;
    }
    unsafe {
        free(mem);
    }
    return payload;
}
