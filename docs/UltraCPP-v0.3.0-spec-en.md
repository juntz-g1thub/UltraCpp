# UltraCPP 0.3.0 Language Specification

> **Version**: 0.3.0
>
> **Previous version**: 0.2.0 — [`UltraCPP-v0.2.0-spec-en.md`](./UltraCPP-v0.2.0-spec-en.md)
>
> **Status**: draft
>
> **Date**: 2026-08-07
>
> Same-version Chinese translation: [简体中文](./UltraCPP-v0.3.0-spec-zh-CN.md)

---

## Revision Summary

This version applies **21+ language design decisions** on top of 0.2.0 (the historical D-1..D-8 plus the new Rule 1, Rule 2A/2B, Q1..Q6, and Rule 22..28). The two pivotal changes are **splitting two permissions into independent concepts** (Rule 22) and the new **`#modlaw` directive** (Rule 23).

| Topic | Decision | One-line summary |
|-------|----------|------------------|
| Permission split | Rule 22 | Decouple ownership from modification rights |
| `#modlaw` policy | Rule 23 | Set per-module `mod()` policy |
| `mod()` / `unmod()` | Rule 24 | Acquire / release modification rights explicitly |
| `shared` annotation | Rule 25 | Force cross-thread visibility annotation |
| `move_to_thread()` | Rule 26 | Cross-thread explicit ownership transfer |
| `__thread` TLS | Rule 27 | GCC-style thread-local storage |
| Four storage classes | Rule 28 | Stack / heap / global / TLS threading rules |
| `unique T ≡ T*` | Q1 | Generic type constructor (unchanged) |
| Single `T&` ref | Q2 / Q3 reversal | Remove `T&mut` (0.2.0 reversed) |
| Assignment `p1=p2` | Q4 | Implicit `mod(p1)`, not ownership transfer |
| `move(p)` only path | Q5 | Explicit ownership transfer (unchanged) |
| `const T*` / `T* const` | Q6 | Custom semantics, opposite of C++ |

---

## Changelog

| Decision | Section | Description |
|----------|---------|-------------|
| Rule 1 | §3.2 | Owning type is not limited to heap/stack; storage location is inferred from the initializer expression. |
| Rule 2A | §3.3 | `&` references cannot own; compile-time reports null ref; per-thread scope isolation. |
| Rule 2B | §3.2 | `T*` is distinguished as owning vs. non-owning by its initializer; assignment `p1 = p2` does **not** copy ownership but **grants p1 modification right** (implicit `mod(p1)`). |
| Q1 | §3.7 | `unique T` ≡ `T*`; `unique` may be omitted. |
| Q2 | §3.3, §12.1 | Reference declaration RHS is an lvalue; pointer declaration RHS is address-of. |
| Q3 | §3.3 | **Single reference type** `T&`: all references unified to `T&`; exclusivity is controlled by `#modlaw exclusive` + `mod()` request, not by type-level distinction. |
| Q4 | §3.2, §7.1 | Assignment `p1 = p2` **does not copy ownership**, but **gives p1 modification right** (implicit `mod(p1)`). |
| Q5 | §7.4 | `move(p)` remains necessary; explicit ownership transfer. |
| Q6 | §3.8, §7.10 | `const T*` / `T* const` custom semantics (opposite of C++); owning heap forbidden. |
| **Rule 22** | §1.2, §3, §7 | Ownership and modification right fully split. |
| **Q3 reversal** *(0.3.0 revision)* | §3.3, §7.8, §7.9 (removed), §12.1, §12.2, §2.3, §2.6 | **Remove `T&mut` type**: references unified to `T&`; exclusivity is controlled by `#modlaw exclusive` + `mod()` request, no longer at the type level. |
| **Rule 23** | §9.9 | New `#modlaw <perm> <scope>` directive (6 legal combinations). |
| **Rule 24** | §4.9, §4.10 | New `mod(ref_expr)` and `unmod(ref_expr)` expressions. |
| **Rule 25** | §13.1, §13.3 | Cross-thread access requires `shared` annotation; mutex is compiler-auto + programmer-explicit. |
| **Rule 26** | §13.2 | `move_to_thread(p, tid)` / `spawn_thread_with(tid, p)`. |
| **Rule 27** | §13.5 | `__thread` thread-local storage. |
| **Rule 28** | §13.4 | Multi-thread memory layout: stack (thread-local) / heap (shared) / global (shared) / TLS (thread-local). |

---

## 1. Overview

### 1.1 Language Goals

UltraCPP is a systems-level programming language that combines **C++ syntax familiarity** with **Rust-style memory safety guarantees**, without requiring a garbage collector.

**Core goals:**

- Zero-cost abstractions
- Compile-time memory safety
- C++ migration compatibility
- No garbage collector

### 1.2 Design Principles (key point: two permissions are independent)

1. **Safety by default**: enforce memory safety at compile time wherever possible
2. **Explicit over implicit**: ownership, mutability, and safety semantics are visible in syntax
3. **Pragmatic compatibility**: leverage C++ developer familiarity
4. **Minimal runtime**: no heavy runtime, suitable for systems programming
5. **Two permissions are independent** *(0.3.0 new: Rule 22)*:
   - **Ownership**: decides who is responsible for `delete` / `free`; **single owner**; transferable via `move(p)`; non-copyable.
   - **Modification right (`mod`)**: decides who may write the value through the reference; may be shared, exclusive, or zero; requested by the programmer as needed.

> **[0.3.0 · Rule 22]** This is the most consequential conceptual change in 0.3.0. In 0.2.0, the "ownership" concept conflated modification rights with ownership; 0.3.0 separates the two. Example: `int& r = x;` (shared reference) by default does **not** carry modification rights (C++-style reference); to modify `x`, you must additionally call `mod(r)` to request modification rights (see §3.3, §4.9 for the latently-writable reference semantics).

### 1.3 Symbol Conventions

| Symbol | Meaning |
|--------|---------|
| `T` | owned value (stack / global / TLS) |
| `unique T` ≡ `T*` | owning pointer (generic form, see §3.7) |
| `T*` | pointer: RHS `&b` → non-owning stack/global pointer; RHS `alloc(T)` → owning pointer |
| `T&` | **single reference type**: must be initialized, non-null, non-owning; writable via `mod()`; multiple may coexist; thread-shareable |
| `T* p = &x` | p is a non-owning stack/global pointer (similar rules to references) |
| `T* p = alloc(T)` | p is an owning pointer |
| `&expr` | create a `T&` reference to `expr` |
| `mod(ref_expr)` | request modification rights (per `#modlaw` policy) |
| `unmod(ref_expr)` | release modification rights (usually omitted, scoped) |
| `move(p)` | transfer ownership (p becomes invalid) |
| `move_to_thread(p, tid)` | cross-thread ownership transfer |
| `shared` | cross-thread visibility annotation (compile-time enforced) |
| `__thread T x` | thread-local storage (per-thread independent) |
| `#modlaw <perm> <scope>` | policy directive (6 legal combinations) |
| `const T*` | **pointer-locked** (cannot rebind) + lifetime-safe + write-through-it forbidden (custom, **opposite of C++**) |
| `T* const` | **read-only data view** (rebindable, but write-through-it forbidden) |
| `alloc(T)` | allocate heap memory for type T |
| `free(ptr)` | free heap memory |
| `clone(ptr)` | clone pointer (independent ownership) |

---

## 2. Lexical Structure

### 2.1 Source File Conventions

```
*.uc   — UltraCPP source file (recommended)
*.upp  — UltraCPP source file (legacy)
```

### 2.2 Token Types

