# UltraCPP 0.2.0 Language Specification

> **Version**: 0.2.0
>
> **Previous version**: 0.1.0 — [`UltraCPP-v0.1.0-spec-en.md`](./UltraCPP-v0.1.0-spec-en.md)
>
> **Status**: draft — pending user acceptance
>
> **Date**: 2026-08-07
>
> **Authority**: The Chinese edition [`UltraCPP-v0.2.0-spec-zh-CN.md`](./UltraCPP-v0.2.0-spec-zh-CN.md) is the **authoritative** version. This English edition is a companion translation; where the two disagree, the Chinese edition governs.
>
> **Implementation decision**: This revision is made on the basis of the decision that the `src-c/` C host is the production compiler (2026-08).

---

## Revision Summary

This version applies 8 language design decisions (D-1 .. D-8) on top of 0.1.0. Every section not listed below is **verbatim identical to 0.1.0**.

| Decision | One-line summary |
|----------|------------------|
| **D-1** | `unique` is a generic type constructor usable over any type as `unique T`; it is no longer hard-coded to `unique int`. |
| **D-2** | Unary prefix `&` means **immutable borrow**, not "address-of". |
| **D-3** | `&mut` enters the language: new token, new type `T&mut`, new expression production. |
| **D-4** | `move(p)` is a **built-in primitive** (not sugar): it nulls the source pointer at runtime. |
| **D-5** | Assignment `p1 = p2` on `unique T` is a move: the **source** (`p2`) becomes null, the destination (`p1`) takes ownership. |
| **D-6** | `alloc` / `free` pairing is the programmer's responsibility; the compiler does **not** enforce it. `MissingFree` / `DoubleFree` are recorded as future extensions. |
| **D-7** | Defines the trigger conditions for `DanglingReference`, with positive/negative examples and diagnostic format. |
| **D-8** | This revision is made on the basis of the `src-c/` C-host-as-production-compiler decision (2026-08). |

---

## Changelog

Format: `D-N | §section | one-line description`

| Decision | Section | Description |
|----------|---------|-------------|
| D-1 | §3.7 | `unique` entry rewritten as a generic type constructor; `unique T` is legal for any `T`. |
| D-1 | §12.1 | EBNF adds `unique_type ::= 'unique' type` and wires it into `type`; no longer implies `unique int`. |
| D-2 | §1.3 | Symbol table: `T&` row changed to "immutable borrow of T"; `&expr` row added. |
| D-2 | §2.6 | `&` row changed from "Bitwise AND/address-of" to "unary prefix: reference / borrow (immutable); binary infix: bitwise AND"; "address-of" wording removed. |
| D-2 | §3.3 | Reference-types section rewritten: `T&` is explicitly the **immutable borrow** type. |
| D-2 | §4.5 | Bitwise-operator table annotated: it describes the **binary infix** form only. |
| D-2 | §7.8 | **New** "Immutable borrow `&T`": creation rules and coexistence rules. |
| D-2 | §12.3 | Precedence table level 8 `&` narrowed to "bitwise AND (binary infix)". |
| D-3 | §1.3 | Symbol table adds `T&mut` and `&mut expr` rows. |
| D-3 | §2.2 | Token table: `&mut` added to Operators; `mut` added to Keywords. |
| D-3 | §2.3 | Reserved-word list adds `mut`. |
| D-3 | §2.6 | Operator table adds the `&mut` token row. |
| D-3 | §3.3 | New type `T&mut` (mutable borrow). |
| D-3 | §7.9 | **New** "Mutable borrow `&mut T`": exclusivity rules. |
| D-3 | §12.1 | EBNF: `unary_expression` gains `'&' 'mut' unary_expression`; `type` gains `mutable_reference_type ::= type '&' 'mut'`. |
| D-3 | §12.2 | Reserved-keyword list adds `mut`. |
| D-4 | §7.4 | `move(p)` rewritten from "function / sugar" to "built-in primitive"; runtime nulling of the source made normative; cites devhandbook §4.8 `record_move` as the implementation reference. |
| D-4 | §1.3 | `move(ptr)` row reworded as "built-in primitive". |
| D-5 | §7.1 | New normative rule: in `p1 = p2` the **source** `p2` becomes null and the destination `p1` takes ownership; move points for arguments and returns added. |
| D-5 | §7.1 / §3.2 | **Erratum finding**: 0.1.0 reads `int* p2 = p1; // p1 becomes null`, where `p1` is the **source** — this **agrees** with Rust move semantics, so no correction was needed; only the explicit rule was added. See "Note on D-5" below. |
| D-6 | §11.5 | **New** "`alloc` / `free` pairing convention": programmer's responsibility, not compiler-enforced; `MissingFree` / `DoubleFree` recorded as future direction. |
| D-7 | §7.10 | **New** "Dangling Reference": two trigger conditions (a)/(b), negative + positive examples, diagnostic format. |
| D-8 | Header | Added: this revision is based on the `src-c/` C host production-compiler decision (2026-08). |
| D-8 | Appendix 12.4 | **New** "Impact on future work". |

### Note on D-5

Decision D-5 is phrased as `p1 = p2` (`p1` destination, `p2` source), whereas the 0.1.0 examples read `int* p2 = p1;` (`p2` destination, `p1` source). The roles of `p1` / `p2` are **exactly reversed** between the two.

Checking 0.1.0 §3.2, §7.1 and §7.4 line by line confirms: **all three null the source**, which matches Rust move semantics. There is **no semantic error**, so this version leaves those examples untouched and only adds a naming-independent normative rule in §7.1.

### Note on section numbering

- D-7 asks for "§7.7 Dangling Reference", but §7.7 in 0.1.0 is already "Pointer Arithmetic". To **keep unchanged sections unchanged**, the new sections are numbered §7.8 / §7.9 / §7.10 and §7.7 stays put.
- D-6 asks for "a section in §9 (standard library)", but §9 in 0.1.0 is "Modules and Preprocessor"; the standard library is §11. The section was therefore written as §11.5.

---

## 1. Overview

### 1.1 Language Goals

UltraCPP is a systems programming language that combines **C++ syntax familiarity** with **Rust-style memory safety guarantees**, without a garbage collector.

**Core Goals:**
- Zero-cost abstractions
- Compile-time memory safety
- C++ compatibility for easy migration
- No garbage collector

### 1.2 Design Principles

1. **Safety by Default**: Enforce memory safety at compile time whenever possible
2. **Explicit over Implicit**: Ownership, mutability, and safety semantics are visible in syntax
3. **Pragmatic Compatibility**: Leverage C++ developer familiarity
4. **Minimal Runtime**: No heavy runtime, suitable for systems programming

### 1.3 Symbol Conventions *(0.2.0 revised: D-2, D-3, D-4)*

| Symbol | Meaning |
|--------|---------|
| `T*` | Pointer to T (non-copyable by default, assignment transfers ownership) |
| `unique T` | Exclusive-ownership pointer to T (generic form, see §3.7) |
| `T&` | **Immutable borrow** of T |
| `T&mut` | **Mutable borrow** of T |
| `&expr` | Create an immutable borrow of `expr` (see §7.8) |
| `&mut expr` | Create a mutable borrow of `expr` (see §7.9) |
| `T(*)(U)` | Function pointer type (C++ style) |
| `alloc(T)` | Allocate memory for type T |
| `free(ptr)` | Free memory |
| `move(ptr)` | **Built-in primitive**: transfer ownership (source becomes null at runtime, see §7.4) |
| `clone(ptr)` | Clone pointer (copy, independent ownership) |

