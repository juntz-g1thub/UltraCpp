# UltraCPP 0.3.4 Language Specification

> **Version**: 0.3.4
>
> **Previous version**: 0.3.3 — [`UltraCPP-v0.3.3-spec-en.md`](./UltraCPP-v0.3.3-spec-en.md)
>
> **Status**: draft
>
> **Date**: 2026-08-21
>
> Same-version Chinese translation: [简体中文](./UltraCPP-v0.3.4-spec-zh-CN.md)

---

## Revision Summary

This version, building on 0.2.0, implements **21+ language design decisions** (decision numbering continues the historical D-1 .. D-8; this version newly adds Rule 1, 2A, 2B, Q1 .. Q6, Rule 22 .. 28). **The complete split of two permissions** (Rule 22) and the **`#modlaw` directive** (Rule 23) are the core conceptual changes of this revision.

> **[0.3.1]** This version, building on 0.3.0, **adds the complete §4.13 "Expression Classification: lvalue and rvalue"** chapter, and adds a new level-2.5 row for unary operator precedence in the §4.1 precedence table, unifies the 11 rows of the §4.6 assignment operator table with the "LHS must be an lvalue" constraint, rewrites rule 1 of §7.8 with a cross-reference to §4.13.2, adds `lvalue` / `rvalue` non-terminals to the §12.1 EBNF, and synchronizes the §12.3 appendix precedence table with §4.1 by adding level 2.5. **No new syntax is introduced, no existing semantics are modified**; only the lvalue concept and terminology are filled in, and the m0_42 deref-assign bug is fixed. See the 0.3.1 row of the changelog for details.