| Category | Examples |
|----------|----------|
| **Keywords** | `if`, `else`, `while`, `for`, `return`, `struct`, `export`, `import`, `const`, `unique`, `move`, `free`, `alloc`, `null`, `true`, `false`, `void`, `extern`, `unsafe`, `typedef`, `clone`, `mod`, `unmod`, `shared`, `__thread`, `move_to_thread` |
| **Identifiers** | `foo`, `myVariable`, `_private`, `CamelCase` |
| **Literals** | `42`, `3.14`, `'x'`, `"hello"`, `true`, `false` |
| **Operators** | `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `\|\|`, `!`, `&`, `\|`, `^`, `~`, `<<`, `>>`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `\|=`, `^=`, `<<=`, `>>=` |
| **Delimiters** | `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `::` |
| **Preprocessor** | `#import`, `#include`, `#ifdef`, `#ifndef`, `#endif`, `#define`, `#modlaw` *(0.3.0 new)* |

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

> **[0.3.0 new]** `mod`, `unmod`, `shared`, `__thread`, `move_to_thread`. See §4.9, §4.10, §13.

### 2.4 Identifier Rules

```
identifier ::= letter (letter | digit)*
letter     ::= 'a'..'z' | 'A'..'Z' | '_'
digit      ::= '0'..'9'
```

- Identifiers are case-sensitive
- No length limit (implementation-defined)
- Must not collide with keywords

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

### 2.6 Operators and Delimiters

| Operator | Description |
|----------|-------------|
| `+` | addition |
| `-` | subtraction / negation |
| `*` | multiplication / dereference |
| `/` | division |
| `%` | modulo |
| `=` | assignment (between `T*` triggers **modification-right transfer** rather than ownership transfer, see §7.1) |
| `==` | equality |
| `!=` | inequality |
| `<` | less than |
| `>` | greater than |
| `<=` | less than or equal |
| `>=` | greater than or equal |
| `&&` | logical AND |
| `\|\|` | logical OR |
| `!` | logical NOT |
| `&` | unary prefix: create `T&` reference (see §3.3); binary infix: bitwise AND |
| `\|` | bitwise OR |
| `^` | bitwise XOR |
| `~` | bitwise NOT |
| `<<` | left shift |
| `>>` | right shift |
| `++` | increment |
| `--` | decrement |
| `->` | arrow (pointer member access) |
| `.` | dot (member access) |
| `::` | scope resolution |

> **[0.3.0 revised]** Unary prefix `&` creates the single reference type `T&`. Whether it is actually writable is decided by `mod()` / `#modlaw` (see Rule 24). 0.3.0 no longer distinguishes "shared-writable" vs. "exclusive-writable" references — `T&mut` has been removed.

### 2.7 Comments

```cpp
// single-line comment

/*
 * multi-line comment
 */
```

### 2.8 Whitespace

Spaces, tabs, and newlines are ignored when separating tokens. Line endings are preserved for error reporting.

---

## 3. Type System *(0.3.0 rewritten)*

### 3.1 Primitive Types

| Type | Description | Size |
|------|-------------|------|
| `void` | no value | - |
| `bool` | boolean | 1 byte |
| `char` | character | 1 byte |
| `int` | signed integer | 4 bytes |
| `i8` | 8-bit signed | 1 byte |
| `i16` | 16-bit signed | 2 bytes |
| `i32` | 32-bit signed | 4 bytes |
| `i64` | 64-bit signed | 8 bytes |
| `uint` | unsigned integer | 4 bytes |
| `u8` | 8-bit unsigned | 1 byte |
| `u16` | 16-bit unsigned | 2 bytes |
| `u32` | 32-bit unsigned | 4 bytes |
| `u64` | 64-bit unsigned | 8 bytes |
| `f32` | 32-bit float | 4 bytes |
| `f64` | 64-bit double | 8 bytes |
| `usize` | unsigned size | platform-dependent |
| `isize` | signed size | platform-dependent |

### 3.2 Pointer Types *(0.3.0 rewritten: Rule 2B, Q4)*

**`T*` is distinguished as owning vs. non-owning by its initializer expression** (Rule 2B). This is the core 0.3.0-vs-0.2.0 difference in pointer semantics.

```cpp
T*              // pointer: RHS &x → non-owning stack/global pointer; RHS alloc(T) → owning heap pointer
T*              // at any moment a single owner; assignment p1 = p2 does NOT copy ownership, but gives p1 modification rights (implicit mod(p1))
```

**Example (must distinguish the two forms):**

```cpp
int x = 42;

// === Non-owning stack pointer: RHS is address-of ===
int* p = &x;        // p points to x; p itself does NOT own x
*p;                 // ✅ read
// *p = 100;        // ❌ compile error: p has no default mod-right (requires mod(p), see §4.9)
int* p2 = &x;       // ✅ multiple non-owning pointers may coexist

// === Owning heap pointer: RHS is alloc ===
int* h = alloc(int);   // h owns
*h = 100;              // ✅ h has modification rights by default (owning pointer "mod-by-default")
int* h2 = alloc(int);
int* h3 = h2;          // ⚠️ implicit mod(h3): h3 gets modification rights, h2 still owns
                      //    ownership is NOT transferred; h3 does not own, so free(h3) is illegal
free(h2);              // ✅ h2 is the owner
```

**Example A (canonical declaration syntax, Q2):**

```cpp
int x = 42;

// ✓ Reference declaration: RHS is lvalue
int& r = x;       // r is a reference to x (latently writable, C++-style)

// ✓ Pointer declaration: RHS is address-of
int* p = &x;      // p is the address of x (non-owning)

// ✗ Type mismatch (must appear as counter-example)
// int& r2 = &x;   // error: T& cannot take T*
// int* p2 = x;    // error: T* cannot take T
```

**Key rules (Rule 2B):**

1. **Decided by initializer expression**: `T* p = &x` → p does not own x; `T* p = alloc(T)` → p owns.
2. **Single owner at any moment**: an allocated object has at most one owning pointer (`unique T` is equivalent to `T*`).
3. **Assignment `p1 = p2` does NOT copy ownership**, **but gives p1 modification rights** (route 2: implicit `mod(p1)`); owner is unchanged (Q4).
4. **`move(p)` explicitly** transfers ownership (Q5).
5. **`readonly` / `const T*` / `T* const` control read-only rights** — see §3.8 and §7.10 (**semantics opposite of C++**).

### 3.3 Reference Types *(0.3.0 rewritten: remove T&mut, Q2, Rule 22)*

UltraCPP has a single reference type `T&` — **must be initialized**, **cannot be null**, **does not own** the referent; ownership always remains with the owner.

> **[0.3.0 · Rule 22 + Q3 reversal]** 0.2.0 split references into `T&` (immutable) and `T&mut` (exclusive mutable). 0.3.0 **removes `T&mut`** and unifies into a single reference type `T&`. **Exclusivity** is no longer distinguished at the type level; it is controlled by the `#modlaw exclusive` policy + `mod()` request. Whether actual writing is allowed is also decided by `mod()` and `#modlaw`.

```cpp
T&     // latently-writable reference: no mod by default; requires mod() to write
```

**Declaration syntax (Q2):**

```cpp
int x = 42;
int& r = x;          // reference declaration: RHS is lvalue (no & written)
int& r2 = x;         // ✅ multiple T& may coexist
// int& r3 = &x;      // ❌ error: T& cannot take T*
// int& r4 = 42;      // ❌ error: RHS of T& must be lvalue
```

**Writability (Rule 24):**

```cpp
int x = 42;
int& r = x;          // r is a reference to x; no mod by default
int v = r;           // ✅ read
// r = 100;          // ❌ compile error: mod-right not requested
mod(r);              // ✅ request modification rights (per #modlaw)
*r = 100;            // ✅ write
```

**Comparison table (0.1.0 / 0.2.0 / 0.3.0):**

| Version | Reference Type | Exclusivity Mechanism | Write Permission |
|---------|----------------|------------------------|------------------|
| 0.1.0 | T& (C++-style mutable reference) | none | direct write |
| 0.2.0 | T& (immutable) + T&mut (exclusive mutable) | at type level | T& read-only; T&mut exclusive write |
| 0.3.0 | **T& only** | `#modlaw exclusive` + `mod()` | requires mod(); behavior decided by #modlaw |