---

## 2. Lexical Structure

### 2.1 Source File Conventions

```
*.uc   — UltraCPP source file (recommended)
*.upp  — UltraCPP source file (compatible)
```

### 2.2 Token Types *(0.2.0 revised: D-3)*

| Category | Examples |
|----------|----------|
| **Keywords** | `if`, `else`, `while`, `for`, `return`, `struct`, `export`, `import`, `const`, `unique`, `mut`, `move`, `free`, `alloc`, `null`, `true`, `false`, `void`, `extern`, `unsafe`, `typedef`, `clone` |
| **Identifiers** | `foo`, `myVariable`, `_private`, `CamelCase` |
| **Literals** | `42`, `3.14`, `'x'`, `"hello"`, `true`, `false` |
| **Operators** | `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`, `&`, `&mut`, `|`, `^`, `~`, `<<`, `>>`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=` |
| **Delimiters** | `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `::` |
| **Preprocessor** | `#import`, `#include`, `#ifdef`, `#ifndef`, `#endif`, `#define` |

> **[0.2.0 · D-3]** `&mut` is a **single token**. After reading `&` the lexer must look ahead for an adjacent `mut` keyword and merge; whitespace between `&` and `mut` is allowed. `mut` also becomes a reserved word and may not be used as an identifier.

### 2.3 Keywords (Reserved) *(0.2.0 revised: D-3)*

```
if          else        while       for         return
struct      export      import      const       typedef
unique      move        free        alloc       null
true        false       void        extern      unsafe
as          static      clone       mut
```

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

#### Floating-Point Literals
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
true  // Boolean true
false // Boolean false
```

### 2.6 Operators and Delimiters *(0.2.0 revised: D-2, D-3)*

| Operator | Description |
|----------|-------------|
| `+` | Addition |
| `-` | Subtraction/negation |
| `*` | Multiplication/dereference |
| `/` | Division |
| `%` | Modulo |
| `=` | Assignment (triggers ownership transfer for pointer types, see §7.1) |
| `==` | Equality |
| `!=` | Inequality |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |
| `&&` | Logical AND |
| `\|\|` | Logical OR |
| `!` | Logical NOT |
| `&` | **Unary prefix: reference / borrow (immutable, see §7.8)**; binary infix: bitwise AND |
| `&mut` | **Unary prefix: reference / borrow (mutable, see §7.9)** |
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

> **[0.2.0 · D-2]** 0.1.0 described `&` as "Bitwise AND/address-of". This version **removes the address-of meaning**: UltraCPP has no separate address-of operator, and unary prefix `&` is always an **immutable borrow**. Binary infix `&` (as in `a & b`) is still bitwise AND; the two forms are distinguished by grammatical position (see `bitwise_and_expression` vs. `unary_expression` in §12.1).

### 2.7 Comments

```cpp
// Single-line comment

/*
 * Multi-line comment
 */
```

### 2.8 Whitespace

Spaces, tabs, and newlines are ignored when separating tokens. Line endings are preserved for error reporting.

---

## 3. Type System

### 3.1 Fundamental Types

| Type | Description | Size |
|------|-------------|------|
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
| `f32` | 32-bit floating | 4 bytes |
| `f64` | 64-bit double | 8 bytes |
| `usize` | Unsigned size | Platform-dependent |
| `isize` | Signed size | Platform-dependent |

### 3.2 Pointer Types

**All pointers are non-copyable by default; assignment transfers ownership.**

```cpp
T*              // Pointer to T (non-copyable, assignment transfers ownership)
const T*        // Read-only pointer to const T
T* const        // Constant pointer (pointer itself immutable)
const T* const  // Constant pointer to const T
void*           // Generic pointer (unsafe)
```

**Examples:**
```cpp
int* p1;              // Pointer to int, non-copyable
int* p2 = p1;         // p1 becomes null, p2 gets ownership
int* p3 = clone(p1);  // Clone: both pointers independent

const int* cp;        // Read-only pointer to immutable int
int* const cp2;       // Constant pointer to mutable int
const int* const cp3; // Constant pointer to immutable int
void* vptr;           // Generic pointer
```

> **[0.2.0 · D-5]** In `int* p2 = p1;` above, the **source** is `p1` and the **destination** is `p2`; the comment "p1 becomes null" therefore means **the source is nulled**, consistent with the normative rule in §7.1.

### 3.3 Reference Types (Borrow Types) *(0.2.0 revised: D-2, D-3)*

UltraCPP has two borrow types. Both **must be initialized**, **cannot be null**, and **do not own** the referenced value — ownership stays with the owner.

```cpp
T&     // Immutable borrow of T: read-only, many may coexist
T&mut  // Mutable borrow of T: writable, at most one at a time
```

**`T&` — immutable borrow**

`T&` is **the type of an immutable borrow**. A `T&` may only read the borrowed value; any write through it is a compile-time error. An owner may have any number of live `T&` borrows at once.

**Side-by-side: 0.1.0 form vs 0.2.0 form** (0.2.0 is the recommended form):

**0.1.0 form (retained for comparison)**

```cpp
int x = 42;
int& r = x;      // 0.1.0 spelling: implicit address-of, equivalent to &x
r = 100;         // write through the reference (C++ semantics)
```

**0.2.0 form (recommended)**

```cpp
int x = 42;
int& r1 = &x;    // immutable borrow, explicit address-of
int& r2 = &x;    // ✅ multiple immutable borrows may coexist
int y = r1;      // ✅ read
// r1 = 100;     // ❌ error: cannot write through an immutable borrow
```

**Comparison**:
- (i) 0.2.0 requires the initializer of a borrow type to be a **borrow expression** `&x`, so the borrow point is visible in the syntax (design principle §1.2, "Explicit over Implicit").
- (ii) The 0.1.0 form `int& r = x;` (no `&`) is retained as a **compatibility spelling**; a compiler may treat it as an implicit `&x` and emit a hint. Under 0.2.0, this spelling still preserves the immutable-borrow semantics (writes remain forbidden).

**`T&mut` — mutable borrow**

`T&mut` permits writes through the borrow. An owner may have **at most one** live `T&mut`, and it may not coexist with any `T&`.

**0.2.0 form (recommended; no 0.1.0 counterpart)**

```cpp
int x = 42;
int&mut m = &mut x;  // mutable borrow
m = 100;             // ✅ modifies x; x == 100 afterwards
// int& r = &x;      // ❌ error: m is still live; immutable and mutable borrows cannot coexist
```

Creation rules are in §7.8 (immutable) and §7.9 (mutable); lifetime constraints are in §7.10.

> **[0.2.0 · D-2]** 0.1.0 wrote `int& r = x;` (no `&`). This version standardizes on `int& r = &x;`: the initializer of a borrow type must be a **borrow expression**, so the borrow point is visible in the syntax (design principle §1.2, "Explicit over Implicit"). The 0.1.0 form is retained as a compatibility spelling; a compiler may treat it as an implicit `&x` and emit a hint.

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
T[n]  // Fixed-size array of n elements of type T
```

