# UltraCPP 0.3.0 Language Specification

> **Version**: 0.3.0
>
> **Previous version**: 0.2.0 — [`UltraCPP-v0.2.0-spec-en.md`](./UltraCPP-v0.2.0-spec-en.md)
>
> **Status**: draft
>
> **Date**: 2026-08-07
>
> **Authority**: The Chinese edition [`UltraCPP-v0.3.0-spec-zh-CN.md`](./UltraCPP-v0.3.0-spec-zh-CN.md) is the **authoritative** version. This English edition is a companion translation; where the two disagree, the Chinese edition governs.

---

## Revision Summary

This version applies **21+ language design decisions** on top of 0.2.0 (the historical D-1..D-8 plus the new Rule 1, Rule 2A/2B, Q1..Q6, and Rule 22..28). The two pivotal changes are **splitting two permissions into independent concepts** (Rule 22) and the new **`#modlaw` directive** (Rule 23).

| Topic | Decision | One-line summary |
|-------|----------|------------------|
| Basics | Rule 1 | Owning types may live on stack/heap; storage location is inferred from the initializer. |
| References | Rule 2A | `&` references cannot own; null-ref is a compile-time error. |
| Pointers | Rule 2B | `T*` is owning or non-owning based on the initializer; `p1 = p2` does **not** transfer ownership but **grants `p1` modification rights** (implicit `mod(p1)`). |
| Pointers | Q1 | `unique T` ≡ `T*`; `unique` may be omitted. |
| Declarations | Q2 | Reference: `T& a = b` (initializer is lvalue). Pointer: `T* a = &b` (initializer is address-of). |
| References | Q3 | **Single reference type** `T&`; exclusivity is controlled by `#modlaw exclusive` + `mod()`, not by a type distinction. |
| Pointers | Q4 | `p1 = p2` does not move; `p1` gains modification rights (implicit `mod(p1)`). |
| Pointers | Q5 | `move(p)` is still required for explicit ownership transfer. |
| Pointers | Q6 | `const T*` / `T* const` get custom semantics (opposite of C++); owning-heap values may **not** create read-only views. |
| Permissions | **Rule 22** | **Two permissions split**: ownership (decides who deletes/frees) and modification rights (decides who may modify) are independent. |
| Directive | **Rule 23** | New `#modlaw` directive with exactly 6 legal combinations. |
| Expressions | **Rule 24** | New `mod(ref_expr)` to acquire modification rights; new `unmod(ref_expr)` to release (usually omitted). |
| Threads | **Rule 25** | Cross-thread access **must** carry the `shared` annotation (compile-time enforced); compiler synthesizes mutexes when needed. |
| Threads | **Rule 26** | Explicit `move_to_thread(p, tid)` or `spawn_thread_with(tid, p)` to move ownership across threads. |
| Threads | **Rule 27** | GCC-style `__thread int x;` for thread-local storage. |
| Threads | **Rule 28** | Multi-threaded memory layout: stack/TLS thread-local; heap/global shared. |

> **0.2.0 vs 0.3.0 textual differences**: this version **rewrites** 0.2.0 §7.1's assignment-move rule to "assignment = implicit `mod()` + ownership not transferred" (Q4=a), **rewrites** 0.2.0 §3.3's reference declaration syntax to "`T& a = b` (initializer is lvalue)" (Q2), and **removes the `T&mut` type** (Q3 reversal) — references are unified to `T&`; exclusivity is now controlled by `#modlaw exclusive` + `mod()`. Remaining sections carry forward 0.2.0 design with new chapters §4.9–4.10, §9.9, and §13.

---

## Changelog

| Decision | Section | Description |
|----------|---------|-------------|
| Rule 1 | §3.2 | Owning types are not limited to stack or heap; storage location is inferred from the initializer. |
| Rule 2A | §3.3 | `&` references cannot own; null-ref is a compile-time error. |
| Rule 2B | §3.2 | `T*` is owning or non-owning depending on the initializer; assignment does not transfer ownership but grants modification rights. |
| Q1 | §3.7 | `unique T` ≡ `T*`; `unique` may be omitted. |
| Q2 | §3.3, §12.1 | Reference initializer must be an lvalue; pointer initializer must be an `&b` expression. |
| Q3 | §3.3 | **Single reference type** `T&`; exclusivity is controlled by `#modlaw exclusive` + `mod()`, not by a type distinction. |
| Q4 | §3.2, §7.1 | Assignment `p1 = p2` does **not** copy ownership; it **grants `p1` modification rights** (implicit `mod(p1)`). |
| Q5 | §7.4 | `move(p)` is still required for explicit ownership transfer. |
| Q6 | §3.8, §7.10 | `const T*` / `T* const` have custom semantics (opposite of C++); owning heap forbidden. |
| **Rule 22** | §1.2, §3, §7 | Ownership and modification rights split into two independent concepts. |
| **Q3 reversal** *(0.3.0 revised)* | §3.3, §7.8, §7.9 (removed), §12.1, §12.2, §2.3, §2.6 | **Removed `T&mut` type**: references unified to `T&`; exclusivity is now controlled by `#modlaw exclusive` + `mod()`, no longer a type-level distinction. |
| **Rule 23** | §9.9 | New `#modlaw <perm> <scope>` directive (6 legal combinations). |
| **Rule 24** | §4.9, §4.10 | New `mod(ref_expr)` and `unmod(ref_expr)` expressions. |
| **Rule 25** | §13.1, §13.3 | Cross-thread access requires `shared`; mutex synthesized automatically or declared explicitly. |
| **Rule 26** | §13.2 | `move_to_thread(p, tid)` / `spawn_thread_with(tid, p)`. |
| **Rule 27** | §13.5 | `__thread` thread-local storage. |
| **Rule 28** | §13.4 | Multi-threaded memory layout. |

---

## 1. Overview

### 1.1 Language Goals

UltraCPP is a systems programming language that combines **C++ syntax familiarity** with **Rust-style memory safety guarantees**, without a garbage collector.