### 3.4 Function Pointer Types

```cpp
int (*)(int, int)       // function type: takes two ints, returns int (C++ style)
void (*)(const char*)   // function pointer returning void
```

**Example:**
```cpp
typedef int (*Comparator)(int, int);
typedef void (*Callback)(const char*);

Comparator cmp;
Callback cb;
```

### 3.5 Array Types

```cpp
T[n]  // fixed-length array of n elements of type T
```

**Example:**
```cpp
int[10] arr;           // 10 ints
char[256] buffer;      // 256-byte buffer
int[3] nums = [1, 2, 3];  // initialize array
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

### 3.7 Type Modifiers (`unique T` ≡ `T*`) *(Q1, inherited from 0.2.0)*

| Modifier | Meaning |
|----------|---------|
| `const` | value cannot be modified (combined with `#modlaw`, custom semantics — see §3.8) |
| `unique` | **generic type constructor**: `unique T` ≡ `T*` (Q1, may be omitted) |
| `static` | internal linkage |
| `shared` | cross-thread visibility annotation (compile-time enforced, else compile error — see §13.1) |

```cpp
unique int      x1;   // ≡ int*
unique char     x2;   // ≡ char*
unique Point    x3;   // user-defined struct also legal
unique int*     x4;   // T itself is a pointer type — also legal
unique int[10]  x5;   // T is an array type
```

**Rules (inherited from 0.2.0 + Q1):**

1. The `T` in `unique T` may be **any type** from §3.1–§3.6.
2. `unique T` and `T*` are **equivalent** in the type system; `unique` may be omitted.
3. `unique` cannot apply to borrowed types: `unique T&` is **illegal** — borrows do not own values.
4. `unique` **cannot** be written as `shared unique T*` — `shared` is a runtime storage class (sibling to `__thread`), not a type constructor.

### 3.8 Pointer Modifiers and Ownership Boundaries (Q6 custom `const T*` / `T* const`) *(0.3.0 new)*

> **[0.3.0 · Q6 custom semantics, opposite of C++]** This section introduces new pointer modifier rules. 0.2.0 inherited C++'s `const T*` / `T* const` meanings; 0.3.0 **rewrites** them as custom semantics.

| Declaration | Meaning | Rebindable | Write-through | Owning heap allowed |
|-------------|---------|------------|---------------|---------------------|
| `const T*` | **pointer-locked**: pointer itself **cannot rebind** + lifetime-safe + write-through forbidden | ❌ | ❌ | ❌ (owning heap forbidden) |
| `T* const` | **read-only data view**: pointer **may rebind**, write-through forbidden | ✅ | ❌ | ❌ (owning heap forbidden) |
| `const T* const` | double-locked (neither rebind nor write) | ❌ | ❌ | ❌ |

> **Opposite of C++:**
>
> | Type | C++ Meaning | UltraCPP Meaning |
> |------|-------------|------------------|
> | `const T*` | data cannot change (pointer may change) | **pointer cannot change** (data also write-forbidden) |
> | `T* const` | pointer cannot change (data may change) | **data write-forbidden** (pointer may change) |

**Example D (Q6 custom semantics):**

```cpp
int x = 42;

// === const T*: pointer-locked + lifetime + non-writable ===
const int* p = &x;
*p;                             // ✅ read
// *p = 100;                    // ❌ write forbidden
// p = &y;                      // ❌ pointer-locked, cannot rebind

// === T* const: read-only data view ===
T* const r = &x;
*r;                             // ✅ read
// *r = 100;                    // ❌ data read-only
r = &y;                         // ✅ may rebind

// === Owning heap forbidden ===
unique int* h = alloc(int);
const int* bad = h;             // ❌ error: cannot create read-only pointer to owning heap
T* const bad2 = h;              // ❌ same prohibition
```

**Why is owning heap forbidden?** An owning pointer may later be `move()`-ed or `free()`-d; at that point any `const T*` / `T* const` view would **dangle**. The release timing of an owning pointer is controlled by the programmer, so a "read-only view" of an owning heap object cannot be guaranteed safe at compile time.

For further detail (diagnostics on violation, dereference semantics), see §7.10.

---

## 4. Expressions *(0.3.0 new: §4.9, §4.10)*

### 4.1 Operator Precedence and Associativity

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| 1 | `::` | left to right |
| 2 | `()` `[]` `.` `->` `++` `--` | left to right |
| 3 | `*` `/` `%` | left to right |
| 4 | `+` `-` | left to right |
| 5 | `<<` `>>` | left to right |
| 6 | `<` `>` `<=` `>=` | left to right |
| 7 | `==` `!=` | left to right |
| 8 | `&` | left to right |
| 9 | `^` | left to right |
| 10 | `\|` | left to right |
| 11 | `&&` | left to right |
| 12 | `\|\|` | left to right |
| 13 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | right to left |
| 14 | `?:` (ternary conditional) | right to left |
| 15 | `move` `clone` | - |

> **[0.3.0]** The level-8 `&` in the table refers to **binary infix** bitwise AND. Unary-prefix `&` (reference) and `mod` / `unmod` (modification rights) are unary operators — see §4.9–§4.10.

### 4.2 Arithmetic Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `+` | addition | `a + b` |
| `-` | subtraction | `a - b` |
| `*` | multiplication | `a * b` |
| `/` | division | `a / b` |
| `%` | modulo | `a % b` |

### 4.3 Comparison Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `==` | equality | `a == b` |
| `!=` | inequality | `a != b` |
| `<` | less than | `a < b` |
| `>` | greater than | `a > b` |
| `<=` | less than or equal | `a <= b` |
| `>=` | greater than or equal | `a >= b` |

### 4.4 Logical Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `&&` | logical AND | `a && b` |
| `\|\|` | logical OR | `a \|\| b` |
| `!` | logical NOT | `!a` |

### 4.5 Bitwise Operators

> **[0.3.0]** This table describes the **binary infix** forms only. Unary-prefix `&` is not a bitwise operator; it is reference (see §3.3).

| Operator | Description | Example |
|----------|-------------|---------|
| `&` | bitwise AND (binary infix) | `a & b` |
| `\|` | bitwise OR | `a \| b` |
| `^` | bitwise XOR | `a ^ b` |
| `~` | bitwise NOT | `~a` |
| `<<` | left shift | `a << 2` |
| `>>` | right shift | `a >> 2` |

### 4.6 Assignment Operators

| Operator | Description |
|----------|-------------|
| `=` | simple assignment (between `T*` triggers **implicit `mod()`**, does not transfer ownership — see §7.1) |
| `+=` | add-assign |
| `-=` | sub-assign |
| `*=` | mul-assign |
| `/=` | div-assign |
| `%=` | modulo-assign |
| `&=` | bitwise-AND-assign |
| `\|=` | bitwise-OR-assign |
| `^=` | bitwise-XOR-assign |
| `<<=` | left-shift-assign |
| `>>=` | right-shift-assign |

### 4.7 Conditional Operator

```cpp
condition ? expr1 : expr2
```

### 4.8 Parenthesized Expression

```cpp
(expr)  // any expression in parentheses
```

### 4.9 `mod()` Expression — Request Modification Rights *(0.3.0 new: Rule 24)*

> **[0.3.0 · Rule 24]** `mod(ref_expr)` requests modification rights on the object referred to by a reference. Its behavior is determined by the current `#modlaw` policy in scope.

**Syntax**:
```
mod '(' reference_expression ')'
```

**Operand**: `reference_expression` must be an lvalue of reference type `T&` (i.e. `int&`, `Point&`, etc.). Note: 0.3.0 has removed `T&mut`, so `mod()` only accepts `T&` references.

**Semantics (Rule 24)**: behavior is decided by the current `#modlaw` policy.