**Examples:**
```cpp
int[10] arr;           // Array of 10 ints
char[256] buffer;      // 256-byte buffer
int[3] nums = [1, 2, 3];  // Initialized array
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

### 3.7 Type Modifiers *(0.2.0 revised: D-1, D-3)*

| Modifier | Meaning |
|---------|---------|
| `const` | Value cannot be modified |
| `unique` | **Generic type constructor**: `unique T` is the exclusive-ownership pointer to any type `T` (same as the `T*` default, can be omitted) |
| `mut` | Appears only as part of the `&mut` token (see §2.6); it is not a standalone type modifier |
| `static` | Internal linkage |

#### 3.7.1 `unique` is a generic type constructor *(0.2.0 new: D-1)*

> **[0.2.0 · D-1]** `unique` is a **generic type constructor** and may be applied to any type as `unique T`. It is **not** hard-coded to `unique int`, and not a keyword that modifies a single built-in type.

Formally: for any type `T`, `unique T` is a legal type whose runtime representation is identical to `T*` and whose static meaning is "exclusive ownership of `T`".

```cpp
unique int      x1;   // equivalent to int*
unique char     x2;   // equivalent to char*
unique Point    x3;   // user-defined struct is equally legal
unique int*     x4;   // T may itself be a pointer type
unique int[10]  x5;   // T may be an array type
```

**Rules:**

1. `T` in `unique T` may be **any** type from §3.1–§3.6, including user-defined `struct`s, array types, and pointer types.
2. `unique T` and `T*` are **equivalent** in the type system; `unique` just spells out the default exclusive-ownership property.
3. `unique` may not be applied to borrow types: `unique T&` and `unique T&mut` are **illegal** — a borrow does not own its referent, so exclusive ownership is meaningless.
4. For the grammatical form, see `unique_type ::= 'unique' type` in §12.1.

**Implementation note**: as of 0.1.0 both parsers lowered `unique` unconditionally to `Pointer(Int)` (`src-c/src/parser.c` and `src/frontend/parser.rs`). That conflicts with this rule and is an implementation defect to be fixed per this section.

---

## 4. Expressions

### 4.1 Operator Precedence and Associativity

| Precedence | Operators | Associativity |
|------------|-----------|---------------|
| 1 | `::` | Left to right |
| 2 | `()` `[]` `.` `->` `++` `--` | Left to right |
| 3 | `*` `/` `%` | Left to right |
| 4 | `+` `-` | Left to right |
| 5 | `<<` `>>` | Left to right |
| 6 | `<` `>` `<=` `>=` | Left to right |
| 7 | `==` `!=` | Left to right |
| 8 | `&` | Left to right |
| 9 | `^` | Left to right |
| 10 | `\|` | Left to right |
| 11 | `&&` | Left to right |
| 12 | `\|\|` | Left to right |
| 13 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | Right to left |
| 14 | `?:` (ternary) | Right to left |
| 15 | `move` `clone` | - |

> **[0.2.0 · D-2, D-3]** The `&` at level 8 is the **binary infix** bitwise AND. Unary prefix `&` (immutable borrow) and `&mut` (mutable borrow) are unary operators at the same level as `*`, `!`, `~`, binding tighter than every binary operator.

### 4.2 Arithmetic Operators

| Operator | Description | Example |
|---------|-------------|---------|
| `+` | Addition | `a + b` |
| `-` | Subtraction | `a - b` |
| `*` | Multiplication | `a * b` |
| `/` | Division | `a / b` |
| `%` | Modulo | `a % b` |

### 4.3 Comparison Operators

| Operator | Description | Example |
|---------|-------------|---------|
| `==` | Equality | `a == b` |
| `!=` | Inequality | `a != b` |
| `<` | Less than | `a < b` |
| `>` | Greater than | `a > b` |
| `<=` | Less than or equal | `a <= b` |
| `>=` | Greater than or equal | `a >= b` |

### 4.4 Logical Operators

| Operator | Description | Example |
|---------|-------------|---------|
| `&&` | Logical AND | `a && b` |
| `\|\|` | Logical OR | `a \|\| b` |
| `!` | Logical NOT | `!a` |

### 4.5 Bitwise Operators *(0.2.0 revised: D-2)*

> **[0.2.0 · D-2]** This table describes the **binary infix** form only. Unary prefix `&` is not a bitwise operator; it is an immutable borrow (see §7.8).

| Operator | Description | Example |
|---------|-------------|---------|
| `&` | Bitwise AND (binary infix) | `a & b` |
| `\|` | Bitwise OR | `a \| b` |
| `^` | Bitwise XOR | `a ^ b` |
| `~` | Bitwise NOT | `~a` |
| `<<` | Left shift | `a << 2` |
| `>>` | Right shift | `a >> 2` |

### 4.6 Assignment Operators

| Operator | Description |
|----------|-------------|
| `=` | Simple assignment (triggers ownership transfer for pointer types) |
| `+=` | Addition assignment |
| `-=` | Subtraction assignment |
| `*=` | Multiplication assignment |
| `/=` | Division assignment |
| `%=` | Modulo assignment |
| `&=` | Bitwise AND assignment |
| `\|=` | Bitwise OR assignment |
| `^=` | Bitwise XOR assignment |
| `<<=` | Left shift assignment |
| `>>=` | Right shift assignment |

### 4.7 Conditional Operator

```cpp
condition ? expr1 : expr2
```

### 4.8 Parenthesized Expressions

```cpp
(expr)  // Any expression in parentheses
```

---

## 5. Statements

### 5.1 Expression Statements

```cpp
x + y;        // Evaluate and discard
func(10);     // Function call
```

### 5.2 Compound Statements (Blocks)

```cpp
{
    int x = 10;
    int y = 20;
    x = x + y;
}
```

### 5.3 if Statements

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

### 5.4 while Statements

```cpp
while (condition) {
    // code
}
```

### 5.5 for Statements

```cpp
for (int i = 0; i < 10; i++) {
    // code
}

for (int x : array) {
    // code
}
```

### 5.6 return Statements

```cpp
return;              // Return void
return 42;           // Return value
return x + y;        // Return expression result
```

### 5.7 break Statements

```cpp
while (true) {
    if (condition) {
        break;       // Exit loop
    }
}
```

### 5.8 continue Statements

```cpp
for (int i = 0; i < 10; i++) {
    if (i % 2 == 0) {
        continue;    // Skip iteration
    }
}
```

### 5.9 free Statements

```cpp
int* p = alloc(int);
*p = 42;
free(p);  // Free memory
```

### 5.10 Declaration Statements

```cpp
int x;                // Variable declaration
int x = 42;           // Declaration with initializer
const int y = 100;    // Constant
int* p = alloc(int);  // Pointer allocation
```

---

## 6. Functions

### 6.1 Function Definitions

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

### 6.2 Function Calls

```cpp
int result = add(10, 20);
greet("Hello");
int fact = factorial(5);
```

### 6.3 Parameter Passing

**Pass-by-value semantics:**
- Parameters are copied to function parameters
- Modifying parameters doesn't affect arguments

```cpp
void inc(int x) {
    x = x + 1;  // Doesn't affect caller
}