**Core goals**: zero-cost abstractions, compile-time memory safety, C++-migration compatibility, no garbage collector.

### 1.2 Design Principles (key: two permissions are independent)

1. **Safety by default**: enforce memory safety at compile time wherever possible.
2. **Explicit over implicit**: ownership, mutability, and safety semantics are visible in syntax.
3. **Pragmatic compatibility**: leverage C++ developer familiarity.
4. **Minimal runtime**: no heavy runtime, suitable for systems programming.
5. **Two permissions are independent** *(0.3.0 new — Rule 22)*:
   - **Ownership** — decides who is responsible for `delete` / `free`; **single owner**; transferable via `move(p)`; non-copyable.
   - **Modification rights (`mod`)** — decides who may write through the reference; may be shared or exclusive; requested by the programmer.

> **[0.3.0 · Rule 22]** This is the most consequential conceptual change in 0.3.0. Where 0.2.0's "ownership" conflated permission to mutate with permission to dispose, 0.3.0 separates them. `int& r = x;` (shared reference) does **not** by itself give you the right to modify `x`; you must additionally call `mod(r)` (under the active `#modlaw` policy — see §4.9).

### 1.3 Symbol Conventions

| Symbol | Meaning |
|--------|---------|
| `T` | Owned value (stack / global / TLS) |
| `unique T` ≡ `T*` | Owning pointer (generic form, see §3.7) |
| `T*` | Pointer: `T* p = &x` → non-owning stack/global pointer; `T* p = alloc(T)` → owning pointer |
| `T&` | **Single reference type**: must be initialized, non-null, non-owning; writable via `mod()`; many may coexist; thread-shareable |
| `&expr` | Create a `T&` reference to `expr` |
| `mod(ref_expr)` | Acquire modification rights (per `#modlaw`) |
| `unmod(ref_expr)` | Release modification rights (usually omitted — scope does it) |
| `move(p)` | Transfer ownership (`p` becomes invalid) |
| `move_to_thread(p, tid)` | Cross-thread ownership transfer |
| `shared` | Cross-thread visibility annotation (compile-time enforced) |
| `__thread T x` | Thread-local storage (per-thread independent) |
| `#modlaw <perm> <scope>` | Policy directive (6 legal combinations) |
| `const T*` | **Pointer-locked** (cannot rebind) + lifetime-safe + write-through-it forbidden (custom, **opposite of C++**) |
| `T* const` | **Read-only data view** (rebindable; write-through-it forbidden) |
| `alloc(T)` | Allocate heap memory for type T |
| `free(ptr)` | Free heap memory |
| `clone(ptr)` | Clone pointer (independent ownership) |

---

## 2. Lexical Structure

### 2.1 Source File Conventions

```
*.uc   — UltraCPP source file (recommended)
*.upp  — UltraCPP source file (legacy)
```

### 2.2 Token Types *(0.3.0 revised)*

| Category | Examples |
|----------|----------|
| **Keywords** | `if`, `else`, `while`, `for`, `return`, `struct`, `export`, `import`, `const`, `unique`, `move`, `free`, `alloc`, `null`, `true`, `false`, `void`, `extern`, `unsafe`, `typedef`, `clone`, `mod`, `unmod`, `shared`, `__thread`, `move_to_thread` |
| **Identifiers** | `foo`, `myVariable`, `_private`, `CamelCase` |
| **Literals** | `42`, `3.14`, `'x'`, `"hello"`, `true`, `false` |
| **Operators** | `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`, `&`, `|`, `^`, `~`, `<<`, `>>`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=` |
| **Delimiters** | `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `::` |
| **Preprocessor** | `#import`, `#include`, `#ifdef`, `#ifndef`, `#endif`, `#define`, `#modlaw` *(0.3.0 new)* |

### 2.3 Keywords (Reserved) *(0.3.0 revised)*

```
if          else        while       for         return
struct      export      import      const       typedef
unique      move        free        alloc       null
true        false       void        extern      unsafe
as          static      clone
mod         unmod       shared      __thread    move_to_thread
```

> **[0.3.0 new]** `mod`, `unmod`, `shared`, `__thread`, `move_to_thread`. **`mut` removed in 0.3.0** (was only used as part of `&mut`; with `T&mut` removed, no longer needed). See §4.9, §4.10, and §13.

### 2.4 Identifier Rules

```
identifier ::= letter (letter | digit)*
letter     ::= 'a'..'z' | 'A'..'Z' | '_'
digit      ::= '0'..'9'
```

Case-sensitive; no length limit (implementation-defined); may not collide with keywords.

### 2.5 Literals

Same as 0.2.0:
```
int-literal     ::= decimal | hex | octal | binary
float-literal   ::= [0-9]+'.'[0-9]+ | '.'[0-9]+ | [0-9]+'.'
char-literal    ::= "'" character "'"
string-literal  ::= '"' (character | escape)* '"'
```

### 2.6 Operators and Delimiters *(0.3.0 revised)*

| Operator | Description |
|----------|-------------|
| `=` | Assignment (for `T*`: triggers **implicit `mod()`** on the destination, **not** ownership transfer — see §7.1) |
| `&` | Unary prefix: create `T&` reference (§3.3). Binary infix: bitwise AND. |

The remaining operators are unchanged from 0.2.0.

> **[0.3.0 revised]** Unary prefix `&` creates the single reference type `T&`. Actual write access is gated by `mod()` and `#modlaw` (see Rule 24). 0.3.0 no longer distinguishes "shared-writable" from "exclusive-writable" references — `T&mut` is removed.

### 2.7 Comments

```cpp
// single-line
/* multi-line */
```

### 2.8 Whitespace

Ignored when separating tokens. Line endings preserved for error reporting.

---

## 3. Type System *(0.3.0 rewritten)*

### 3.1 Fundamental Types

Unchanged from 0.2.0 (16 primitive integer/float types plus `void`, `bool`, `char`, `usize`, `isize`).

### 3.2 Pointer Types *(0.3.0 rewritten: Rule 2B, Q4)*