| `#modlaw` Policy | Behavior of `mod(r)` |
|------------------|-----------------------|
| `none` | ❌ compile-time error: `mod() forbidden by #modlaw none policy` |
| `exclusive` | ✅ compile-time conflict check; if another `T&` already has mod, exclusivity is denied (`BorrowConflict`) |
| `shared` | ✅ multiple `T&` may simultaneously hold modification rights (programmer is responsible for data races) |

**Example snippet:**

```cpp
#modlaw shared module

int x = 42;
int& r = x;
mod(r);              // ✅ allowed under shared policy
*r = 100;
mod(r);              // may repeat in scope (compiler auto-unmods)
```

**Implicit `mod()` (Rule 24 + Q4):**

The `T*` form of assignment `p1 = p2` **automatically** generates `mod(p1)`, see §7.1. So the following holds implicitly:

```cpp
int* p1 = alloc(int);
int* p2 = p1;          // implicit mod(p1): p1 gets modification rights; p2 still owns
*p1 = 100;             // ✅ no explicit mod() needed
```

### 4.10 `unmod()` Expression — Release Modification Rights *(0.3.0 new: Rule 24)*

`unmod(ref_expr)` explicitly releases modification rights. **Usually omitted** — the compiler auto-unmods at scope end or at last use.

**Syntax**:
```
unmod '(' reference_expression ')'
```

**Use case** (when omitted, it degenerates):

```cpp
mod(r);                  // request
*r = compute();          // use
unmod(r);                // ✅ release immediately, shortening critical section (may be omitted)

// The following two blocks are equivalent:
#modlaw exclusive module
{
    mod(m);
    *m = 100;            // m's last use → compiler auto-unmods m
}
*m2 = 200;               // unlocked; m2 may request
```

---

## 5. Statements

### 5.1 Expression Statements

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
return;              // return void
return 42;           // return value
return x + y;        // return expression result
```

### 5.7 break Statement

```cpp
while (true) {
    if (condition) {
        break;       // exit loop
    }
}
```

### 5.8 continue Statement

```cpp
for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;    // skip iteration
    }
}
```

### 5.9 free Statement

```cpp
int* p = alloc(int);
*p = 42;
free(p);  // release memory
```

### 5.10 Declaration Statements

```cpp
int x;                // variable declaration
int x = 42;           // declaration with initializer
const int y = 100;    // constant
int* p = alloc(int);  // owning pointer allocation
int* p2 = &x;         // non-owning pointer (RHS &x)
int& r = x;           // single reference type T& (RHS is lvalue)
```

---

## 6. Functions

### 6.1 Function Definition

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

### 6.2 Function Call

```cpp
int result = add(10, 20);
greet("Hello");
int fact = factorial(5);
```

### 6.3 Parameter Passing *(0.3.0 rewritten: single T& + mod())*

**Pass-by-value semantics:**

- The parameter is copied into the function's formal parameter
- Modifying the formal parameter does not affect the caller

```cpp
void inc(int x) {
    x = x + 1;  // does not affect caller
}

