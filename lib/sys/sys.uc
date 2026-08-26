// lib/sys/sys.uc - UltraCPP System Call Namespace
// Per .dev/drafts/0.3.4-implementation-plan.md commit 10e.
// 0.3.4 §10.4 sys:: namespace + runtime-architecture §6.
//
// High-level wrappers over uc_syscall. Provides sys::name(args) syntax
// for user code (per commit 8c parser partial).

// Linux x86_64 syscall numbers (most-used subset)
const int SYS_READ   = 0;
const int SYS_WRITE  = 1;
const int SYS_OPEN    = 2;
const int SYS_CLOSE   = 3;
const int SYS_EXIT    = 60;
const int SYS_MMAP    = 9;
const int SYS_BRK     = 12;

// write(fd, buf, n) -> n_written or -errno
export int sys::write(int fd, i8* buf, int n) {
    return uc_syscall(SYS_WRITE, fd, (int)buf, n, 0, 0, 0);
}

// read(fd, buf, n) -> n_read or -errno
export int sys::read(int fd, i8* buf, int n) {
    return uc_syscall(SYS_READ, fd, (int)buf, n, 0, 0, 0);
}

// open(path, flags, mode) -> fd or -errno
export int sys::open(i8* path, int flags, int mode) {
    return uc_syscall(SYS_OPEN, (int)path, flags, mode, 0, 0, 0);
}

// close(fd) -> 0 or -errno
export int sys::close(int fd) {
    return uc_syscall(SYS_CLOSE, fd, 0, 0, 0, 0, 0);
}

// exit(status)
export void sys::exit(int status) {
    uc_syscall(SYS_EXIT, status, 0, 0, 0, 0, 0);
}

// mmap(addr, len, prot, flags, fd, offset) -> addr or -errno
export int sys::mmap(int addr, int len, int prot, int flags,
                       int fd, int offset) {
    return uc_syscall(SYS_MMAP, addr, len, prot, flags, fd, offset);
}

// brk(addr) -> new break or -errno
export int sys::brk(int addr) {
    return uc_syscall(SYS_BRK, addr, 0, 0, 0, 0, 0);
}