int n = 10;
inc(n);
// n is still 10
```

**Mutable-borrow parameters for modification (side-by-side: 0.1.0 vs 0.2.0):**

**0.1.0 form (retained for comparison)**

```cpp
void inc(int& x) {     // 0.1.0: T& doubles as a writable parameter
    x = x + 1;         // write through the reference
}

int n = 10;
inc(n);                // 0.1.0: call site passes the bare variable
// n is now 11
```

**0.2.0 form (recommended)**

```cpp
void inc(int&mut x) {  // 0.2.0: mutable-borrow parameter
    x = x + 1;         // Affects caller
}

int n = 10;
inc(&mut n);           // 0.2.0: call site spells out &mut n
// n is now 11
```

**Comparison**:
- (i) Because 0.2.0's `T&` is an **immutable** borrow (§3.3), writing through it is a compile-time error, so a "modify the caller's value" parameter must be typed `T&mut` (a combined effect of decisions D-2 and D-3).
- (ii) The call site `&mut n` makes the mutable-borrow intent explicit.
- (iii) Read-only parameters keep using `T&`:
  ```cpp
  int read(int& x) { return x; }   // read-only, immutable borrow
  int n = 10;
  int v = read(&n);
  ```

> **[0.2.0 · D-2, D-3]** 0.1.0 wrote `void inc(int& x)` called as `inc(n)`. Since `T&` is now explicitly **immutable** (§3.3), writing through it is a compile-time error, so a "modify the caller's value" parameter must be typed `int&mut` and the call site must spell out `&mut n`. Read-only parameters keep using `T&`.

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
    return p;  // Return by value (copy)
}
```

> **[0.2.0 · D-7]** Returning a local **by value** (as in `return p;` above) is always legal. Returning a **borrow** of a local is a dangling reference; see §7.10.

### 6.5 main Function Convention

```cpp
int main() {
    // program code
    return 0;
}
```

The `main` function:
- Returns `int`
- Can take no arguments, or:
  - `int main(int argc, char** argv)`

---

## 7. Memory Management

### 7.1 Ownership Semantics *(0.2.0 revised: D-5)*

**All pointers are non-copyable by default; assignment triggers ownership transfer (move semantics).**

```cpp
int* p1 = alloc(int);
int* p2 = p1;    // p1 becomes null, ownership transfers to p2
int* p3 = p2;    // p2 becomes null, ownership transfers to p3
```

#### 7.1.1 The move rule for assignment *(0.2.0 new: D-5)*

> **[0.2.0 · D-5]** Assignment `p1 = p2` on a `unique T` (equivalently, `T*`) is **move semantics**: `p2` (the **source**) becomes null after the assignment, and `p1` takes over ownership.

Normative statement, independent of variable naming:

> Let the assignment be `dst = src`, where both `dst` and `src` have type `unique T`. After evaluation:
> 1. `dst` becomes the **sole owner** of the allocation;
> 2. the runtime value of `src` becomes `null`;
> 3. `src` is statically marked **moved-out**; any subsequent **use** of `src` (read, dereference, re-move, pass as argument, `free`) is a compile-time `UseAfterMove` error;
> 4. **re-assigning** `src` (`src = <new value>`) is legal and returns it to the initialized state.

Example (`p1` destination, `p2` source, matching the D-1..D-8 naming):

```cpp
unique int p2 = alloc(int);
*p2 = 7;

unique int p1 = p2;   // move: p2 becomes null, p1 takes ownership

// int v = *p2;       // ❌ compile-time UseAfterMove: p2 has been moved out
int v = *p1;          // ✅ v == 7
free(p1);             // ✅ freed by the new owner
```

**Other places where a move occurs** (all four rules above apply):

| Site | Source | Destination |
|------|--------|-------------|
| Initialization `unique T d = s;` | `s` | `d` |
| Assignment `d = s;` | `s` | `d` |
| Call argument `f(s)` (parameter typed `unique T`) | `s` | the parameter |
| Return `return s;` (return type `unique T`) | `s` | the caller's receiving site |
| Explicit `move(s)` (see §7.4) | `s` | the `move` expression's result |

**Places where no move occurs**: `clone(s)` (§7.5), `&s` and `&mut s` (borrows, §7.8 / §7.9), and comparisons such as `s == null` that merely read the pointer value.

**Erratum check**: 0.1.0 §7.1 and §3.2 read `int* p2 = p1; // p1 becomes null`. There `p1` is the **source**, so "the source is nulled" already matches Rust move semantics. The original text is **correct** and is retained.

### 7.2 alloc Function

```cpp
int* alloc(int)                  // Allocate single element
int* alloc(int, int count)      // Allocate array of count elements
```

**Examples:**
```cpp
int* p = alloc(int);         // Single int
int* arr = alloc(int, 10);   // Array of 10 ints
char* buf = alloc(char, 256); // 256-byte buffer
```

### 7.3 free Function

```cpp
free(ptr)    // Free memory
```

**Examples:**
```cpp
int* p = alloc(int);
*p = 42;
free(p);

int* arr = alloc(int, 10);
free(arr);
```

> **[0.2.0 · D-6]** Pairing `alloc` with `free` is the programmer's responsibility; the compiler does **not** enforce it. See §11.5.

### 7.4 The move(p) Expression *(0.2.0 revised: D-4)*

> **[0.2.0 · D-4]** `move(p)` is a **built-in primitive** of the language — **not** syntactic sugar and **not** an ordinary function. 0.1.0 titled this section "move Function" and described it as shorthand for assignment behavior; this version promotes it to a first-class semantic construct.

**Syntax** (see `unary_expression` in §12.1):

```
move '(' expression ')'
```

**Semantics:**

1. The **operand must be an addressable lvalue** of type `unique T`. Forms with rvalue operands such as `move(alloc(int))` or `move(f())` are compile-time errors — an rvalue has no owner to invalidate.
2. The **result** of `move(p)` is the pointer value `p` previously held, of type `unique T`.
3. **Runtime effect**: after evaluating `move(p)`, the compiler **must** emit code that stores `null` into `p`'s storage. This is the crux of the decision — nulling the source is **observable runtime behavior**, not merely a compile-time marker. The following therefore has defined behavior:

   ```cpp
   unique int p1 = alloc(int);
   unique int p2 = move(p1);
   // at this point p1's storage really does hold null
   ```

4. **Static effect**: `p` is marked moved-out; any subsequent use of `p` is a compile-time `UseAfterMove` error (same rules as §7.1.1).
5. `move` **allocates nothing** and copies no referent; it only transfers ownership.
6. `move(p)` is **semantically identical** to an implicit move (`q = p`). Its value is making the transfer point visible in the source, and disambiguating positions such as call arguments where an implicit move could be misread.

```cpp
unique int p1 = alloc(int);
*p1 = 42;

unique int p2 = move(p1);   // p1 becomes null at runtime; statically marked moved-out

if (p1 == null) {           // ❌ compile-time UseAfterMove: p1 has been moved out
    // even though p1 really is null at runtime, the static check still rejects
    // any read of a moved-out variable
}

*p2 = 43;                   // ✅
free(p2);                   // ✅
```