int n = 10;
inc(n);
// n is still 10
```

**Pass-by-reference for modification (0.3.0 form — the only form):**

```cpp
// === 0.3.0 form (the only one): request via mod() ===
void inc(int& x) {
    mod(x);
    *&x = *&x + 1;  // or x = x + 1 (depending on concrete syntax)
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

> **[0.3.0 · Q3 reversal, Rule 22, Rule 24]** 0.2.0 provided two parameter forms: `void inc(int&mut x)` (exclusive mod-by-default; call site `inc(&mut n)`) and `void inc(int& x)` (read-only). 0.3.0 **removes `T&mut`** and unifies to a single reference type `T&` parameter: writing requires `mod(x)` first, and `#modlaw` decides whether multiple mods may coexist. Exclusivity is achieved through `#modlaw exclusive` + `mod()`, no longer distinguished at the type level.

### 6.4 Return Values

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

> **[0.3.0 inherited from 0.2.0 · D-7]** Returning a local variable **by value** (e.g. `return p;` above) is always legal. Returning a **borrow** that points to a local variable is a dangling reference — see §7.9.

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

### 7.1 Ownership Semantics (**Two Permissions Split**) *(0.3.0 rewritten: Rule 22, Q4, Q5)*

> **[0.3.0 · Rule 22]** 0.3.0 splits 0.2.0's "ownership" into two independent permissions:
>
> | Permission | Decides | Default | Transfer |
> |------------|---------|---------|----------|
> | **Ownership** | who delete / free | owned by `T* p = alloc(T)`; not owned by `T* p = &x` | explicit `move(p)` |
> | **Modification right** | who may modify the value | owning pointer has it by default; `T&` has none by default (requires `mod()`) | implicit `mod()` (per `#modlaw`) |

**Pointer and Ownership (Rule 2B, Q4):**

- **Single owner at any moment**: an allocated object has at most one owning pointer.
- **`T* p = alloc(T)`**: p holds **both ownership** and **modification right** of the object.
- **`T* p = &x`**: p only holds the address of x, **does not own** x, and does **not** by default have modification rights on x.
- **Assignment `p1 = p2` does NOT copy ownership** (Q4): the source `p2` still owns; the destination `p1` receives **modification right** (implicit `mod(p1)`).

#### 7.1.1 Assignment and Modification Rights (**0.3.0 rewrite of 0.2.0 §7.1.1**)

**Example C (ownership transfer vs. modification rights):**

```cpp
unique int* p1 = alloc(int);  // p1 owns
*p1 = 100;

unique int* p2 = p1;           // implicit mod(p2): p2 gets modification right; p1 still owns
*p2 = 200;                      // ✅
// free(p2);                    // ❌ p2 does not own

move(p2);                       // explicit transfer
// p1 has been moved-out
free(p2);                       // ✅ now p2 owns
```

**Rules (key 0.3.0 vs. 0.2.0 difference):**

> Let the assignment be `dst = src`, where `dst` and `src` both have type `unique T` (equivalent to `T*`).
>
> 0.3.0 behavior:
> 1. `dst` receives **modification right** (implicit `mod(dst)`; whether multiple mods or exclusive depends on the current `#modlaw`).
> 2. `src`'s **ownership is unchanged** — it still owns; it only transfers ownership on explicit `move(src)`.
> 3. `dst`'s ownership state: does not own. So `free(dst)` while `dst` does not own is a compile-time error.

> 0.2.0 behavior (D-5): `src` becomes null, `dst` takes over ownership.
>
> 0.3.0 **rewrites** to: `src` unchanged, `dst` receives modification right. `move(p)` remains the only explicit path for ownership transfer (Q5).

**Where ownership transfer (`move()`) occurs** (inherited from 0.2.0, Q5):

| Location | Behavior | Source | Destination |
|----------|----------|--------|-------------|
| explicit `move(p)` | transfer ownership | `p` | `p2` (receiver) |
| function argument `f(p)` (parameter is `unique T`) | transfer ownership | `p` | parameter |
| function return `return p;` (return type `unique T`) | transfer ownership | `p` | caller receiving slot |

**Where modification right is granted** (Q4):

| Location | Behavior | Source | Destination |
|----------|----------|--------|-------------|
| assignment `dst = src` (type is `unique T`) | implicit `mod(dst)` | — | `dst` |
| explicit `mod(ref_expr)` (§4.9) | request per `#modlaw` | — | ref_expr |

**Where neither transfer occurs**: `clone(s)`, `&s`, `s == null` and other read-only comparisons.

### 7.2 alloc Function

```cpp
int* alloc(int)                  // allocate a single element
int* alloc(int, int count)      // allocate an array of count elements
```

**Example:**
```cpp
int* p = alloc(int);         // single int
int* arr = alloc(int, 10);    // 10 ints
char* buf = alloc(char, 256); // 256-byte buffer
```

### 7.3 free Function

```cpp
free(ptr)    // release memory
```

**Example:**
```cpp
int* p = alloc(int);
*p = 42;
free(p);

int* arr = alloc(int, 10);
free(arr);
```

> **[0.3.0 inherited from 0.2.0 · D-6]** Pairing of `alloc` and `free` is the programmer's responsibility; the compiler **does not enforce** it.

### 7.4 move(p) Expression *(inherited from 0.2.0 · D-4, Q5)*

> **[0.3.0 · D-4, Q5]** `move(p)` is a language **builtin primitive**, **not** syntactic sugar, and **not** an ordinary function. It is the **only explicit path for ownership transfer**.

**Syntax** (see §12.1 `unary_expression`):
```
move '(' expression ')'
```

**Semantics:**

1. The operand must be an addressable lvalue with type `unique T` (equivalent to `T*`).
2. The **result** of `move(p)` is the pointer value previously held by `p`, of type `unique T`.
3. **Runtime effect**: after evaluating `move(p)`, the compiler must emit code that writes `null` into `p`'s storage cell.
4. **Static effect**: `p` is marked as moved-out; any subsequent use of `p` is a compile-time error `UseAfterMove`.
5. `move` does not allocate, and does not copy the pointed-to object; it only transfers ownership.
6. Relationship between `move(p)` and implicit move: under 0.3.0, implicit assignment does **not** trigger move; `move(p)` is the only explicit means.

```cpp
unique int p1 = alloc(int);
*p1 = 42;

unique int p2 = move(p1);   // p1 becomes null at runtime, statically marked moved-out

// int v = *p1;             // ❌ compile-time error UseAfterMove
int v = *p2;                // ✅ v == 42
free(p2);                   // ✅
```

### 7.5 clone Function

`clone()` copies a pointer, yielding two independent ownership shares.

```cpp
int* p1 = alloc(int);
int* p2 = clone(p1);  // p1 and p2 are independent; each can be freed
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
ptr[5] = ptr[0];    // copy value

int* p1 = alloc(int, 10);
int* p2 = p1 + 5;   // offset pointer
```

### 7.8 Reference `&T` (rewriting 0.2.0 §7.8) *(0.3.0 rewritten: Rule 22, Rule 24, remove T&mut)*

> **[0.3.0 rewritten]** 0.3.0 removes `T&mut`. `T&` is the only reference type; whether writing is allowed is decided by `mod()` + `#modlaw`.

**Syntax** (see §12.1):
```
'&' unary_expression
```

`&expr` creates a reference to the object denoted by `expr`; the result type is `T&`.

**Creation rules:**

1. The operand must be an lvalue.
2. The owner must be live and initialized.
3. The reference must not outlive the owner (violation reports `DanglingReference` — see §7.9).
4. `&` does not change the owner's ownership state.

**Writability (Rule 24):**

5. Writing requires `mod()` first. `r = v;` requires `mod(r);` to be issued first; otherwise it is a compile-time error.
6. Multiple `T&` may coexist.
7. Exclusivity is controlled by `#modlaw exclusive` (compile-time check of `mod()` conflicts).

```cpp
#modlaw shared module

int x = 42;
int& r1 = x;
int& r2 = x;        // ✅ multiple references coexist
int v = r1 + r2;    // ✅ read
// r1 = 100;        // ❌ compile error: no mod right
mod(r1);            // ✅ request modification right (behavior decided by #modlaw)
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
// mod(r);         // ❌ compile error: forbidden by none policy
r;                  // ✅ read-only
// *r = 100;       // ❌ no mod right
```

### 7.9 Dangling Reference *(0.3.0 inherited from 0.2.0 · D-7)*

Definition and trigger conditions are the same as 0.2.0 §7.10; this section is preserved for reference.

**Definition**: A `T&` borrow is **dangling** if it remains reachable after the owner's storage has been destroyed.

**Trigger conditions** (inherited from 0.2.0):

- **(a)** The owner borrowed by `&T` has gone out of scope.
- **(b)** A function returns a reference but the owner is a local variable of that function.

**Example (a)** (inherited from 0.2.0):

```cpp
int main() {
    int& r;
    {
        int x = 42;
        r = x;              // borrow x (0.3.0 syntax: RHS is lvalue, no &x)
    }                       // ← x goes out of scope
    return r;               // ❌ DanglingReference
}
```

**Positive example** (inherited from 0.2.0):

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

### 7.10 Custom Semantics for `const T*` / `T* const` (Q6) *(0.3.0 new)*

> **[0.3.0 · Q6 custom semantics, opposite of C++]** See §3.8 for the conceptual overview; this section gives the deeper semantic rules.

#### 7.10.1 `const T*`

- **Pointer-locked**: after declaring `const T* p = &x;`, p **cannot be reassigned** (`p = &y;` is a compile-time error).
- **Read-only access**: writing through `p` is forbidden (`*p = v;` is a compile-time error).
- **Lifetime safety**: the compiler verifies that the pointed-to object's lifetime is at least as long as the borrow's use range.
- **Not allowed to point at owning heap variables** (an owning heap object may later be moved/freed, causing dangling).

#### 7.10.2 `T* const`

- **Read-only data view**: writing through `r` is forbidden (`*r = v;` is a compile-time error).
- **Rebinding allowed**: `r = &y;` is legal.
- **Same prohibition against owning heap variables** — if `h` is owning, `T* const r = h;` is rejected at compile time.

#### 7.10.3 Diagnostic for Owning Heap Prohibition

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

Both `const T*` and `T* const` are **read-only** — `mod()` does not apply to them, because their design intent is "no code is allowed to write through them".

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

Structures use **sequential layout**; padding may be added for alignment:

```cpp
struct Example {
    char  c;    // 1 byte
    int   i;    // 4 bytes (possibly 3 bytes padding)
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

## 9. Modules and Preprocessor *(0.3.0 new: §9.9)*

### 9.1 Preprocessing Stage

UltraCPP runs a **preprocessing stage** before lexical analysis that handles the following directives:

| Directive | Behavior |
|-----------|----------|
| `#include "path"` | Expand file contents at the current location |
| `#import "module"` | Record module dependency (does not expand contents) |
| `#modlaw <perm> <scope>` *(0.3.0 new)* | Set the modification-right policy for the current file / module (see §9.9) |
| `export <declaration>` | Mark a function/variable as exported |

**Preprocessor output:**

1. Plain code after `#include` expansion
2. Module dependency list
3. Current `#modlaw` policy
4. List of exported symbols

### 9.2 #include Syntax and Semantics

**Source copying**: `#include` literally copies the contents of the specified file into the location of the `#include` directive.

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

- Code after `#include` directly acquires all definitions from the included file
- No extra declarations are needed to call functions in the included file
- File paths are resolved relative to the **current source file's directory**

**Search path order:**

1. Relative to the current source file's directory
2. Relative to the project root directory
3. Relative to specified include paths

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

1. Records module dependency without expanding source code
2. Imported modules need to be compiled separately
3. Calling module functions requires an `extern` declaration or `#include` of its interface

**Generated dependency file:**
```json
{
  "module": "math",
  "file": "lib/math.uc",
  "exports": ["add", "multiply"]
}
```

### 9.4 export Declaration

Marks a function or variable as **module-exported**; external code can access it via `#import`.

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
- Exported functions/variables are accessible by code that `#import`s the module

### 9.5 Module Design Rules

UltraCPP's module system follows these rules:

1. **No forward declarations needed for same-file functions** — calling a function defined later in the same file does not require a prior declaration.
2. **External module functions need import** — calling functions from other modules requires `#import` of that module.
3. **Internal functions are inaccessible externally** — non-exported functions are usable only inside the module.

**Example:**
```cpp
// lib/math.uc
export int add(int a, int b);  // exported interface

int _internal_helper(int x) {  // internal function, invisible externally
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
    add(5, 3);           // ✅ callable
    _internal_helper(1); // ❌ not callable (not exported)
    return 0;
}
```

### 9.6 Module Search Paths

| Path Type | Resolution |
|-----------|------------|
| relative path | relative to the current source file's directory |
| absolute path | relative to the project root directory |
| specified path | search path added via `-I` argument |

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

**Detection rule**: if two modules mutually `#include` each other, the compiler should report an error.

### 9.8 Compilation Pipeline

```
source code (.uc/.upp)
    │
    ▼
┌─────────────────┐
│ preprocessing   │
│  - expand #include │
│  - record #import  │
│  - parse #modlaw   │ (0.3.0 new)
│  - parse export    │
└─────────────────┘
    │
    ▼
┌─────────────────┐
│   lexical analysis   │
│   syntax analysis    │
│   code generation    │
└─────────────────┘
    │
    ▼
  LLVM IR (.ll)
    │
    ▼
  llc (assembly)
    │
    ▼
  object file (.o)
    │
    ▼
  linker (gcc/ld)
    │
    ▼
  executable
```

### 9.9 `#modlaw` Directive *(0.3.0 new: Rule 23)*

> **[0.3.0 · Rule 23]** Keyword: `#modlaw` (all lowercase, underscore). Syntax: `#modlaw <perm> <scope>`.

**Syntax**:
```
#modlaw <perm> <scope>
```

**Legal combinations (only 6):**

| `perm`     | `scope`    | Meaning |
|------------|------------|---------|
| `none`     | `global`   | globally forbid `mod()`; references read-only |
| `none`     | `module`   | forbid `mod()` in this module; references read-only |
| `exclusive`| `global`   | global `mod()` is exclusive (compile-time conflict check) |
| `exclusive`| `module`   | `mod()` is exclusive in this module (compile-time conflict check) |
| `shared`   | `global`   | global `mod()` is shared (multiple mods allowed; programmer responsible for races) |
| `shared`   | `module`   | `mod()` is shared in this module (multiple mods allowed; programmer responsible for races) |

**Illegal combinations**: the other 10 combinations are compile-time errors. For example: `#modlaw none shared`, `#modlaw exclusive none`, `#modlaw shared exclusive`, etc.

**Scope (`scope`):**

- `global`: affects the entire file and all symbols introduced by `#include`.
- `module`: affects only the symbols within the current `.uc` module; files brought in by `#include` do not inherit.
- If no `#modlaw` is specified, it is equivalent to `#modlaw shared module`.

**Position**: the `#modlaw` directive must appear at the **top** of a module (after `import` / `include` / `export`, before the first function / variable declaration). At most one `#modlaw` per module; duplicates are a compile-time error.

**Example B (three legal policies):**

```cpp
// === Shared mode (default) ===
#modlaw shared module

int x = 42;
int& r1 = x;       // shared reference
int& r2 = x;       // also shared
mod(r1);            // ✅ r1 gets shared mod
mod(r2);            // ✅ r2 also gets it (programmer responsible for data race)
*r1 = 100; *r2 = 200;

// === Exclusive mode ===
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

## 10. FFI (Foreign Function Interface)

### 10.1 extern "C" Blocks

```cpp
extern "C" {
    // C function declarations
}
```

### 10.2 C Type Mapping

| C Type | UltraCPP Type |
|--------|---------------|
| `T*` | `T*` |
| `const T*` | UltraCPP meaning **differs from C** (see §3.8 / §7.10); explicit `readonly` annotation is required when bridging at the FFI boundary |
| `void*` | `void*` |
| `int (*)(T)` | `int (*)(T)` |
| `int` | `int` |
| `double` | `double` |
| `char` | `char` |
| `char*` | `char*` |

> **[0.3.0 new note]** At the FFI boundary, C's standard `const char*` (pointer to const data) corresponds in UltraCPP to both "pointer-locked" and "data read-only". Both semantics are equivalent to C in the write-forbidden sense (neither can write), but the rebind semantics differ: if UltraCPP code holds a `const char*` originating from C, the compiler treats it as **pointer-locked** — which differs from C's `const char*` behavior (pointer may change). This is the direct consequence of 0.3.0's custom semantics in §3.8.

### 10.3 unsafe Blocks

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

> **[0.3.0 inherited from 0.2.0 · D-2, D-7]** Inside an `unsafe` block, ownership and borrow checks are **suppressed**: implicit-`mod()` checks in §7.1, borrow rules in §7.8, `DanglingReference` in §7.9 are not reported inside `unsafe`. `unsafe` does **not** change semantics — `move` still nulls the source (§7.4), `&` is still a reference (§7.8) — it merely shifts the safety obligation to the programmer.

### 10.4 Inline Assembly

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
|------------|---------|
| `r` | general-purpose register |
| `=` | output register |
| `&` | early clobber (must not be reused) |
| `m` | memory reference |

---

## 11. Standard Library Conventions

### 11.1 Basic I/O Functions

```cpp
void print_num(int n);        // print integer
void print(const char* s);    // print string
void print_float(double f);   // print float
```

### 11.2 String Functions

```cpp
int strlen(const char* s);  // string length
char* strcpy(char* dest, const char* src);
int strcmp(const char* a, const char* b);
```

### 11.3 Memory Functions

```cpp
void* memcpy(void* dest, const void* src, int n);
void* memmove(void* dest, const void* src, int n);
void* memset(void* s, int c, int n);
```

### 11.4 Utility Functions

```cpp
int sizeof_impl();       // type size
int alignof_impl();      // type alignment
bool is_null(int* ptr);  // null check
int* clone_impl(int* ptr);  // pointer clone
```

### 11.5 Thread Primitives *(0.3.0 new)*

```cpp
// --- Cross-thread ownership ---
move_to_thread(T* p, int tid);    // explicitly transfer ownership to thread tid
spawn_thread(int tid, void fn()); // start a thread without ownership transfer
spawn_thread_with(int tid, T* p); // start a thread with implicit move(p)
join_thread(int tid);             // wait for thread to finish
join_all();                       // wait for all spawned threads
current_tid();                    // current thread id

// --- Thread-local storage ---
__thread int x;                   // per-thread independent
__thread int buf[256];            // per-thread independent array

// --- Shared primitives ---
mutex<T>          mx;             // explicit mutex wrapper (stdlib type, see §11.5.1)
atomic<T>         at;             // explicit atomic wrapper (stdlib type, see §11.5.1)
shared T*         sp = ...;       // cross-thread visible (compile-time enforced)

// --- Implicit thread support ---
thread1() { ... }                 // compiler auto-generates mutex / barrier
```

#### 11.5.1 Shared Synchronization Types `mutex<T>` / `atomic<T>` *(0.3.0 new)*

> **[0.3.0 new]** `mutex<T>` and `atomic<T>` are **standard-library types** (defined in `lib/sync.uc`), **not keywords**. They provide explicit wrappers around cross-thread modification rights.

**`mutex<T>`**:

| Member | Description |
|--------|-------------|
| `lock()` | lock (block until acquired) |
| `unlock()` | unlock |
| `try_lock()` | try to lock, return `false` on failure |
| `wait()` | wait on a condition variable (paired with `notify`) |
| `notify()` / `notify_all()` | wake waiters |

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
|--------|-------------|
| `load()` | atomic load |
| `store(v)` | atomic store |
| `exchange(v)` | atomic exchange |
| `compare_exchange(expected, desired)` | CAS |

```cpp
atomic<int> flag;

void worker() {
    int v = flag.load();
    flag.store(v + 1);
}
```

**Relationship with `#modlaw`:**

- `mutex<T>` / `atomic<T>` lock / load / store operations **do not** go through the `mod()` system — they have their own synchronization primitives.
- When a variable's type is `mutex<T>`, the **compiler-auto-generated mutex is suppressed** (programmer already controls it explicitly), avoiding double-locking.
- `atomic<T>` works the same way: hardware atomic instructions already guarantee visibility, so the compiler does not need to generate additional synchronization code.

### 11.6 `alloc` / `free` Pairing Convention *(0.3.0 inherited from 0.2.0 · D-6)*

> **[0.3.0 inherited]** Pairing of `alloc(T)` and `free(p)` is the programmer's responsibility; the compiler **does not enforce** it.

**Canonical rules:**

1. Every successful `alloc(T)` produces a fresh allocation; its ownership belongs to the `unique T` variable receiving the result.
2. Every allocation **should** be freed exactly once. This is a **programmer responsibility**.
3. The compiler **does not** report: unfreed (memory leak), double-free, free of non-`alloc` result.
4. The compiler **still** reports `UseAfterMove` / `UseAfterDrop` / `DanglingReference`.

---

## 12. Appendices

### 12.1 Complete EBNF Grammar *(0.3.0 revised)*

> **[0.3.0]** Additions and changes vs. 0.2.0 (all 0.2.0 productions are preserved; mod / unmod / move_to_thread / __thread / shared are added):
> 1. Keyword list adds `mod`, `unmod`, `shared`, `__thread`, `move_to_thread`.
> 2. `unary_expression` adds `'mod' '(' expression ')'` and `'unmod' '(' expression ')'` (Rule 24).
> 3. New top-level `preprocessor_directive` production includes the `#modlaw` rule (Rule 23).
> 4. `unary_expression` adds `'move_to_thread' '(' expression ',' expression ')'` (Rule 26).
> 5. `declaration` / `variable_declaration` accept an optional storage class `('shared' | '__thread')` (Rule 25, 27).
> 6. `pointer_type` in `type` adds `const T*` and `T* const` (Q6, custom semantics opposite of C++).

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

assignment_expression
                   ::= conditional_expression
                    | unary_expression '=' assignment_expression
                    | unary_expression compound_assignment assignment_expression

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
                    | ('+' | '-' | '!' | '~' | '*' | '&') unary_expression
                    | 'sizeof' '(' type ')'
                    | 'move' '(' expression ')'
                    | 'clone' '(' expression ')'
                    | 'mod' '(' expression ')'                       // [0.3.0 Rule 24] acquire mod-right
                    | 'unmod' '(' expression ')'                     // [0.3.0 Rule 24] release mod-right
                    | 'move_to_thread' '(' expression ',' expression ')'  // [0.3.0 Rule 26]

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
```

#### 12.1.1 Parsing Note on `&` *(0.3.0 revised)*

0.3.0 **removes the `&mut` production**. `'&' unary_expression` is the only reference production, no lookahead required — `&` is always followed by a unary expression. `mut` is no longer a keyword.

#### 12.1.2 Parsing Note on `const T*` / `T* const` *(0.3.0 new: Q6)*

The position of `const` and `*` on `type` requires **1-token lookahead**:

- `const type *` → pointer-locked (pointer cannot rebind).
- `type * const` → read-only data view.

This is **opposite** to the C++ parsing rule — in UltraCPP, `const` always qualifies the token immediately to its **right** or **left**, but does **not** propagate to "the object pointed to". See §3.8 / §7.10 for the precise semantics.

### 12.2 Reserved Keywords *(0.3.0 revised)*

```
as         break      char       const      continue
else       export     extern     false      for
free       if         import     include    move
null       return     static     struct
true       typedef    unique     unsafe     void
while      clone
mod        unmod      shared     __thread   move_to_thread
```

> **[0.3.0 new]** `mod`, `unmod`, `shared`, `__thread`, `move_to_thread`. **0.3.0 removes** `mut` (was only used as part of the `&mut` token; with `T&mut` removed, no longer needed). See §4.9, §4.10, §13.

### 12.3 Operator Precedence Table *(0.3.0 revised)*

| Level | Operators | Description |
|-------|-----------|-------------|
| 1 | `::` | scope resolution |
| 2 | `()` `[]` `.` `->` `++` `--` | postfix |
| 3 | `*` `/` `%` | multiplicative |
| 4 | `+` `-` | additive |
| 5 | `<<` `>>` | shift |
| 6 | `<` `>` `<=` `>=` | relational |
| 7 | `==` `!=` | equality |
| 8 | `&` | bitwise AND (**binary infix**) |
| 9 | `^` | bitwise XOR |
| 10 | `\|` | bitwise OR |
| 11 | `&&` | logical AND |
| 12 | `\|\|` | logical OR |
| 13 | `?:` | ternary conditional |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | assignment |
| 15 | `move` `clone` `move_to_thread` *(0.3.0 new)* | ownership operations |

**Unary operators** (higher precedence than all binary operators in the table above, right-to-left associative):

| Operator | Description |
|----------|-------------|
| `+` `-` | unary plus / negation |
| `!` | logical NOT |
| `~` | bitwise NOT |
| `*` | dereference |
| `&` | **reference** (§3.3) |
| `mod` `unmod` *(0.3.0 new)* | request / release modification rights (Rule 24) |

### 12.4 Appendix X: Impact on Future Work *(0.3.0 revised)*

#### 12.4.1 Implementation Strategy (inherited from 0.2.0)

The C-host (`src-c/`) is the production compiler; this specification's semantics take it as the first landing target. For concrete algorithms, see `.dev/plans/0.1.0-devhandbook.md`:

| Specification Section | devhandbook Reference | Content |
|-----------------------|------------------------|---------|
| §3.2 / §7.1 ownership + mod split | §4.4 | dual state table (ownership bit + mod bit) |
| §7.4 `move(p)` | §4.8 `record_move` | move points |
| §7.8 references (`T&`) | §4.5, §4.7 | borrow + mod rules |
| §7.9 dangling | §4.6, §12.4 | lifetime inference |
| §9.9 `#modlaw` | §4.5 (extended) | mod policy table |
| §11.5.1 `mutex<T>` / `atomic<T>` | §4.6 (extended) | explicit synchronization types |
| §13.1 cross-thread visibility | §4.6 (extended) | `shared` forced annotation |
| §13.2 cross-thread ownership | §4.8 (extended) | `move_to_thread` implementation |

**New artifacts (0.3.0):**

1. Lexical: `mod`, `unmod`, `shared`, `__thread`, `move_to_thread` keywords and `#modlaw` directive (`src-c/src/lexer.c`).
2. Grammar: 4 new productions + new `const T*` / `T* const` parsing rule (`src-c/src/parser.c`).
3. Error kinds: `UC_ERR_MODLAW`, `UC_ERR_CROSS_THREAD_MOVE`, `UC_ERR_OWNING_HEAP_VIEW` (`src-c/include/uc_error.h`).
4. Compiler phase: ownership checker upgraded to "dual state" — ownership bit + mod bit (`src-c/src/ownership.c`).
5. Threading support: `spawn_thread`, `join_thread`, `mutex<T>`, `atomic<T>` implementation (`src-c/src/thread.c`).
6. Test corpus: `test/modlaw/{pass,fail}/`, `test/threading/{pass,fail}/`, `test/const_ptr/{pass,fail}/`.

#### 12.4.2 Items Not Landing in This Version

| Item | Status | Destination |
|------|--------|-------------|
| `MissingFree` / `DoubleFree` | not implemented | §11.6, candidate optional diagnostics |
| `Owned<T>` / `Ref<T>` / `RefMut<T>` internal types | not in user language | devhandbook §4.1 |
| precise definition of non-lexical lifetimes | only described as "until last use" | to backfill after algorithm implementation |
| concrete syntax for `__thread` at declaration position | `__thread T x;` form supported | awaiting implementation details |
| runtime switching of `#modlaw` | not implemented | 0.3.0 only has compile-time policy; runtime switching deferred |

---

## 13. Threading Model *(0.3.0 new chapter)*

> **[0.3.0]** This chapter is entirely new in 0.3.0. UltraCPP adds a threading model in 0.3.0: cross-thread visibility `shared` annotation (Rule 25), cross-thread explicit ownership transfer (Rule 26), thread-local storage `__thread` (Rule 27), and four storage classes (Rule 28).

### 13.1 Shared Variables (`shared` Annotation Enforced)

> **[0.3.0 · Rule 25]** Cross-thread access must use the `shared` annotation; otherwise it is a compile error.

**Syntax**:
```cpp
shared T var;
shared T* p = alloc(T);    // owning + shared (shared heap object)
shared int* counter;       // cross-thread visible
```

**Enforcement rules:**

- Any **global / static** variable accessed inside a `spawn_thread(...)` function body **must** be declared `shared`. Otherwise it is a compile-time error: `non-shared global accessed from spawned thread`.
- `__thread` variables are exempt (per-thread independent; do not need `shared`).

**Compiler-auto-generated mutex:**

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);  // shared owning pointer