**`T*` is owning or non-owning depending on the initializer** (Rule 2B). This is the core 0.3.0 vs 0.2.0 difference for pointers.

```cpp
T*              // initializer &x → non-owning; initializer alloc(T) → owning
T*              // at most one owner at a time; assignment p1 = p2 does not copy ownership, grants p1 modification rights (implicit mod(p1))
```

**Example A (canonical declaration syntax, Q2):**

```cpp
int x = 42;

// ✓ Reference declaration: initializer is an lvalue
int& r = x;       // r is a reference to x (latently-writable)

// ✓ Pointer declaration: initializer is address-of
int* p = &x;      // p is x's address (non-owning)

// ✗ Type mismatches (must appear as counter-examples)
// int& r2 = &x;   // wrong: T& cannot take T*
// int* p2 = x;    // wrong: T* cannot take T
```

**Example (owning vs non-owning distinction):**

```cpp
// === Non-owning stack pointer (initializer is address-of) ===
int* p = &x;
*p;                                  // ✅ read
// *p = 100;                          // ❌ no mod right by default; need mod(p)
int* p2 = &x;                        // ✅ multiple non-owning pointers coexist

// === Owning heap pointer (initializer is alloc) ===
int* h = alloc(int);                 // h owns
*h = 100;                            // ✅ default mod right
int* h2 = alloc(int);
int* h3 = h2;                        // implicit mod(h3): h3 gets mod right,
//                                   // h2 still owns; ownership is NOT transferred
//                                   // so free(h3) is wrong
free(h2);                            // ✅ h2 is the owner
```

**Key rules (Rule 2B)**:
1. The **initializer decides**: `T* p = &x` → p does not own; `T* p = alloc(T)` → p owns.
2. At most one owner at a time for any allocated object.
3. Assignment `p1 = p2` does **not** copy ownership — it grants `p1` modification rights (Q4). The owner is unchanged.
4. `move(p)` is the **explicit** ownership transfer (Q5).
5. `readonly` / `const T*` / `T* const` control read access — see §3.8 / §7.10 (**opposite of C++**).

### 3.3 Reference Types *(0.3.0 rewritten: T&mut removed, Q2, Rule 22)*

UltraCPP has a **single** reference type `T&` — must be initialized, cannot be null, and **does not own** the referent (ownership stays with the owner).

> **[0.3.0 · Rule 22 + Q3 reversal]** 0.2.0 split references into `T&` (immutable) and `T&mut` (exclusive-mutable). 0.3.0 **removes `T&mut`** and unifies to the single reference type `T&`. **Exclusivity** is no longer a type-level distinction; it is controlled by `#modlaw exclusive` + `mod()`. Whether a write is actually allowed is also decided by `mod()` + `#modlaw`.

```cpp
T&     // latently-writable reference: no mod by default; needs mod() to write
```

**Declaration syntax (Q2):**

```cpp
int x = 42;
int& r = x;          // reference declaration: initializer is an lvalue (NO '&' prefix)
int& r2 = x;         // ✅ many T& may coexist
// int& r3 = &x;     // ❌ T& cannot take T*
// int& r4 = 42;     // ❌ T& initializer must be an lvalue
```

**Writability (Rule 24):**

```cpp
int x = 42;
int& r = x;          // r is a reference to x; no mod by default
int v = r;           // ✅ read
// r = 100;          // ❌ compile error: no mod right
mod(r);              // ✅ acquire mod right (per #modlaw)
*r = 100;            // ✅ write
```

**Version comparison (0.1.0 / 0.2.0 / 0.3.0):**

| Version | Reference types | Exclusivity mechanism | Write access |
|---|---|---|---|
| 0.1.0 | T& (C++-style mutable reference) | none | direct write |
| 0.2.0 | T& (immutable) + T&mut (exclusive-mutable) | type-level | T& read-only; T&mut exclusive write |
| 0.3.0 | **T& only** | `#modlaw exclusive` + `mod()` | needs mod(); behavior per #modlaw |

### 3.4 Function Pointer Types

Unchanged from 0.2.0 (`int (*)(int, int)`, `void (*)(const char*)`, etc.).

### 3.5 Array Types

Unchanged (`T[n]`).

### 3.6 Struct Types

Unchanged.

### 3.7 Type Modifiers (`unique T ≡ T*`) *(Q1, retained from 0.2.0)*

| Modifier | Meaning |
|---------|---------|
| `const` | Value immutable (with custom semantics — see §3.8) |
| `unique` | **Generic type constructor**: `unique T` ≡ `T*` (Q1, omittable) |
| `static` | Internal linkage |
| `shared` | Cross-thread visibility annotation (compile-time enforced — see §13.1) |

`unique T` rules carried over from 0.2.0:
1. `T` may be any type from §3.1–§3.6.
2. `unique T` ≡ `T*`.
3. `unique` cannot be applied to borrow types (`unique T&` is illegal).
4. `shared` is a storage class, **not** a type constructor — it cannot be written as `shared unique T*`.

### 3.8 Pointer Modifiers and Ownership Boundary (Q6 custom `const T*` / `T* const`) *(0.3.0 new)*

> **[0.3.0 · Q6 custom semantics, opposite of C++]** 0.2.0 inherited C++ meaning for these qualifiers; 0.3.0 swaps them.

| Declaration | Meaning | Rebindable? | Writable through it? | Owning-heap allowed? |
|-------------|---------|-------------|----------------------|----------------------|
| `const T*` | **Pointer-locked**: pointer cannot rebind + lifetime-safe + write-through-forbidden | ❌ | ❌ | ❌ |
| `T* const` | **Read-only data view**: pointer rebindable + write-through-forbidden | ✅ | ❌ | ❌ |
| `const T* const` | Double-locked | ❌ | ❌ | ❌ |

> **Opposite of C++**:

> | Type | C++ meaning | UltraCPP 0.3.0 meaning |
> |------|-------------|--------------------------|
> | `const T*` | "data immutable, pointer rebindable" | **pointer immutable, data also unwritable** |
> | `T* const` | "pointer immutable, data mutable" | **data unwritable, pointer rebindable** |

**Example D (Q6 custom semantics):**

```cpp
int x = 42;

// === const T*: pointer-locked + lifetime-safe + read-only ===
const int* p = &x;
*p;                                  // ✅ read
// *p = 100;                         // ❌ cannot write
// p = &y;                           // ❌ pointer is locked

// === T* const: read-only data view ===
T* const r = &x;
*r;                                  // ✅ read
// *r = 100;                         // ❌ data is read-only
r = &y;                              // ✅ rebind is allowed

// === owning heap is forbidden ===
unique int* h = alloc(int);
const int* bad = h;                  // ❌ cannot build read-only view of owning heap
T* const bad2 = h;                   // ❌ same prohibition
```

**Why is owning heap forbidden?** An owning pointer may later be `move()`d or `free()`d; any `const T*` / `T* const` view would dangle. The release time of an owning pointer is under programmer control, so the compiler cannot statically prove the read-only view safe.

Further detail (diagnostics, dereference semantics) — see §7.10.

---

## 4. Expressions *(0.3.0 new: §4.9, §4.10)*

### 4.1 Operator Precedence and Associativity

Same 15-level table as 0.2.0, with one addition: level 15 now lists `move`, `clone`, **and `move_to_thread`**.

> **[0.3.0]** Binary infix `&` at level 8 remains bitwise AND. Unary prefix `&`, `mod`, `unmod`, and `move_to_thread` are unary operators that bind tighter than any binary operator.

### 4.2–4.8 Arithmetic / Comparison / Logical / Bitwise / Assignment / Conditional / Parenthesized Expressions

Unchanged from 0.2.0.

### 4.9 `mod()` Expression — Acquire Modification Rights *(0.3.0 new: Rule 24)*

> **[0.3.0 · Rule 24]** `mod(ref_expr)` requests modification rights on the referent of a `T&`. The behavior is decided by the active `#modlaw` policy. **0.3.0 removed `T&mut`**; `mod()` accepts only `T&` references.

**Syntax:**
```
mod '(' reference_expression ')'
```

**Operand**: a `reference_expression` of reference type `T&` (i.e. `int&`, `Point&`, ...).

**Semantics** (Rule 24): behavior depends on `#modlaw`:

| `#modlaw` policy | `mod(r)` behavior |
|------------------|-------------------|
| `none` | ❌ compile-time error: `mod() forbidden by #modlaw none policy` |
| `exclusive` | ✅ compile-time conflict check; another `T&` request → `BorrowConflict` |
| `shared` | ✅ multiple `T&` may hold mod simultaneously (programmer handles data races) |

**Example fragment:**
```cpp
#modlaw shared module

int x = 42;
int& r = x;
mod(r);              // ✅ under shared policy
*r = 100;
mod(r);              // repeatable within scope (compiler auto-unmods after last use)
```

**Implicit `mod()`** (Rule 24 + Q4): assignment `p1 = p2` over `unique T` automatically generates `mod(p1)` (see §7.1).

```cpp
int* p1 = alloc(int);
int* p2 = p1;         // implicit mod(p1): p1 gets mod right, p2 still owns
*p1 = 100;            // ✅ no explicit mod() needed
```

### 4.10 `unmod()` Expression — Release Modification Rights *(0.3.0 new: Rule 24)*

`unmod(ref_expr)` explicitly releases modification rights. Usually omitted — scope end / last use auto-unmods.

**Syntax:**
```
unmod '(' reference_expression ')'
```

```cpp
mod(r);              // acquire
*r = compute();      // use
unmod(r);            // optional — release immediately to shorten critical section
```

---

## 5. Statements

Unchanged from 0.2.0. Includes declaration statement examples covering owning pointer (`int* p = alloc(int);`), non-owning pointer (`int* p = &x;`), and the single reference type (`int& r = x;`). 0.3.0 removed `int&mut`.

---

## 6. Functions

### 6.1–6.2 Function Definitions and Calls

Unchanged.

### 6.3 Parameter Passing *(0.3.0 rewritten: single T& + mod())*

**Pass-by-value:**
```cpp
void inc(int x) { x = x + 1; }            // does not affect caller
int n = 10;
inc(n);                                   // n still 10
```

**Reference parameter for write (0.3.0 form — the only form):**
```cpp
// === 0.3.0 form (only): acquire via mod() ===
void inc(int& x) {
    mod(x);
    *&x = *&x + 1;  // or x = x + 1 depending on syntax
}

int n = 10;
mod(n);  // or mod at the call site
inc(&n);
// n is now 11
```

**Read-only parameter still uses `T&` (no `mod` needed):**
```cpp
int read(int& x) { return x; }            // no mod()
int n = 10;
int v = read(n);                          // v == 10
```

> **[0.3.0 · Q3 reversal, Rule 22, Rule 24]** 0.2.0 provided two reference parameter forms — `void inc(int&mut x)` (exclusive mod-by-default, called as `inc(&mut n)`) and `void inc(int& x)` (read-only). 0.3.0 **removes `T&mut`** and unifies to a single reference type `T&` parameter: writing requires a prior `mod(x)` whose behavior is decided by the active `#modlaw`. Exclusivity is achieved via `#modlaw exclusive` + `mod()`, no longer a type-level distinction.

### 6.4 Return Values

Unchanged from 0.2.0. Returning a local by value is legal; returning a borrow of a local is `DanglingReference` — see §7.9.

### 6.5 main Convention

Unchanged.

---

## 7. Memory Management

### 7.1 Ownership Semantics (Two Permissions Split) *(0.3.0 rewritten: Rule 22, Q4, Q5)*

> **[0.3.0 · Rule 22]** 0.3.0 splits 0.2.0's single "ownership" concept into two independent permissions:

> | Permission | Decides | Default | Transfer |
> |------------|---------|---------|----------|
> | **Ownership** | Who deletes / frees | `T* p = alloc(T)` owns; `T* p = &x` doesn't | Explicit `move(p)` |
> | **Modification rights** | Who may write | Owning pointer: yes by default; `T&`: no (needs `mod()`) | Implicit `mod()` (per `#modlaw`) |

**Pointers and ownership (Rule 2B, Q4):**
- At most one owner at a time.
- `T* p = alloc(T)`: p owns both ownership and modification rights.
- `T* p = &x`: p holds only the address; **does not own** and **does not** have default mod rights.
- Assignment `p1 = p2` does **not** copy ownership (Q4); it grants `p1` modification rights (implicit `mod(p1)`).

#### 7.1.1 Assignment and Modification Rights *(0.3.0 rewrites 0.2.0 §7.1.1)*

**Example C (ownership transfer vs. modification rights):**

```cpp
unique int* p1 = alloc(int);  // p1 owns
*p1 = 100;

unique int* p2 = p1;           // implicit mod(p2): p2 gets mod right, p1 still owns
*p2 = 200;                      // ✅
// free(p2);                    // ❌ p2 does not own

move(p2);                       // explicit transfer
// p1 is now moved-out
free(p2);                       // ✅ p2 now owns
```

**Key rule (0.3.0 vs 0.2.0 difference):**

> Let `dst = src` where both have type `unique T` (≡ `T*`).
>
> 0.3.0:
> 1. `dst` gains **modification rights** (implicit `mod(dst)`, governed by the active `#modlaw`).
> 2. `src`'s **ownership is unchanged** — it still owns; transfer only via explicit `move(src)`.
> 3. `dst` does **not** own — so `free(dst)` is a compile-time error while p2 is non-owning.
>
> 0.2.0 (D-5): `src` becomes null, `dst` takes ownership.
>
> 0.3.0 **rewrites** this: `src` is untouched; `dst` gains mod rights. `move(p)` is the only explicit ownership-transfer mechanism (Q5).

**Where ownership transfer occurs** (Q5):

| Site | Behavior | Source | Destination |
|------|----------|--------|-------------|
| Explicit `move(p)` | Transfer ownership | `p` | `p2` (receiver) |
| Call argument `f(p)` (param typed `unique T`) | Transfer ownership | `p` | parameter |
| Return `return p;` (return type `unique T`) | Transfer ownership | `p` | caller's receiver |

**Where modification rights are granted** (Q4):

| Site | Behavior | Source | Destination |
|------|----------|--------|-------------|
| Assignment `dst = src` (type `unique T`) | Implicit `mod(dst)` | — | `dst` |
| Explicit `mod(ref_expr)` (§4.9) | Per `#modlaw` | — | ref_expr |

**Neither** occurs at: `clone(s)`, `&s`, `s == null`-style reads.

### 7.2 alloc / 7.3 free / 7.4 move(p) / 7.5 clone / 7.6 null / 7.7 Pointer Arithmetic

Carried over from 0.2.0. `move(p)` retains its built-in-primitive status with runtime source-nulling; `alloc`/`free` pairing remains a programmer obligation (compiler does not enforce).

### 7.8 Reference `&T` *(0.3.0 rewritten: Rule 22, Rule 24, T&mut removed)*

> **[0.3.0 rewritten]** 0.3.0 removes the `T&mut` type. `T&` is the single reference type; whether writing is allowed is decided by `mod()` + `#modlaw`.

**Syntax:**
```
'&' unary_expression
```

`&expr` creates a reference to `expr`'s referent, of type `T&`.

**Creation rules:**
1. Operand must be an lvalue.
2. Owner must be live and initialized.
3. Reference may not outlive its owner (`DanglingReference` — see §7.9).
4. `&` does not change the owner's ownership state.

**Writability (Rule 24):**
5. **Write requires `mod()` first.** `r = v;` without prior `mod(r)` is a compile-time error.
6. Multiple `T&` may coexist.
7. Exclusivity is controlled by `#modlaw exclusive` (compile-time `mod()` conflict check).

```cpp
#modlaw shared module

int x = 42;
int& r1 = x;
int& r2 = x;        // ✅ multiple references coexist
int v = r1 + r2;    // ✅ read
// r1 = 100;        // ❌ no mod right
mod(r1);            // ✅ acquire mod right (per #modlaw)
*r1 = 100;          // ✅ write

#modlaw exclusive module

int y = 10;
int& m = y;
mod(m);             // ✅ exclusive mod
*m = 100;
// mod(m2);         // ❌ conflict

#modlaw none module

int z = 5;
int& r = z;
// mod(r);         // ❌ forbidden by none policy
r;                  // ✅ read-only
// *r = 100;       // ❌ no mod right
```

### 7.9 Dangling Reference *(0.3.0 retained from 0.2.0 · D-7)*

Same definition and trigger conditions as 0.2.0:
- (a) `&T`'s owner leaves scope.
- (b) Function returns a reference to a local of that function.

**Definition:** a `T&` borrow is **dangling** when it remains reachable after its owner's storage has been destroyed.

**Example (a) (carried from 0.2.0):**
```cpp
int main() {
    int& r;
    {
        int x = 42;
        r = x;              // borrow x (0.3.0 syntax: initializer is lvalue, not &x)
    }                       // ← x leaves scope
    return r;               // ❌ DanglingReference
}
```

**Positive example (carried from 0.2.0):**
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

### 7.10 `const T*` / `T* const` Custom Semantics Detail *(0.3.0 new)*

> **[0.3.0 · Q6 custom semantics, opposite of C++]** See §3.8 for the conceptual table. This section gives deeper rules.

**`const T*`** (continuation of §7.10):
- **Pointer-locked**: cannot reassign (`p = &y;` is a compile-time error).
- **Read-only access**: cannot write through (`*p = v;` is a compile-time error).
- **Lifetime-safe**: compiler verifies that the lifetime of the referent ≥ the borrow's last use.
- **May not** point at an owning-heap variable (owner may be moved/freed later → dangling).