**Implementation reference**: the C host's ownership checker should implement the static marking per `.dev/plans/0.1.0-devhandbook.md` §4.8 "移动语义" (`record_move(OwnershipChecker* checker, const char* var, size_t location)`); the runtime null store is emitted by codegen.

### 7.5 clone Function

`clone()` copies the pointer; both have independent ownership.

```cpp
int* p1 = alloc(int);
int* p2 = clone(p1);  // p1 and p2 independent, each must be freed
```

### 7.6 null Constant

```cpp
int* p = null;      // Null pointer
if (p == null) {    // Comparison
    // p is null
}
```

### 7.7 Pointer Arithmetic

```cpp
int* ptr = alloc(int, 10);
ptr[0] = 1;          // Index access
ptr[5] = ptr[0];     // Copy value

int* p1 = alloc(int, 10);
int* p2 = p1 + 5;   // Offset pointer
```

### 7.8 Immutable Borrow `&T` *(0.2.0 new: D-2)*

> **[0.2.0 · D-2]** New section defining the semantics of unary prefix `&`. 0.1.0 had no borrow rules and described `&` as "address-of".

**Syntax** (see `unary_expression` in §12.1):

```
'&' unary_expression
```

**`&expr` creates an immutable borrow** of the object designated by `expr`, of type `T&` where `T` is the type of `expr`. A borrow **does not transfer ownership**: the owner of `expr` is unchanged and is not nulled.

#### 7.8.1 Creation rules

1. **The operand must be an lvalue.** `&x`, `&s.field`, `&arr[i]`, `&*p` are legal; `&42`, `&(a + b)`, `&f()` are compile-time errors.
2. **The owner must be live and initialized.** Borrowing a moved-out variable is `UseAfterMove`; borrowing a freed allocation is `UseAfterDrop`.
3. **The borrow is read-only.** Writing through a `T&` (`r = v;`, `*r = v;`, `r.field = v;`) is a compile-time error.
4. **A borrow may not outlive its owner.** Violations are reported as `DanglingReference`; see §7.10.
5. **`&` does not change the owner's ownership state**, so after `&p` the variable `p` is still usable and still needs to be freed.

#### 7.8.2 Coexistence rules

- An owner may have **any number** of live `T&` borrows at once.
- A `T&` may **not** coexist with any live `T&mut` of the same owner (see §7.9).
- A borrow is live from its creation point to its **last use** (non-lexical, matching the borrow-check algorithm in devhandbook §4.7).

```cpp
int x = 42;
int& r1 = &x;
int& r2 = &x;        // ✅ multiple immutable borrows coexist
int sum = r1 + r2;   // ✅ reads
// r1 = 100;         // ❌ error: cannot write through an immutable borrow
x = 7;               // ✅ r1 / r2 are not used after this point; borrows have ended
```

Borrowing through an owned allocation:

```cpp
unique int p = alloc(int);
*p = 10;

int& r = &*p;    // borrow the int that p points to; p is still the owner
int v = r;       // ✅ v == 10

free(p);         // ✅ r's last use was before this point
```

#### 7.8.3 Diagnostic

```
error[E0502]: cannot assign through an immutable borrow
  --> src/main.uc:4:5
   |
 3 |     int& r1 = &x;
   |               --- immutable borrow of `x` created here
 4 |     r1 = 100;
   |     ^^^^^^^^ cannot write through `int&`
   |
   = help: use `int&mut r1 = &mut x;` if you need to modify `x`
```

### 7.9 Mutable Borrow `&mut T` *(0.2.0 new: D-3)*

> **[0.2.0 · D-3]** New section. 0.1.0's grammar had no `&mut` at all, which left the borrow-conflict rules of devhandbook §4.5–§4.7 unreachable from the specified grammar. This section closes that gap.

**Syntax** (see `unary_expression` in §12.1):

```
'&' 'mut' unary_expression
```

**`&mut expr` creates a mutable borrow** of the object designated by `expr`, of type `T&mut`. As with `&`, a mutable borrow **does not transfer ownership**.

#### 7.9.1 Creation rules

1. **The operand must be an lvalue** (same as §7.8.1 rule 1).
2. **The owner must be mutable.** Taking `&mut` of a `const` variable, or of the referent of a `const T*`, is a compile-time error.
3. **The owner must be live and initialized** (same as §7.8.1 rule 2).
4. **A mutable borrow is writable.** Both reads and writes through a `T&mut` are legal.
5. **A borrow may not outlive its owner** (see §7.10).

#### 7.9.2 Exclusivity rules

These are the core borrow-check constraints, matching devhandbook §4.5:

1. **Many** immutable borrows `T&` may coexist;
2. **at most one** mutable borrow `T&mut` may be live;
3. immutable and mutable borrows **may not coexist**.

Violating any of the three is a `BorrowConflict`.

```cpp
int x = 42;

int&mut m = &mut x;   // ✅ the one mutable borrow
m = 100;              // ✅ write; x == 100 afterwards
int v = m;            // ✅ read

// While m is still live, all three of the following are errors:
// int&mut m2 = &mut x;  // ❌ BorrowConflict: second mutable borrow
// int&  r   = &x;       // ❌ BorrowConflict: no immutable borrow while a mutable one is live
// int   w   = x;        // ❌ BorrowConflict: no read through the owner while mutably borrowed
```

Once the borrow ends, the owner is fully usable again:

```cpp
int x = 42;
{
    int&mut m = &mut x;
    m = 100;              // last use of m
}
int v = x;                // ✅ borrow has ended; v == 100
int& r = &x;              // ✅ immutable borrow is now allowed
```

#### 7.9.3 Diagnostic

```
error[E0499]: cannot borrow `x` as mutable more than once at a time
  --> src/main.uc:4:18
   |
 3 |     int&mut m  = &mut x;
   |                  ------ first mutable borrow of `x` occurs here
 4 |     int&mut m2 = &mut x;
   |                  ^^^^^^ second mutable borrow of `x` occurs here
 5 |     m = 100;
   |     ------- first borrow is still in use here
   |
   = note: `&mut T` grants exclusive access; only one may be live at a time
```

### 7.10 Dangling Reference *(0.2.0 new: D-7)*

> **[0.2.0 · D-7]** This section defines, at the user-language level, what triggers devhandbook §4.6's `DanglingReference`. 0.1.0 defined the `T&` type in §3.3 but never said when it becomes invalid.

**Definition**: a `T&` or `T&mut` borrow is **dangling** when it remains reachable after its owner's storage has been destroyed. UltraCPP rejects, at compile time, every dangling reference it can determine statically; the error kind is `DanglingReference`.

#### 7.10.1 Trigger conditions

The compiler reports `DanglingReference` in the following two cases:

**(a) The owner of a `&T` / `&mut T` has gone out of scope.**
The borrow's live range extends past the end of the owner's scope. The owner may be a block-local variable, or an allocation released by `free` (the latter falls under the same rule: `free(p)` ends the storage lifetime of `p`'s referent).

**(b) A function returns a reference whose owner is a local variable of that function.**
The return type is `T&` or `T&mut`, and the returned expression borrows a function-local variable, a local array, or an allocation made inside the function and not transferred out. This is a special case of (a) — every local goes out of scope at function exit — but it is listed separately because it is the most common shape and warrants a distinct diagnostic.

**Not triggered:**

- The borrow's owner is reached through a **borrow parameter** (`T&` / `T&mut`) and the return lifetime is supplied by that parameter — the borrow is merely forwarded, and the owner is still live in the caller.
- The borrow's last use precedes the owner going out of scope.
- Returning **by value** (`T`, `unique T`) — the value is moved or copied out, producing no borrow.

#### 7.10.2 Example (a): owner goes out of scope

**Negative example (must be rejected):**

```cpp
int main() {
    int& r;               // not yet initialized borrow
    {
        int x = 42;
        r = &x;           // borrow x
    }                     // ← x goes out of scope here
    return r;             // ❌ DanglingReference: r points at the destroyed x
}
```

Diagnostic:

```
error[E0597]: `x` does not live long enough
  --> src/main.uc:5:5
   |
 4 |         int x = 42;
   |             - binding `x` declared here
 5 |         r = &x;
   |             ^^ borrowed value does not live long enough
 6 |     }
   |     - `x` dropped here while still borrowed
 7 |     return r;
   |            - borrow later used here
   |
   = note: the borrow must not outlive the owner's scope
```

**Positive example (must be accepted):**

```cpp
int main() {
    int result;
    {
        int x = 42;
        int& r = &x;      // borrow x
        result = r;       // last use of r; the borrow ends here
    }                     // ← no live borrow when x goes out of scope
    return result;        // ✅
}
```

#### 7.10.3 Example (b): returning a reference to a local

**Negative example (must be rejected):**

```cpp
int& make_answer() {
    int x = 42;
    return &x;            // ❌ DanglingReference: x is a local
}
```

Diagnostic:

```
error[E0106]: cannot return reference to local variable `x`
  --> src/main.uc:3:12
   |
 2 |     int x = 42;
   |         - `x` is a local variable of `make_answer`
 3 |     return &x;
   |            ^^ returns a reference to data owned by the current function
   |
   = help: return `int` by value, or return `unique int` to transfer ownership
```

**Positive example — forwarding a parameter borrow:**

```cpp
int& first(int& a, int& b) {
    return &a;            // ✅ the owner is in the caller; the borrow is forwarded
}

int main() {
    int x = 1;
    int y = 2;
    int& r = first(&x, &y);
    return r;             // ✅ x is still live in main
}
```

**Positive example — returning by value or by ownership transfer:**

```cpp
int make_answer_by_value() {
    int x = 42;
    return x;             // ✅ returned by value (copy)
}

unique int make_answer_owned() {
    unique int p = alloc(int);
    *p = 42;
    return p;             // ✅ move: ownership transfers to the caller (§7.1.1)
}
```

#### 7.10.4 Interaction with `free`

`free(p)` ends the storage lifetime of `p`'s referent. Borrows still live afterwards also trigger `DanglingReference`:

```cpp
int main() {
    unique int p = alloc(int);
    *p = 10;

    int& r = &*p;
    free(p);              // ← storage destroyed here
    return r;             // ❌ DanglingReference: r points into a freed allocation
}
```

**Implementation reference**: lifetime inference follows `.dev/plans/0.1.0-devhandbook.md` §12.4 "借用检查算法"; the error kind reuses §4.6's `DanglingReference { size_t location, size_t owner_location }`, whose two positions correspond to "borrow later used here" and "binding declared here" in the diagnostics above.

---

## 8. struct Types

### 8.1 struct Definitions

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

Structs use **sequential layout** with padding for alignment:

```cpp
struct Example {
    char  c;    // 1 byte
    int   i;    // 4 bytes (may have 3 bytes padding)
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

## 9. Modules and Preprocessor

### 9.1 Preprocessing Phase

UltraCPP runs a **preprocessing phase** before lexical analysis to handle directives:

| Directive | Behavior |
|-----------|----------|
| `#include "path"` | Expand file contents at this location |
| `#import "module"` | Record module dependency (don't expand contents) |
| `export <declaration>` | Mark function/variable as exported |

**Preprocessor Output:**
1. Pure code after `#include` expansion
2. Module dependency list
3. Exported symbol list

### 9.2 #include Syntax and Semantics

**Source Copy**: `#include` literally copies the specified file's content to the `#include` directive location.

```cpp
#include "file_path"
```

**Examples:**
```cpp
// main.upp
#include "../../lib/math.uc"

int main() {
    int result = add(5, 3);  // add() defined in math.uc
    return result;
}
```

**Behavior:**
- Code after `#include` directly gains all definitions from the included file
- No additional declarations needed to call functions from the included file
- File paths are resolved relative to the **current source file directory**

**Search Path Order:**
1. Relative to current source file directory
2. Relative to project root
3. Relative to specified include paths

### 9.3 #import Syntax and Semantics

**Module Import**: `#import` records a dependency on a module, **does not expand its contents**.

```cpp
#import "module_path"
#import "module_path" as alias
```

**Examples:**
```cpp
#import "math";           // Import math module
#import "io" as stdio;    // Import io module with alias
```

**Behavior:**
1. Records module dependency but doesn't expand source code
2. Imported modules need separate compilation
3. Calling module functions requires `extern` declaration or `#include` its interface

**Generated Dependency File:**
```json
{
  "module": "math",
  "file": "lib/math.uc",
  "exports": ["add", "multiply"]
}
```

### 9.4 export Declarations

Mark functions or variables as **module exports**, accessible via `#import`.

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
- `export` can only be used at module top level
- Exported functions/variables can be accessed by code that `#import`s this module

### 9.5 Module Design Rules

UltraCPP's module system follows these rules:

1. **Functions don't need declarations to define** - Intra-file function calls don't need forward declarations
2. **External module functions need import** - Calling other module's functions must `#import` that module
3. **Internal functions not externally accessible** - Non-exported functions only usable within module

**Examples:**
```cpp
// lib/math.uc
export int add(int a, int b);  // Export interface

int _internal_helper(int x) {  // Internal function, not externally visible
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
    add(5, 3);           // ✅ Can call
    _internal_helper(1); // ❌ Cannot call (not exported)
    return 0;
}
```

### 9.6 Module Search Paths

| Path Type | Resolution |
|-----------|------------|
| Relative path | Relative to current source file directory |
| Absolute path | Relative to project root |
| Specified path | Via `-I` parameter |

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
#include "a.uc"  // Error: circular dependency detected

int func_b() {
    return 42;
}
```

**Detection Rule**: If two modules `#include` each other, the compiler should report an error.

### 9.8 Compilation Flow

```
Source Code (.uc/.upp)
     │
     ▼
┌─────────────────┐
│  Preprocessing  │
│  - Expand #include│
│  - Record #import │
│  - Parse export  │
└─────────────────┘
     │
     ▼
┌─────────────────┐
│  Lexical Analysis│
│  Parsing        │
│  Code Generation│
└─────────────────┘
     │
     ▼
   LLVM IR (.ll)
     │
     ▼
   LLC (assemble)
     │
     ▼
   Object File (.o)
     │
     ▼
   Linker (gcc/ld)
     │
     ▼
   Executable
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
| `const T*` | `const T*` |
| `void*` | `void*` |
| `int (*)(T)` | `int (*)(T)` |
| `int` | `int` |
| `double` | `double` |
| `char` | `char` |
| `char*` | `char*` |

### 10.3 unsafe Blocks

```cpp
unsafe {
    // FFI calls and raw pointer operations
}
```

**Examples:**
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

> **[0.2.0 · D-2, D-3, D-7]** An `unsafe` block **suppresses** ownership and borrow checking: `UseAfterMove` (§7.1.1), `BorrowConflict` (§7.9.2), and `DanglingReference` (§7.10) are not reported inside it. `unsafe` does **not** change semantics — `move` still nulls the source (§7.4), `&` is still a borrow (§7.8) — it only shifts the safety obligation to the programmer.

### 10.4 Inline Assembly

Inline assembly must be placed in `unsafe` blocks.

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

**Examples:**
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

**Constraint Characters:**
| Constraint | Meaning |
|------------|---------|
| `r` | General register |
| `=` | Output register |
| `&` | Early clobber (must not be reused) |
| `m` | Memory reference |

---

## 11. Standard Library Conventions

### 11.1 Basic I/O Functions

```cpp
void print_num(int n);        // Print integer
void print(const char* s);    // Print string
void print_float(double f);   // Print floating-point
```

### 11.2 String Functions

```cpp
int strlen(const char* s);  // String length
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
int sizeof_impl();       // Type size
int alignof_impl();      // Type alignment
bool is_null(int* ptr);  // Null check
int* clone_impl(int* ptr);  // Pointer clone
```

### 11.5 `alloc` / `free` Pairing Convention *(0.2.0 new: D-6)*

> **[0.2.0 · D-6]** Pairing `alloc(T)` with `free(p)` is **the programmer's responsibility**; the compiler does **not** enforce it.

**Normative rules:**

1. Every successful `alloc(T)` produces a new allocation whose ownership belongs to the `unique T` variable receiving the result.
2. Every allocation **should** be freed exactly once. This is a **programmer obligation**, not a compiler-enforced static constraint.
3. The compiler does **not** report:
   - **missing free** (an allocation with no matching `free`) — a memory leak;
   - **double free** (the same allocation freed twice);
   - **freeing a non-`alloc` value** (calling `free` on a borrow of a stack variable).
4. The compiler **does still** report these related errors, which fall out of ownership tracking rather than pairing analysis:
   - `free` on a moved-out variable → `UseAfterMove` (§7.1.1);
   - any **use** (read, dereference, borrow) of a variable after `free` → `UseAfterDrop`;
   - a live borrow surviving a `free` → `DanglingReference` (§7.10.4).

**Rationale**: 0.2.0 draws the checker's boundary at **ownership and borrowing**, not at **allocation counting**. Deciding `MissingFree` requires whole-program interprocedural reachability; deciding `DoubleFree` requires path-sensitive analysis in the presence of branches and loops. Both exceed this version's implementation scope, and both produce heavy false-positive rates without the full analysis.

**Conventional usage:**

```cpp
unique int p = alloc(int);
*p = 42;
// ... use p ...
free(p);                  // the programmer-maintained pairing
```

When ownership crosses a function boundary, the obligation to `free` travels with it:

```cpp
unique int make() {
    unique int p = alloc(int);
    *p = 42;
    return p;             // ownership transfers to the caller; this function does not free
}

int main() {
    unique int q = make();
    free(q);              // the caller frees
    return 0;
}
```

#### 11.5.1 Future extension direction

The following error kinds are **not implemented in this version** and are recorded as candidates for later versions:

| Candidate error | Meaning | Analysis required |
|-----------------|---------|-------------------|
| `MissingFree` | An allocation is never freed on any path (memory leak) | Interprocedural reachability + escape analysis |
| `DoubleFree` | The same allocation is freed twice on some path | Path-sensitive ownership state machine |

If implemented later, both should arrive as **opt-in diagnostics** (e.g. `--warn-leaks` / `--strict-free`), off by default, so existing code is not broken.

#### 11.5.2 As a future optional safety level *(0.2.0 decision refinement: 2026-08-07)*

> **This subsection is the formal commitment of §11.5's `alloc`/`free` pairing policy**: 0.2.0 does **not enforce** `alloc`/`free` pairing; future versions may activate it as an **optional safety compile level**.

**0.2.0 commitment (explicit boundary)**:

1. 0.2.0 treats `alloc`/`free` pairing as the **programmer's responsibility**. The compiler does not report `MissingFree`, `DoubleFree`, or "calling `free` on a non-`alloc` value".
2. 0.2.0 does **not** provide a command-line flag to force-enable the above checks. This boundary does not change with any frontend flag in 0.2.0.
3. The authoritative source of error kinds is `devhandbook` §4.6 (`OwnershipError`); the ownership/borrow errors reported in 0.2.0 are exactly the three listed in §11.5 item 4 (`UseAfterMove`, `UseAfterDrop`, `DanglingReference`).

**Future versions (0.3.0+) direction (candidates only; not a 0.2.0 commitment)**:

1. A later version **may** introduce an optional safety compile level, with a candidate name such as `--safe-mem` (or equivalent), that activates `MissingFree` / `DoubleFree` checks. This level is **off by default** and takes effect only when the user explicitly enables it.
2. The semantics of this level will start from the table in §11.5.1: it adds "allocation counting" checks on top of the "ownership/borrowing" checks already defined in 0.2.0.
3. A **future revision** of this spec will fix the exact flag name, activation conditions, and the relationship to existing diagnostic levels.

**Rationale**: Forcing `alloc`/`free` pairing checks at 0.2.0 launch would generate heavy false positives (see §11.5 paragraph "Rationale") and the compatibility cost on existing code would be too high. The capability is therefore deferred to an opt-in mechanism rather than made the default.

---

## 12. Appendices

### 12.1 Complete EBNF Grammar *(0.2.0 revised: D-1, D-3)*

> **[0.2.0 · D-1, D-3]** Four changes relative to 0.1.0, all **additions**; no existing production was deleted or altered:
> 1. `type` gains two alternatives: `unique_type` and `mutable_reference_type`;
> 2. new `unique_type ::= 'unique' type` (D-1);
> 3. new `mutable_reference_type ::= type '&' 'mut'` (D-3);
> 4. `unary_expression` gains `'&' 'mut' unary_expression` (D-3).
>
> Changed lines are marked below with inline `// [0.2.0 D-N]` comments.

```
// === Program ===
program           ::= top_level_declaration*

top_level_declaration
                   ::= function_definition
                    | struct_definition
                    | export_declaration
                    | import_directive
                    | include_directive
                    | const_declaration
                    | ';'

// === Imports ===
import_directive  ::= '#import' string_literal ('as' identifier)? ';'
include_directive ::= '#include' string_literal ';'

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
declaration       ::= variable_declaration
                    | const_declaration

variable_declaration
                   ::= type identifier ('=' expression)? ';'

const_declaration  ::= 'const' type identifier '=' expression ';'

// === Types ===
type              ::= fundamental_type
                    | pointer_type
                    | unique_type              // [0.2.0 D-1]
                    | reference_type
                    | mutable_reference_type   // [0.2.0 D-3]
                    | array_type
                    | function_type
                    | type_identifier

fundamental_type  ::= 'void' | 'bool' | 'char' | 'int' | 'i8' | 'i16' | 'i32' | 'i64'
                    | 'uint' | 'u8' | 'u16' | 'u32' | 'u64'
                    | 'f32' | 'f64' | 'usize' | 'isize'

pointer_type      ::= type '*'
                    | type '*' 'const'

// [0.2.0 D-1] `unique` is a generic type constructor: `unique T` for any T.
// It is NOT hard-coded to `unique int`.
unique_type       ::= 'unique' type

reference_type    ::= type '&'

// [0.2.0 D-3] mutable borrow type `T&mut`
mutable_reference_type
                   ::= type '&' 'mut'

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

// NOTE: binary infix '&' remains bitwise AND. Unary prefix '&' is a borrow.
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
                    | '&' 'mut' unary_expression        // [0.2.0 D-3] mutable borrow
                    | 'sizeof' '(' type ')'
                    | 'move' '(' expression ')'
                    | 'clone' '(' expression ')'

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
letter             ::= 'a'..'z' | 'A'..'Z' | '_'
digit              ::= '0'..'9'
```

#### 12.1.1 Parsing note for `&` / `&mut` *(0.2.0 new: D-2, D-3)*

`'&' 'mut' unary_expression` and `('&') unary_expression` require **one token of lookahead** after `&`: if the adjacent token is the keyword `mut`, take the mutable-borrow production; otherwise take the immutable-borrow production. Because `mut` is a reserved word (§2.3), `&x` and `&mut x` are never ambiguous.

The `&` in `bitwise_and_expression` and the `&` in `unary_expression` are distinguished by **grammatical position**: the former follows a complete operand (infix), the latter precedes one (prefix). This is exactly how C/C++ handle `*` (multiplication vs. dereference), and needs no extra machinery in a recursive-descent parser.

### 12.2 Reserved Keywords *(0.2.0 revised: D-3)*

```
as         break      char       const      continue
else       export     extern     false      for
free       if         import     include    move
mut        null       return     static     struct
true       typedef    unique     unsafe     void
while      clone
```

> **[0.2.0 · D-3]** `mut` added. It appears only inside the `&mut` token (§2.6), but as a reserved word it may not be used as an identifier.

### 12.3 Operator Precedence Table *(0.2.0 revised: D-2)*

| Level | Operators | Description |
|-------|----------|-------------|
| 1 | `::` | Scope resolution |
| 2 | `()` `[]` `.` `->` `++` `--` | Postfix |
| 3 | `*` `/` `%` | Multiplication |
| 4 | `+` `-` | Addition |
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
| 15 | `move` `clone` | Ownership operations |

**Unary operators** (bind tighter than every binary operator above; right-to-left):

| Operator | Description |
|----------|-------------|
| `+` `-` | Unary plus / negation |
| `!` | Logical NOT |
| `~` | Bitwise NOT |
| `*` | Dereference |
| `&` | **Immutable borrow** (§7.8) |
| `&mut` | **Mutable borrow** (§7.9) |

### 12.4 Appendix X: Impact on Future Work *(0.2.0 new: D-8)*

This section records the constraints this revision places on subsequent implementation work. It is not part of the language definition.

#### 12.4.1 The C host is the production compiler

This revision is made on the basis of the decision that **`src-c/`, the C host, is the production compiler** (2026-08). Every semantic introduced by 0.2.0 (D-1 .. D-7) targets the C host as the **first and only** landing site.

The C host's ownership checker will be implemented against this specification; for the algorithms, see these sections of `.dev/plans/0.1.0-devhandbook.md`:

| This spec | devhandbook reference | Content |
|-----------|----------------------|---------|
| §7.1.1 assignment move rule | §4.4 所有权跟踪 | Per-variable ownership state table |
| §7.1.1 / §7.4 moved-out marking | §4.8 移动语义 (`record_move`) | Recording move points |
| §7.8 / §7.9 borrow rules | §4.5 所有权规则, §4.7 借用检查算法 | The three coexistence rules |
| §7.9.2 `BorrowConflict` | §4.6 所有权错误, §4.7 `check_borrow_conflict` | Conflict decision |
| §7.10 `DanglingReference` | §4.6 所有权错误, §12.4 借用检查算法 | Lifetime inference |
| All diagnostics | §12.2 所有权跟踪 (`ownership_checker_*`) | Checker interface |

Artifacts to be added, in dependency order:

1. **Lexer**: the `mut` keyword and the `&mut` token (`src-c/include/uc_token.h`, `src-c/src/lexer.c`).
2. **Grammar**: `unique_type`, `mutable_reference_type`, and the `&mut` unary production (`src-c/src/parser.c`); also fix the defect where `unique` is hard-coded to `Pointer(Int)` (D-1).
3. **Error kinds**: `UC_ERR_OWNERSHIP` with sub-kinds `UseAfterMove` / `UseAfterDrop` / `BorrowConflict` / `DanglingReference` (`src-c/include/uc_error.h`).
4. **A new compilation stage**: ownership checking inserted between `uc_parser_parse` and `uc_codegen_generate` (`src-c/src/main.c`).
5. **Codegen**: the "null the source" store for both `move(p)` and implicit moves (`src-c/src/codegen.c`, per §7.4 rule 3).
6. **Test corpus**: `test/borrowck/pass/` and `test/borrowck/fail/`, covering the positive and negative examples of §7.1.1, §7.8, §7.9, §7.10 one by one.

#### 12.4.2 The Rust reference implementation is deferred

**The Rust reference implementation is deferred.** `src/semantic/{checker,ownership,resolver}.rs` are currently dead code never called from `compile()`. This version does **not** require reviving them, nor does it require them to follow D-1 .. D-7.

Consequences:

- Divergence between `src/` and this specification is **known and accepted**, and should not be filed as a defect.
- If the Rust reference is ever resumed, it must be re-aligned against 0.2.0 (or later), not against 0.1.0.
- All conformance claims made by this specification apply to `src-c/` only.

#### 12.4.3 Items deliberately left out of this version

| Item | Status | Where it went |
|------|--------|---------------|
| `MissingFree` / `DoubleFree` | Not implemented | §11.5.1, candidate opt-in diagnostics |
| `Owned<T>` / `Ref<T>` / `RefMut<T>` internal type representation | Not exposed in the user language | devhandbook §4.1; an implementation detail |
| Polymorphic typing of `null` | Undefined | Carried over from 0.1.0; a later version |
| Precise definition of non-lexical lifetimes | Only described as "until last use" in §7.8.2 | To be filled in once the §12.4 algorithm is implemented |

---

## Document History

| Version | Date | Description |
|---------|------|-------------|
| 0.1 | 2026-04-13 | Initial specification |
| 0.1 | 2026-04-17 | Updated ownership default semantics (all pointers non-copyable), added clone(), updated inline assembly syntax |
| 0.1.0 | 2026-04-18 | 0.1.0 finalized |
| 0.2.0 | 2026-08-07 | Applied the 8 design decisions D-1 .. D-8: generic `unique`, `&` fixed as immutable borrow, `&mut` added to the language, `move` promoted to a built-in primitive, assignment move rule made explicit, `alloc`/`free` pairing left unenforced, `DanglingReference` trigger conditions defined, C-host-first decision recorded. Status: draft. |

---

*UltraCPP 0.2.0 Language Specification (draft)*