thread1() {
    int& r = counter;
    mod(r);                          // compile-time OK; runtime auto-generates mutex wrapper
    *r = *r + 1;                     // exclusive write
} // r leaves scope → auto unmod

thread2() {
    int& r = counter;
    mod(r);                          // blocks if thread1 still holds the lock
    *r = *r + 1;
}
```

**Programmer-optional explicit wrapper:**

```cpp
shared mutex<int*> counter;     // explicit mutex wrapper (mutex<T> is a stdlib type, see §11.5.1)
shared atomic<int> flag;        // explicit atomic wrapper (atomic<T> is a stdlib type, see §11.5.1)
```

> **[0.3.0 revised]** `mutex<T>` and `atomic<T>` are **stdlib types in §11.5.1**, not keywords. **When a variable's type is `mutex<T>` or `atomic<T>`, the compiler-auto-generated mutex is suppressed** — the programmer already controls synchronization explicitly, avoiding double-locking.

### 13.2 Cross-Thread Ownership (Rule 26)

> **[0.3.0 · Rule 26]** Cross-thread ownership must be transferred explicitly via `move_to_thread(p, tid)` or `spawn_thread_with(tid, p)`.

**Example F (cross-thread ownership):**

```cpp
int x = 42; // created in thread1

move_to_thread(x, thread2_id);  // x transferred to thread2
// x in thread1 is now moved-out