> **[0.3.4 Revisions]**:
> - S6 FFI extern body source policy (§S6)
> - S8 deref-assign type safety (§S8)
> - S9 main return / exit code 8-bit truncation semantics (§S9)
> - §11.0.2 UltraCPP stdlib paths materialized (lib/*.uc 6 files)
> - §11.0.3 codegen integration rewritten (abs_int migrated to lib/math.uc)
> - §11.0.5 UltraCPP stdlib bootstrap path new section
>
> See the 0.3.4 row of the changelog for details.

| Topic | Decision | One-line summary |
|------|---------|-----------|
| Basics | Rule 1 | Owning types are unrestricted between heap/stack; immutable during their live interval; freely accessible after expiry; storage location is inferred from the initialization expression. |
| References | Rule 2A | `&` references forbid owning; compile-time null-ref reports; per-thread scope isolation. |
| Pointers | Rule 2B | `T*` distinguishes owning from non-owning by the initialization expression; assignment `p1 = p2` **does not copy ownership** and **grants modification right to p1** (implicit `mod(p1)`). |
| Pointers | Q1 | `unique T` ≡ `T*`, the `unique` keyword may be omitted. |
| Declarations | Q2 | Reference declaration `T& a = b` (right side is an lvalue); pointer declaration `T* a = &b` (right side is address-of). |
| References | Q3 | A single reference type `T&`; exclusivity is controlled by `#modlaw exclusive` + `mod()` requests, not distinguished at the type level. |
| Pointers | Q4 | `p1 = p2` does not move; p1 obtains the modification right (implicit `mod(p1)`). |
| Pointers | Q5 | `move(p)` remains necessary to explicitly transfer ownership. |
| Pointers | Q6 | `const T*` / `T* const` have custom semantics (opposite of C++); **creating** a read-only pointer from an owning heap variable **is forbidden**. |
| Permissions | **Rule 22** | **Complete split of two permissions**: ownership (decides who deletes/frees) and modification right (decides who may modify the value) are independent. |
| Directives | **Rule 23** | New `#modlaw` directive, with 6 legal combinations. |
| Expressions | **Rule 24** | New `mod(ref_expr)` to request the modification right; `unmod(ref_expr)` to release it (usually omitted). |
| Threads | **Rule 25** | Cross-thread access must be marked `shared` (compile-time enforced); the compiler auto-generates mutexes. |
| Threads | **Rule 26** | Explicit `move_to_thread(p, tid)` or `spawn_thread_with(tid, p)` for cross-thread ownership transfer. |
| Threads | **Rule 27** | `__thread int x;` (GCC style) thread-local storage. |
| Threads | **Rule 28** | Multi-threaded memory layout: stack / heap / global / TLS each have their own lifetime domain. |

> **On the text differences between 0.2.0 and 0.3.0**: This version **rewrites** the assignment-move rule of 0.2.0 §7.1 to "assignment = implicit `mod()` + does not transfer ownership" (Q4=a), **rewrites** the reference declaration syntax in §3.3 to "`T& a = b` (right side is an lvalue)" (Q2), and **removes the `T&mut` type** (Q3 inverted) — references are unified as `T&`, and exclusivity is controlled by `#modlaw exclusive` + `mod()` requests. The remaining chapters keep the 0.2.0 design, with newly added chapters §4.9–4.10, §9.9, and §13.

---

## Changelog

| Decision | Section | Description |
|------|------|------|
| **0.3.4** | **2026-08-21** | **This version (S6/S8/S9 spec gap fixes + stdlib bootstrap path + §11.0.2 materialization + abs_int → uc_abs migration + §11.0.5 new section). 4 Type A chapter diffs (§7.0/§10.1-§10.4/§11.0.2/§11.0.3) + 4 Type B new sections (§11.0.5/§14/§S6/§S8/§S9).** |
| **S4 (new in 0.3.2)** | §4.1, §4.8 (revised), §4.8.1 (new), §12.1, §12.3 | **C-style explicit type cast `(T)expr`**: add the complete §4.8.1 subsection (5 sub-sections: 8-row semantic type-conversion table; fully equivalent to functional `T(expr)`; 4 example categories: integer↔pointer / width / FFI / type assertion; compile-time checks; cross-references). §4.8 adds a note that `(T)` is a cast when T is a type name; §4.1 main table + §12.3 appendix table synchronously add a C-style cast row at level 2.5; §12.1 EBNF adds the `cast_expression` production and adds `'cast' '(' type ',' expression ')'` to `unary_expression`. **Fixes m0_42 deref-assign** by making `*((int*)malloc(8))` compile, backward compatible. |
| **S5 (new in 0.3.2)** | §6.1 (revised), §6.2 (revised), §6.2.1 (new) | **Function return-type inference rules**: add the complete §6.2.1 subsection (5 sub-sections: 3-level priority builtin > user > extern; 7-row context-propagation table; void-function constraints; codegen-integration pseudocode; cross-references). §6.1 adds 3 return-type compile-time checks; §6.2 adds cross-references to §6.2.1 and §11.0. **Fixes m0_41 abs_int link**: the extern symbol table is populated by the parser when parsing `extern "C"` blocks (commit b45f851); codegen uses `lookup_extern_func` to obtain the return type, avoiding the hard-coded `i32` mismatch. |
| **§11.0 (new in 0.3.2)** | §11 (revised), §11.0 (new), §11.1–§11.6 (cross-references) | **Builtin signature master table**: add §11.0 (4 sub-sections: 16-row builtin table covering I/O / string / memory / utility / math; 12-row LLVM IR type-mapping table; codegen integration `builtin_sigs[]` array pseudocode; flow for adding new builtins; cross-references). §11 heading gains the 0.3.2 revised note + §11.0 overview reference; §11.1–§11.6 each gain a two-way cross-reference note to §11.0. `move` / `alloc` are marked as type-parameterized builtins. |
| Rule 1 | §3.2 | Owning types are unrestricted between heap/stack; storage location is inferred from init. |
| Rule 2A | §3.3 | References `&` forbid owning; compile-time null-ref reports. |
| Rule 2B | §3.2 | `T*` decides owning vs non-owning by init; assignment does not transfer ownership, grants the modification right. |
| Q1 | §3.7 | `unique T` ≡ `T*`; `unique` may be omitted. |
| Q2 | §3.3, §12.1 | Right side of reference declaration is an lvalue; right side of pointer declaration is address-of. |
| Q3 | §3.3 | **Single reference type** `T&`: all references are unified as `T&`; exclusivity is controlled by `#modlaw exclusive` + `mod()` requests, not distinguished at the type level. |
| Q4 | §3.2, §7.1 | Assignment `p1 = p2` **does not copy ownership** and **grants modification right to p1** (implicit `mod(p1)`). |
| Q5 | §7.4 | `move(p)` remains necessary to explicitly transfer ownership. |
| Q6 | §3.8, §7.10 | `const T*` / `T* const` have custom semantics (opposite of C++); owning heap is forbidden. |
| **Rule 22** | §1.2, §3, §7 | Ownership and modification right are completely split. |
| **Q3 inverted** *(revised in 0.3.0)* | §3.3, §7.8, §7.9 (removed), §12.1, §12.2, §2.3, §2.6 | **Remove the `T&mut` type**: references are unified as `T&`; exclusivity is now controlled by `#modlaw exclusive` + `mod()` requests, no longer a type-level distinction. |
| **Rule 23** | §9.9 | New `#modlaw <perm> <scope>` directive (6 legal combinations). |
| **Rule 24** | §4.9, §4.10 | New `mod(ref_expr)` and `unmod(ref_expr)` expressions. |
| **Rule 25** | §13.1, §13.3 | Cross-thread access must be marked `shared`; mutexes are auto-generated by the compiler and may be explicit from the programmer. |
| **Rule 26** | §13.2 | `move_to_thread(p, tid)` / `spawn_thread_with(tid, p)`. |
| **Rule 27** | §13.5 | `__thread` thread-local storage. |
| **Rule 28** | §13.4 | Multi-threaded memory layout: stack (thread-local) / heap (shared) / global (shared) / TLS (thread-local). |
| **S2 (revised in 0.3.1)** | §4.1, §4.6, §4.13 (new), §7.8, §12.1, §12.3 | **lvalue / rvalue concepts made explicit**: add the complete §4.13 chapter (definitions + classification table + assignment-context constraint + codegen implementation constraint + cross-references); §4.1 precedence table adds level 2.5 unary ops (` * ` ` & ` ` + ` ` - ` ` ! ` ` ~ ` `mod` `unmod` prefix `++` `--`); all 11 rows of the §4.6 table uniformly add the "LHS must be an lvalue (§4.13.2)" constraint; §7.8 creation rule 1 references §4.13.2 + adds legal/illegal examples; §12.1 EBNF adds `lvalue` / `rvalue` rules + `assignment_expression` LHS annotation; §12.3 appendix precedence table synchronously adds level 2.5. **Fixes m0_42 deref-assign bug** (`*view = payload`). No new syntax, no semantic change, backward compatible. |

---

## 1. Overview

### 1.1 Language Goals

UltraCPP is a systems programming language that combines the **familiarity of C++ syntax** with **Rust-style memory-safety guarantees**, without needing a garbage collector.

**Core goals:**
- Zero-cost abstractions
- Compile-time memory safety
- C++ compatibility for easy migration
- No garbage collector

### 1.2 Design Principles (focus: two independent permissions)

1. **Safe by default**: enforce memory safety at compile time wherever possible
2. **Explicit over implicit**: ownership, mutability, and safety semantics are visible in the syntax
3. **Pragmatic compatibility**: leverage C++ developer familiarity
4. **Minimal runtime**: no heavy runtime, suitable for systems programming
5. **Two independent permissions** *(new in 0.3.0: Rule 22)*:
   - **Ownership**: decides who is responsible for `delete` / `free`; **a single owner**; transferable (`move(p)`); non-copyable.
   - **Modification right (`mod`)**: decides who can modify the value through a reference; may be many, few, or exclusive; requested on demand by the programmer.

> **[0.3.0 · Rule 22]** This is the most important conceptual change of 0.3.0. The 0.2.0 notion of "ownership" actually conflated the modification right; 0.3.0 splits the two. Example: `int& r = x;` (a shared reference) **has** ownership borrow rights by default (C++-style reference), but to modify `x`, the programmer must **additionally** call `mod(r)` to request the modification right (see §3.3, §4.9 for the latently-writable reference semantics).

### 1.3 Symbol Conventions

| Symbol | Meaning |
|------|------|
| `T` | owned value (stack / global / TLS) |
| `unique T` ≡ `T*` | owning pointer (generic form, see §3.7) |
| `T*` | Pointer: right side is `&b` → non-owning stack/global pointer; right side is `alloc(T)` → owning pointer |
| `T&` | **The sole reference type**: must be initialized, non-null, non-owning; potentially writable (requires `mod()`); multiple may coexist; thread-shareable |
| `T* p = &x` | p is a non-owning stack/global pointer (similar to the reference rule) |
| `T* p = alloc(T)` | p is an owning pointer |
| `&expr` | Create a `T&` reference to `expr` |
| `mod(ref_expr)` | Request the modification right (per `#modlaw` policy) |
| `unmod(ref_expr)` | Release the modification right (usually omitted; relies on scope) |
| `move(p)` | Transfer ownership (p becomes invalid) |
| `move_to_thread(p, tid)` | Cross-thread ownership transfer |
| `shared` | Cross-thread visibility annotation (compile-time enforced) |
| `__thread T x` | Thread-local storage (per-thread independent) |
| `#modlaw <perm> <scope>` | Policy directive (6 legal combinations) |
| `const T*` | **Pointer-lock** (cannot rebind) + lifetime-safe + cannot write through it (custom semantics, **opposite of C++**) |
| `T* const` | **Data read-only view** (can rebind, but cannot write through it) |
| `alloc(T)` | Allocate heap memory for type T |
| `free(ptr)` | Release heap memory |
| `clone(ptr)` | Clone the pointer (an independent copy with its own ownership) |

---

## 2. Lexical Structure

### 2.1 Source File Conventions

```
*.uc   — UltraCPP source files (recommended)
*.upp  — UltraCPP source files (compatible)
```

### 2.2 Token Types

| Category | Examples |
|------|------|
| **Keywords** | `if`, `else`, `while`, `for`, `return`, `struct`, `export`, `import`, `const`, `unique`, `move`, `free`, `alloc`, `null`, `true`, `false`, `void`, `extern`, `unsafe`, `typedef`, `clone`, `mod`, `unmod`, `shared`, `__thread`, `move_to_thread` |
| **Identifiers** | `foo`, `myVariable`, `_private`, `CamelCase` |
| **Literals** | `42`, `3.14`, `'x'`, `"hello"`, `true`, `false` |
| **Operators** | `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`, `&`, `|`, `^`, `~`, `<<`, `>>`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=` |
| **Separators** | `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `::` |
| **Preprocessor** | `#import`, `#include`, `#ifdef`, `#ifndef`, `#endif`, `#define`, `#modlaw` *(new in 0.3.0)* |

> **[0.3.0 · Rule 23, 24, 26, 27]** New keywords: `mod`, `unmod`, `shared`, `__thread`, `move_to_thread`. New preprocessor directive: `#modlaw`.

### 2.3 Keywords (Reserved Words)

```
if          else        while       for         return
struct      export      import      const       typedef
unique      move        free        alloc       null
true        false       void        extern      unsafe
as          static      clone
mod         unmod       shared      __thread    move_to_thread
```

> **[New in 0.3.0]** `mod`, `unmod`, `shared`, `__thread`, `move_to_thread`. See §4.9, §4.10, §13.

### 2.4 Identifier Rules

```
identifier ::= letter (letter | digit)*
letter     ::= 'a'..'z' | 'A'..'Z' | '_'
digit      ::= '0'..'9'
```

- Identifiers are case-sensitive
- No length limit (implementation-defined)
- Must not conflict with keywords

### 2.5 Literals

#### Integer Literals
```
int-literal ::= decimal | hex | octal | binary
decimal     ::= [1-9][0-9]*
hex         ::= '0x'[0-9a-fA-F]+
octal       ::= '0'[0-7]+
binary      ::= '0b'[01]+
```

#### Float Literals
```
float-literal ::= [0-9]+'.'[0-9]+ | '.'[0-9]+ | [0-9]+'.'
```

#### Character Literals
```
char-literal ::= "'" character "'"
```

#### String Literals
```
string-literal ::= '"' (character | escape)* '"'
escape         ::= '\n' | '\t' | '\r' | '\\' | '\'' | '\"' | '\x'[0-9a-fA-F]{2}
```

#### Boolean Literals
```
true  // boolean true
false // boolean false
```

### 2.6 Operators and Separators

| Operator | Description |
|--------|------|
| `+` | Addition |
| `-` | Subtraction / negation |
| `*` | Multiplication / dereference |
| `/` | Division |
| `%` | Modulo |
| `=` | Assignment (between `T*` triggers **modification-right transfer** rather than ownership transfer, see §7.1) |
| `==` | Equality |
| `!=` | Inequality |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |
| `&&` | Logical AND |
| `\|\|` | Logical OR |
| `!` | Logical NOT |
| `&` | Unary prefix: create a `T&` reference (see §3.3); binary infix: bitwise AND |
| `\|` | Bitwise OR |
| `^` | Bitwise XOR |
| `~` | Bitwise NOT |
| `<<` | Left shift |
| `>>` | Right shift |
| `++` | Increment |
| `--` | Decrement |
| `->` | Arrow (pointer member access) |
| `.` | Dot (member access) |
| `::` | Scope resolution |

> **[Revised in 0.3.0]** The unary prefix `&` creates the sole reference type `T&`. Whether writing is actually allowed is decided by `mod()` / `#modlaw` (see Rule 24). 0.3.0 no longer distinguishes "shared-writable" from "exclusive-writable" references — `T&mut` has been removed.

### 2.7 Comments

```cpp
// Single-line comment

/*
 * Multi-line comment
 */
```

### 2.8 Whitespace

Spaces, tabs, and newlines are ignored when separating tokens. Line endings are retained for error reporting.

---

## 3. Type System *(rewritten in 0.3.0)*

### 3.1 Basic Types

| Type | Description | Size |
|------|------|------|
| `void` | No value | - |
| `bool` | Boolean | 1 byte |
| `char` | Character | 1 byte |
| `int` | Signed integer | 4 bytes |
| `i8` | 8-bit signed | 1 byte |
| `i16` | 16-bit signed | 2 bytes |
| `i32` | 32-bit signed | 4 bytes |
| `i64` | 64-bit signed | 8 bytes |
| `uint` | Unsigned integer | 4 bytes |
| `u8` | 8-bit unsigned | 1 byte |
| `u16` | 16-bit unsigned | 2 bytes |
| `u32` | 32-bit unsigned | 4 bytes |
| `u64` | 64-bit unsigned | 8 bytes |
| `f32` | 32-bit float | 4 bytes |
| `f64` | 64-bit double | 8 bytes |
| `usize` | Unsigned size | platform-dependent |
| `isize` | Signed size | platform-dependent |

### 3.2 Pointer Types *(rewritten in 0.3.0: Rule 2B, Q4)*

**`T*` distinguishes owning from non-owning by its initialization expression** (Rule 2B). This is the core difference in pointer semantics between 0.3.0 and 0.2.0.

```cpp
T*              // Pointer: right side &x → non-owning stack/global pointer; right side alloc(T) → owning heap pointer
T*              // At any time, exactly one owner; assignment p1 = p2 does not copy ownership, grants p1 the modification right (implicit mod(p1))
```

**Examples (must distinguish two forms):**

```cpp
int x = 42;

// === Non-owning stack pointer: right side is address-of ===
int* p = &x;        // p points to x, p itself does not own x
*p;                 // ✅ read
// *p = 100;        // ❌ compile error: p has no mod right by default (requires mod(p), see §4.9)
int* p2 = &x;       // ✅ multiple non-owning pointers may coexist

// === Owning heap pointer: right side is alloc ===
int* h = alloc(int);   // h owns
*h = 100;              // ✅ h has the modification right by default (owning pointer is "mod-by-default")
int* h2 = alloc(int);
int* h3 = h2;          // ⚠️ implicit mod(h3): h3 gets the modification right, h2 still owns
                      //    ownership is not transferred; h3 does not own, so it cannot free(h3)
free(h2);              // ✅ h2 is the owner
```

**Example A (canonical declaration syntax, Q2):**

```cpp
int x = 42;

// ✓ Reference declaration: right side is an lvalue
int& r = x;       // r is a reference to x (potentially writable, C++ style)

// ✓ Pointer declaration: right side is address-of
int* p = &x;      // p is x's address (non-owning)

// ✗ Type mismatch (must appear as a counter-example)
// int& r2 = &x;   // error: T& cannot accept T*
// int* p2 = x;    // error: T* cannot accept T
```

**Key rules (Rule 2B):**

1. **Decided by the initialization expression**: `T* p = &x` → p does not own x; `T* p = alloc(T)` → p owns.
2. **Exactly one owner at any time**: an allocated object has at most one owning pointer (`unique T` is equivalent to `T*`).
3. **Assignment `p1 = p2` does not copy ownership** and **grants p1 the modification right** (route 2: implicit `mod(p1)`); the owner is unchanged (Q4).
4. **`move(p)`** explicitly transfers ownership (Q5).
5. **`readonly` / `const T*` / `T* const`** control read-only permissions, see §3.8 and §7.10 (**semantics opposite to C++**).

### 3.3 Reference Types *(rewritten in 0.3.0: remove T&mut, Q2, Rule 22)*

UltraCPP has only one reference type `T&` — **must be initialized**, **cannot be null**, **does not own** the referent; ownership always remains with the owner.

> **[0.3.0 · Rule 22 + Q3 inverted]** 0.2.0 divided references into `T&` (immutable) and `T&mut` (exclusive mutable). 0.3.0 **removes `T&mut`** and unifies them into the single reference type `T&`. **Exclusivity** is no longer a type-level distinction; it is controlled by the `#modlaw exclusive` policy + `mod()` requests. Whether writing is actually allowed is also decided by `mod()` and `#modlaw`.

```cpp
T&     // latently-writable reference: no mod by default, requires mod() to write
```

**Declaration syntax (Q2):**

```cpp
int x = 42;
int& r = x;          // Reference declaration: right side is an lvalue (no &)
int& r2 = x;         // ✅ multiple T& may coexist
// int& r3 = &x;      // ❌ error: T& cannot accept T*
// int& r4 = 42;      // ❌ error: T& right side must be an lvalue
```

**Writability (Rule 24):**

```cpp
int x = 42;
int& r = x;          // r is a reference to x, no mod by default
int v = r;           // ✅ read
// r = 100;          // ❌ compile error: no mod requested
mod(r);              // ✅ request modification right (per #modlaw)
*r = 100;            // ✅ write
```

**Comparison table (0.1.0 / 0.2.0 / 0.3.0):**

| Version | Reference Types | Exclusivity Mechanism | Write Permission |
|---|---|---|---|
| 0.1.0 | T& (C++-style mutable reference) | None | Direct write |
| 0.2.0 | T& (immutable) + T&mut (exclusive mutable) | Type-level | T& read-only; T&mut exclusive write |
| 0.3.0 | **T& sole** | `#modlaw exclusive` + `mod()` | Requires mod(), behavior per #modlaw |

### 3.4 Function Pointer Types

```cpp
int (*)(int, int)       // Function type: takes two ints, returns int (C++ style)
void (*)(const char*)   // Function pointer returning void
```

**Examples:**
```cpp
typedef int (*Comparator)(int, int);
typedef void (*Callback)(const char*);

Comparator cmp;
Callback cb;
```

### 3.5 Array Types

```cpp
T[n]  // Fixed-length array containing n elements of type T
```

**Examples:**
```cpp
int[10] arr;           // array of 10 ints
char[256] buffer;      // 256-byte buffer
int[3] nums = [1, 2, 3];  // initialized array
```

### 3.6 Struct Types

```cpp
struct Point {
    double x;
    double y;
}

struct Rectangle {
    Point origin;
    double width;
    double height;
}
```

### 3.7 Type Modifiers (`unique T` ≡ `T*`) *(Q1, carried over from 0.2.0)*

| Modifier | Meaning |
|--------|------|
| `const` | Value cannot be modified (custom semantics with `#modlaw`, see §3.8) |
| `unique` | **Generic type constructor**: `unique T` ≡ `T*` (Q1, may be omitted) |
| `static` | Internal linkage |
| `shared` | Cross-thread visibility annotation (compile-time enforced, otherwise compile error, see §13.1) |

```cpp
unique int      x1;   // ≡ int*
unique char     x2;   // ≡ char*
unique Point    x3;   // user-defined struct is also valid
unique int*     x4;   // valid when T itself is a pointer type
unique int[10]  x5;   // T is an array type
```

**Rules (carried over from 0.2.0 + Q1):**

1. The `T` in `unique T` can be **any type** from §3.1–§3.6.
2. `unique T` and `T*` are **equivalent** in the type system; `unique` may be omitted.
3. `unique` cannot apply to borrowed types: `unique T&` is **illegal** — borrows do not own values.
4. `unique` **cannot be written as** `shared unique T*` — `shared` is a runtime storage class (at the same level as `__thread`), not a type constructor.

### 3.8 Pointer Modifiers and Ownership Boundaries (Q6 custom const T* / T* const) *(new in 0.3.0)*

> **[0.3.0 · Q6 custom semantics, opposite of C++]** This section contains the pointer modifier rules newly added in 0.3.0. 0.2.0 followed the C++ meaning of `const T*` / `T* const`; 0.3.0 **rewrites** them with custom semantics.

| Declaration | Meaning | Rebindable | Writable through it | Owning heap allowed |
|------|------|-----------|----------|------------------|
| `const T*` | **Pointer-lock**: the pointer itself **cannot rebind** + lifetime-safe + cannot write through it | ❌ | ❌ | ❌ (owning heap forbidden) |
| `T* const` | **Data read-only view**: the pointer **can rebind**, but cannot write through it | ✅ | ❌ | ❌ (owning heap forbidden) |
| `const T* const` | Double lock (neither rebindable nor writable) | ❌ | ❌ | ❌ |

> **Opposite of C++**:
>
> | Type | C++ Meaning | UltraCPP Meaning |
> |------|----------|---------------|
> | `const T*` | Data cannot be modified (pointer can) | **Pointer cannot be modified** (and data cannot be written) |
> | `T* const` | Pointer cannot be modified (data can) | **Data cannot be written** (pointer can) |

**Example D (Q6 custom semantics):**

```cpp
int x = 42;

// === const T*: pointer-lock + lifetime + read-only ===
const int* p = &x;
*p;                             // ✅ read
// *p = 100;                    // ❌ not writable
// p = &y;                      // ❌ pointer-lock, cannot rebind

// === T* const: data read-only view ===
T* const r = &x;
*r;                             // ✅ read
// *r = 100;                    // ❌ data is read-only
r = &y;                         // ✅ can rebind

// === owning heap forbidden ===
unique int* h = alloc(int);
const int* bad = h;             // ❌ error: cannot form a read-only pointer from owning heap
T* const bad2 = h;              // ❌ same prohibition
```

**Why is owning heap forbidden?** An owning pointer may later be `move()`-ed or `free()`-d; at that point all `const T*` / `T* const` views would **dangle**. The release timing of an owning pointer is controlled by the programmer, so a "read-only view" of owning heap cannot be guaranteed safe at compile time.

For further details (diagnostics when violated, dereference semantics) see §7.10.

---

## 4. Expressions *(new in 0.3.0 §4.9, §4.10)*

### 4.1 Operator Precedence and Associativity *(revised in 0.3.1, revised in 0.3.3)*

| Level | Operator | Associativity |
|--------|--------|--------|
| 1 | `::` | left-to-right |
| 2 | `()` `[]` `.` `->` `++` `--` (postfix) | left-to-right |
| 2.5 | **unary `*` `&` `+` `-` `!` `~` `mod` `unmod` `++` `--` (prefix)** *(added in 0.3.1)* | **right-to-left** |
| 2.5 | `(`*type*`)` | **C-style cast** *(added in 0.3.2)* |
| 3 | `*` `/` `%` (binary) | left-to-right |
| 4 | `+` `-` (binary) | left-to-right |
| 5 | `<<` `>>` (binary) | left-to-right |
| 6 | `<` `>` `<=` `>=` | left-to-right |
| 7 | `==` `!=` | left-to-right |
| 8 | `&` (binary, bitwise AND) | left-to-right |
| 9 | `^` | left-to-right |
| 10 | `\|` | left-to-right |
| 11 | `&&` | left-to-right |
| 12 | `\|\|` | left-to-right |
| 13 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | right-to-left |
| 14 | `?:` (ternary conditional) | right-to-left |
| 15 | `move` `clone` | - |

> **[0.3.0]** The level-8 `&` in the table refers to **binary infix** bitwise AND. The unary prefix `&` (reference) and `mod` / `unmod` (modification right) are unary operators; see §4.9–§4.10.
> **[Added in 0.3.1]** Unary `*` (dereference) see §4.13; binary `*` (multiplication) see §4.2.

### 4.2 Arithmetic Operators

| Operator | Description | Example |
|--------|------|------|
| `+` | Addition | `a + b` |
| `-` | Subtraction | `a - b` |
| `*` | Multiplication | `a * b` |
| `/` | Division | `a / b` |
| `%` | Modulo | `a % b` |

### 4.3 Comparison Operators

| Operator | Description | Example |
|--------|------|------|
| `==` | Equality | `a == b` |
| `!=` | Inequality | `a != b` |
| `<` | Less than | `a < b` |
| `>` | Greater than | `a > b` |
| `<=` | Less than or equal | `a <= b` |
| `>=` | Greater than or equal | `a >= b` |

### 4.4 Logical Operators

| Operator | Description | Example |
|--------|------|------|
| `&&` | Logical AND | `a && b` |
| `\|\|` | Logical OR | `a \|\| b` |
| `!` | Logical NOT | `!a` |

### 4.5 Bitwise Operators

> **[0.3.0]** This table describes only **binary infix** forms. The unary prefix `&` is not a bitwise operator; it is the reference (see §3.3).

| Operator | Description | Example |
|--------|------|------|
| `&` | Bitwise AND (binary infix) | `a & b` |
| `\|` | Bitwise OR | `a \| b` |
| `^` | Bitwise XOR | `a ^ b` |
| `~` | Bitwise NOT | `~a` |
| `<<` | Left shift | `a << 2` |
| `>>` | Right shift | `a >> 2` |

### 4.6 Assignment Operators *(revised in 0.3.1)*

> **[0.3.1]** The **left-hand side** of an assignment operator must be an lvalue (see §4.13.2). A non-lvalue on the left
> is a compile-time error (`AssignmentToRvalueError`).

| Operator | Description |
|--------|------|
| `=` | Simple assignment — LHS must be an lvalue (§4.13.2); RHS is evaluated and assigned to LHS; between `T*` triggers **implicit `mod()`** without ownership transfer, see §7.1 |
| `+=` | Add-assign — LHS must be an lvalue; `LHS = LHS + RHS` |
| `-=` | Sub-assign — LHS must be an lvalue; `LHS = LHS - RHS` |
| `*=` | Multiply-assign — LHS must be an lvalue; `LHS = LHS * RHS` |
| `/=` | Divide-assign — LHS must be an lvalue; `LHS = LHS / RHS` |
| `%=` | Modulo-assign — LHS must be an lvalue; `LHS = LHS % RHS` |
| `&=` | Bitwise-AND-assign — LHS must be an lvalue; `LHS = LHS & RHS` |
| `\|=` | Bitwise-OR-assign — LHS must be an lvalue; `LHS = LHS \| RHS` |
| `^=` | Bitwise-XOR-assign — LHS must be an lvalue; `LHS = LHS ^ RHS` |
| `<<=` | Left-shift-assign — LHS must be an lvalue; `LHS = LHS << RHS` |
| `>>=` | Right-shift-assign — LHS must be an lvalue; `LHS = LHS >> RHS` |

**Associativity**: assignment operators are **right-associative** (`a = b = c` is equivalent to `a = (b = c)`; the inner
`(b = c)` as a whole is an lvalue and may serve as the outer LHS — see §4.13.2 table).

**lvalue context**: the LHS of an assignment takes the lvalue path (§4.13.4), and the RHS takes the rvalue path. The result
of `=` (the whole assignment expression) is an lvalue with the value of the LHS after assignment.

### 4.7 Conditional Operator

```cpp
condition ? expr1 : expr2
```

### 4.8 Parenthesized Expression *(revised in 0.3.2)*

```cpp
(expr)  // any expression inside parentheses
```

> **[0.3.2]** When `T` inside `(T)` is a **type name** rather than an **expression**, it denotes a **C-style explicit type cast**; see §4.8.1. The C-style cast is fully equivalent to the functional cast `T(expr)` (see §4.8.1.2).

#### 4.8.1 C-style Explicit Type Cast `(`*type*`)`*expr* *(new in 0.3.2)*

The C-style cast is the most common explicit type-conversion form in UltraCPP, with the same syntax as C/C++:

```
'(' type ')' unary_expression
```

`type` is defined in §3 (Type System); `unary_expression` is defined in §4.1 (Operator Precedence).

##### 4.8.1.1 Semantics *(new in 0.3.2)*

`(T)expr` converts the value of expression `expr` to type `T`. Conversion rules:

| Source → Target | Behavior | Codegen layer |
|-------------------|------|-----------|
| `T*` → `T*` (same type) | ✅ no-op (compile-time confirmation only) | pass pointer value through |
| `T*` → `void*` | ✅ implicit, allowed | pass pointer value through |
| `void*` → `T*` | ✅ explicit cast (programmer guarantees type correctness) | bitcast (`bitcast T* ... to T*`) |
| `void*` → `int` (i64) | ✅ explicit cast | ptrtoint (`ptrtoint T* ... to i64`) |
| `int` (i32/i64) → `T*` | ✅ explicit cast | inttoptr (`inttoptr i64 ... to T*`) |
| `int` (i32) → `int` (i64) | ✅ sign extension | sext (`sext i32 ... to i64`) |
| `int` (i64) → `int` (i32) | ✅ truncation | trunc (`trunc i64 ... to i32`) |
| `int` → `float` / `double` | ✅ (if supported) | sitofp (`sitofp i32 ... to float`) |
| Unrelated types (e.g. `int` → `Point`) | ❌ compile-time error `InvalidCastError` | — |

##### 4.8.1.2 Relationship with Functional Cast `T(expr)` *(new in 0.3.2)*

UltraCPP supports both cast forms:

| Form | Name | Example |
|------|------|------|
| `T(expr)` | functional cast | `int(3.14)` → 3 |
| `(T)expr` | C-style cast | `(int)3.14` → 3 |

The two forms are **fully equivalent**. The programmer may choose either. UltraCPP recommends the C-style cast (aligned with the C/C++ ecosystem; more intuitive for FFI interop).

> **History**: 0.3.0 / 0.3.1 specs only explicitly supported `T(expr)`; `(T)expr` was missing, which made the form `*((int*)malloc(8))` in m0_42 semantically ambiguous. After 0.3.2 added the C-style cast, the two forms are equivalent.

##### 4.8.1.3 Examples *(new in 0.3.2)*

```cpp
// === Integer ↔ pointer conversion (common in FFI / syscalls) ===
int* p = alloc(int);          // p: int*
void* vp = (void*)p;          // T* → void*: implicit, allowed
int addr = (int)vp;           // void* → int: ptrtoint
int* p2 = (int*)addr;         // int → int*: inttoptr

// === Integer width conversions ===
int big = (int)1000000L;      // i64 → i32: trunc (semantics: take the low 32 bits)
long huge = (long)42;         // i32 → i64: sext

// === FFI scenario ===
extern "C" {
    void* malloc(int size);
}

// === Type assertion (programmer guarantees type correctness) ===
void* raw = malloc(8);        // malloc is brought in via §10 extern "C"
int* typed = (int*)raw;       // void* → int*: bitcast; programmer guarantees raw is actually int*

int* arr = (int*)malloc(8);   // must cast void* → int*
*arr = 42;                     // write through the typed pointer
```

##### 4.8.1.4 Compile-Time Checks *(new in 0.3.2)*

Compile-time checks for the C-style cast include:

1. **`type` must be a known type** (types defined in §3 Type System, including built-in + user-defined).
2. **The conversion must be legal** (8 allowed rows + 1 disallowed unrelated-types row in the table above).
3. **Pointer ↔ integer conversions require explicit cast** (no implicit conversion — constrained by §3.5 type-conversion rules; unchanged in 0.3.2).

Error kinds: `InvalidCastError` (illegal conversion) / `UnknownTypeError` (unrecognized type).

##### 4.8.1.5 Cross-References with Other Sections *(new in 0.3.2)*

| Section | Relationship |
|------|------|
| §3 Type System | cast target `T` must be a type defined in §3 |
| §4.1 Precedence | C-style cast is a level-2.5 unary op (same level as `*` / `&`, right-to-left) |
| §4.8 (this section) | C-style cast is a special form of parenthesized expression |
| §6.2 Function call | call result can be the cast source: `(int)factorial(5)` |
| §7.4 `alloc` and C stdlib | `malloc` returns `void*`, usually requiring a cast to a concrete type |
| §10 FFI | FFI functions returning `void*` must be cast |
| §11.0 builtin signature table | builtin function return types are known (no cast needed) |
| §12.1 EBNF | the `cast_expression` production is defined in §12.1 *(new in 0.3.2)* |

### 4.9 `mod()` Expression — Request the Modification Right *(new in 0.3.0: Rule 24, revised in 0.3.3 — Reclassification as builtin functions)*

> **[0.3.0 · Rule 24]** `mod(ref_expr)` requests the modification right of the object referred to by a **reference**. Its behavior is decided by the `#modlaw` policy in the current scope.

**Syntax**:
```
mod '(' reference_expression ')'
```

**Operand**: `reference_expression` must be an lvalue of a reference type `T&` (i.e., `int&`, `Point&`, etc.). Note that 0.3.0 has removed the `T&mut` type; `mod()` accepts only `T&` references.

**Semantics (Rule 24)**: behavior is decided by the current `#modlaw` policy.

| `#modlaw` Policy | Behavior of `mod(r)` |
|----------------|------------------|
| `none` | ❌ compile-time error: `mod() forbidden by #modlaw none policy` |
| `exclusive` | ✅ compile-time conflict check; if another `T&` has already requested mod, the exclusive request is denied (`BorrowConflict`) |
| `shared` | ✅ multiple `T&` may simultaneously obtain the modification right (data races are the programmer's responsibility) |

**Snippet example:**

```cpp
#modlaw shared module

int x = 42;
int& r = x;
mod(r);              // ✅ allowed under shared policy
*r = 100;
mod(r);              // repeatable within scope (compiler auto-unmods)
```

**Implicit `mod()` (Rule 24 + Q4):**

The `T*` version of assignment `p1 = p2` **automatically** generates `mod(p1)`, see §7.1. So the following works implicitly:

```cpp
int* p1 = alloc(int);
int* p2 = p1;          // implicit mod(p1): p1 gets the modification right, p2 still owns
*p1 = 100;             // ✅ no explicit mod() needed
```

### 4.10 `unmod()` Expression — Release the Modification Right *(new in 0.3.0: Rule 24, revised in 0.3.3 — Reclassification as builtin functions)*

`unmod(ref_expr)` explicitly releases the modification right. **Usually omitted** — the compiler auto-unmods at scope exit or after the last use.

**Syntax**:
```
unmod '(' reference_expression ')'
```

**Use case (or omitted form):**

```cpp
mod(r);                  // request
*r = compute();          // use
unmod(r);                // ✅ want to release immediately to shorten the critical section (can be omitted)

// The following two blocks are equivalent:
#modlaw exclusive module
{
    mod(m);
    *m = 100;            // last use of m → compiler auto-unmod(m)
}
*m2 = 200;               // unlocked; m2 can request
```

---

### 4.13 Expression Classification: lvalue and rvalue *(new in 0.3.1, deref clarified in 0.3.3)*

In UltraCPP every expression belongs to one of two categories: **lvalue** or **rvalue**. This classification is the
basis for assignment, address-of, `mod()` / `unmod()` requests, reference binding, and other core semantics.

#### 4.13.1 Definitions *(new in 0.3.1)*

**lvalue** — names an object and has persistent identity:

- Has identity (occupies a definite storage location)
- May serve as the left side of assignment `=`
- May be the operand of address-of `&`
- In codegen: takes the **lvalue path** — emit address-taking instructions (`getelementptr`, `alloca`
  reference, etc.); does **not** auto-`load` the value

**rvalue** — represents a temporary value with no persistent identity:

- No identity (a temporary computed result)
- **Cannot** serve as the left side of assignment `=`
- **Cannot** be the operand of address-of
- In codegen: takes the **rvalue path** — emit evaluation instructions (`load`, immediates, results of arithmetic, etc.)

#### 4.13.2 Which Expressions Are lvalues *(new in 0.3.1)*

| Expression Form | Category | Reason |
|-----------|------|------|
| Variable name `x` | lvalue | names the declared object |
| Dereference `*p` (p is a `T*` pointer type) | lvalue | names the object pointed to by p |
| Field access `s.field` | lvalue | names the member within the struct |
| Index `a[i]` | lvalue | names an element of the array |
| Use `r` of a reference `T& r` | lvalue | the reference is an alias for the object |
| Prefix increment `++x` | lvalue | the object after modification |
| Prefix decrement `--x` | lvalue | the object after modification |
| Assignment expression `x = v` | lvalue | (the result of the whole assignment expression is the LHS) |
| Result of function call `f()` | rvalue | temporary return value |
| Literals `42`, `"hello"` | rvalue | immediates |
| Arithmetic `a + b`, `a * b` | rvalue | computed result |
| Comparison `a < b`, `a == b` | rvalue | bool value |
| Logical `a && b`, `a \|\| b` | rvalue | bool value |
| Postfix increment `x++` | rvalue | the value of the expression is the **old** value before modification |
| Postfix decrement `x--` | rvalue | same as above |
| Result of address-of `&x` | rvalue | the result is a pointer value (even though the operand is an lvalue) |
| Conditional `c ? a : b` | rvalue | computed result |

#### 4.13.3 Assignment-Context Constraint *(new in 0.3.1)*

The **left-hand side** of an assignment operator (`=` and compound assignments such as `+=` / `-=` / `*=` etc.) must be an lvalue.
A **non-lvalue on the left** is a compile-time error (`AssignmentToRvalueError`):

```cpp
42 = x;          // ❌ literal is an rvalue
x + 1 = 2;       // ❌ arithmetic expression is an rvalue
f() = x;         // ❌ function-call result is an rvalue
x++ = 1;         // ❌ postfix-increment result is an rvalue

*x = v;          // ✅ dereference is an lvalue (§4.13.2 table row 2)
s.field = v;     // ✅ field access is an lvalue
a[i] = v;        // ✅ index is an lvalue
++x = 1;         // ✅ prefix-increment is an lvalue
x = y = z;       // ✅ right-associative; inner `y = z` is an lvalue as the outer LHS
```

Compound assignments (`+=` / `-=` etc.) also require their LHS to be an lvalue, with the same semantic constraint as `=`.

#### 4.13.4 Codegen Implementation Constraint *(new in 0.3.1)*

For an lvalue expression, `gen_expr` behaves differently in different contexts. The UltraCPP compiler in codegen
maintains two pieces of context information for every expression:

1. **Value category** (lvalue / rvalue) — defined in this section
2. **Concrete type** — the specific type (e.g., `int`, `int*`, `Point`)

| Call Context | Codegen Behavior for lvalue Expressions |
|-----------|--------------------------|
| LHS of assignment `=` (`UC_EXPR_ASSIGN.target`) | lvalue path: emit address-taking (`getelementptr`, `alloca` reference) |
| Operand of address-of `&x` | lvalue path: emit stack / global / field address |
| Expression statement, function argument, RHS of `=`, subexpression | rvalue path: emit `load` or value copy |

Example:

```c
int x = 42;
int* p = &x;
*p = v;           // LHS: lvalue path → emit gep, store v
y = *p;           // RHS: rvalue path → emit load
int z = *p + 1;   // RHS: rvalue path → emit load + add
&x;               // address-of operand: lvalue path → emit stack address
```

**Comparison with C/C++**: UltraCPP's two-way lvalue / rvalue classification is consistent with C/C++ (see K&R §A7.1,
C++17 [basic.lval]). But UltraCPP does **not** distinguish the three-way split introduced by C++11 — xvalue (eXpiring value),
prvalue (pure rvalue), glvalue (generalized lvalue) — UltraCPP keeps only two categories for simplicity and clarity.

#### 4.13.5 deref Semantics *(new in 0.3.3)*

Complete semantics of the deref operator `*` (unary prefix) in UltraCPP. This section supplements the row "deref `*p` is an lvalue" of the §4.13.2 classification table with full boundary details.

##### 4.13.5.1 Basic deref `*p` (p is `T*`) *(new in 0.3.3)*

```cpp
T* p = alloc(T);   // p: T*
T v = *p;          // deref: read the object p points to
*p = new_val;      // deref: write to the object p points to
```

- **Type**: `T` (the pointee type of `p`)
- **Category**: **lvalue** (see §4.13.2 lvalue classification table — deref always yields an lvalue because it names a memory location)
- **Meaning**: access the object pointed to by `p`
- **codegen**: emit `load T, T* p_slot` (read) or `store T new_val, T* p_slot` (write)

##### 4.13.5.2 Compound deref forms *(new in 0.3.3)*

**`&*p`** (addr-of then deref):

```cpp
T* p = alloc(T);
T** pp = &*p;  // equivalent to pp = p
```

- **Equivalence**: `&*p ≡ p` (identity)
- **codegen**: compiler optimizes this — no `load` + `getelementptr` is emitted; the SSA value of `p` is returned directly.

**`*&x`** (when x is an lvalue):

```cpp
int x = 42;
int v = *&x;  // equivalent to v = x
```

- **Precondition**: `x` must be an lvalue (has a memory address)
- **Equivalence**: `*&x ≡ x` (identity)
- **codegen**: compiler optimizes this — no `getelementptr` + `load` is emitted; the SSA value of `x` is returned directly.

**Multi-level deref `**pp`** (pp is `T**`):

```cpp
T** pp = alloc(T*);   // pointer to pointer
*pp = alloc(T);       // *pp: lvalue, type T*
T* p = *pp;           // read
**pp = new_val;       // multi-level deref + write
```

- **Type**: `T` (two pointer levels are peeled)
- **Category**: **lvalue**
- **codegen**: emit two `load T*, T** pp_slot` + `load T, T* inner_slot` (read); or `getelementptr` + `store` (write)

**`p->field`** (deref + field access):

```cpp
struct Point { int x; int y; };
Point* p = alloc(Point);
int v = p->x;       // ≡ (*p).x
p->y = 100;         // ≡ (*p).y = 100
```

- **Equivalence**: `p->field ≡ (*p).field` (§4.13.5.2 is consistent with C/C++)
- **codegen**: emit `getelementptr` + `load` / `store`

##### 4.13.5.3 Relation with §3.5 pointer types *(new in 0.3.3)*

`p->field` and `(*p).field` involve struct field access:

- struct fields are defined in §3.7
- `Point* p` yields a `Point` lvalue through deref, then `.field` is used to access
- `p->field` is syntactic sugar; codegen is equivalent to `(*p).field`

##### 4.13.5.4 Precedence *(new in 0.3.3)*

`*` (deref) as a unary prefix sits at **level 2.5** of §4.1, **right-to-left**.

It differs from arithmetic `*` (level 3, **left-to-right**) — the two are distinguished by **operand position** via positional overloading:

```cpp
int a = *p;     // level 2.5: * is deref, unary prefix
int b = a * b;  // level 3: * is multiplication, binary infix
int c = **pp;   // level 2.5: two derefs, right-to-left, equivalent to *(*pp)
```

##### 4.13.5.5 Error Cases *(new in 0.3.3)*

| Error | Error type | Trigger |
|------|------|------|
| `p` in `*p` is not a pointer type | `InvalidDereferenceError` | e.g. `*42` (int cannot be deref'd) |
| `pp` in `**pp` is not `T**` | `InvalidDereferenceError` | e.g. `**42` |
| `*null` | compile-time legal, runtime segfault | `null` is a `T*` and may be deref'd; runtime triggers a segmentation fault |

##### 4.13.5.6 Cross-References *(new in 0.3.3)*

| Section | Relationship |
|------|------|
| §3.5 pointer types | `p` must be a pointer type for deref |
| §3.7 struct fields | `p->field` accesses struct fields |
| §4.1 precedence table | deref is at level 2.5 (unary prefix) |
| §4.13.2 lvalue category | deref yields an lvalue |
| §4.13.3 rvalue | deref does not produce an rvalue (it has a memory location) |
| §4.13.4 codegen path | deref goes through the lvalue path (load / store) |
| §7.10 custom pointer | `const T*` deref is still an lvalue (but read-restricted) |
| §12.1 EBNF | `*` production in `unary_expression` |

#### 4.13.6 Cross-References with Other Sections *(new in 0.3.1, revised in 0.3.3)*

| Section | Relationship |
|------|------|
| §4.1 precedence table | unary `*` / `&` / `++` / `--` see §4.1 level 2.5 (0.3.3 removes `mod` / `unmod`) |
| §4.6 assignment operators | "LHS must be an lvalue" references §4.13.2 classification table |
| §4.9 `mod()` | operand lvalue requirement references §4.13.2 (0.3.3 revision: `mod` is a builtin function, see §11.0.1) |
| §4.10 `unmod()` | operand lvalue requirement references §4.13.2 (0.3.3 revision: `unmod` is a builtin function, see §11.0.1) |
| §4.13.5 deref Semantics | full boundary of "deref always yields an lvalue" see §4.13.5 |
| §7.1 ownership + implicit mod | the target of implicit `mod()` is an lvalue, references §4.13.2 |
| §7.8 reference `&T` | "operand must be an lvalue" references §4.13.2 |
| §12.1 EBNF | `lvalue` / `rvalue` rules see §12.1 *(new in 0.3.1)*; the `*` / `&` productions in `unary_expression` see §12.1 (0.3.3 revised under `builtin_function_call`) |

---

## 5. Statements

### 5.1 Expression Statement

```cpp
x + y;        // evaluate and discard
func(10);     // function call
```

### 5.2 Compound Statement (Block)

```cpp
{
    int x = 10;
    int y = 20;
    x = x + y;
}
```

### 5.3 if Statement

```cpp
if (condition) {
    // code
}

if (condition) {
    // code
} else {
    // code
}

if (a > b) {
    // code
} else if (a == b) {
    // code
} else {
    // code
}
```

### 5.4 while Statement

```cpp
while (condition) {
    // code
}
```

### 5.5 for Statement

```cpp
for (int i = 0; i < 10; i++) {
    // code
}

for (int x : array) {
    // code
}
```

### 5.6 return Statement

```cpp
return;              // returns void
return 42;           // returns a value
return x + y;        // returns the result of an expression
```

### 5.7 break Statement

```cpp
while (true) {
    if (condition) {
        break;       // exit the loop
    }
}
```

### 5.8 continue Statement

```cpp
for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;    // skip this iteration
    }
}
```

### 5.9 free Statement

```cpp
int* p = alloc(int);
*p = 42;
free(p);  // release memory
```

### 5.10 Declaration Statement

```cpp
int x;                // variable declaration
int x = 42;           // declaration with initializer
const int y = 100;    // constant
int* p = alloc(int);  // owning pointer allocation
int* p2 = &x;         // non-owning pointer (right side is &x)
int& r = x;           // the sole reference type T& (right side is an lvalue)
```

---

## 6. Functions

### 6.1 Function Definition *(revised in 0.3.2)*

```cpp
int add(int a, int b) {
    return a + b;
}

void greet(const char* name) {
    print(name);
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
```

> **[0.3.2]** A function definition **must declare a return type** (`int` / `void` / user type / `T*` etc.). The return type determines the result type of the function-call expression; see §6.2.1.
>
> **Compile-time checks**:
>
> 1. The value of every `return` statement in the body **must match** the declared return type (implicit numeric widening such as `int` → `long` is allowed; narrowing or unrelated types are not).
> 2. If the declared return type is non-`void`, the body **must** contain at least one `return` statement (or guarantee by control-flow that a value has been returned before reaching the end; the compiler may relax this check).
> 3. If the declared return type is `void`, the body may contain `return;` (no value) or omit `return`.
>
> Error kind: `ReturnTypeMismatchError` (return type mismatch).

### 6.2 Function Call *(revised in 0.3.2)*

```cpp
int result = add(10, 20);
greet("Hello");
int fact = factorial(5);
```

> **[0.3.2]** The **return type** of a function call is decided by the **signature** of the call target. See §6.2.1 for the return-type rules. In short:
>
> - builtin functions (e.g. `print`, `abs_int`): the return type is looked up in the §11.0 builtin signature table
> - user functions (e.g. `add`): the return type equals the type declared in the function definition (§6.1)
> - `extern "C"` functions (e.g. `malloc`): the return type equals the type declared in the extern declaration (§10)
>
> Return-type propagation rules are described in §6.2.1.2 (assignment RHS / subexpression / function argument / cast source / return value).

#### 6.2.1 Function Return Type *(new in 0.3.2)*

In UltraCPP, the return type of a function call is decided by the following priority order:

##### 6.2.1.1 Return-Type Determination Rules *(new in 0.3.2)*

1. If the callee is a **builtin** (see §11.0 builtin signature table)
   → return type = the signature in the §11.0 table
   Example: the return type of `factorial(5)` = `int` (from the §11.0 table)
            the return type of `abs_int(-5)` = `int` (from the §11.0 table)

2. If the callee is a **user-defined function** (§6.1)
   → return type = the type declared in the function definition
   Example: `int add(int a, int b)` → return type = `int`
            `void greet(const char* name)` → return type = `void`

3. If the callee is `extern "C"` (§10 FFI)
   → return type = the type declared in the extern declaration (programmer-guaranteed)
   Example: `extern "C" int abs_int(int x);` → return type = `int`

###### 6.2.1.1.1 extern Symbol-Table Storage and Population Timing *(added in 0.3.2)*

Implementing priority 3 (extern path) of §6.2.1.1 requires codegen to hold an "extern function symbol table". This
sub-section specifies the table's **structure**, **population timing** and **lookup interface**, so that Bug C2 is
truly fixed during 0.3.2 implementation rather than left only superficially addressed.

**1. Data structure**:

```c
// Added in the codegen context (CGen / UCCodeGenerator):
typedef struct {
    char* name;            // function name, e.g. "malloc", "free"
    char* ret_type;        // LLVM IR return-type string, e.g. "i8*", "void"
    char** param_types;    // parameter types (in declaration order)
    int param_count;
} extern_func_sig_t;

// Global table (compilation-unit level):
extern_func_sig_t* g_extern_funcs;  // dynamic array
int g_extern_func_count;
int g_extern_func_capacity;
```

**2. Population timing**:

- **When**: when the parser parses an `extern "C" { ... }` block, it **immediately** records each function
  declaration's (name, return type, parameter types) into `g_extern_funcs`.
- **Trigger point**: at the end of `parse_extern_decl`; specifically on the `extern "C"` block-parsing path
  inside `src-c/src/parser.c`.
- **If a declaration inside the extern block is not a function** (e.g. a variable declaration such as
  `extern "C" int errno;`), skip the symbol-table record (codegen takes the normal `UC_EXPR_IDENT` path).

**3. Lookup interface**:

```c
// Implemented in codegen.c:
const extern_func_sig_t* lookup_extern_func(const char* name) {
    for (int i = 0; i < g_extern_func_count; i++) {
        if (strcmp(g_extern_funcs[i].name, name) == 0) {
            return &g_extern_funcs[i];
        }
    }
    return NULL;
}
```

**4. Integration with `UC_EXPR_CALL`**:

```c
case UC_EXPR_CALL: {
    const char* fn_name = ...;
    const char* ret_type = NULL;

    // Priority 1: builtin (§11.0)
    const builtin_sig_t* bsig = lookup_builtin(fn_name);
    if (bsig) { ret_type = bsig->ret_type; }
    // Priority 2: user function (§6.1)
    else {
        const char* user_ret = lookup_user_func_ret_type(fn_name);
        if (user_ret) { ret_type = user_ret; }
    }
    // Priority 3: extern (§10) — **new in 0.3.2**
    if (!ret_type) {
        const extern_func_sig_t* esig = lookup_extern_func(fn_name);
        if (esig) { ret_type = esig->ret_type; }
    }
    // Priority 4: fallback (used only when the first 3 levels all miss)
    if (!ret_type) { ret_type = "i32"; }

    emit_fmt_writeln(g, "%s = call %s %s(%s)", res, ret_type, fn_name, args);
    free(g->last_expr_type);
    g->last_expr_type = cgen_strdup(ret_type);
}
```

**5. Handling void return types**:

- For `void free(void* mem)`, `ret_type = "void"`.
- Emit `call void @free(i8* %mem)` (no `%res =` prefix).
- Do not set `g->last_expr_type` (no value to pass).
- If a later expression uses the result of a void call, the compiler reports an error (per the §6.2.1.3 void constraint).

**6. Relationship with existing facilities**:

- `g_extern_funcs` is stored separately from `g->local_funcs` (user-function table).
- `lookup_user_func_ret_type()` (impl-plan:170) keeps a single function interface; internally it still queries in
  the "user first, then extern" order (merging the two tables into one unified interface).
- §10 FFI implementation must ensure the symbol table is shared between the parser and codegen (either by
  having the `CGEN` struct hold a pointer, or via a global singleton).

##### 6.2.1.2 Return-Type Propagation *(new in 0.3.2)*

The **result** of a call expression has the following uses:

| Context | Behavior |
|--------|------|
| Assignment RHS | `int x = factorial(5);` — the return type of `factorial(5)` (`int`) must be assignable to `x` |
| Expression statement | `factorial(5);` — discards the return value, legal (similar to C) |
| Subexpression | `int y = factorial(5) + 1;` — the return value becomes the input of arithmetic |
| Function argument | `print_num(factorial(5));` — the return value is passed to `print_num` |
| Condition | `if (factorial(5) > 100)` — the return value is used as a bool context |
| Cast source | `(long)factorial(5)` — see §4.8.1 C-style cast |
| Return value | `return factorial(5);` — must match the enclosing function's declared return type |

##### 6.2.1.3 Void Function Calls *(new in 0.3.2)*

For functions whose return type is `void` (such as `print`, `print_num`):

- The "value" of the call expression is `void`, with no concrete type.
- It cannot serve as an assignment RHS, a subexpression, or a return value (unless the enclosing function also returns `void`).
- It may stand alone as a statement: `print("hello");`

Error kinds:
- `VoidUsedAsValueError` — a void result is used as a value.
- `ReturnTypeMismatchError` — return value does not match the function's declared return type.

##### 6.2.1.4 Codegen Integration *(new in 0.3.2)*

The codegen implementation of a function call (in the `UC_EXPR_CALL` case of `src-c/src/codegen.c`):

```
1. Evaluate all arguments (rvalue path, §4.13.4)
2. Emit call instruction: ret_val = call i32 @factorial(i32 5)
3. Set g->last_expr_type to the function signature's return type (from §11.0 table or function definition)
4. Return ret_val (subsequent expressions may continue to use it)
```

Example:

```c
int x = factorial(5);
// codegen:
//   %1 = call i32 @factorial(i32 5)   ; §11.0 table shows factorial returns int (i32)
//   store i32 %1, i32* %x
//   g->last_expr_type = "i32"          ; provided by §11.0; fixes Bug C2 (hard-coded i32 mismatch)
```

##### 6.2.1.5 Cross-References with Other Sections *(new in 0.3.2)*

| Section | Relationship |
|------|------|
| §6.1 Function definition | the return type declared in a function definition is the basis for §6.2.1 priority 2 |
| §10 FFI | the return type declared in an extern function declaration is the basis for §6.2.1 priority 3 |
| §11.0 builtin signature table | builtin function return types are the basis for §6.2.1 priority 1 |
| §4.13.4 codegen paths | the call expression is an rvalue; the return-value type is provided by §11.0 |
| §7.4 `alloc` and C stdlib | `malloc` returns `void*`; the caller must cast to a concrete type (§4.8.1) |
| §12.1 EBNF | the function-call production is defined in §12.1 |

### 6.3 Parameter Passing *(rewritten in 0.3.0: single T& + mod())*

**Pass-by-value semantics:**
- The argument is copied into the parameter
- Modifying the parameter does not affect the caller

```cpp
void inc(int x) {
    x = x + 1;  // does not affect the caller
}

int n = 10;
inc(n);
// n is still 10
```

**Pass-by-reference for modification (0.3.0 form — the only form):**

```cpp
// === 0.3.0 form (only one): request via mod() ===
void inc(int& x) {
    mod(x);
    *&x = *&x + 1;  // or x = x + 1 (depends on exact syntax)
}

int n = 10;
mod(n);  // or mod(n) at the call site
inc(&n);
// n is now 11
```

**Read-only parameters continue to use `T&` (no `mod` needed):**

```cpp
int read(int& x) {          // read-only parameter
    return x;               // does not call mod()
}

int n = 10;
int v = read(n);            // v == 10
```

> **Parameter type matching**: parameter-type determination and call-site checking follow the §6.2.1 priority rules.
>
> **[0.3.0 · Q3 inverted, Rule 22, Rule 24]** 0.2.0 offered `void inc(int&mut x)` (exclusive mod-by-default, call site `inc(&mut n)`) and `void inc(int& x)` (read-only) as two kinds of parameters. 0.3.0 **removes `T&mut`** and unifies parameters to the single reference type `T&`: writing requires calling `mod(x)` first, and `#modlaw` decides whether multiple mods may coexist. Exclusivity is implemented via `#modlaw exclusive` + `mod()`, no longer a type-level distinction.

### 6.4 Return Value

```cpp
int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

Point get_point() {
    Point p = {1.0, 2.0};
    return p;  // return by value (copy)
}
```

> **Return type**: the return type is decided by §6.2.1.
>
> **[0.3.0 carries over 0.2.0 · D-7]** Returning a local variable **by value** (as `return p;` above) is always legal. Returning a **borrow** that points to a local variable is a dangling reference; see §7.9.

### 6.5 main Function Conventions

```cpp
int main() {
    // program code
    return 0;
}
```

The `main` function:
- Returns `int`
- May take no parameters, or:
  - `int main(int argc, char** argv)`

---

## 7. Memory Management

### 7.0 Concept Clarification: Operators vs Builtin Functions *(new in 0.3.3)*

In UltraCPP, "language primitives" fall into two categories:

#### 7.0.1 Operators (§4.1 precedence table)

Unary / binary / ternary operators, with **associativity**, listed in the §4.1 precedence table:

- **Unary prefix**: `*` (deref), `&` (addr-of), `+` `-` `!` `~` `++` `--` (level 2.5, **right-to-left**)
- **Unary postfix**: `++` `--` (level 2, **left-to-right**)
- **Binary**: `*` `/` `%` `+` `-` `<<` `>>` `<` `>` `<=` `>=` `==` `!=` `&` (bitwise AND) `^` `|` `&&` `||` `=` `+=` `-=` `*=` `/=` `%=` `&=` `|=` `^=` `<<=` `>>=` (levels 3–13, **left-to-right**)
- **Ternary**: `?:` (level 14, **right-to-left**)
- **Scope**: `::` (level 1, **left-to-right**)

> **Note**: `*` is positionally overloaded at level 2.5 (prefix, deref) and level 3 (binary, multiplication); the two are distinguished by **operand position**. `&` is similarly positionally overloaded at level 2.5 (prefix, addr-of) and level 8 (binary, bitwise AND), distinguished by the same mechanism.

#### 7.0.2 Builtin Functions (§11.0 signature master table)

**Function-call syntax** (with `()` parentheses), handled through the codegen path in the compiler:

> **[0.3.3 + runtime-architecture integration]** §11.0.1 builtin language primitives (**6 entries**) + compiler intrinsics (4 entries, not in BUILTIN_SIGS[]) + §11.0.2 UltraCPP stdlib (`lib/*.uc`, not in BUILTIN_SIGS[]):

- §11.0.1 **builtin language primitives (6 entries, in BUILTIN_SIGS[])**:
  - §7.2 `alloc(T)` — heap allocation, returns `T*` (owning)
  - §7.3 `free(p)` — release an owning pointer (promoted from FFI to builtin per runtime-arch §3.2)
  - §7.4 `move(p)` — ownership transfer, source becomes moved-out
  - §7.5 `clone(s)` — independent copy (deep clone)
  - §4.9 `mod(ref)` — acquire the modification right; codegen emits the `@uc_borrow_mod_enter` IR marker
  - §4.10 `unmod(ref)` — release the modification right; codegen emits the `@uc_borrow_mod_exit` IR marker
- **Compiler intrinsics (4 entries, not in BUILTIN_SIGS[], per runtime-arch §5.2)**:
  - §7.6 `null` — null pointer constant (codegen emits `i8* null`)
  - `is_null(p)` — codegen emits `icmp eq i8* %p, null`
  - `sizeof(T)` — compile-time constant
  - `alignof(T)` — compile-time constant
- §11.0.2 **UltraCPP stdlib paths (provided by user `import "lib/*.uc"`, not in BUILTIN_SIGS[])**:
  - `lib/print.uc` — `uc_print` / `uc_print_num` / `uc_print_float`
  - `lib/string.uc` — `uc_strlen` / `uc_strcpy` / `uc_strcmp` (UltraCPP-implemented, no libc dependency, per runtime-arch §3.1)
  - `lib/memory.uc` — `uc_memcpy` / `uc_memmove` / `uc_memset`
  - `lib/math.uc` — `uc_abs` (replaces `abs_int`)
  - `lib/alloc.uc` / `lib/sys.uc` (sys:: namespace, §10.4)
  - `lib/uc_runtime.c` (C temporary bootstrap, replaced in 0.4.0+)

**Call form**: `name(arg1, arg2, ...)` (all 6 builtin entries) or `name` (intrinsic `null` is a constant).

**Return type**: see the §11.0.1 signature table (consistent with §6.2.1 function return type rules introduced in 0.3.2). stdlib function return types are determined by the function definitions in `lib/*.uc` (no lookup against the builtin table).

#### 7.0.3 Key Clarification: Reclassification as builtin functions *(new in 0.3.3)*

`mod`, `unmod`, `move`, `clone` were listed as "operators" in 0.2.0 / 0.3.0 specs:

- 0.2.0 spec §4.1 level 15: `move`, `clone` are operators
- 0.3.0 spec §4.1 footnote: `mod` / `unmod` are unary operators
- 0.3.0 spec §12.3 appendix precedence table level 15: `move`, `clone`, `move_to_thread` are "ownership operations"

**0.3.3 correction**: these four are all **builtin function calls**, on the same level as `alloc` / `free` / `move` / `clone` (§11.0.1 language primitives).

**Reason for the correction**:

1. They are **always invoked with parentheses** (`mod(ref)` rather than `mod ref`) — there is no prefix / postfix form.
2. They **have signatures** (return type + parameter type), matching the characteristics of builtin functions.
3. They **do not have a fixed precedence** — in `mod(ref)` the operand `ref` is a complete expression, which differs from the precedence semantics of operators like `+` `-`.
4. Mixing them into the precedence table **causes misreading** (e.g. does `a + move(b)` require precedence lookup?).

**Special nature of `mod` / `unmod`** *(0.3.3 + runtime-architecture §3.2 + §5.1 integration)*:

- `mod` / `unmod` are **builtins (class A)** — no longer in the half-finished "compile-time decision without runtime function" state (revised in the 2026-08-17 runtime-architecture integration).
- They sit alongside `alloc` / `free` / `move` / `clone` in §11.0.1 and enter the BUILTIN_SIGS[] table (BUILTIN_SIGS[] now has 6 entries).
- The codegen layer **does** emit LLVM IR calls — but the calls emit **IR markers** (`@uc_borrow_mod_enter` / `@uc_borrow_mod_exit` intrinsics), not ordinary function calls:
  - `@uc_borrow_mod_enter(%ref)` — marks "entering a modifiable borrow scope" (this marker may be optimized to a no-op because checking is done at compile time).
  - `@uc_borrow_mod_exit(%ref)` — marks "exiting a modifiable borrow scope".
- mod state is maintained by three mechanisms:
  1. **IR marker** (codegen emit) — each `UCExprBuiltinCall` node corresponds to `@uc_borrow_mod_enter` / `@uc_borrow_mod_exit` IR calls.
  2. **`UCExprCall.mod_state` field** (compile-time) — requesting / granted / released.
  3. **Borrow checker** (compile-time) — validates mod pairing at scope exit (Rule 24).
- **Reason for promoting to builtin** (per runtime-architecture §3.2): `mod` / `unmod` are the entry / exit points of borrow-checker state, and the compiler **must** record them explicitly in the IR. The previous "intrinsic / compile-time decision" path could not guarantee every call site emitted correctly, so the unified builtin path was adopted.
- **History** (0.2.0–0.3.3): `mod` / `unmod` have been repeatedly reclassified in 0.2.0 / 0.3.0 / 0.3.1 / 0.3.2 (`operator` → `intrinsic` → `builtin`); the 0.3.3 + runtime-architecture integration is the final form.

**After the correction**:

- §4.1 precedence table **removes** level 15 `move clone`, and **removes** `mod` / `unmod` from the level 2.5 unary row.
- §4.1 footnote is **rewritten** to explain that `mod` / `unmod` are function calls.
- §12.3 appendix precedence table **removes** level 15, which now belongs to §11.0.1 builtin signature table.
- §11.0 is regrouped into §11.0.1 (language primitives — 6 builtin entries: `alloc` / `free` / `move` / `clone` / `mod` / `unmod`) + §11.0.2 (UltraCPP stdlib paths `lib/*.uc`, not in BUILTIN_SIGS[]; per runtime-architecture §3.1 + §5.3).
- §4.9 / §4.10 gain a [0.3.3 revised] note: `mod` / `unmod` are **builtin functions**, and codegen emits IR markers (`@uc_borrow_mod_{enter,exit}`); they are not pure compile-time decisions.

#### 7.0.4 Historical Retrospective *(new in 0.3.3)*

| Version | §4.1 table level 15 | Footnote description of mod / unmod | §11.0 builtin table | Section placement |
|------|------|------|------|------|
| 0.2.0 | `move`, `clone` are operators | (no mod / unmod) | (no table) | §4.1 table |
| 0.3.0 | `move`, `clone` are operators (carried over) | "mod / unmod are unary operators" | (no table) | §4.1 table + §12.3 |
| 0.3.1 (S2) | (unchanged) | (unchanged) | (no table) | §4.1 table + §12.3 |
| 0.3.2 (S4 + S5) | (unchanged) | (unchanged) | "single table, 18 entries" | §4.1 table + §12.3 + §11.0 |
| **0.3.3 + runtime-arch (this document)** | level 15 removed | "mod / unmod are builtin functions (class A, codegen emits IR marker)" | **§11.0.1 (6 language primitives: `alloc` / `free` / `move` / `clone` / `mod` / `unmod`) + §11.0.2 (UltraCPP stdlib paths `lib/*.uc`, not in BUILTIN_SIGS[])** | §11.0.1 + §10.2 / §10.3 / §10.4 |

0.3.3 is the first version to clearly distinguish "operator vs builtin function"; the runtime-architecture integration revision (2026-08-17) further promoted `mod` / `unmod` from "half builtin" to **full** builtin (class A, emits IR marker), and moved stdlib out of the BUILTIN_SIGS[] table (UltraCPP provides its own). For Plan A implementation details, see `.dev/drafts/0.3.3-implementation-plan.md`.

#### 7.0.5 Cross-References *(new in 0.3.3)*

| Section | Relationship |
|------|------|
| §3 type system | type constraints for operator and builtin-function operands come from §3 |
| §4.1 operator precedence | the complete operator list is in §4.1 |
| §4.9 `mod(ref)` | `mod` is a builtin function (§11.0.1, class A, codegen emits IR marker), not an operator |
| §4.10 `unmod(ref)` | `unmod` is a builtin function (§11.0.1, class A, codegen emits IR marker), not an operator |
| §6.2.1 function return type | the return type of a builtin function is determined by the §11.0 signature table |
| §7.1 ownership | `move` / `clone` are the core builtins of §7.1 concepts (`null` is an intrinsic and is not in §11.0.1, per runtime-arch §5.2) |
| §7.3 `free(p)` | `free` is a builtin (§11.0.1, class A, emits `@free` and marks the pointer dangling), not FFI |
| §7.4 `move(p)` | full semantics see §7.4 (this section is classification; §7.4 is semantics) |
| §7.5 `clone(s)` | full semantics see §7.5 |
| §11.0.1 builtin language primitives (6 entries) | §11.0.1 lists the **6** language-primitive builtins: `alloc` / `free` / `move` / `clone` / `mod` / `unmod` (per runtime-arch §5.1) |
| §11.0.2 UltraCPP stdlib paths | §11.0.2 lists `lib/print.uc` / `lib/string.uc` / `lib/memory.uc` / `lib/math.uc` / `lib/alloc.uc` / `lib/sys.uc`; stdlib is not in BUILTIN_SIGS[] (per runtime-arch §5.3 + §9.2) |
| §12.1 EBNF | the `builtin_function_call` production see §12.1 (added 0.3.3) — only the 6 language primitives; stdlib goes through `import "lib/*.uc"` and is not in `builtin_function_call` |
| §12.3 appendix precedence table | synchronized with §4.1 revision |

#### 7.0.6 abs_int → uc_abs Migration Note *(new in 0.3.4)*

abs_int has been removed from `BUILTIN_SIGS[]` (src-c/src/codegen.c) and is now provided by UltraCPP's own implementation in `lib/math.uc::uc_abs` (per commit 11a). Callers may still write `abs_int(x)` syntax; the codegen routes via extern function lookup to link against lib/math.o::uc_abs. The corresponding §11.0.3 codegen integration section is updated accordingly.

**Historical**: From 0.3.2, `abs_int` was the first entry of `BUILTIN_SIGS[]` and was implemented via the inline IR `@builtin_abs_int` (21 lines). During the 0.3.3 phase it was moved out of `BUILTIN_SIGS[]` together with the rest of the stdlib; commit 11a in 0.3.4 completely removes the inline IR implementation.

**0.3.4 Migration Path**:

1. `abs_int` is **completely removed** from `BUILTIN_SIGS[]` (commit 11a)
2. The codegen in the `UCExprCall` main flow takes **P3 extern function lookup** (per §11.0.3 revision): emit `declare external i32 @abs_int(i32)` + `call i32 @abs_int(i32 %x)`
3. At link time the linker resolves the symbol to `lib/math.o::uc_abs` (from `lib/math.uc`)
4. `lib/math.uc` provides the `uc_abs(n: int) -> int` implementation: `n < 0 ? -n : n`

**Regression guarantee**: The behavior of `uc_abs` must be byte-identical to the original inline IR (`uc_abs(-7) == 7` / `uc_abs(0) == 0` / `uc_abs(7) == 7`); the `m0_41_extern_c` test still PASSES after commit 11a (per 0.3.4 plan §6 R2).

**Related Sections**:
- §11.0.3 Codegen Integration — the 6 primitives no longer include abs_int; it takes the P3 extern function lookup path
- §11.0.2 UltraCPP stdlib Paths — `lib/math.uc` provides `uc_abs`
- §S6 FFI extern body source strategy (new section) — extern lookup three-tier priority (stdlib → user code → libc fallback)

### 7.1 Ownership Semantics (**two permissions split**) *(rewritten in 0.3.0: Rule 22, Q4, Q5)*

> **[0.3.0 · Rule 22]** 0.3.0 splits the 0.2.0 notion of "ownership" into two independent permissions:
>
> | Permission | Decides | Default | Transfer |
> |------|---------|--------|----------|
> | **Ownership** | Who deletes / frees | Owned when `T* p = alloc(T)`; not owned when `T* p = &x` | Explicit `move(p)` |
> | **Modification right** | Who may modify the value | Owning pointers have it by default; `T&` does not (requires `mod()`) | Implicit `mod()` (per `#modlaw`)|

**Pointers and ownership (Rule 2B, Q4):**

- **Exactly one owner at any time**: an allocated object has at most one owning pointer.
- **`T* p = alloc(T)`**: p owns the object's **ownership** and **modification right**.
- **`T* p = &x`**: p merely holds x's address, **does not own x**, and **does not** have x's modification right by default.
- **Assignment `p1 = p2` does not copy ownership** (Q4): source `p2` still owns; target `p1` obtains the **modification right** (implicit `mod(p1)`).

#### 7.1.1 Assignment and Modification Right (**0.3.0 rewrites 0.2.0 §7.1.1**)

**Example C (ownership transfer vs. modification right):**

```cpp
unique int* p1 = alloc(int);  // p1 owns
*p1 = 100;

unique int* p2 = p1;           // implicit mod(p2): p2 gets the modification right, p1 still owns
*p2 = 200;                      // ✅
// free(p2);                    // ❌ p2 does not own

move(p2);                       // explicit transfer
// p1 has been moved-out
free(p2);                       // ✅ p2 now owns
```

**Rules (key differences between 0.3.0 and 0.2.0):**

> Given an assignment statement `dst = src` where both `dst` and `src` are of type `unique T` (equivalent to `T*`).
>
> 0.3.0 behavior:
> 1. `dst` obtains the **modification right** (implicit `mod(dst)`, behavior per current `#modlaw`).
> 2. `src`'s **ownership is unchanged** — it still owns; only explicit `move(src)` transfers it.
> 3. `dst`'s ownership status: does not own. So `free(dst)` is a compile-time error when **dst does not own**.
>
> 0.2.0 behavior (D-5): `src` becomes null, `dst` takes over ownership.
>
> 0.3.0 **rewrites** this to: `src` is untouched, `dst` obtains the modification right. `move(p)` remains the only explicit way to transfer ownership (Q5).

**Where ownership transfer `move()` occurs** (carried over from 0.2.0, Q5):

| Location | Behavior | Source | Target |
|------|------|----|------|
| Explicit `move(p)` | Transfer ownership | `p` | `p2` (receiver) |
| Function argument `f(p)` (parameter is `unique T`) | Transfer ownership | `p` | parameter |
| Function return `return p;` (return type `unique T`) | Transfer ownership | `p` | caller receive site |

**Where modification-right granting occurs** (Q4):

| Location | Behavior | Source | Target |
|------|------|----|------|
| Assignment `dst = src` (type `unique T`) | Implicit `mod(dst)` | — | `dst` |
| Explicit `mod(ref_expr)` (§4.9) | Per `#modlaw` | — | ref_expr |

**Where no transfer occurs**: `clone(s)`, `&s`, `s == null`, etc., read-only comparisons.

### 7.2 alloc Function

```cpp
int* alloc(int)                  // allocate a single element
int* alloc(int, int count)      // allocate an array of count elements
```

**Examples:**
```cpp
int* p = alloc(int);         // a single int
int* arr = alloc(int, 10);    // an array of 10 ints
char* buf = alloc(char, 256); // a 256-byte buffer
```

### 7.3 free Function

```cpp
free(ptr)    // release memory
```

**Examples:**
```cpp
int* p = alloc(int);
*p = 42;
free(p);

int* arr = alloc(int, 10);
free(arr);
```

> **[0.3.0 carries over 0.2.0 · D-6]** The pairing of `alloc` and `free` is the programmer's responsibility; the compiler **does not enforce** it.

### 7.4 `move(p)` Expression *(carried over from 0.2.0 · D-4, Q5, revised in 0.3.3 — Reclassification as builtin functions)*

> **`move` is a type-parameterized builtin**, see §11.0 for details.
>
> **[0.3.0 · D-4, Q5]** `move(p)` is the language's **built-in primitive**, **not** syntactic sugar, and **not** an ordinary function. It is the **only way to explicitly transfer ownership**.

**Syntax** (see §12.1 `unary_expression`):
```
move '(' expression ')'
```

**Semantics:**

1. The operand must be an addressable lvalue of type `unique T` (equivalent to `T*`).
2. The **result** of `move(p)` is the pointer value that `p` previously held, of type `unique T`.
3. **Runtime effect**: after evaluating `move(p)`, the compiler must emit code that writes `p`'s storage cell to `null`.
4. **Static effect**: `p` is marked as moved-out; any subsequent use of `p` is a compile-time error `UseAfterMove`.
5. `move` does not allocate or copy the pointed-to object; it only transfers ownership.
6. The relationship between `move(p)` and implicit move (under 0.3.0's "assignment does not trigger move" behavior, `move(p)` is the only explicit means): `move` is the explicit means; implicit assignment only grants the modification right and does not transfer ownership.

```cpp
unique int p1 = alloc(int);
*p1 = 42;

unique int p2 = move(p1);   // p1 becomes null at runtime, marked moved-out statically

// int v = *p1;             // ❌ compile-time error UseAfterMove
int v = *p2;                // ✅ v == 42
free(p2);                   // ✅
```

### 7.5 clone Function *(rewritten in 0.3.0, revised in 0.3.3 — Reclassification as builtin functions)*

`clone()` copies the pointer, yielding two independent ownerships.

```cpp
int* p1 = alloc(int);
int* p2 = clone(p1);  // p1 and p2 are independent, each freed separately
```

### 7.6 null Constant

```cpp
int* p = null;      // null pointer
if (p == null) {    // comparison
    // p is null
}
```

### 7.7 Pointer Arithmetic

```cpp
int* ptr = alloc(int, 10);
ptr[0] = 1;          // index access
ptr[5] = ptr[0];    // copy a value

int* p1 = alloc(int, 10);
int* p2 = p1 + 5;   // offset the pointer
```

### 7.8 Reference `&T` (rewriting 0.2.0 §7.8) *(rewritten in 0.3.0, revised in 0.3.1, revised in 0.3.3 — see §4.13.5 deref Semantics)*

> **[0.3.0 rewrite]** 0.3.0 removes the `T&mut` type. `T&` is the sole reference type; writability is decided by `mod()` + `#modlaw`.
> **[0.3.1 revision]** 0.3.1 explicitly defines the lvalue / rvalue concepts (§4.13); the "operand must be an lvalue"
> wording in this section references the §4.13.2 expression classification table.

**Syntax** (see §12.1):
```
'&' unary_expression
```

`&expr` creates a reference to the object denoted by `expr`; the result type is `T&`.

**Creation rules:**

1. The operand must be an lvalue (see the §4.13.2 expression classification table — which expressions are lvalues).
   - ✅ legal: `&x` (variable), `&*p` (dereference), `&s.field` (field access), `&a[i]` (index), `&++x` (prefix increment)
   - ❌ illegal: `&42` (literal), `&x + 1` (arithmetic result), `&f()` (function-call result), `&x++` (postfix-increment result)
3. The owner must be live and initialized.
3. The reference cannot outlive its owner's lifetime (violation reports `DanglingReference`, see §7.9).
4. `&` does not change the owner's ownership state.

**Writability (Rule 24):**

5. Writing requires `mod()` first. `r = v;` must first call `mod(r);` to request the modification right, otherwise it is a compile-time error.
6. Multiple `T&` may coexist.
7. Exclusivity is controlled by `#modlaw exclusive` (compile-time check of `mod()` conflicts).

```cpp
#modlaw shared module

int x = 42;
int& r1 = x;
int& r2 = x;        // ✅ multiple references coexist
int v = r1 + r2;    // ✅ read
// r1 = 100;        // ❌ compile error: no mod right
mod(r1);            // ✅ request modification right (per #modlaw)
*r1 = 100;          // ✅ write

#modlaw exclusive module

int y = 10;
int& m = y;
mod(m);             // ✅ exclusive mod
*m = 100;
// mod(m2);         // ❌ compile error: conflict

#modlaw none module

int z = 5;
int& r = z;
// mod(r);         // ❌ compile error: none policy forbids
r;                  // ✅ read-only
// *r = 100;       // ❌ no mod right
```

### 7.9 Dangling Reference *(0.3.0 carries over 0.2.0 · D-7)*

The definition and triggering conditions are the same as 0.2.0 §7.10; this section is retained for reference.

**Definition**: a `T&` borrow is **dangling** when it remains reachable after its owner's storage has been destroyed.

**Triggering conditions** (carried over from 0.2.0):

- **(a)** The owner of an `&T` borrow has gone out of scope.
- **(b)** A function returns a reference, but the owner is a local variable of that function.

**Example (a) (carried over from 0.2.0)**:

```cpp
int main() {
    int& r;
    {
        int x = 42;
        r = x;              // borrow x (0.3.0 syntax: right side is an lvalue, not &x)
    }                       // ← x goes out of scope
    return r;               // ❌ DanglingReference
}
```

**Positive example (carried over from 0.2.0)**:

```cpp
int make_answer_by_value() {
    int x = 42;
    return x;               // ✅ return by value (copy)
}

unique int make_answer_owned() {
    unique int p = alloc(int);
    *p = 42;
    return p;               // ✅ move: ownership transfer (see §7.1.1)
}
```

### 7.10 `const T*` / `T* const` Custom Semantics in Detail (Q6) *(new in 0.3.0)*

> **[0.3.0 · Q6 custom semantics, opposite of C++]** See §3.8 for the conceptual part; this section gives deeper semantic rules.

#### 7.10.1 `const T*`

- **Pointer-lock**: after `const T* p = &x;`, **p can no longer be reassigned** (`p = &y;` is a compile-time error).
- **Read-only access**: cannot write through `p` (`*p = v;` is a compile-time error).
- **Lifetime safety**: the compiler verifies that the pointed-to object's lifetime ≥ the borrow's use range.
- **Cannot point to an owning heap variable** (the owning heap may later be moved or freed, causing dangle).

#### 7.10.2 `T* const`

- **Data read-only view**: cannot write through `r` (`*r = v;` is a compile-time error).
- **Rebind allowed**: `r = &y;` is legal.
- **Same prohibition on pointing to owning heap** — if h is owning, `T* const r = h;` is rejected at compile time.

#### 7.10.3 Diagnostic for Owning-Heap Prohibition

```
error[E0650]: cannot create read-only view of owning heap pointer
  --> src/main.uc:5:12
   |
 4 | unique int* h = alloc(int);
   | ----------------- `h` is an owning pointer
 5 | const int* p = h;
   |               ^ `h` may be moved or freed, so a `const T*` view of `h` would dangle
   |
   = help: copy the value first: `int v = *h; const int* p = &v;`
```

#### 7.10.4 Relationship with `mod()`

Both `const T*` and `T* const` are **read-only** — `mod()` does not apply to them, because their design intent is precisely "no code can write through them".

---

## 8. struct Types

### 8.1 struct Definition

```cpp
struct Point {
    double x;
    double y;
}

struct Circle {
    Point center;
    double radius;
}
```

### 8.2 Field Access

```cpp
Point p;
p.x = 1.0;
p.y = 2.0;

Circle c;
c.center.x = 0.0;
c.center.y = 0.0;
c.radius = 5.0;
```

### 8.3 Memory Layout

Structures use **sequential layout**, with padding possibly added for alignment:

```cpp
struct Example {
    char  c;    // 1 byte
    int   i;    // 4 bytes (3 bytes of padding may be added)
    short s;    // 2 bytes
}
```

### 8.4 Function Pointers as Fields

```cpp
struct Comparator {
    int (*compare)(int, int);
    const char* name;
}

int add(int a, int b) { return a + b; }

Comparator cmp = { add, "add" };
int result = cmp.compare(10, 20);  // result = 30
```

---

## 9. Modules and Preprocessor *(new in 0.3.0 §9.9)*

### 9.1 Preprocessing Phase

UltraCPP runs a **preprocessing phase** before lexical analysis, handling the following directives:

| Directive | Behavior |
|------|------|
| `#include "path"` | Expand the file's contents at the current position |
| `#import "module"` | Record a module dependency (do not expand contents) |
| `#modlaw <perm> <scope>` *(new in 0.3.0)* | Set the modification-right policy for the current file / module (see §9.9) |
| `export <declaration>` | Mark exported functions/variables |

**Preprocessing output:**
1. Pure code with `#include` expanded
2. Module dependency list
3. Current `#modlaw` policy
4. Exported symbol list

### 9.2 #include Syntax and Semantics

**Source copy**: `#include` copies the contents of the specified file **verbatim** into the location of the `#include` directive.

```cpp
#include "file_path"
```

**Example:**
```cpp
// main.upp
#include "../../lib/math.uc"

int main() {
    int result = add(5, 3);  // add() defined in math.uc
    return result;
}
```

**Behavior:**
- Code after `#include` directly acquires all definitions of the included file
- No additional declaration is required to call functions in the included file
- File paths are resolved relative to the **current source file's directory**

**Search path order:**
1. Relative to the current source file's directory
2. Relative to the project root
3. Relative to an explicitly specified include path

### 9.3 #import Syntax and Semantics

**Module import**: `#import` records a dependency on a module **without expanding its contents**.

```cpp
#import "module_path"
#import "module_path" as alias
```

**Example:**
```cpp
#import "math";           // import the math module
#import "io" as stdio;    // import the io module with an alias
```

**Behavior:**
1. Records the module dependency but does not expand the source
2. The imported module must be compiled separately
3. Calling module functions requires an `extern` declaration or `#include` of the interface

**Generated dependency file:**
```json
{
  "module": "math",
  "file": "lib/math.uc",
  "exports": ["add", "multiply"]
}
```

### 9.4 export Declarations

Mark a function or variable as a **module export**; the outside can access it via `#import`.

```cpp
export int add(int a, int b) {
    return a + b;
}

export const double PI = 3.14159;

export struct Point {
    double x;
    double y;
}
```

**Rules:**
- Symbols without `export` are **module-private**
- `export` can only be used at the top level of a module
- Exported functions/variables can be accessed by code that `#import`s the module

### 9.5 Module Design Rules

UltraCPP's module system follows these rules:

1. **No declaration required to define functions** - Within the same file, function calls need no forward declaration
2. **Functions in other modules require import** - Calling functions from another module must `#import` that module
3. **Internal functions cannot be accessed externally** - Functions without `export` can only be used inside the module

**Example:**
```cpp
// lib/math.uc
export int add(int a, int b);  // exported interface

int _internal_helper(int x) {  // internal function, not visible outside
    return x + 1;
}

int add(int a, int b) {
    return _internal_helper(a) + _internal_helper(b);
}
```

```cpp
// test/main.upp
#include "../../lib/math.uc"

int main() {
    add(5, 3);           // ✅ can call
    _internal_helper(1); // ❌ cannot call (not exported)
    return 0;
}
```

### 9.6 Module Search Paths

| Path Type | Resolution |
|----------|----------|
| Relative path | Relative to the current source file's directory |
| Absolute path | Relative to the project root |
| Specified path | Added via `-I` argument |

### 9.7 Circular Dependency Detection

```cpp
// a.uc
#include "b.uc"

int func_a() {
    return func_b() + 1;
}
```

```cpp
// b.uc
#include "a.uc"  // error: circular dependency detected

int func_b() {
    return 42;
}
```

**Detection rule**: If two modules mutually `#include` each other, the compiler should report an error.

### 9.8 Compilation Pipeline

```
Source code (.uc/.upp)
    │
    ▼
┌─────────────────┐
│   Preprocessing │
│  - Expand #include│
│  - Record #import │
│  - Parse #modlaw  │ (new in 0.3.0)
│  - Parse export   │
└─────────────────┘
    │
    ▼
┌─────────────────┐
│   Lexical analysis│
│   Syntax analysis │
│   Code generation │
└─────────────────┘
    │
    ▼
  LLVM IR (.ll)
    │
    ▼
  llc (assembly)
    │
    ▼
  Object file (.o)
    │
    ▼
  Linker (gcc/ld)
    │
    ▼
  Executable
```

### 9.9 `#modlaw` Directive *(new in 0.3.0: Rule 23)*

> **[0.3.0 · Rule 23]** Keyword: `#modlaw` (all lowercase, underscore). Syntax: `#modlaw <perm> <scope>`.

**Syntax**:
```
#modlaw <perm> <scope>
```

**Legal combinations (only 6):**

| `perm`     | `scope`    | Meaning                                                      |
|------------|------------|-----------------------------------------------------------|
| `none`     | `global`   | Global `mod()` forbidden; references are read-only         |
| `none`     | `module`   | This module's `mod()` forbidden; references are read-only  |
| `exclusive`| `global`   | Global `mod()` exclusive (compile-time conflict checks)    |
| `exclusive`| `module`   | This module's `mod()` exclusive (compile-time conflict checks) |
| `shared`   | `global`   | Global `mod()` shared (multiple mods allowed; data races are programmer's responsibility) |
| `shared`   | `module`   | This module's `mod()` shared (multiple mods allowed; data races are programmer's responsibility)|

**Illegal combinations**: The other 10 combinations are compile-time errors. For example `#modlaw none shared`, `#modlaw exclusive none`, `#modlaw shared exclusive`, etc.

**Scope (`scope`):**

- `global`: affects the entire file and all symbols introduced via `#include`.
- `module`: affects only symbols within the current `.uc` module; included files do not inherit.
- The default when `#modlaw` is unspecified is equivalent to `#modlaw shared module`.

**Position**: the `#modlaw` directive must appear at the **top** of the module (after `import` / `include` / `export`, before the first function / variable declaration). At most one `#modlaw` per module; duplicates are a compile-time error.

**Example B (three legal policies):**

```cpp
// === shared mode (default) ===
#modlaw shared module

int x = 42;
int& r1 = x;       // shared reference
int& r2 = x;       // also shared
mod(r1);            // ✅ r1 obtains a shared mod
mod(r2);            // ✅ r2 can also obtain it (data race is programmer's responsibility)
*r1 = 100; *r2 = 200;

// === exclusive mode ===
#modlaw exclusive module

int y = 10;
int& m = y;
mod(m);             // ✅ exclusive mod
// mod(m2);         // ❌ compile error: conflict
*m = 100;

// === none mode ===
#modlaw none module

int z = 5;
int& r = z;
// mod(r);         // ❌ compile error: none policy forbids
r;                  // ✅ read-only
// *r = 100;       // ❌ no mod right
```

**Error diagnostic:**

```
error[E0701]: `#modlaw` policy forbids `mod()` under `none`
  --> src/main.uc:7:5
   |
 6 | int& r = z;
   |     - binding declared here
 7 | mod(r);
   | ^^^^^ `mod()` is forbidden by `#modlaw none module`
   |
   = help: change the policy with `#modlaw shared module` or `#modlaw exclusive module`
```

---

## 10. FFI (Foreign Function Interface) *(introduced in 0.3.0, revised in 0.3.3)*

### 10.1 `extern "C"` Block *(introduced in 0.3.0, revised in 0.3.3)*

```cpp
extern "C" {
    // C function declarations
}
```

> **[0.3.3 revision]** The `extern "C"` block is **only for users declaring external C functions themselves** (rare cases such as calling system libraries or legacy C code). It is **not used for stdlib** — stdlib is written by UltraCPP itself in `lib/*.uc` (per runtime-architecture §3.1 "UltraCPP does not use libc as runtime"). Once stdlib uses `lib/*.uc`, `extern "C"` is only used for temporary bootstrap leftover calls during the 0.3.x phase.

#### 10.1.1 Implementation Status *(new in 0.3.4, per 0.3.4 plan §1 + 0.3.4 spec-text-changes §3.2)*

> **[0.3.4 new]** The `extern "C"` block mechanism described in §10.1 was confirmed complete in **0.3.4 commit 11b**.

| Syntax subset | Implementation status | Related commit |
|----------|----------|-------------|
| `extern "C" { fn_decl; }` basic syntax | ✅ Implemented (per 0.3.3 commit 7d) | 7d |
| Multiple function declarations inside the block | ✅ Implemented | 7d |
| Function declarations skip already-declared builtins (UC_TL_EXTERN skip) | ✅ Implemented (per 0.3.3 commit `42d75f7`) | `42d75f7` |
| Function body source (extern three-tier lookup) | ✅ Implemented (per 0.3.3 + 0.3.4 commit 11a) | 11a |
| Complete specification of body source strategy | ⚠️ Partial (see new §S6, 0.3.4 Phase 3) | Deferred to 0.3.4 Phase 3 |

**0.3.4 revision**: Rules unchanged; the body source strategy (extern three-tier lookup: UltraCPP stdlib → user code → libc fallback) is clearly defined by 0.3.4 commit 11b + the new §S6 section.

### 10.2 Preprocessor Macros *(new in 0.3.3, per runtime-architecture §7)*

UltraCPP adopts **C-style preprocessor macro syntax**, not Rust-style `#[cfg(...)]`. Rationale: compatible with the C ecosystem toolchain; seamless with the existing C implementation in `src-c/`; friendly learning curve. The `@` prefix avoids conflicts with user-code identifiers.

```c
@ifdef(TARGET_OS_LINUX)
    // Linux implementation
@end

@ifdef(TARGET_OS_DARWIN)
    // macOS implementation
@end

@if defined(TARGET_OS_LINUX) && defined(TARGET_ARCH_X86_64)
    // x86_64 Linux specific code
@end

@if !defined(TARGET_OS_LINUX)
    @error("lib/sys/raw.uc only supports Linux; other platforms require separate implementations")
@end
```

#### 10.2.1 Syntax Subset *(new in 0.3.3)*

| Syntax | Equivalent C | Description |
|------|------|------|
| `@ifdef(MACRO) ... @end` | `#ifdef MACRO ... #endif` | single condition |
| `@ifndef(MACRO) ... @end` | `#ifndef MACRO ... #endif` | inverse |
| `@if defined(MACRO) && defined(MACRO2) ... @end` | `#if defined(MACRO) && defined(MACRO2) ... #endif` | compound |
| `@if !defined(MACRO) ... @end` | `#if !defined(MACRO) ... #endif` | negation |
| `@else` / `@elif(EXPR)` | `#else` / `#elif` | branch |
| `@define(NAME, VALUE)` | `#define NAME VALUE` | macro definition |
| `@error("msg")` | `#error "msg"` | compile-time error |
| `@warning("msg")` | `#warning "msg"` | compile-time warning |

#### 10.2.2 Predefined Macros *(new in 0.3.3, per runtime-architecture §7.2)*

| Macro | Value | Use |
|---|---|---|
| `TARGET_OS_LINUX` | 1 | Linux platform |
| `TARGET_OS_DARWIN` | 1 | macOS platform |
| `TARGET_OS_WINDOWS` | 1 | Windows platform |
| `TARGET_OS_FREEBSD` | 1 | FreeBSD platform |
| `TARGET_ARCH_X86_64` | 1 | x86_64 architecture |
| `TARGET_ARCH_AARCH64` | 1 | ARM 64 architecture |
| `TARGET_ARCH_RISCV64` | 1 | RISC-V 64 architecture |
| `TARGET_ULTRA_VERSION` | "0.3.3" | UltraCPP compiler version |
| `TARGET_ULTRA_FEATURES` | bit flags | enabled language features |

- **Predefined macro source** (per runtime-architecture §11): the uc compiler auto-detects via `uname` at startup; can be explicitly overridden by `-D NAME=VALUE`.
- **Detection timing**: lexer / parser phase; `@ifdef` / `@if defined()` is evaluated immediately and does not enter the AST.

#### 10.2.3 Implementation Status *(new in 0.3.4, per 0.3.4 plan §1 + 0.3.3 process §3.1 commit 8a)*

> **[0.3.4 new]** The preprocessor macro mechanism described in §10.2 completed its codegen implementation in **0.3.3 commit 8a** (per `.dev/drafts/0.3.3-implementation-process.md` §3.1).

| Syntax subset | Implementation status | Related commit |
|----------|----------|-------------|
| `@ifdef(MACRO) ... @end` | ✅ Implemented (per 0.3.3 commit 8a) | 8a |
| `@ifndef(MACRO) ... @end` | ✅ Implemented | 8a |
| `@if defined(MACRO) && defined(MACRO2) ... @end` | ✅ Implemented | 8a |
| `@if !defined(MACRO) ... @end` | ✅ Implemented | 8a |
| `@else` / `@elif(EXPR)` | ✅ Implemented | 8a |
| `@define(NAME, VALUE)` | ✅ Implemented | 8a |
| `@error("msg")` | ✅ Implemented | 8a |
| `@warning("msg")` | ✅ Implemented | 8a |
| Predefined macro auto-detection (`TARGET_OS_*` / `TARGET_ARCH_*`) | ✅ Implemented (per runtime-arch §11) | 8a |

**0.3.4 confirmation**: 0.3.4 commit 11b confirms that 0.3.3 commit 8a is fully landed with no unimplemented items. The 0.4.0+ Stage 1 self-hosting phase will use this mechanism to cross-platform-distribute `lib/sys/raw.uc` (per §11.0.5).

### 10.3 Inline Assembly *(new in 0.3.3, per runtime-architecture §8)*

UltraCPP adopts **C / GCC-style inline assembly syntax**, not Rust's `asm!` macro. Rationale: LLVM toolchain understands it directly; seamless with the current C implementation in `src-c/`; consistent with §10.2 preprocessor macros in using C ecosystem conventions.

```c
unsafe {
    int result;
    asm {
        "syscall"
        : "=a"(result)               // output operand
        : "0"(num), "D"(arg1)         // input operands (0 = reuse output 0)
        : "rcx", "r11", "cc", "memory"  // clobbers
    }
}
```

#### 10.3.1 Syntax Format *(new in 0.3.3)*

Format (4 sections, separated by colons):

1. **Instruction template**: `"syscall"` — GCC-style string-literal instruction.
2. **Output operands**: `: "=a"(result)` — `=` means write-only.
3. **Input operands**: `: "0"(num), "D"(arg1)` — `"0"` means same register as output 0.
4. **Clobbers**: `: "rcx", "r11", "cc", "memory"` — caller-saved / memory may be modified.

#### 10.3.2 Constraint Characters *(new in 0.3.3, per runtime-architecture §8.2, ref GCC)*

| Constraint | Meaning |
|------|------|
| `r` | any general-purpose register |
| `a` | rax / eax (x86_64 syscall num) |
| `D` | rdi / edi (x86_64 arg 1) |
| `S` | rsi / esi (x86_64 arg 2) |
| `d` | rdx / edx (x86_64 arg 3) |
| `c` | rcx / ecx (x86_64 arg 4, clobbered after syscall) |
| `b` | rbx / ebx (x86_64 arg 5) |
| `=r` | output register (write-only) |
| `+r` | read-write operand |
| `0`–`7` | reuse the same register as operand N |
| `r0`–`r7` | ARM / RISC-V register numbers |
| `m` | memory operand |
| `i` | immediate |
| `cc` | condition-code register (flags) |
| `memory` | memory may be modified (compiler must flush register cache) |

#### 10.3.3 vs Rust `asm!` *(new in 0.3.3)*

UltraCPP chose C style over Rust's `asm!` macro:

| UltraCPP (C style) | Rust `asm!` |
|---|---|
| `asm { "syscall" : "=a"(r) : "0"(n) : "cc" }` | `asm!("syscall", out("rax") r, in("rax") n, lateout("rcx") _, lateout("r11") _, options(preserves_flags))` |
| Clobber list as the fourth section | `lateout(...)` / `options(...)` |
| `"0"` reuses a prior operand | passing the same variable directly |
| `asm volatile { ... }` | `options(preserves_flags, nostack)` etc. |

#### 10.3.4 Implementation Status *(new in 0.3.4, per 0.3.3 process §3.1 commit 8b)*

> **[0.3.4 new]** The inline assembly mechanism described in §10.3 completed its codegen implementation (basic GCC constraint subset) in **0.3.3 commit 8b**.

| Syntax subset | Implementation status | Related commit |
|----------|----------|-------------|
| `asm { "syscall" : ... }` basic format | ✅ Implemented (per 0.3.3 commit 8b) | 8b |
| Constraint characters `r` / `a` / `D` / `S` / `d` / `c` / `b` / `=r` | ✅ Implemented | 8b |
| Constraint characters `+r` / `0`–`7` (register reuse) | ✅ Implemented | 8b |
| Constraint characters `m` / `i` / `cc` / `memory` | ✅ Implemented | 8b |
| `asm volatile { ... }` | ✅ Implemented | 8b |
| Constraint characters `r0`–`r7` (ARM / RISC-V register numbers) | ⚠️ Not implemented (x86_64 only in 0.3.3) | Deferred to 0.3.5+ |

**0.3.4 confirmation**: The basic GCC constraint subset is fully implemented (via `@builtin_asm_*` forwarding to LLVM IR); the complete constraint set is deferred to 0.4.0+. 0.3.4 commit 10e's `lib/sys/raw.uc` uses this mechanism to implement `sys::raw_syscall` (x86_64 only).

### 10.4 `sys::` System Call Namespace *(new in 0.3.3, per runtime-architecture §6)*

UltraCPP introduces the `sys::` namespace as a cross-platform system-call abstraction layer. It mimics the glibc pattern (high-level wrapper + low-level entry + inline assembly), but is implemented entirely in UltraCPP itself, without depending on libc.

#### 10.4.1 Architecture *(new in 0.3.3, per runtime-architecture §6.1)*

```
User code
    sys::read(fd, buf, len)
        ↓
UltraCPP stdlib layer (lib/sys/sys.uc, written in UltraCPP)
        ↓
UltraCPP low-level layer (lib/sys/raw.uc, asm { "syscall" ... })
        ↓
LLVM IR layer
        ↓
Machine code (syscall / svc / ecall)
        ↓
Kernel mode (Linux / macOS / Windows NT / FreeBSD)
```

#### 10.4.2 `sys::` API First Version *(new in 0.3.3, per runtime-architecture §6.2)*

| Category | Function | Description |
|------|------|------|
| **File I/O** | `sys::read(fd, buf, len)` | read `len` bytes from `fd` into `buf` |
| | `sys::write(fd, buf, len)` | write `len` bytes from `buf` to `fd` |
| | `sys::open(path, flags)` | open / create file, return `fd` |
| | `sys::close(fd)` | close `fd` |
| | `sys::lseek(fd, offset, whence)` | move the file pointer |
| **Memory mapping** | `sys::mmap(addr, len, prot, flags, fd, offset)` | memory map |
| | `sys::munmap(addr, len)` | unmap |
| | `sys::mprotect(addr, len, prot)` | modify memory protection |
| **Process** | `sys::exit(code)` | terminate the process |
| | `sys::getpid()` | obtain the process ID |
| | `sys::fork()` | fork a child process |
| | `sys::execve(path, argv, envp)` | execute a new program |
| | `sys::wait4(pid, status, options, rusage)` | wait for child |
| **Thread** | `sys::clone(...)` | create thread / child process |
| | `sys::futex_wait(addr, val)` | futex wait |
| | `sys::futex_wake(addr, max)` | futex wake |
| **Time** | `sys::clock_gettime(clock_id, ts)` | obtain clock time |
| | `sys::nanosleep(req, rem)` | high-precision sleep |
| **raw** | `sys::raw_syscall(num, arg1, arg2, ...)` | low-level entry (direct to kernel) |

#### 10.4.3 Cross-Platform Unified Naming Principle *(new in 0.3.3, per runtime-architecture §6.3)*

- **User code uses `sys::read(fd, buf, len)`** — identical across platforms.
- **Implementations branch on `@ifdef(TARGET_OS_*)`** — the same interface name dispatches to platform-specific implementations (using §10.2 macros).
- **Implementations branch on `@ifdef(TARGET_ARCH_*)`** — the same interface dispatches to different `asm { ... }` blocks per architecture (using §10.3 inline assembly).
- **Directory layout** (`lib/sys/`):
  ```
  lib/sys/
  ├── sys.uc          // sys:: namespace API public header
  ├── raw.uc          // sys::raw_syscall low-level implementation
  ├── linux.uc        // Linux-specific implementation (@ifdef-tagged)
  ├── darwin.uc       // macOS-specific implementation
  ├── windows.uc      // Windows-specific implementation (Win32 syscalls)
  └── freebsd.uc      // FreeBSD-specific implementation
  ```

#### 10.4.4 Relation with stdlib *(new in 0.3.3)*

- `sys::` is **UltraCPP-provided stdlib** (implemented in `lib/sys/*.uc`), **not** a builtin (not listed in §11.0.1).
- Users obtain it via `import "lib/sys/sys.uc"`; no entry in `BUILTIN_SIGS[]` is required (per runtime-architecture §5.3).
- **raw_syscall**: `sys::raw_syscall` accepts a syscall number + args, and is implemented in `lib/sys/raw.uc` using `asm { "syscall" ... }` (x86_64) / `asm { "svc #0" ... }` (aarch64) / `asm { "ecall" ... }` (riscv64) (per §10.3 + runtime-architecture §6.1).

#### 10.4.5 Implementation Status *(new in 0.3.4, per 0.3.3 process §3.1 commit 8c)*

> **[0.3.4 new]** The `sys::` namespace described in §10.4 completed its codegen implementation (basic `sys$*` prefix mapping) in **0.3.3 commit 8c**.

| API | Implementation status | Related commit |
|-----|----------|-------------|
| `sys$write(1, s, len)` (`sys::write` prefix form) | ✅ Implemented (per 0.3.3 commit 8c) | 8c |
| `sys$read(fd, buf, len)` | ✅ Implemented | 8c |
| `sys::write(fd, buf, len)` full form | ⚠️ Partial (in 0.3.3, only the `sys$*` prefix mapping) | Deferred to 0.3.4 commit 10e |
| `sys::read(fd, buf, len)` full form | ⚠️ Partial | Deferred to 0.3.4 commit 10e |
| `sys::open(path, flags)` | ❌ Not implemented (no such call existed in 0.3.3) | Deferred to commit 10e |
| `sys::close(fd)` | ❌ Not implemented | Deferred to commit 10e |
| `sys::exit(code)` | ❌ Not implemented (main still goes through libc exit) | Deferred to commit 10e |
| `sys::raw_syscall(num, ...)` | ❌ Not implemented | Deferred to commit 10e |
| `sys::` namespace syntax parsing (`sys::name(...)` form) | ⚠️ Partial (in 0.3.3, the `sys$*` prefix instead of `sys::*`) | Deferred to 0.3.5+ |
| Cross-platform unified dispatch (`@ifdef(TARGET_OS_*)`) | ✅ Implemented (preprocessor support) | 8c |

**0.3.4 confirmation**: 0.3.4 commit 11b confirms that 0.3.3 commit 8c is fully landed. `sys::name(args)` and `sys$name(args)` coexist; the underlying dispatch goes through `gen_syscall_call` directly emitting libc `@syscall`. The complete `sys::*` full-namespace syntax plus 6–8 fully implemented `sys::*` functions are advanced by 0.3.4 commit 10e (`lib/sys/sys.uc` + `lib/sys/raw.uc`).

### 10.5 Cross-References with Other Sections *(new in 0.3.3)*

| Section | Relationship |
|------|------|
| §10.1 `extern "C"` block | user-declared external C functions; **not used for stdlib** (per runtime-arch §3.1) |
| §10.2 preprocessor macros | C-style `@ifdef` / `@if defined()`; cross-platform dispatch of `sys::` depends on them |
| §10.3 inline assembly | C / GCC-style `asm { ... }`; implementation of `sys::raw_syscall` depends on it |
| §10.4 `sys::` namespace | cross-platform system-call abstraction (implemented by users in `lib/`, not in the builtin table) |
| §11.0.2 UltraCPP stdlib paths | `lib/sys.uc` belongs to UltraCPP stdlib (§11.0.2) |
| §12.1 EBNF | `preprocessor_directive` / `asm_block` / `qualified_call` productions (added 0.3.3) |

#### 10.5.1 Error Code Mapping *(new in 0.3.3, per runtime-architecture §11)*

> **[0.3.3 new]** After integrating the runtime-architecture decision, the following 6 new error codes are added to the enum in `src-c/include/uc_error.h` (matching the existing `UC_ERR_*` format). The 3 error codes introduced in 0.3.0 (`UC_ERR_MODLAW` / `UC_ERR_CROSS_THREAD_MOVE` / `UC_ERR_OWNING_HEAP_VIEW`) are preserved and recorded in the §12.4 history table.

| Error code | Trigger | Related section |
|---|---|---|
| `UC_ERR_ParserUnknownDirective` | parser encounters an unknown `@directive` (e.g. `@foo`, not in the `@ifdef` / `@if` / `@ifndef` / `@else` / `@elif` / `@end` / `@define` / `@error` / `@warning` list) | §10.2 preprocessor macros |
| `UC_ERR_CodegenAllocNoType` | type `T` cannot be inferred in an `alloc(T)` call (no type annotation + no argument hint) | §7.2 / §11.0.1 |
| `UC_ERR_CodegenModNonRef` | argument of `mod(expr)` / `unmod(expr)` is not an `&T` reference (e.g. a temporary, a literal) | §4.9 / §4.10 / §11.0.1 / Rule 24 |
| `UC_ERR_CodegenAsmInvalidSegment` | one of the 4 sections of an `asm { ... }` block has a syntax error (e.g. output constraint is not `=X` form) | §10.3 inline assembly |
| `UC_ERR_CodegenSysInvalidName` | `name` of `sys::name(args)` is not on the allow-list (e.g. `sys::foo`) | §10.4 `sys::` namespace |
| `UC_ERR_AsmTooManyConstraints` | operands (outputs + inputs) of an `asm` block exceed 16 (the LLVMInlineAsm limit) | §10.3 inline assembly |

**Implementation site**: `src-c/include/uc_error.h` (enum incremental 6 entries) + `src-c/src/error.c` (string mapping 6 lines).

### 10.6 C Type Mapping

| C Type | UltraCPP Type |
|--------|---------------|
| `T*` | `T*` |
| `const T*` | Meaning in UltraCPP **differs from C** (see §3.8 / §7.10); at the FFI bridge, must be explicitly annotated `readonly` |
| `void*` | `void*` |
| `int (*)(T)` | `int (*)(T)` |
| `int` | `int` |
| `double` | `double` |
| `char` | `char` |
| `char*` | `char*` |

> **[0.3.0 new note]** At the FFI boundary, the C standard library's `const char*` (pointing to const data) is in UltraCPP both "pointer-locked" and "data read-only". Both semantics are equivalent to C (neither can write), but the rebind semantics differ: if UltraCPP code holds a `const char*` coming from C, the compiler treats it as **pointer-locked** — this differs from C's `const char*` behavior (pointer is mutable), a direct consequence of the custom semantics in §3.8 of 0.3.0.

> **FFI return type** follows §6.2.1 priority 3 (extern declaration).

### 10.7 unsafe Block

```cpp
unsafe {
    // FFI calls and raw pointer operations
}
```

**Example:**
```cpp
extern "C" {
    void* malloc(int size);
    void free(void* ptr);
}

char* allocate_buffer(int size) {
    unsafe {
        return malloc(size);
    }
}
```

> **[0.3.0 carries over 0.2.0 · D-2, D-7]** Inside an `unsafe` block, **ownership and borrow checks are suppressed**: the implicit `mod()` check of §7.1, the borrow rules of §7.8, and the `DanglingReference` of §7.9 are not reported inside `unsafe`. `unsafe` **does not change semantics** — `move` still nulls the source (§7.4), and `&` is still a reference (§7.8) — it merely shifts the safety responsibility to the programmer.

### 10.8 Inline Assembly *(legacy simple syntax)*

Embedded assembly must be placed in an `unsafe` block.

**Syntax:**
```cpp
unsafe {
    asm {
        "assembly instruction"
        : [name] "constraint"(output_var)
        : [name] "constraint"(input_var)
        : "clobbered_register"
    }
}
```

**Example:**
```cpp
int result;
int input = 10;

unsafe {
    asm {
        "mov eax, %[in_val]"
        "add eax, 5"
        "mov %[out_val], eax"
        : [out_val] "=r"(result)
        : [in_val] "r"(input)
        : "eax"
    }
}
```

**Constraint characters:**
| Constraint | Meaning |
|------|------|
| `r` | General register |
| `=` | Output register |
| `&` | Early clobber (must not be reused) |
| `m` | Memory reference |

---

## 11. Standard Library Conventions *(revised in 0.3.2, revised in 0.3.3, runtime-architecture integrated)*

> **[0.3.2]** This chapter describes UltraCPP standard-library functions' **signature conventions** and **semantic conventions**. The **builtin signature master table** is in §11.0 (new in 0.3.2). The compiler queries the §11.0 table during codegen to determine a function call's return type; see §6.2.1 for details.

### 11.0 Builtin Signature Master Table *(new in 0.3.2)*

UltraCPP provides the following **builtin functions**. Builtin functions are **intrinsically recognized** by the compiler:

- No `#include` of any header is required
- No declaration is needed in user code
- During codegen, the compiler directly emits the corresponding LLVM IR call

> **History**: 0.3.1 spec did not have a builtin signature master table; the signatures were scattered across §11.1–§11.6. 0.3.2 centralizes them as §11.0 to simplify codegen queries and spec maintenance.

#### 11.0.1 builtin language primitives *(new category in 0.3.3, runtime-architecture §5.1 integration)*

The "language primitives" builtins described in §7.0, directly related to memory ownership / borrow checking. **Total of 6 entries**, all class A builtins, all codegen-emitted, all entering `BUILTIN_SIGS[]`:

| Function | Return Type | Parameter Signature | Spec Section | Notes |
|------|----------|----------|----------|------|
| `alloc(T)` | `T*` (owning) | `T` type | §7.2 | heap allocation, returns owning pointer; codegen emits `call i8* @malloc(sizeof(T))` |
| `free(p)` | `void` | `T*` (owning) | §7.3 | release owning pointer; codegen emits `call void @free(i8* %p)` and marks `p` as freed |
| `move(p)` | `T*` (new owner, source moved-out) | `T*` | §7.4 | ownership transfer; codegen emits IR marker + load + returns new owner |
| `clone(s)` | `T*` (independent owning copy) | `T*` | §7.5 | deep copy / independent clone; codegen emits `alloc(T) + LLVMBuildMemcpy` |
| `mod(ref)` | `void` | `T&` (reference) | §4.9 | acquire modification right (Rule 24); codegen emits IR marker (`@uc_borrow_mod_enter`, borrow-checker state entry) |
| `unmod(ref)` | `void` | `T&` (reference) | §4.10 | release modification right (Rule 24); codegen emits IR marker (`@uc_borrow_mod_exit`, borrow-checker state exit) |

**Type parameterization**: `alloc(T)` / `move(p)` / `clone(s)` are type-parameterized builtins — the parameter `T` is inferred from context during codegen (e.g. `alloc(int)` → allocate `sizeof(int)` bytes).

**Precedence**: `mod`, `unmod`, `move`, `clone` are **not in the §4.1 operator precedence table** (0.3.3 revision); see §7.0.3 Key Clarification for details.

**`free` promotion note** (runtime-architecture §3.2 + §5.1):
- In 0.3.2 / 0.3.3 original plans, `free` went through `extern "C"` FFI; now it is promoted to a builtin.
- Rationale: `free` is a state-change point the borrow checker must track (after free, `p` is considered dangling); it cannot transparently pass through libc (otherwise the IR cannot mark the freed state), so the compiler must emit it explicitly.

**`mod` / `unmod` promotion note** (runtime-architecture §3.2 + §5.1):
- In 0.3.2 / 0.3.3 original plans, `mod` / `unmod` were "compile-time decisions without runtime function"; now they are promoted to builtins.
- Rationale: they are the entry / exit of borrow-checker state, and the compiler must record them explicitly in the IR (emit `@uc_borrow_mod_enter` / `@uc_borrow_mod_exit` intrinsics); the dedicated `UC_INTRINSIC_MOD` / `UC_INTRINSIC_UNMOD` indirect path is no longer needed.

#### 11.0.2 UltraCPP stdlib Paths (0.3.4+ Materialized) *(new category in 0.3.3, materialized in 0.3.4, per runtime-architecture §3.1 + §9.2)*

> **[0.3.4 materialization — per D1 + 0.3.4 plan §3.1 commits 10a–10e]** 0.3.3 spec described §11.0.2 as the "future UltraCPP stdlib path" + "0.3.x phase `lib/uc_runtime.c` as a temporary C bootstrap". The 0.3.4 investigation (per 0.3.3 process §2.5) discovered that **`lib/uc_runtime.c` never existed**; 0.3.4 directly bootstraps `lib/*.uc`, and **does not create** the temporary C bootstrap.

**0.3.4 current status** (per commits 10a–10e):

| File | Contents | 0.3.4 Commit | LOC |
|---|---|---|---|
| `lib/print.uc` | `uc_print` / `uc_print_num` / `uc_print_float` (stub for float) | 10a | ~50–80 |
| `lib/string.uc` | `uc_strlen` / `uc_strcpy` / `uc_strcmp` | 10b | ~40–60 |
| `lib/memory.uc` | `uc_memcpy` / `uc_memmove` / `uc_memset` | 10c | ~50–70 |
| `lib/math.uc` | `uc_add` / `uc_abs` (replaces abs_int) | 10d | ~15–20 |
| `lib/alloc.uc` | `uc_alloc` / `uc_free` (thin wrappers) | 10d | ~10 |
| `lib/sys/sys.uc` + `lib/sys/raw.uc` | `sys::write/read/open/close/exit/...` + `raw_syscall` | 10e | ~130–200 |

**Note**: `lib/sync.uc` (`mutex<T>` / `atomic<T>`) is deferred to 0.3.5+ (M5 prerequisite, not in the 0.3.4 scope).

**After 0.3.4, stdlib calls follow this path**:
- `BUILTIN_SIGS[]` contains only 6 primitives (alloc / free / move / clone / mod / unmod, per §11.0.1)
- Historical builtin names (such as `abs_int`) route via extern function lookup to link against `lib/math.o` etc.
- codegen emits `call void @uc_abs(i32 %x)` or similar; the linker resolves the symbol at link time

**Relation to the 0.3.3 text**: 0.3.3 spec §11.0.2 described the "14-entry stdlib removal + `lib/uc_runtime.c` temporary bootstrap" plan. 0.3.4 materializes that plan as a 6-file landing. `BUILTIN_SIGS[]` **no longer** contains `abs_int` / `print` / `strlen` / `memcpy` and other historical builtins; everything goes through extern lookup (per §11.0.3).

See §11.0.5 UltraCPP stdlib Bootstrap Path for details.

#### 11.0.3 Codegen Integration (0.3.4 Revision) *(revised in 0.3.2, revised in 0.3.3, rewritten in 0.3.4, runtime-architecture integration)*

`BUILTIN_SIGS[]` (`src-c/src/codegen.c`) now contains only **6 primitives** (per §11.0.1):
- alloc / free / move / clone / mod / unmod

**Historical builtins (such as `abs_int`) have been removed from `BUILTIN_SIGS[]`** (per commit 11a):
- `abs_int` has simple behavior (`n < 0 ? -n : n`) and is implemented by UltraCPP `lib/math.uc::uc_abs`
- Callers may still write `abs_int(x)`; codegen routes via extern function lookup to link against `lib/math.o::uc_abs`
- Other historical builtins (print / strlen / memcpy etc.) take the extern path through stdlib (`lib/*.uc`)

**No C bootstrap**: The originally planned `lib/uc_runtime.c` C bootstrap was disproven (per 0.3.3 process §2.5). UltraCPP bootstraps `lib/*.uc` directly; see §11.0.5.

The current `BUILTIN_SIGS[]` definition in `src-c/src/codegen.c`:

```c
// Pseudocode (actual implementation in src-c/src/codegen.c)
typedef struct {
    const char* name;
    const char* ret_type;  // LLVM IR type string
    int is_void;
} builtin_sig_t;

// Comments preserved from 0.3.3:
// - null / is_null / sizeof / alignof are not in this table (intrinsic, codegen special handling, per runtime-architecture §5.2)
// - stdlib (print / strlen / memcpy / ...) is not in this table (implemented by UltraCPP stdlib lib/*.uc, per runtime-architecture §5.3)
//
// [0.3.4 commit 11a — D2] abs_int has been removed from BUILTIN_SIGS[] (the original 1st entry):
// - The original inline IR `@builtin_abs_int` 21-line implementation is completely removed from `get_builtins_defs()`
// - The abs_int call goes through the codegen main flow P3 `lookup_extern_func("abs_int")` path
// - At link time the linker resolves to lib/math.o::uc_abs (from lib/math.uc)
//
// Type parameterization is carried by the `UCExprCall.return_type` field (added in 0.3.2 §6.2.1);
// no `is_type_parametric` field is needed.
//
// mod/unmod are **builtins**, emitting IR markers (`@uc_borrow_mod_enter` / `@uc_borrow_mod_exit`),
// not placeholder signatures; they also no longer go through the separate UC_INTRINSIC_MOD / UC_INTRINSIC_UNMOD path.
static const builtin_sig_t builtin_sigs[] = {
    /* [0.3.4 commit 11a] Removed abs_int per D2: migrate to lib/math.uc::uc_abs */
    { "alloc",   "T*",   0 },  // emit: call @malloc(sizeof(T)) + type tag
    { "free",    "void", 1 },  // emit: call @free(%p) + mark %p as freed (NEW, promoted from FFI to builtin)
    { "move",    "T*",   0 },  // emit: IR marker + load + return new owner
    { "clone",   "T*",   0 },  // emit: alloc(T) + LLVMBuildMemcpy
    { "mod",     "void", 1 },  // emit: @uc_borrow_mod_enter (borrow-checker state entry)
    { "unmod",   "void", 1 },  // emit: @uc_borrow_mod_exit (borrow-checker state exit)
    // type-parameterized alloc/move/clone use the ret_type from UCExprCall.return_type
};
```

**UCExprCall codegen main flow** (per 0.3.2 §6.2.1 + 0.3.4 revision, 4-level priority chain):

```c
// 4-level priority chain (per 0.3.2 §6.2.1)
// P1: sys$* prefix → direct mapping to libc / sys:: (0.3.3 commit 8c)
// P2: BUILTIN_SIGS[] lookup → 6 language primitives emit
// P3: lookup_extern_func(name) → extern lookup (user extern "C" or stdlib)
// P4: default → assume i32 return (fallback)

static LLVMValueRef gen_call(CodeGen* g, UCExprCall* call) {
    const char* fn_name = call->fn_name;

    // P1: sys$* prefix (libc / sys:: forwarding)
    if (strncmp(fn_name, "sys$", 4) == 0) {
        return gen_syscall_call(g, call, fn_name + 4);
    }

    // P2: BUILTIN_SIGS[] lookup — 6 language primitives
    const builtin_sig_t* sig = lookup_builtin_sig(fn_name);
    if (sig != NULL) {
        return emit_builtin_call(g, call, sig);
    }

    // P3: extern lookup — [0.3.4 commit 11a] abs_int goes through this branch
    // - User `extern "C" { int abs_int(int x); }` declaration
    // - stdlib (lib/math.uc::uc_abs) provides the @abs_int symbol
    // - libc fallback (fully removed in 0.4.0+)
    LLVMValueRef fn = lookup_extern_func(g, fn_name);
    if (fn != NULL) {
        return emit_extern_call(g, call, fn);
    }

    // P4: default — assume i32 return (fallback)
    return emit_default_call(g, call, "i32");
}
```

> **[0.3.3 Plan A simplification + runtime-architecture integration + 0.3.4 rewrite]**
>
> - `BUILTIN_SIGS[]` **only contains the 6 §11.0.1 language primitives** (was 7 entries; `null` changed to intrinsic, per runtime-architecture §5.2)
> - **stdlib is not in BUILTIN_SIGS[]** — replaced by UltraCPP stdlib (`lib/*.uc`); no longer goes through `extern "C" libc`
> - **No need** for the 14 stdlib entries — all provided by UltraCPP's own stdlib
> - **No need** for a separate `UC_INTRINSIC_MOD` / `UC_INTRINSIC_UNMOD` path — mod / unmod are moved into BUILTIN_SIGS[], going through the `BUILTIN_SIGS[]` lookup + unified `UC_EXPR_CALL` emit path (consistent with §6.2.1 priority chain)
> - **No more placeholder signatures** — mod / unmod are all real emit (about 5 LOC each, outputting `@uc_borrow_mod_enter` / `@uc_borrow_mod_exit` IR markers)
> - **`abs_int` has been removed** (0.3.4 commit 11a) — takes the P3 extern lookup path, linking to `lib/math.o::uc_abs`

#### 11.0.4 Flow for Adding a New Builtin *(new in 0.3.2, revised in 0.3.3)*

To add a new builtin in the future (e.g. `sqrt`, `pow`):

1. Add the detailed description (semantics, examples) in a §11.x sub-section.
2. Add a signature row in the §11.0 table.
3. Add an entry to `builtin_sigs[]` in `src-c/src/codegen.c`.
4. (Optional) Add a stub implementation in `src-c/src/stdlib/` if the builtin needs runtime support.

#### 11.0.5 Cross-References with Other Sections *(new in 0.3.2, revised in 0.3.3)*

| Section | Relationship |
|------|------|
| §6.2.1 Function return type | the builtin signature is the basis for priority 1 |
| §7.4 `alloc` and C stdlib | `malloc` is brought in via §10 `extern "C"`; codegen consults the extern symbol table (not the builtin table) |
| §7.5 `move` | `move` is a builtin (type-parameterized), see the §11.0.1 table |
| §11.1–§11.6 | detailed sub-section descriptions reference §11.0 |
| §12.1 EBNF | the function-call production is defined in §12.1 |

### 11.1 Basic I/O Functions

> **[0.3.2]** This sub-section details the following builtins: `print`, `print_num`, `print_float`. For the complete signature table see §11.0.

```cpp
void print_num(int n);        // print integer
void print(const char* s);    // print string
void print_float(double f);   // print float
```

### 11.2 String Functions

> **[0.3.2]** This sub-section details the following builtins: `strlen`, `strcpy`, `strcmp`. For the complete signature table see §11.0.

```cpp
int strlen(const char* s);  // string length
char* strcpy(char* dest, const char* src);
int strcmp(const char* a, const char* b);
```

### 11.3 Memory Functions

> **[0.3.2]** This sub-section details the following builtins: `memcpy`, `memmove`, `memset`, `alloc` (type-parameterized), `move` (type-parameterized). For the complete signature table see §11.0.

```cpp
void* memcpy(void* dest, const void* src, int n);
void* memmove(void* dest, const void* src, int n);
void* memset(void* s, int c, int n);
```

### 11.4 Utility Functions

> **[0.3.2]** This sub-section details the following builtins: `sizeof_impl`, `alignof_impl`, `is_null`, `clone_impl`, `abs_int`. For the complete signature table see §11.0.

```cpp
int sizeof_impl();       // size of a type
int alignof_impl();      // alignment of a type
bool is_null(int* ptr);  // null check
int* clone_impl(int* ptr);  // pointer clone
```

### 11.5 Thread Primitives *(new in 0.3.0)*

```cpp
// --- Cross-thread ownership ---
move_to_thread(T* p, int tid);    // explicitly transfer ownership to thread tid
spawn_thread(int tid, void fn()); // start a thread without ownership transfer
spawn_thread_with(int tid, T* p); // start a thread and implicitly move(p)
join_thread(int tid);             // wait for the thread to finish
join_all();                       // wait for all spawned threads
current_tid();                    // current thread id

// --- Thread-local storage ---
__thread int x;                   // per-thread independent
__thread int buf[256];            // per-thread independent array

// --- Shared primitives ---
mutex<T>          mx;             // explicit mutex wrapper (stdlib type, see §11.5.1)
atomic<T>         at;             // explicit atomic wrapper (stdlib type, see §11.5.1)
shared T*         sp = ...;       // visible across threads (enforced at compile time)

// --- Implicit thread support ---
thread1() { ... }                 // compiler auto-generates mutex / barrier
```

#### 11.5.1 Shared Synchronization Types `mutex<T>` / `atomic<T>` *(new in 0.3.0)*

> **[new in 0.3.0]** `mutex<T>` and `atomic<T>` are **standard-library types** (defined in `lib/sync.uc`), **not keywords**. They provide explicit wrappers for cross-thread modification rights.

**`mutex<T>`**:

| Member | Description |
|------|------|
| `lock()` | Acquire the lock (blocks until obtained) |
| `unlock()` | Release the lock |
| `try_lock()` | Try to acquire the lock; returns `false` on failure |
| `wait()` | Wait on a condition variable (used with `notify`) |
| `notify()` / `notify_all()` | Wake waiters |

```cpp
mutex<int> mx;

void worker() {
    mx.lock();
    // critical section
    mx.unlock();
}
```

**`atomic<T>`**:

| Member | Description |
|------|------|
| `load()` | Atomic read |
| `store(v)` | Atomic write |
| `exchange(v)` | Atomic swap |
| `compare_exchange(expected, desired)` | CAS |

```cpp
atomic<int> state;

void worker() {
    int v = state.load();
    state.store(v + 1);
}
```

**Relationship with `#modlaw`**:

- The lock / load / store operations on `mutex<T>` / `atomic<T>` do **not** go through the `mod()` system — they bring their own synchronization primitives.
- When a variable's type is `mutex<T>`, the compiler's auto-generated mutex is **suppressed** (the programmer already controls it explicitly), avoiding double locking.
- The same applies to `atomic<T>`: hardware atomic instructions already guarantee visibility, so the compiler does not need to generate synchronization code.

### 11.6 `alloc` / `free` Pairing Convention *(carried over from 0.2.0 in 0.3.0 · D-6, plus §11.0 cross-reference in 0.3.2)*

> **[carried over in 0.3.0]** The pairing of `alloc(T)` with `free(p)` is the programmer's responsibility; the compiler does **not** enforce it.
>
> **[0.3.2]** `alloc` and `move` are listed in the §11.0 builtin signature master table (type-parameterized builtins); `free` / `malloc` are C standard-library functions and must be brought in via §10 `extern "C"` blocks; they are **not** in the §11.0 builtin table. See §11.0.4 and §10.1 for details.

**Specification rules**:

1. Every successful `alloc(T)` produces a fresh allocation whose ownership belongs to the `unique T` variable that receives the result.
2. Every allocation **shall** be passed to `free` exactly once. This is a **programmer responsibility**.
3. The compiler does **not** report: missing free (memory leak), double free, or freeing a pointer that did not come from `alloc`.
4. The compiler **still** reports `UseAfterMove` / `UseAfterDrop` / `DanglingReference`.

---

## 12. Appendix

### 12.1 Complete EBNF Grammar *(revised in 0.3.0, reclassified in 0.3.3)*

> **[0.3.3 · final state]** The productions below reflect the **final state** after reclassifying `mod`, `unmod`, `move`, `clone` as **builtin function calls** (§7.0.3 + §11.0.1) rather than operators. `move_to_thread` likewise becomes a builtin function call. The `unary_expression` / `rvalue` productions below no longer list these keywords; instead they are emitted under `builtin_function_call`. Five new productions are appended at the end of this section: `builtin_function_call`, `intrinsic_call`, `preprocessor_directive`, `asm_block`, `qualified_call`. This is the final state per runtime-architecture §5 and spec-text-changes §6.2.
>
> **[0.3.0]** Additions and changes relative to 0.2.0 (preserves all 0.2.0 productions; adds mod/unmod/move_to_thread/__thread/shared):
> 1. The keyword list adds `mod`, `unmod`, `shared`, `__thread`, `move_to_thread`.
> 2. `unary_expression` adds `'mod' '(' expression ')'` and `'unmod' '(' expression ')'` (Rule 24).
> 3. A new `preprocessor_directive` top-level production is added, including the `#modlaw` rule (Rule 23).
> 4. `unary_expression` adds `'move_to_thread' '(' expression ',' expression ')'` (Rule 26).
> 5. `declaration` / `variable_declaration` accept an optional storage class `('shared' | '__thread')` (Rules 25, 27).
> 6. The `pointer_type` under `type` adds `const T*` and `T* const` (Q6; custom semantics opposite to C++).

```
// === Program ===
program           ::= top_level_declaration*

top_level_declaration
                   ::= function_definition
                    | struct_definition
                    | export_declaration
                    | import_directive
                    | include_directive
                    | modlaw_directive           // [0.3.0 Rule 23]
                    | const_declaration
                    | ';'

// === Preprocessor ===
import_directive  ::= '#import' string_literal ('as' identifier)? ';'
include_directive ::= '#include' string_literal ';'

// [0.3.0 Rule 23] #modlaw directive — 6 legal combinations only
modlaw_directive  ::= '#modlaw' modlaw_perm modlaw_scope
modlaw_perm       ::= 'none' | 'exclusive' | 'shared'
modlaw_scope      ::= 'global' | 'module'

// === Export ===
export_declaration ::= 'export' declaration

// === Functions ===
function_definition
                   ::= type identifier '(' parameter_list? ')' compound_statement

parameter_list     ::= parameter (',' parameter)*
parameter          ::= identifier ':' type

// === Struct ===
struct_definition  ::= 'struct' identifier '{' struct_field_list '}'
struct_field_list ::= struct_field (';' struct_field)*
struct_field      ::= identifier ':' type

// === Declarations ===
declaration       ::= storage_class? variable_declaration
                    | const_declaration

// [0.3.0 Rule 25, 27] optional storage class on declarations
storage_class     ::= 'shared' | '__thread'

variable_declaration
                   ::= type identifier ('=' expression)? ';'

const_declaration  ::= 'const' type identifier '=' expression ';'

// === Types ===
type              ::= fundamental_type
                    | pointer_type
                    | unique_type              // [0.2.0 D-1]
                    | reference_type
                    | array_type
                    | function_type
                    | type_identifier

fundamental_type  ::= 'void' | 'bool' | 'char' | 'int' | 'i8' | 'i16' | 'i32' | 'i64'
                    | 'uint' | 'u8' | 'u16' | 'u32' | 'u64'
                    | 'f32' | 'f64' | 'usize' | 'isize'

// [0.3.0 Q6] custom `const T*` / `T* const` semantics — opposite of C++
pointer_type      ::= type '*'
                    | type '*' 'const'                              // [0.3.0 Q6] data read-only view
                    | 'const' type '*'                              // [0.3.0 Q6] pointer-locked read-only
                    | 'const' type '*' 'const'

// [0.2.0 D-1] `unique` is a generic type constructor: `unique T` for any T.
unique_type       ::= 'unique' type

// [0.3.0 Q2] reference declaration initializer is an lvalue (no `&` prefix)
reference_type    ::= type '&'

array_type        ::= type '[' integer_literal ']'
function_type     ::= type '(' type_list? ')'

// === Statements ===
statement         ::= expression_statement
                    | compound_statement
                    | selection_statement
                    | iteration_statement
                    | jump_statement
                    | free_statement
                    | declaration_statement

expression_statement
                   ::= expression? ';'

compound_statement ::= '{' statement* '}'

selection_statement
                   ::= 'if' '(' expression ')' statement ('else' statement)?

iteration_statement
                   ::= 'while' '(' expression ')' statement
                    | 'for' '(' for_init? expression? ';' expression? ')' statement
                    | 'for' '(' identifier ':' expression ')' statement

for_init          ::= declaration | expression

jump_statement    ::= 'return' expression? ';'
                    | 'break' ';'
                    | 'continue' ';'

free_statement    ::= 'free' '(' expression ')' ';'

declaration_statement
                   ::= declaration

// === Expressions ===
expression        ::= assignment_expression

// [0.3.1] Assignment LHS must be an lvalue (see §4.13.2). The grammar permits
// unary_expression on the LHS; the compiler enforces lvalue category in semantic
// analysis (or codegen for legacy paths).
assignment_expression
                   ::= lvalue '=' assignment_expression             // [0.3.1] LHS constraint
                    | lvalue compound_assignment assignment_expression

cast_expression    ::= '(' type ')' unary_expression
                    | unary_expression

// [0.3.1 Rule S2] lvalue / rvalue categorization — see §4.13 for full semantics.
lvalue            ::= identifier                                     // variable
                    | '*' cast_expression                            // unary deref
                    | postfix_expression '[' expression ']'         // index
                    | postfix_expression '.' identifier              // field access
                    | '(' lvalue ')'                                 // parenthesized lvalue
                    | lvalue '++'                                    // prefix increment (§4.13.2 row 6)
                    | lvalue '--'                                    // prefix decrement
                    | assignment_expression                          // result of `x = v` is lvalue

rvalue            ::= literal                                        // integer / float / string / char / bool / null
                    | postfix_expression '(' argument_list? ')'      // function call result
                    | postfix_expression '++'                        // postfix increment
                    | postfix_expression '--'                        // postfix decrement
                    | '(' rvalue ')'                                 // parenthesized rvalue
                    | '&' unary_expression                           // address-of result (pointer rvalue)
                    | builtin_function_call                          // [0.3.3] alloc/free/move/clone/mod/unmod
                    | 'move_to_thread' '(' expression ',' expression ')'  // [0.3.3] kept here as builtin (per plan §3.7 conservative)

conditional_expression
                   ::= logical_or_expression ('?' expression ':' conditional_expression)?

logical_or_expression
                   ::= logical_and_expression ('||' logical_and_expression)*

logical_and_expression
                   ::= bitwise_or_expression ('&&' bitwise_or_expression)*

bitwise_or_expression
                   ::= bitwise_xor_expression ('|' bitwise_xor_expression)*

bitwise_xor_expression
                   ::= bitwise_and_expression ('^' bitwise_and_expression)*

// Binary infix '&' remains bitwise AND. Unary prefix '&' is a reference.
bitwise_and_expression
                   ::= equality_expression ('&' equality_expression)*

equality_expression
                   ::= relational_expression (('==' | '!=') relational_expression)*

relational_expression
                   ::= shift_expression (('<' | '>' | '<=' | '>=') shift_expression)*

shift_expression   ::= additive_expression (('<<' | '>>') additive_expression)*

additive_expression
                   ::= multiplicative_expression (('+' | '-') multiplicative_expression)*

multiplicative_expression
                   ::= unary_expression (('*' | '/' | '%') unary_expression)*

unary_expression   ::= postfix_expression
                    | '(' type ')' unary_expression                 // C-style cast [0.3.2]
                    | 'cast' '(' type ',' expression ')'              // explicit cast builtin [0.3.2]
                    | ('+' | '-' | '!' | '~' | '*' | '&') unary_expression
                    | 'sizeof' '(' type ')'                           // [0.3.3] intrinsic_call (codegen-special, not in BUILTIN_SIGS[])
                    // [0.3.3] removed: 'move' / 'clone' / 'mod' / 'unmod' / 'move_to_thread' — reclassified as builtin_function_call (§7.0.3)

postfix_expression ::= primary_expression
                    | postfix_expression '[' expression ']'
                    | postfix_expression '(' argument_list? ')'
                    | postfix_expression '.' identifier
                    | postfix_expression '->' identifier
                    | postfix_expression '++'
                    | postfix_expression '--'

primary_expression ::= identifier
                    | literal
                    | '(' expression ')'
                    | 'null'
                    | builtin_function_call                          // [0.3.3] alloc/free/move/clone/mod/unmod
                    | intrinsic_call                                 // [0.3.3] null/is_null/sizeof/alignof (codegen-handled)
                    | asm_block                                      // [0.3.3] 'asm' '{' ... '}'
                    | qualified_call                                 // [0.3.3] identifier '::' identifier '(' ... ')'

argument_list      ::= expression (',' expression)*

literal            ::= integer_literal
                    | float_literal
                    | char_literal
                    | string_literal
                    | 'true'
                    | 'false'

// === Lexical ===
integer_literal    ::= decimal | hex | octal | binary
hex                ::= '0x' [0-9a-fA-F]+
octal              ::= '0' [0-7]+
binary             ::= '0b' [01]+
decimal            ::= [1-9] [0-9]* | '0'

float_literal      ::= [0-9]+ '.' [0-9]*
                    | '.' [0-9]+
                    | [0-9]+ '.'

char_literal       ::= "'" character "'"
string_literal     ::= '"' (character | escape)* '"'

identifier         ::= letter (letter | digit)*
letter             ::= 'a'..'z' | 'A'..'Z' | '_'                      // '__thread' uses '_'_'t'...
digit              ::= '0'..'9'

// === [0.3.3] Reclassification as builtin functions (final state) ===
// mod / unmod / move / clone are NO LONGER operators; they are builtin function
// calls dispatched by codegen. alloc / free remain builtin function calls (they
// were already function-call form in 0.3.2). See §7.0.3 Reclassification as
// builtin functions and §11.0.1 builtin language primitives.
builtin_function_call ::= 'alloc' '(' type ')'
                       |  'free' '(' expression ')'
                       |  'move' '(' expression ')'
                       |  'clone' '(' expression ')'
                       |  'mod' '(' expression ')'                       // [0.3.3] acquires mod-right (was operator in 0.3.0/0.3.1/0.3.2)
                       |  'unmod' '(' expression ')'                     // [0.3.3] releases mod-right (was operator in 0.3.0/0.3.1/0.3.2)

// === [0.3.3] Intrinsic calls (codegen-special; NOT in BUILTIN_SIGS[]) ===
// See runtime-architecture §5.2 and §11.0.1 (intrinsics are 4 entries).
intrinsic_call     ::= 'null' '(' ')'
                    |  'is_null' '(' expression ')'
                    |  'sizeof' '(' type ')'
                    |  'alignof' '(' type ')'

// === [0.3.3] Preprocessor directives (C-style, @-prefixed) ===
// See §10.2 Preprocessor Macros.
preprocessor_directive ::= '@ifdef' '(' identifier ')' directive_body_list '@end'
                       |  '@ifndef' '(' identifier ')' directive_body_list '@end'
                       |  '@if' preprocessor_expr directive_body_list '@end'
                       |  '@elif' preprocessor_expr
                       |  '@else' directive_body_list '@end'
                       |  '@define' '(' identifier ',' preprocessor_expr ')'
                       |  '@error' '(' string_literal ')'
                       |  '@warning' '(' string_literal ')'

// === [0.3.3] Inline assembly (GCC constraint style) ===
// See §10.3 Inline Assembly.
asm_block          ::= 'asm' '{' asm_template '}'
                    |  'asm' 'volatile' '{' asm_template '}'

// === [0.3.3] Qualified (namespaced) call ===
// See §10.4 sys:: System Call Namespace.
qualified_call     ::= identifier '::' identifier '(' argument_list? ')'
```

#### 12.1.1 Note on Parsing `&` *(revised in 0.3.0)*

0.3.0 **removes the `&mut` production**. `'&' unary_expression` is the sole reference production; no lookahead is required — `&` is always followed by a unary expression. `mut` is no longer a keyword.

#### 12.1.2 Note on Parsing `const T*` / `T* const` *(new in 0.3.0: Q6)*

The position of `const` and `*` in `type` requires **one token of lookahead**:

- `const type *` → pointer is locked (the pointer cannot rebind).
- `type * const` → data is a read-only view.

This is **opposite to the C++** parsing rules — in UltraCPP, `const` always modifies the token **immediately to its right** or **immediately to its left**, but does **not** propagate to "the pointed-to object". See §3.8 / §7.10 for detailed semantics.

### 12.2 Reserved Keywords *(revised in 0.3.0)*

```
as         break      char       const      continue
else       export     extern     false      for
free       if         import     include    move
null       return     static     struct
true       typedef    unique     unsafe     void
while      clone
mod        unmod      shared     __thread   move_to_thread
```

> **[new in 0.3.0]** `mod`, `unmod`, `shared`, `__thread`, `move_to_thread`. **0.3.0 removes** `mut` (it existed only as a component of the `&mut` token; no longer needed once `T&mut` was removed). See §4.9, §4.10, §13 for details.

### 12.3 Operator Precedence Table *(revised in 0.3.0; level 2.5 added in 0.3.1; reclassified in 0.3.3)*

> **[0.3.3 · final state]** 0.3.3 reclassifies `move`, `clone`, `mod`, `unmod` from **operators** to **builtin function calls** (§7.0.3 Reclassification as builtin functions + §11.0.1 builtin language primitives). Level 15 is therefore **removed**. The level 2.5 prefix row likewise drops `mod` / `unmod`. These four names are still reserved keywords (§12.2), but they no longer occupy any operator-precedence level. `move_to_thread` likewise becomes a builtin function call (per §13.2 cross-thread ownership).

| Level | Operator | Description |
|------|--------|------|
| 1 | `::` | Scope resolution |
| 2 | `()` `[]` `.` `->` `++` `--` | Postfix |
| 2.5 | `*` `&` `+` `-` `!` `~` `++` `--` (prefix) | Unary *(added in 0.3.1; 0.3.3 removed `mod` / `unmod`)* |
| 2.5 | `(`*type*`)` | **C-style cast** *(added in 0.3.2)* |
| 3 | `*` `/` `%` | Multiplicative |
| 4 | `+` `-` | Additive |
| 5 | `<<` `>>` | Shift |
| 6 | `<` `>` `<=` `>=` | Relational |
| 7 | `==` `!=` | Equality |
| 8 | `&` | Bitwise AND (**binary infix**) |
| 9 | `^` | Bitwise XOR |
| 10 | `\|` | Bitwise OR |
| 11 | `&&` | Logical AND |
| 12 | `\|\|` | Logical OR |
| 13 | `?:` | Ternary conditional |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | Assignment |

> *[0.3.3 removed]* Level 15 (`move` / `clone` / `move_to_thread`) — these names are no longer operators. They are dispatched as builtin function calls via `builtin_function_call` (§12.1 EBNF; §7.0.3 Reclassification).

**Unary operators** (higher precedence than every binary operator above; right-to-left associativity):

| Operator | Description |
|--------|------|
| `+` `-` | Unary plus / negation |
| `!` | Logical NOT |
| `~` | Bitwise NOT |
| `*` | Dereference (§4.13.5) |
| `&` | **Reference** (§3.3) |

> *[0.3.3 removed]* `mod` / `unmod` from the unary-operators sub-table — they are now `builtin_function_call` (§7.0.3 Reclassification as builtin functions), not operators.

### 12.4 Appendix X: Impact on Future Work *(revised in 0.3.0)*

#### 12.4.1 Implementation Strategy (carried over from 0.2.0)

The C host (`src-c/`) is the production compiler; this specification's semantics take it as the first landing target. See `.dev/plans/0.1.0-devhandbook.md` for the concrete algorithms:

| Specification section | devhandbook reference | Content |
|-----------|------------------|------|
| §3.2 / §7.1 Ownership + modification-right split | §4.4 | Dual-state table (ownership bit + mod bit) |
| §7.4 `move(p)` | §4.8 `record_move` | Move points |
| §7.8 References (`T&`) | §4.5, §4.7 | Borrow + mod rules |
| §7.9 Dangling | §4.6, §12.4 | Lifetime inference |
| §9.9 `#modlaw` | §4.5 (extended) | Modification-right policy table |
| §11.5.1 `mutex<T>` / `atomic<T>` | §4.6 (extended) | Explicit synchronization types |
| §13.1 Cross-thread visibility | §4.6 (extended) | `shared` enforced annotation |
| §13.2 Cross-thread ownership | §4.8 (extended) | `move_to_thread` implementation |

**New artifacts (0.3.0):**

1. Lexical: `mod`, `unmod`, `shared`, `__thread`, `move_to_thread` keywords and the `#modlaw` directive (`src-c/src/lexer.c`).
2. Grammar: 4 new productions + new parsing rules for `const T*` / `T* const` (`src-c/src/parser.c`).
3. Error kinds: `UC_ERR_MODLAW`, `UC_ERR_CROSS_THREAD_MOVE`, `UC_ERR_OWNING_HEAP_VIEW` (`src-c/include/uc_error.h`).
4. Compilation phase: ownership checker upgraded to "dual state" — ownership bit + modification-right bit (`src-c/src/ownership.c`).
5. Threading support: `spawn_thread`, `join_thread`, `mutex<T>`, `atomic<T>` implementations (`src-c/src/thread.c`).
6. Test corpus: `test/modlaw/{pass,fail}/`, `test/threading/{pass,fail}/`, `test/const_ptr/{pass,fail}/`.

#### 12.4.2 Items Not Covered in This Version

| Item | Status | Destination |
|------|------|------|
| `MissingFree` / `DoubleFree` | Not implemented | §11.6, candidate optional diagnostics |
| `Owned<T>` / `Ref<T>` / `RefMut<T>` internal types | Not exposed in the user language | devhandbook §4.1 |
| Precise definition of non-lexical lifetimes | Only described as "until last use" | To be backfilled after algorithm implementation |
| Concrete syntax for `__thread` at declaration sites | `__thread T x;` form already supported | Implementation details pending |
| Runtime switching of `#modlaw` | Not implemented | 0.3.0 is compile-time policy only; runtime switching deferred |

**12.4.3 0.3.3-Specific Items Not Yet Implemented *(new in 0.3.3)***

> **[0.3.3]** 0.3.3 is **spec clarification only** — no new syntax, no bug fixes, no semantic changes. The new sections (§4.13.5 deref Semantics, §7.0 Concept Clarification, §10.2 / §10.3 / §10.4 preprocessor / inline assembly / `sys::`, §11.0.1 / §11.0.2 builtin language primitives vs stdlib split) are documentation additions and future-proofing. This subsection records items that the 0.3.3 clarification explicitly defers to later versions. The 0.3.4–0.3.8 consolidated plan (`.dev/plans/0.3.4-0.3.8-plan.md`) provides the implementation roadmap.

| Item | Status | Destination |
|------|------|------|
| Full codegen for §10.2 `@if` / `@elif` preprocessor expressions with non-trivial `preprocessor_expr` | Spec only | 0.3.4+ plan |
| §10.3 `asm` block constraint mapping for non-x86_64 architectures (aarch64, riscv64) | Only x86_64 codegen path verified | 0.3.5+ plan |
| §10.4 `sys::` syscall namespace for non-Linux platforms (macOS, Windows) | Only Linux implemented | 0.3.5+ plan |
| §11.0.2 UltraCPP stdlib `lib/*.uc` paths (print / string / memory / math / alloc / sync / sys) | Stub implementations only | 0.3.6+ plan, full stdlib coverage |
| §11.0.1 builtin function-call codegen for all 6 entries (`alloc` / `free` / `move` / `clone` / `mod` / `unmod`) on non-x86_64 | Verified for x86_64 only | 0.3.5+ plan |
| §4.13.5.4 positional-overloading rule (compiler distinguishes `*` deref vs `*` mul by operand context) | Documentation only; current codegen uses 0.3.2 logic | Tracked for 0.3.4 review |
| §7.0.3 Reclassification as builtin functions: codegen path that emits `@uc_borrow_mod_{enter,exit}` IR markers for `mod` / `unmod` | Spec-defined; codegen follow-up | 0.3.4 implementation |
| `move_to_thread` reclassification as `builtin_function_call` (currently kept in `rvalue` per plan §3.7 conservative) | Spec stable; pending cleanup | 0.3.4 EBNF review |

> **Note**: All items in §12.4.3 are **clarification-driven** — the 0.3.3 spec makes the language model explicit but does not commit to a specific implementation schedule. The unified 0.3.4–0.3.8 plan (`.dev/plans/0.3.4-0.3.8-plan.md`) is the canonical roadmap for these items.

---

## 13. Threading Model *(new chapter in 0.3.0)*

> **[0.3.0]** This chapter is an entirely new chapter added in 0.3.0. UltraCPP introduces a threading model in 0.3.0: the `shared` annotation for cross-thread visibility (Rule 25), explicit cross-thread ownership transfer (Rule 26), thread-local storage `__thread` (Rule 27), and four storage classes (Rule 28).

### 13.1 Shared Variables (`shared` Annotation Enforced)

> **[0.3.0 · Rule 25]** Cross-thread access must use the `shared` annotation; otherwise it is a compile error.

**Syntax**:
```cpp
shared T var;
shared T* p = alloc(T);    // owning + shared (shared heap object)
shared int* counter;       // visible across threads
```

**Enforcement rules**:

- Any **global / static** variable accessed inside a `spawn_thread(...)` body **must** be declared `shared`. Otherwise the compile-time error is `non-shared global accessed from spawned thread`.
- `__thread` variables are exempt (each thread has its own; `shared` is not required).

**Compiler auto-generated mutex**:

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);  // shared owning pointer

thread1() {
    int& r = counter;
    mod(r);                          // compile-time OK; runtime auto-generates a mutex wrapper
    *r = *r + 1;                     // exclusive write
} // r leaves scope → automatic unmod

thread2() {
    int& r = counter;
    mod(r);                          // blocks if thread1 still holds the lock
    *r = *r + 1;
}
```

**Optional explicit programmer wrapper**:

```cpp
shared mutex<int*> counter;     // explicit mutex wrapper (`mutex<T>` is a stdlib type, see §11.5.1)
shared atomic<int> state;        // explicit atomic wrapper (`atomic<T>` is a stdlib type, see §11.5.1)
```

> **[revised in 0.3.0]** `mutex<T>` and `atomic<T>` are **§11.5.1 standard-library types**, not keywords. **When a variable's type is `mutex<T>` or `atomic<T>`, the compiler's auto-generated mutex is suppressed** — the programmer explicitly controls synchronization, avoiding double locking.

### 13.2 Cross-thread Ownership (Rule 26)

> **[0.3.0 · Rule 26]** Cross-thread ownership must be transferred explicitly via `move_to_thread(p, tid)` or `spawn_thread_with(tid, p)`.

**Example F (cross-thread ownership):**

```cpp
int x = 42; // created by thread1

move_to_thread(x, thread2_id);  // x is transferred to thread2
// x in thread1 is now moved-out

// Or via spawn:
int y = 100;
spawn_thread_with(thread2_id, y);  // implicit move

// In thread2:
void thread2() {
    // both x and y are visible and accessible
}
```

**Rules**:

1. **`move_to_thread(p, tid)`**: explicitly transfers ownership of `p` (must be owning or a reference) to thread `tid`; the caller has no further access to `p` afterwards.
2. **`spawn_thread_with(tid, p)`**: implicitly moves at thread launch; equivalent to `move_to_thread(p, tid)` + `spawn_thread(...)`.
3. **Stack references cannot cross threads**: if the owner of a `T&` borrow is a stack variable, you cannot `move_to_thread` it — the stack is thread-local (see §13.4).
4. **Limits on cross-thread references**: a reference passed across threads can only be: heap (shared owner), global (shared), or TLS (thread-local — but the sender and receiver must correctly match the TLS).

### 13.3 Cross-thread Modification Rights (mutex / atomic)

> **[0.3.0 · Rule 25 cont.]** Cross-thread modification rights are supported by the compiler's auto-generated mutex or the programmer's explicit `mutex<T>` / `atomic<T>` wrappers. `mutex<T>` and `atomic<T>` are §11.5.1 standard-library types, not keywords.

**Auto mutex (compile time):**

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);

void worker() {
    int& r = counter;
    mod(r);           // compiler emits pthread_mutex_lock
    *r = *r + 1;      // critical section
}                     // leaving scope → pthread_mutex_unlock
```

**Explicit `mutex<T>`:**

```cpp
mutex<int> mx;

void worker_explicit() {
    mx.lock();         // explicit lock (not mod() — see §11.5.1)
    // critical section
    mx.unlock();       // explicit unlock
}
```

**Explicit `atomic<T>`:**

```cpp
atomic<int> state;

void worker_atomic() {
    int v = state.load();    // hardware-atomic load
    state.store(v + 1);      // hardware-atomic store
}
```

> **Relationship with `mod()`**: the lock / load / store operations on `mutex<T>` / `atomic<T>` do **not** go through the `mod()` channel — they bring their own synchronization primitives. `mod()` requests the "modification right", which applies to ordinary `T&` references. The two mechanisms do not conflict.

**`shared` combined with `#modlaw`:**

| `#modlaw` | `shared` + mod behavior |
|------------|----------------------|
| `shared` | Multiple threads can mod simultaneously; the programmer is responsible for data races |
| `exclusive` | Compile-time / runtime checks enforce a single lock holder; blocks when not holding the lock |
| `none` | `shared` is not allowed to mod (compile-time error) |

### 13.4 Multi-thread Memory Layout (Rule 28)

> **[0.3.0 · Rule 28]** Thread visibility of each of the four memory regions.

| Storage class | Thread visibility | Cross-thread reference transfer allowed? | Notes |
|--------|------------|------------------|------|
| **Stack** (function-local variables) | thread-local | ❌ No | Owner is destroyed when it leaves scope |
| **Heap** (`alloc(T)`) | **shared** | ✅ Yes (requires `shared` or `move_to_thread`) | Single owner, movable |
| **Global / static** (file scope / `static`) | **shared** | ✅ Yes (requires the `shared` annotation) | Process-level lifetime |
| **TLS** (`__thread`) | thread-local | ❌ No | GCC-style; per-thread independent |

**Key constraints**:

- **Stack** references **cannot** be passed to other threads (the owner is destroyed when the other thread leaves scope).
- Cross-thread reference passing is allowed only for **heap / global / TLS** — for TLS, the sender and receiver must be the same thread.
- **Not allowed**: implicitly capturing stack variables via `spawn_thread`. The compiler emits the error `cannot capture stack reference into spawned thread`.

### 13.5 `__thread` Thread-local Storage (Rule 27)

> **[0.3.0 · Rule 27]** `__thread T x;` (GCC-style) declares a thread-local variable.

**Syntax**:
```cpp
__thread int x;               // per-thread independent int
__thread int buf[256];        // per-thread independent array
__thread int* p;              // per-thread independent pointer (the pointer itself is thread-local; what it points to is not necessarily so)
```

**Semantics**:

- Each thread has an **independent copy** of `x`; they do not affect each other.
- Initialization happens exactly once (the main thread); other threads inherit the main thread's initial value. This is the standard GCC `__thread` semantics.
- The `shared` annotation is not required (thread-local is naturally isolated).

**Examples**:

```cpp
__thread int local = 42;     // per-thread independent

void worker() {
    local = local + 1;       // only affects the current thread
    print_int(local);
}
```

### 13.6 Complete Threading Model Examples

> **Example E (comprehensive threading model example):**

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);  // visible across threads (shared is enforced)

thread1() {
    int& r = counter;
    mod(r);                          // compile-time OK; runtime mutex
    *r = *r + 1;                      // exclusive write
} // r leaves scope → automatic unmod

thread2() {
    int& r = counter;
    mod(r);                          // blocks if thread1 still holds the lock
    *r = *r + 1;
}

main() {
    spawn_thread(thread1);
    spawn_thread(thread2);
    join_all();
    free(counter);                  // main is the owner
    return 0;
}

// === __thread thread-local ===
__thread int local = 42;  // per-thread independent
```

---

## Document History

| Version | Date | Description |
|------|------|------|
| 0.1 | 2026-04-13 | Initial specification |
| 0.1 | 2026-04-17 | Updated default ownership semantics; added `clone()`; updated inline assembly syntax |
| 0.1.0 | 2026-04-18 | 0.1.0 finalized |
| 0.2.0 | 2026-08-07 | Implemented the 8 design decisions D-1 .. D-8: `unique` generalized, `&` immutable borrow, `&mut` enters the language, `move` as a builtin primitive, explicit assignment move rules, `alloc`/`free` not enforced pairing, `DanglingReference` trigger conditions, C-host-first decision. *(Note: 0.3.0 reverses D-3 and removes `&mut`; D-2's `&` semantics are superseded by the Q3 reversal.)* |
| **0.3.0** | **2026-08-07** | **This version**: complete split of the two permissions (Rule 22); added the `#modlaw` directive (Rule 23); added `mod()` / `unmod()` expressions (Rule 24); added the threading model chapter §13 (Rules 25–28); rewrote §3.3 reference types as **a single `T&`** (Q3 reversal, **removing `T&mut`**); rewrote §3.8 pointer modifiers to the Q6 custom semantics "opposite to C++"; rewrote §7.1 assignment as "implicit `mod()` + no ownership transfer" (Q4=a); rewrote §3.2 to distinguish owning vs non-owning pointers (Rule 2B); §11.5.1 added the `mutex<T>` / `atomic<T>` standard-library types (revisions #5, #6). Status: draft. |
| **0.3.1** | **2026-08-12** | **This version (lvalue / rvalue concept clarification, S2)**: added the full §4.13 "Expression classification: lvalue and rvalue" chapter (§4.13.1 definition + §4.13.2 lvalue classification table + §4.13.3 assignment-context constraints + §4.13.4 codegen implementation constraints + §4.13.5 cross-references); §4.1 precedence table gains level 2.5 unary ops (`*` `&` `+` `-` `!` `~` `mod` `unmod` prefix `++` `--`, right-to-left); §4.6 assignment-operator table's 11 rows uniformly gain the "LHS must be an lvalue (§4.13.2)" constraint + header note + associativity + lvalue context note; §7.8 reference-creation rule 1 references §4.13.2 and adds 5 valid + 4 invalid examples; §12.1 EBNF adds the `lvalue` / `rvalue` non-terminals and updates the `assignment_expression` LHS annotation; §12.3 appendix precedence table synchronously adds level 2.5. **Fixed the m0_42 deref-assign bug** (`*view = payload` from compile_failed → PASS). Introduces no new syntax, no semantic changes, fully backward compatible. Status: draft. |
| **0.3.2** | **2026-08-12** | **This version (C-style cast + function return type + §11.0 builtin signature table, S4+S5)**: added C-style cast `(T)expr` (§4.8.1) and the `cast(T, x)` builtin (§4.8.2); added §6.2.1 "Function Return Type" with the `UCExprCall.return_type` field type-parameterization mechanism; added the §11.0 "Builtin Signature Master Table" (single category, 16 rows: print/print_num/print_float + strlen/strcpy/strcmp + memcpy/memmove/memset + sizeof_impl/alignof_impl/is_null/clone_impl + move/alloc + abs_int); §12.1 EBNF adds the `cast_expression` production and `cast` builtin call. **Fixed the m0_41 abs_int link error and the second part of m0_42 deref-assign**. Status: draft. |
| **0.3.3** | **2026-08-18** | **This version (spec clarification, S1+S3+Reclassification as builtin functions)**: added §4.13.5 "deref Semantics" full sub-chapter (§4.13.5.1 basic deref + §4.13.5.2 compound forms + §4.13.5.3 relation with §3.5 + §4.13.5.4 precedence (positional overloading between `*` and `*` mul) + §4.13.5.5 error cases + §4.13.5.6 cross-references; `*` and `&` are now positional overloading between deref/addr-of prefix and mul/bitwise-AND infix); added §7.0 "Concept Clarification" full sub-chapter (§7.0.1 operators + §7.0.2 builtin functions + §7.0.3 Reclassification as builtin functions for mod/unmod/move/clone + §7.0.4 historical retrospective + §7.0.5 cross-references); §4.1 precedence table adds level 2.5 unary prefix row (right-to-left) and removes level 15 `move clone` (now builtin functions); §4.9/§4.10 mod/unmod annotations updated to call out builtin-function classification + IR marker codegen; §7.8 reference section adds cross-reference to §4.13.5; §11.0 dual classification per runtime-architecture §5: §11.0.1 builtin language primitives (6 entries: alloc/free/move/clone/mod/unmod, all codegen-emit); §11.0.2 UltraCPP stdlib paths (`lib/print.uc` / `lib/string.uc` / `lib/memory.uc` / `lib/math.uc` / `lib/alloc.uc` / `lib/sync.uc` / `lib/sys.uc`, replacing the 0.3.2 14-row stdlib table — stdlib no longer backed by libc); §11.0.3 codegen integration: `BUILTIN_SIGS[]` simplified to 6 entries (mod/unmod promoted from intrinsic to builtin emit IR marker `@uc_borrow_mod_{enter,exit}`; `free` promoted from FFI to builtin emit `call @free` + mark freed); §10 FFI gains 4 new sub-sections (§10.2 C-style `@ifdef` preprocessor / §10.3 GCC-style `asm { ... }` inline assembly / §10.4 `sys::` syscall namespace / §10.5 cross-references + errno mapping); §10.2/§10.3/§10.4 inserted; §10.6 (was 10.2 C Type Mapping) / §10.7 (was 10.3 unsafe Block) / §10.8 (was 10.4 Inline Assembly legacy) renumbered. **This is spec clarification only — no bug fixes, no new syntax; mod/unmod/move/clone call syntax unchanged.** Status: draft. |

---

*UltraCPP 0.3.3 Language Specification (draft)*
