# UltraCPP 0.3.4 Release Notes

> **Version**: 0.3.4
> **Date**: 2026-08-27
> **Status**: Released
> **Previous version**: 0.3.3 (see [UltraCPP v0.3.4 Specification §0.3.3 → 0.3.4 deltas](./UltraCPP-v0.3.4-spec-en.md#033--034-deltas))

## Highlights

UltraCPP 0.3.4 introduces the UltraCPP-implemented standard library
bootstrap path (Stage 1 self-hosting) and completes the `abs_int` →
`uc_abs` migration. This version also closes three spec gaps (S6 FFI
extern body source policy, S8 deref-assign type safety, S9 main exit
code 8-bit semantics) that have been pending since the 0.3.3
implementation. The standard library functions previously emitted as
inline IR builtins are now written in UltraCPP itself under `lib/*.uc`.

## What's New

### Stdlib Bootstrap (`lib/*.uc`, 7 files)

UltraCPP now ships its own standard library written in UltraCPP itself.
These files live under `lib/` and are imported with `import "lib/...uc"`.

| File | Contents |
|---|---|
| `lib/print.uc`    | `uc_print_str` / `uc_print_num` / `uc_print_float` |
| `lib/string.uc`   | `uc_strlen` / `uc_strcpy` / `uc_strcmp` |
| `lib/memory.uc`   | `uc_memcpy` / `uc_memmove` (overlap-safe) / `uc_memset` |
| `lib/math.uc`     | `uc_abs` (replaces `abs_int`) |
| `lib/alloc.uc`    | `uc_alloc` / `uc_free` |
| `lib/sys/sys.uc`  | `sys::write` / `sys::read` / `sys::open` / `sys::close` / `sys::exit` / `sys::mmap` / `sys::brk` |
| `lib/sys/raw.uc`  | `uc_syscall` (x86_64 inline-asm syscall layer) |

### `abs_int` → `uc_abs` Migration

The historical `abs_int` builtin is no longer emitted as inline IR. It
has been removed from the `BUILTIN_SIGS[]` table in
`src-c/src/codegen.c` and is now implemented in UltraCPP at
`lib/math.uc::uc_abs`. Callers may continue to write `abs_int(x)` in
their source code — the codegen routes the call through extern
function lookup, resolved at link time against `uc_abs` in `lib/math.o`.

### Codegen Table Reduction

Following the runtime-architecture principle that the standard library
does not belong in the codegen builtins table, `BUILTIN_SIGS[]` now
contains only the language primitives plus the 6 stdlib-library names:

```
alloc / free / move / clone / mod / unmod / abs_int  (abs_int retained for back-compat, routes to uc_abs)
```

The 13 other builtin names that were previously emitted as inline IR
(`print` / `print_num` / `print_float` / `strlen` / `strcpy` /
`strcmp` / `memcpy` / `memmove` / `memset` / `sizeof_impl` /
`alignof_impl` / `is_null` / `clone_impl`) have been removed; their
functionality now lives in UltraCPP stdlib under `lib/*.uc` and is
resolved at link time.

### Spec Gap Fixes

Three pending spec gaps are closed in 0.3.4:

- **S6 — FFI extern body source policy**: A three-tier lookup path
  (stdlib → user code → libc) is now defined; codegen emits
  `declare external fn <name>` for linker resolution.
- **S8 — Deref-assign type safety**: `*p = X` now emits
  `store T X, T* %p` with the correct pointee type `T` (previously
  hardcoded `store i32 X, i8* %p`).
- **S9 — main exit code 8-bit truncation semantics**: The spec now
  states that UltraCPP preserves the full `int` return value and does
  **not** truncate it to 8 bits; the 8-bit truncation seen in scripts
  is bash `$?` behavior, not UltraCPP semantics.

### Spec Documentation

New bilingual specifications have been published
([zh-CN](./UltraCPP-v0.3.4-spec-zh-CN.md) /
[en](./UltraCPP-v0.3.4-spec-en.md)) covering the changes above. The
spec introduces a §11.0.4 "UltraCPP stdlib Bootstrap Path (Stage 1
design)" section, a new §14 "Spec Gap Index" (with entries for §S6 /
§S8 / §S9), and revisions to §7.0 / §10.1–§10.4 / §11.0.2 / §11.0.3.

## Migration from 0.3.3

**No breaking changes** for user code:

- `abs_int(x)` calls continue to work — codegen routes them to `uc_abs`.
- `print(...)`, `strlen(...)`, and similar calls continue to resolve
  at link time via the stdlib lookup or libc fallback.
- `BUILTIN_SIGS[]` is internal; there is no user-facing impact from
  the table reduction.

**Opting into the new stdlib** (optional):

```c
import "lib/print.uc"

int main() {
    uc_print_str("Hello, UltraCPP!\n");
    return 0;
}
```

## Known Limitations

The following items remain deferred to 0.3.5 / M1 or later:

- `lib/sync.uc` (mutex<T> / atomic<T>) — required for M5 threading tests.
- `lib/sys/raw.uc` arm64 / riscv64 support — x86_64 only in 0.3.4.
- `lib/print.uc::uc_print_float` complete itoa/ftoa — stub in 0.3.4.
- `UC_TYPE_MUTABLE_POINTER` dead-code cleanup — M1 scope.
- 16 m0 baseline tests still FAIL (P2 / P3 scope — array / struct /
  fn-ptr / multi-file / integration) — target 0.3.5.

## Baseline Status

- **M0 baseline**: 32/48 PASS / 16 FAIL (unchanged from the 0.3.3
  end-state; no new flips in 0.3.4 because the expected flips for S6 /
  S8 / S9 require the additional codegen / spec amendments currently
  scheduled for the 0.3.5 chain).
- **C unit tests**: 505/505 PASS (no new tests added; `lib/*.uc`
  files are UltraCPP source, not C, and are not exercised by the
  existing C unit-test framework).
- **0.3.3 → 0.3.4 net test impact**: 0 (no regressions, no new flips).

## Commit Manifest

7 implementation commits in the 0.3.4 chain (newest first):

```
3c6d625 codegen(0.3.4): remove abs_int inline IR + emit @uc_abs
0929d4e stdlib(0.3.4): add lib/sys/sys.uc + lib/sys/raw.uc system call layer
2b8872f stdlib(0.3.4): add uc_abs to lib/math.uc + new lib/alloc.uc
72fefca stdlib(0.3.4): add lib/memory.uc UltraCPP-implemented memory library
e42d8ac stdlib(0.3.4): add lib/string.uc UltraCPP-implemented string library
6d72aa8 stdlib(0.3.4): add lib/print.uc UltraCPP-implemented I/O library
```

(Plus 6 spec / docs commits: 0.3.4 bilingual spec publication,
HANDOFF / AGENTS / README refresh.)

## See Also

- [UltraCPP v0.3.4 Chinese Specification](./UltraCPP-v0.3.4-spec-zh-CN.md)
- [UltraCPP v0.3.4 English Specification](./UltraCPP-v0.3.4-spec-en.md)
- [0.3.4 Implementation Process](../.dev/drafts/0.3.4-implementation-process.md)
- [0.3.4 Implementation Plan](../.dev/drafts/0.3.4-implementation-plan.md)
- [0.3.4 Spec Text Changes](../.dev/drafts/0.3.4-spec-text-changes.md)
- [HANDOFF.md](../HANDOFF.md) — current session-resume state