// Or via spawn:
int y = 100;
spawn_thread_with(thread2_id, y);  // implicit move

// In thread2:
void thread2() {
    // x and y are both visible and accessible
}
```

**Rules:**

1. **`move_to_thread(p, tid)`**: explicitly transfers ownership of `p` (must be owning or a reference) to thread `tid`; after the call, the caller has no right to access `p`.
2. **`spawn_thread_with(tid, p)`**: implicitly moves `p` while starting the thread — equivalent to `move_to_thread(p, tid)` + `spawn_thread(...)`.
3. **Stack references cannot cross threads**: if the owner of a `T&` borrow is a stack variable, `move_to_thread` is forbidden — stacks are thread-local (see §13.4).
4. **Limits on cross-thread references**: cross-thread references may only be passed for: heap (shared owner), global (shared), TLS (thread-local, but the receiver must match the TLS correctly).

### 13.3 Cross-Thread Modification Rights (mutex / atomic)

> **[0.3.0 · Rule 25 cont'd]** Cross-thread modification rights are supported either by the compiler-auto-generated mutex or by the programmer's explicit `mutex<T>` / `atomic<T>` wrapper. `mutex<T>` and `atomic<T>` are stdlib types from §11.5.1, not keywords.

**Auto mutex (compile-time):**

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
    mx.lock();         // explicit lock (NOT mod() — see §11.5.1)
    // critical section
    mx.unlock();       // explicit unlock
}
```