**`T* const`**:
- **Read-only data view**: cannot write through (`*r = v;` is a compile-time error).
- **Rebindable**: `r = &y;` is legal.
- **May not** point at an owning-heap variable.

Diagnostic on the owning-heap violation:

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

**`mod()` does not apply** to `const T*` / `T* const` — these are designed to be unwritable.

---

## 8. struct Types

Unchanged from 0.2.0.

---

## 9. Modules and Preprocessor *(0.3.0 new: §9.9)*

### 9.1 Preprocessing Phase

| Directive | Behavior |
|-----------|----------|
| `#include "path"` | Expand file contents at this location |
| `#import "module"` | Record module dependency (no content expansion) |
| `#modlaw <perm> <scope>` *(0.3.0 new)* | Set the modification-rights policy for this file/module (see §9.9) |
| `export <declaration>` | Mark function/variable as exported |

**Preprocessor output**:
1. Code after `#include` expansion
2. Module dependency list
3. Active `#modlaw` policy
4. Exported symbol list

### 9.2 #include, 9.3 #import, 9.4 export, 9.5 Module Design Rules, 9.6 Search Paths, 9.7 Circular Detection

All unchanged from 0.2.0.

### 9.8 Compilation Flow

Same flowchart, with `#modlaw 解析` added to the preprocessor step.

### 9.9 `#modlaw` Directive *(0.3.0 new: Rule 23)*

> **[0.3.0 · Rule 23]** Keyword `#modlaw` (all lowercase, underscore). Syntax: `#modlaw <perm> <scope>`.

**Syntax:**
```
#modlaw <perm> <scope>
```

**Legal combinations (exactly 6)**:

| `perm` | `scope` | Meaning |
|--------|---------|---------|
| `none` | `global` | Module-wide: `mod()` forbidden; references read-only |
| `none` | `module` | This module: `mod()` forbidden; references read-only |
| `exclusive` | `global` | Module-wide: `mod()` is exclusive (compile-time conflict check) |
| `exclusive` | `module` | This module: `mod()` is exclusive (compile-time conflict check) |
| `shared` | `global` | Module-wide: `mod()` is shared (multiple allowed; programmer handles data races) |
| `shared` | `module` | This module: `mod()` is shared (multiple allowed; programmer handles data races) |

The other 10 combinations (e.g. `#modlaw none shared`, `#modlaw exclusive none`) are compile-time errors.

**Scope (`scope`)**:
- `global`: affects the whole file and any symbols brought in by `#include`.
- `module`: only affects symbols in the current `.uc` module; `#include`d files do not inherit.
- Default (no `#modlaw`): equivalent to `#modlaw shared module`.

**Placement**: `#modlaw` must appear at the **top** of the module (after `import`/`include`/`export`, before the first function/variable declaration). At most one `#modlaw` per module; duplicates are compile-time errors.

**Example B (three strategies):**

```cpp
// === shared (default) ===
#modlaw shared module

int x = 42;
int& r1 = x;
int& r2 = x;
mod(r1);            // ✅ r1 acquires shared mod
mod(r2);            // ✅ r2 may also (data-race is programmer's responsibility)
*r1 = 100; *r2 = 200;

// === exclusive ===
#modlaw exclusive module

int y = 10;
int& m = y;
mod(m);             // ✅ exclusive mod
// mod(m2);         // ❌ conflict
*m = 100;

// === none ===
#modlaw none module

int z = 5;
int& r = z;
// mod(r);         // ❌ forbidden by none policy
r;                  // ✅ read-only
// *r = 100;       // ❌ no mod right
```

---

## 10. FFI

### 10.1 extern "C" Blocks, 10.2 C Type Mapping, 10.3 unsafe Blocks, 10.4 Inline Assembly

Unchanged from 0.2.0 — with one note added to §10.2 explaining that C's `const char*` (write-forbidden data) coincidentally agrees with UltraCPP's `const T*` (which is **also** write-forbidden) for non-rebinding cases, but the rebind semantics differ at the boundary. See §7.10 for full detail.

> **[0.3.0 retained from 0.2.0 · D-2, D-7]** `unsafe` blocks **suppress** ownership and borrow checks: implicit-`mod()` checks in §7.1, borrow rules in §7.8, `DanglingReference` in §7.9 are not reported inside `unsafe`. `unsafe` does **not** change semantics — `move` still nulls the source (§7.4), `&` is still a reference (§7.8) — it only shifts the safety obligation to the programmer.

---

## 11. Standard Library

### 11.1–11.4 Basic I/O, Strings, Memory, Utilities

Unchanged from 0.2.0.

### 11.5 Thread Primitives *(0.3.0 new)*

```cpp
// --- cross-thread ownership ---
move_to_thread(T* p, int tid);       // explicit ownership transfer to thread tid
spawn_thread(int tid, void fn());    // launch thread without ownership transfer
spawn_thread_with(int tid, T* p);    // launch thread with implicit move(p)
join_thread(int tid);                // wait for one thread
join_all();                          // wait for all spawned threads
current_tid();                       // current thread id

// --- thread-local storage ---
__thread int x;                      // per-thread independent int
__thread int buf[256];               // per-thread independent array

// --- shared primitives ---
mutex<T>     mx;                     // explicit mutex wrapper (stdlib type, see §11.5.1)
atomic<T>    at;                     // explicit atomic wrapper (stdlib type, see §11.5.1)
shared T*    sp = ...;               // cross-thread visible (compile-time enforced)
```

#### 11.5.1 Synchronization Types `mutex<T>` / `atomic<T>` *(0.3.0 new)*

> **[0.3.0 new]** `mutex<T>` and `atomic<T>` are **standard-library types** (defined in `lib/sync.uc`), **not keywords**. They provide explicit wrappers for cross-thread modification rights.

**`mutex<T>`**:

| Member | Description |
|--------|-------------|
| `lock()` | Acquire (blocks until obtained) |
| `unlock()` | Release |
| `try_lock()` | Non-blocking attempt; returns `false` on failure |
| `wait()` | Wait on condition variable (paired with `notify`) |
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
|--------|-------------|
| `load()` | Atomic read |
| `store(v)` | Atomic write |
| `exchange(v)` | Atomic swap |
| `compare_exchange(expected, desired)` | CAS |

```cpp
atomic<int> flag;

void worker() {
    int v = flag.load();
    flag.store(v + 1);
}
```

**Relationship to `#modlaw`**:

- `mutex<T>` / `atomic<T>` `lock` / `load` / `store` operations **do not** flow through `mod()` — they carry their own synchronization primitives.
- When a variable's type is `mutex<T>`, the **compiler-synthesized mutex is suppressed** (programmer is already in explicit control), avoiding double locking.
- Same for `atomic<T>`: hardware atomic instructions already guarantee visibility, so no further compiler-generated sync is needed.

### 11.6 `alloc` / `free` Pairing Convention *(0.3.0 retained from 0.2.0 · D-6)*

Programmer's responsibility; compiler does not enforce. The three categories still reported (`UseAfterMove` / `UseAfterDrop` / `DanglingReference`) carry over.

---

## 12. Appendices

### 12.1 Complete EBNF *(0.3.0 revised)*

> **[0.3.0]** Additions vs 0.2.0 (every 0.2.0 production is retained):
> 1. `modlaw_directive` added at top level (Rule 23).
> 2. `mod` / `unmod` / `move_to_thread` added to `unary_expression` (Rule 24, 26).
> 3. Optional storage class `('shared' | '__thread')` added to declarations (Rule 25, 27).
> 4. `const T*` / `T* const` productions added to `pointer_type` with **swapped** semantics (Q6).
> 5. `modlaw_perm` / `modlaw_scope` non-terminals define exactly 6 legal combinations.

```
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

modlaw_directive  ::= '#modlaw' modlaw_perm modlaw_scope
modlaw_perm       ::= 'none' | 'exclusive' | 'shared'
modlaw_scope      ::= 'global' | 'module'

declaration       ::= storage_class? variable_declaration
                    | const_declaration

storage_class     ::= 'shared' | '__thread'      // [0.3.0 Rule 25, 27]

pointer_type      ::= type '*'
                    | type '*' 'const'                              // [0.3.0 Q6] data read-only view
                    | 'const' type '*'                              // [0.3.0 Q6] pointer-locked
                    | 'const' type '*' 'const'

unary_expression   ::= postfix_expression
                    | ('+' | '-' | '!' | '~' | '*' | '&') unary_expression
                    | 'sizeof' '(' type ')'
                    | 'move' '(' expression ')'
                    | 'clone' '(' expression ')'
                    | 'mod' '(' expression ')'                       // [0.3.0 Rule 24]
                    | 'unmod' '(' expression ')'                     // [0.3.0 Rule 24]
                    | 'move_to_thread' '(' expression ',' expression ')' // [0.3.0 Rule 26]
```

(Other productions — fundamentals, types, statements, expressions, lexical — retained from 0.2.0 verbatim, with the `mutable_reference_type` production **removed** in 0.3.0.)

#### 12.1.1 `&` Parsing Note *(0.3.0 revised)*

0.3.0 **removes the `&mut` production**. `'&' unary_expression` is the only reference production; no lookahead needed — `&` is always followed by a unary expression. `mut` is no longer a keyword.

#### 12.1.2 `const T*` / `T* const` Parsing Note *(0.3.0 new: Q6)*

`const` in `type` has a 1-token lookahead to decide which side it qualifies — **opposite of C++**:

- `const type *` → pointer-locked (pointer cannot rebind).
- `type * const` → read-only data view.

The semantics (§3.8 / §7.10) make `const` qualify its **adjacent** side, **not** transitively to the referent.

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

> **[0.3.0 new]** `mod`, `unmod`, `shared`, `__thread`, `move_to_thread`. **0.3.0 removed** `mut` (was only used as part of `&mut`; with `T&mut` removed, no longer needed).

### 12.3 Operator Precedence Table *(0.3.0 revised)*

Level 15 now lists `move`, `clone`, **`move_to_thread`**. Unary operators table gains `mod` and `unmod`; the `&mut` row is **removed** (with `T&mut` deleted). Everything else from 0.2.0 is retained.

### 12.4 Appendix X: Impact on Future Work *(0.3.0 revised)*

C host (`src-c/`) remains the production compiler. New mapping rows for §9.9 (`#modlaw`), §13.1 (`shared`), and §13.2 (`move_to_thread`). New artifacts:

1. Lexer: 5 new keywords + `#modlaw` directive.
2. Grammar: 4 new productions + swapped `const T*` / `T* const` rules.
3. Error kinds: `UC_ERR_MODLAW`, `UC_ERR_CROSS_THREAD_MOVE`, `UC_ERR_OWNING_HEAP_VIEW`.
4. Checker upgrade: ownership + mod is now a **dual-state** table (owner bit + mod bit).
5. Threading: `spawn_thread` / `join_thread` / `mutex<T>` (stdlib, §11.5.1) / `atomic<T>` (stdlib, §11.5.1).
6. Tests: `test/modlaw/{pass,fail}/`, `test/threading/{pass,fail}/`, `test/const_ptr/{pass,fail}/`.

#### 12.4.2 Items Deliberately Left Out

| Item | Status | Where |
|------|--------|-------|
| `MissingFree` / `DoubleFree` | Not implemented | §11.6 |
| `Owned<T>` / `Ref<T>` / `RefMut<T>` internals | Not user-visible | devhandbook §4.1 |
| Precise non-lexical lifetimes | Only "until last use" | To be filled in |
| `__thread` placement detail | `__thread T x;` form supported | Implementation detail |
| Runtime `#modlaw` switching | Not implemented | 0.3.0 is compile-time only |

---

## 13. Threading Model *(0.3.0 new chapter)*

### 13.1 Shared Variables (`shared` Annotation Enforced)

> **[0.3.0 · Rule 25]** Cross-thread access requires the `shared` annotation; otherwise compile-time error.

