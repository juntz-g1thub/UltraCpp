# UltraCPP 0.1 Language Specification

> Version 0.1 - Initial Release
>
> Last Updated: 2026-04-18

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

### 1.3 Symbol Conventions

| Symbol | Meaning |
|--------|---------|
| `T*` | Pointer to T (non-copyable by default, assignment transfers ownership) |
| `T&` | Reference to T |
| `T(*)(U)` | Function pointer type (C++ style) |
| `alloc(T)` | Allocate memory for type T |
| `free(ptr)` | Free memory |
| `move(ptr)` | Transfer ownership (source becomes null) |
| `clone(ptr)` | Clone pointer (copy, independent ownership) |

---

## 2. Lexical Structure

### 2.1 Source File Conventions

```
*.uc   — UltraCPP source file (recommended)
*.upp  — UltraCPP source file (compatible)
```

### 2.2 Token Types

| Category | Examples |
|----------|----------|
| **Keywords** | `if`, `else`, `while`, `for`, `return`, `struct`, `export`, `import`, `const`, `unique`, `move`, `free`, `alloc`, `null`, `true`, `false`, `void`, `extern`, `unsafe`, `typedef`, `clone` |
| **Identifiers** | `foo`, `myVariable`, `_private`, `CamelCase` |
| **Literals** | `42`, `3.14`, `'x'`, `"hello"`, `true`, `false` |
| **Operators** | `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`, `&&`, `||`, `!`, `&`, `|`, `^`, `~`, `<<`, `>>`, `++`, `--`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=` |
| **Delimiters** | `(`, `)`, `{`, `}`, `[`, `]`, `,`, `;`, `:`, `.`, `::` |
| **Preprocessor** | `#import`, `#include`, `#ifdef`, `#ifndef`, `#endif`, `#define` |

### 2.3 Keywords (Reserved)

```
if          else        while       for         return
struct      export      import      const       typedef
unique      move        free        alloc       null
true        false       void        extern      unsafe
as          static      clone
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

### 2.6 Operators and Delimiters

| Operator | Description |
|----------|-------------|
| `+` | Addition |
| `-` | Subtraction/negation |
| `*` | Multiplication/dereference |
| `/` | Division |
| `%` | Modulo |
| `=` | Assignment (triggers ownership transfer for pointer types) |
| `==` | Equality |
| `!=` | Inequality |
| `<` | Less than |
| `>` | Greater than |
| `<=` | Less than or equal |
| `>=` | Greater than or equal |
| `&&` | Logical AND |
| `\|\|` | Logical OR |
| `!` | Logical NOT |
| `&` | Bitwise AND/address-of |
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

### 3.3 Reference Types

```cpp
T&  // Reference to T (must be initialized, cannot be null)
```

**Examples:**
```cpp
int x = 42;
int& r = x;  // Reference must be initialized
r = 100;     // Modifies x
```

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

### 3.7 Type Modifiers

| Modifier | Meaning |
|---------|---------|
| `const` | Value cannot be modified |
| `unique` | Explicit exclusive ownership pointer (same as default, can be omitted) |
| `static` | Internal linkage |

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

### 4.5 Bitwise Operators

| Operator | Description | Example |
|---------|-------------|---------|
| `&` | Bitwise AND | `a & b` |
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

**Reference parameters for modification:**
```cpp
void inc(int& x) {
    x = x + 1;  // Affects caller
}

int n = 10;
inc(n);
// n is now 11
```

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

### 7.1 Ownership Semantics

**All pointers are non-copyable by default; assignment triggers ownership transfer (move semantics).**

```cpp
int* p1 = alloc(int);
int* p2 = p1;    // p1 becomes null, ownership transfers to p2
int* p3 = p2;    // p2 becomes null, ownership transfers to p3
```

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

### 7.4 move Function

`move()` explicitly transfers ownership; source pointer becomes null.

```cpp
int* p1 = alloc(int);
int* p2 = move(p1);  // p1 becomes null
```

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

---

## 12. Appendices

### 12.1 Complete EBNF Grammar

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
                    | reference_type
                    | array_type
                    | function_type
                    | type_identifier

fundamental_type  ::= 'void' | 'bool' | 'char' | 'int' | 'i8' | 'i16' | 'i32' | 'i64'
                    | 'uint' | 'u8' | 'u16' | 'u32' | 'u64'
                    | 'f32' | 'f64' | 'usize' | 'isize'

pointer_type      ::= type '*'
                    | type '*' 'const'
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

### 12.2 Reserved Keywords

```
as         break      char       const      continue
else       export     extern     false      for
free       if         import     include    move
null       return     static     struct     true
typedef    unique     unsafe     void       while
clone
```

### 12.3 Operator Precedence Table

| Level | Operators | Description |
|-------|----------|-------------|
| 1 | `::` | Scope resolution |
| 2 | `()` `[]` `.` `->` `++` `--` | Postfix |
| 3 | `*` `/` `%` | Multiplication |
| 4 | `+` `-` | Addition |
| 5 | `<<` `>>` | Shift |
| 6 | `<` `>` `<=` `>=` | Relational |
| 7 | `==` `!=` | Equality |
| 8 | `&` | Bitwise AND |
| 9 | `^` | Bitwise XOR |
| 10 | `\|` | Bitwise OR |
| 11 | `&&` | Logical AND |
| 12 | `\|\|` | Logical OR |
| 13 | `?:` | Ternary conditional |
| 14 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` | Assignment |
| 15 | `move` `clone` | Ownership operations |

---

## Document History

| Version | Date | Description |
|---------|------|-------------|
| 0.1 | 2026-04-13 | Initial specification |
| 0.1 | 2026-04-17 | Updated ownership default semantics (all pointers non-copyable), added clone(), updated inline assembly syntax |

---

*UltraCPP 0.1 Language Specification*