**Explicit `atomic<T>`:**

```cpp
atomic<int> flag;

void worker_atomic() {
    int v = flag.load();    // hardware atomic load
    flag.store(v + 1);      // hardware atomic store
}
```

> **Relationship with `mod()`**: the lock / load / store operations of `mutex<T>` / `atomic<T>` **do not** go through the `mod()` channel — they have their own synchronization primitives. `mod()` requests "modification rights" and applies to ordinary `T&` references. These two mechanisms do not conflict.

**`shared` combined with `#modlaw`:**

| `#modlaw` | `shared` + mod Behavior |
|------------|--------------------------|
| `shared` | multiple threads may mod concurrently; programmer responsible for data races |
| `exclusive` | compile-time / runtime check for unique lock holder; blocks when not held |
| `none` | shared is not allowed to mod (compile-time error) |

### 13.4 Multi-Thread Memory Layout (Rule 28)

> **[0.3.0 · Rule 28]** Thread visibility of the four memory regions.

| Storage Class | Thread Visibility | May Pass Reference Across Threads | Notes |
|---------------|-------------------|-----------------------------------|-------|
| **Stack** (function-local variables) | thread-local | ❌ forbidden | owner is destroyed when leaving scope |
| **Heap** (`alloc(T)`) | **shared** | ✅ allowed (requires `shared` or `move_to_thread`) | single owner, may be moved |
| **Global / Static** (file-scope / `static`) | **shared** | ✅ allowed (requires `shared` annotation) | process-lifetime |
| **TLS** (`__thread`) | thread-local | ❌ forbidden | GCC style; per-thread independent |

**Key constraints:**

- **Stack** references **cannot** be passed to other threads (the owner is destroyed when the source thread's scope ends).
- Cross-thread reference passing is allowed only for **heap / global / TLS** — for TLS, the sender and receiver must be the same thread.
- **Implicit capture of stack variables by `spawn_thread` is not allowed**: the compiler reports `cannot capture stack reference into spawned thread`.

### 13.5 `__thread` Thread-Local Storage (Rule 27)

> **[0.3.0 · Rule 27]** `__thread T x;` (GCC style) declares a thread-local variable.

**Syntax**:
```cpp
__thread int x;               // per-thread independent int
__thread int buf[256];        // per-thread independent array
__thread int* p;              // per-thread independent pointer (the pointer itself is thread-local; what it points to may not be)
```

**Semantics:**

- Each thread has an **independent copy** of `x`; they do not affect each other.
- Initialization happens exactly once (main thread); other threads inherit the main thread's initial value. This is the standard GCC `__thread` semantics.
- No `shared` annotation needed (thread-local is naturally isolated).

**Example:**

```cpp
__thread int local = 42;     // per-thread independent

void worker() {
    local = local + 1;       // affects only the current thread
    print_int(local);
}
```

### 13.6 Complete Threading Model Example

> **Example E (comprehensive threading model):**

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);  // cross-thread visible (forced shared)

thread1() {
    int& r = counter;
    mod(r);                          // compile-time OK; runtime mutex
    *r = *r + 1;                      // exclusive write
} // r leaves scope → auto unmod

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
|---------|------|-------------|
| 0.1 | 2026-04-13 | Initial specification |
| 0.1 | 2026-04-17 | Updated ownership default semantics, added `clone()`, updated inline-asm syntax |
| 0.1.0 | 2026-04-18 | 0.1.0 finalized |
| 0.2.0 | 2026-08-07 | Applied 8 design decisions D-1 .. D-8: generic `unique`, `&` fixed as immutable borrow, `&mut` entered, `move` promoted to primitive, assignment move rule explicit, `alloc`/`free` unenforced, `DanglingReference` defined, C-host-first decision recorded *(Note: 0.3.0 reversed D-3 and removed `&mut`; Q3 reversal overrides D-2's `&` semantics.)* |
| **0.3.0** | **2026-08-07** | **This version**: split ownership from modification rights (Rule 22); new `#modlaw` directive (Rule 23); new `mod()` / `unmod()` (Rule 24); new threading chapter §13 (Rule 25–28); **§3.3 rewritten with single `T&` (Q3 reversal — `T&mut` removed)**; §3.8 rewritten with custom Q6 semantics opposite of C++; §7.1 rewritten as implicit `mod()` + no ownership transfer (Q4=a); §3.2 rewritten distinguishing owning vs. non-owning pointers (Rule 2B); **§11.5.1 adds `mutex<T>` / `atomic<T>` standard-library types (fixes #5, #6)**. Status: draft. |

---

*UltraCPP 0.3.0 Language Specification (draft)*