```cpp
shared T var;
shared T* p = alloc(T);
shared int* counter;
```

**Mandatory rules:**
- Any global/static variable accessed from inside a `spawn_thread(...)` body **must** be declared `shared`. Otherwise: `non-shared global accessed from spawned thread`.
- `__thread` variables are exempt (they are thread-local).

**Compiler-synthesized mutex** (Rule 25):

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);

thread1() {
    int& r = counter;
    mod(r);             // ✅ compile-time OK; runtime mutex synthesized
    *r = *r + 1;
}
```

**Programmer-supplied wrappers:**
```cpp
shared mutex<int*> counter;          // explicit mutex wrapper (stdlib type, see §11.5.1)
shared atomic<int> flag;             // explicit atomic wrapper (stdlib type, see §11.5.1)
```

> **[0.3.0 revised]** `mutex<T>` and `atomic<T>` are **§11.5.1 standard-library types**, not keywords. **When a variable's type is `mutex<T>` or `atomic<T>`, the compiler-synthesized mutex is suppressed** — the programmer is in explicit control, avoiding double locking.

### 13.2 Cross-Thread Ownership (Rule 26)

> **[0.3.0 · Rule 26]** Cross-thread ownership requires explicit `move_to_thread(p, tid)` or `spawn_thread_with(tid, p)`.

**Example F (cross-thread ownership):**

```cpp
int x = 42;                          // created in thread1

move_to_thread(x, thread2_id);      // explicit ownership transfer
// thread1: x is now moved-out

int y = 100;
spawn_thread_with(thread2_id, y);   // implicit move
```

**Rules:**
1. `move_to_thread(p, tid)` — explicit transfer; caller loses access.
2. `spawn_thread_with(tid, p)` — implicit transfer at thread launch (equivalent to `move_to_thread` + `spawn_thread`).
3. **Stack references cannot cross threads**: owner (a stack variable) would die on leaving its thread's scope.
4. Cross-thread references may only go through heap / global / TLS — see §13.4.

### 13.3 Cross-Thread Modification Rights

> **[0.3.0 · Rule 25 cont'd]** Modification rights across threads are supported by compiler-synthesized mutexes or explicit `mutex<T>` / `atomic<T>`. `mutex<T>` and `atomic<T>` are §11.5.1 standard-library types, not keywords.

**Automatic mutex (compile-time):**
```cpp
#modlaw exclusive global

shared int* counter = alloc(int);

void worker() {
    int& r = counter;
    mod(r);             // compiler emits pthread_mutex_lock
    *r = *r + 1;
}                       // scope exit → pthread_mutex_unlock
```

**Explicit `mutex<T>`:**
```cpp
mutex<int> mx;
void worker_explicit() {
    mx.lock();         // explicit lock (not mod() — see §11.5.1)
    /* critical section */
    mx.unlock();
}
```

**Explicit `atomic<T>`:**
```cpp
atomic<int> flag;
void worker_atomic() {
    int v = flag.load();   // hardware atomic load
    flag.store(v + 1);     // hardware atomic store
}
```

> **Relationship to `mod()`**: `mutex<T>` / `atomic<T>` `lock` / `load` / `store` operations **do not** flow through `mod()` — they carry their own synchronization primitives. `mod()` requests modification rights for plain `T&` references. The two mechanisms are independent and non-conflicting.

**`shared` ↔ `#modlaw`:**

| `#modlaw` | `shared` + mod behavior |
|-----------|--------------------------|
| `shared` | Multiple threads may mod simultaneously; programmer owns data-race responsibility |
| `exclusive` | Compile-time / runtime single-holder check; blocks if held |
| `none` | `shared` may not be modded (compile-time error) |

### 13.4 Multi-Threaded Memory Layout (Rule 28)

> **[0.3.0 · Rule 28]** Four storage regions, with their own thread-visibility rules.

| Region | Visibility | Cross-thread ref allowed? | Notes |
|--------|------------|---------------------------|-------|
| **Stack** (function-local) | thread-local | ❌ | Owner dies when scope exits |
| **Heap** (`alloc(T)`) | **shared** | ✅ (via `shared` or `move_to_thread`) | Single owner; transferable |
| **Global / static** (file-scope / `static`) | **shared** | ✅ (via `shared` annotation) | Process lifetime |
| **TLS** (`__thread`) | thread-local | ❌ (must be same thread) | GCC-style per-thread |

**Key constraints:**
- **Stack** references cannot be sent to another thread.
- Cross-thread references are only allowed for heap / global / TLS — TLS only when sender and receiver are the same thread.
- **Stack captures in `spawn_thread` are forbidden**: compiler reports `cannot capture stack reference into spawned thread`.

### 13.5 `__thread` Thread-Local Storage (Rule 27)

> **[0.3.0 · Rule 27]** `__thread T x;` (GCC-style) declares a thread-local variable.

```cpp
__thread int x;                  // per-thread independent
__thread int buf[256];           // per-thread independent array
__thread int* p;                 // the pointer itself is thread-local; what it points to may not be
```

**Semantics**:
- Each thread has its own independent copy of `x`.
- Initialization happens once (main thread); other threads inherit main's initial values — GCC `__thread` semantics.
- `shared` is not required (thread-local isolation is implicit).

### 13.6 Full Threading Example

> **Example E (combined threading model):**

```cpp
#modlaw exclusive global

shared int* counter = alloc(int);   // cross-thread visible (shared enforced)

thread1() {
    int& r = counter;
    mod(r);                          // compile-time OK; runtime mutex
    *r = *r + 1;
}
// r out of scope → auto unmod

thread2() {
    int& r = counter;
    mod(r);                          // blocks if thread1 still holds the lock
    *r = *r + 1;
}

main() {
    spawn_thread(thread1);
    spawn_thread(thread2);
    join_all();
    free(counter);                  // main is owner
    return 0;
}

// === __thread (thread-local) ===
__thread int local = 42;            // per-thread independent
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
