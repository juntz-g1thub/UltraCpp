# UltraCPP

**A familiar C++ surface, backed by a safety model designed for this language.**

> **Language editions**: [English (this file)](./README.md) · [简体中文](./README-zh-CN.md)
>
> **Current spec**: [UltraCPP v0.3.1 (English)](./docs/UltraCPP-v0.3.1-spec-en.md) · [UltraCPP v0.3.1 (简体中文)](./docs/UltraCPP-v0.3.1-spec-zh-CN.md)
>
> **Status**: draft · **Last revised**: 2026-08-17

> **Current Status (2026-08-08)**
>
> Version 0.3.1 is defining new rules for ownership, modification rights, and threads. Those rules are a language design, not a safety guarantee already delivered by the compiler. The C99 port in `src-c/` is the production host compiler; the Rust port in `src/` is a reference. The semantic checking stage introduced by the 0.3.1 design has not been implemented yet.
>
> Start here:
>
> - [UltraCPP v0.3.1 Chinese Specification](docs/UltraCPP-v0.3.1-spec-zh-CN.md)
> - [UltraCPP v0.3.1 English Specification](docs/UltraCPP-v0.3.1-spec-en.md)
> - [0.1.0 borrow-checker spec versus implementation audit](.dev/drafts/0.1.0-borrowck-spec-vs-impl.md), the record of why the old model had to be reconsidered
> - [C host compiler README](src-c/README.md), the current implementation

## 自我介绍 / Self-introduction

I'm UltraCPP, an experimental systems language still working out how to keep C++ familiarity without borrowing someone else's safety model wholesale. My C compiler can lex, parse, and emit LLVM IR. My 0.3.1 semantic rules still live in the specification, so I won't pretend that design work is already enforcement.

## 设计历程 / Design Journey

### 第一阶段：天真 / Phase 1: Naivety

I thought copying Rust onto C++ syntax would work. My first slogan was "C++ syntax × Rust safety = UltraCPP," and from there the mapping seemed almost automatic:

| UltraCPP spelling | The role I assigned to it |
|---|---|
| `T*` | Owning pointer, roughly `Box<T>` |
| `T&` | Immutable borrow |
| `T&mut` | Mutable borrow |

It looked neat on paper. Three spellings lined up with three familiar Rust concepts, and the remaining work appeared to be implementation: track moves, reject conflicting borrows, check lifetimes. I had skipped the harder question. C++ programmers already read those symbols in particular ways, and those meanings do not disappear because a new specification gives them Rust-shaped names.

### 第二阶段：裂缝 / Phase 2: Cracks

C++ syntax started answering back.

I had made `T*` both the owner and the ordinary pointer. Ownership wants one party with disposal responsibility. An ordinary C++ pointer wants arithmetic, aliases, and rebinding. The same token was now expected to obey two different sets of instincts.

`T&` was worse. One section treated it as an immutable borrow. Function examples still used it as the familiar C++ mechanism for modifying an argument. Elsewhere, unary `&` drifted between address-of syntax and the act of borrowing. I was using one family of symbols for three stories and hoping the reader would infer which story applied.

Then there was `unique T`. It was meant to say "exclusive owner" explicitly, but `T*` had already been declared owning by default. The two forms overlapped instead of clarifying each other.

A line of ordinary C++ exposed the problem without any theory:

```cpp
int b = 42;
int* p = &b;
```

If every `int*` owns, does `p` free the stack variable `b`? Of course not. If `p` does not own, then my blanket rule for `T*` was false.

I was pretending to be Rust, but C++ syntax wouldn't let me.

### 第三阶段：怀疑 / Phase 3: Questioning

The spec versus implementation audit was embarrassingly clear. The old documents described 12 ownership and borrow rules. The number semantically enforced by either compiler was **0/12**. Several keywords reached the lexer, and several constructs reached the AST, but both compilation paths went from parsing to code generation without a working borrow checker in between.

That was an implementation failure, but it also gave me room to ask whether the target itself deserved implementing. The answer was no. Doing it right would still be wrong: the model was built for Rust, not for C++.

The premise "C++ syntax × Rust safety" had been doing too much thinking for me. A complete implementation of a contradictory model would merely make the contradictions executable.

### 第四阶段：转向 / Phase 4: The Turn

The user asked the question that changed my direction. Why was I treating ownership and permission to modify as one thing? They answer different questions:

1. Who must eventually release the resource?
2. Who may write the value right now?

That split became **Rule 22**.

| Permission | Question it answers | Cardinality | How it changes | Main syntax |
|---|---|---|---|---|
| Ownership | Who calls `delete` / `free`? | One owner per resource | Explicit transfer, never copy | `T*`, `alloc(T)`, `move(p)` |
| Modification right | Who may write through a reference? | None, shared, or exclusive | Acquired and released under policy | `T&`, `mod(r)`, `unmod(r)` |

Under the current rules, an owning `T*` gets modification rights by default, but the two states are tracked separately. A non-owner can receive a modification right without inheriting disposal responsibility. "Who frees?" no longer smuggles in an answer to "who writes?"

```cpp
int* owner = alloc(int);  // owner must eventually free
int& view = *owner;       // view does not own the object
mod(view);                // request write permission separately
*view = 42;
free(owner);              // ownership never moved to view
```

This was the first change that removed a contradiction instead of renaming it.

### 第五阶段：指令化 / Phase 5: Directive-ification

Once modification rights stood on their own, type-level mutability stopped looking inevitable. Projects make different choices about writable aliases — a strict module may want one writer, a low-level module may deliberately allow several and take responsibility for races, another module may want every reference to stay read-only — so encoding each policy as another reference type would rebuild the same maze. I moved the choice into a user-configured directive instead: `#modlaw`.

There are exactly six legal combinations:

| `perm` | `scope` | Effect |
|---|---|---|
| `none` | `global` | Forbid `mod()` throughout the file and included content |
| `none` | `module` | Forbid `mod()` in the current module |
| `exclusive` | `global` | Require globally exclusive modification rights and check conflicts |
| `exclusive` | `module` | Require module-local exclusive modification rights and check conflicts |
| `shared` | `global` | Allow multiple modification rights globally; races are the programmer's responsibility |
| `shared` | `module` | Allow multiple modification rights in the module; races are the programmer's responsibility |

With no directive, the policy is `#modlaw shared module`. The reference type stays the same while policy changes its behavior:

```cpp
int value = 1;
int& r = value;
```

```cpp
#modlaw none module
mod(r);                 // compile error: this policy forbids modification
```

```cpp
#modlaw exclusive module
mod(r);                 // allowed, but a competing mod is rejected
```

```cpp
#modlaw shared module
mod(r);                 // allowed, including alongside other mod holders
```

`T&` now says what the value is. `#modlaw` says how a module governs writes. That division is more useful to me than baking policy into every reference declaration.

### 第六阶段：简化 / Phase 6: Simplification

This split let me delete syntax rather than add more. `T&mut` became redundant. `#modlaw exclusive` plus `mod()` already expresses an exclusive modification right, so 0.3.1 has one reference type: `T&`.

```cpp
#modlaw exclusive module

int n = 0;
int& r = n;
mod(r);                 // acquire the exclusive modification right here
*r = 1;
```

I also chose not to preserve C++ const-pointer semantics. UltraCPP 0.3.1 deliberately reverses them:

| Declaration | UltraCPP 0.3.1 meaning |
|---|---|
| `const T*` | Pointer-locked: cannot rebind and cannot write through it |
| `T* const` | Read-only data view: may rebind but cannot write through it |
| `const T* const` | Both the pointer and write access are locked |

```cpp
int x = 1;
int y = 2;

const int* pinned = &x;
// pinned = &y;         // error: pointer is locked
// *pinned = 3;         // error: read-only

int* const view = &x;
view = &y;              // rebinding is allowed
// *view = 3;           // error: read-only view
```

Neither read-only form may be created from an owning heap pointer. That owner could later move or free the allocation and leave the view dangling.

The thread model follows the same separation. Cross-thread state must be marked `shared`. GCC-style `__thread` declares thread-local storage. `mutex<T>` and `atomic<T>` are standard-library types, not keywords. Under an exclusive policy, the compiler may synthesize a mutex around `mod()` for ordinary shared values. An explicit wrapper suppresses that automatic mutex because the programmer has already chosen the synchronization mechanism.

```cpp
#modlaw exclusive global

shared int counter = 0;
__thread int scratch = 0;

mutex<int> guarded;     // standard-library type
atomic<int> ready;      // standard-library type
```

### 第七阶段：当前形态 / Phase 7: Current Form

Version 0.3.1 is not "C++ × Rust." It is C++ familiarity plus an UltraCPP-native safety model: one `T&`, independent ownership and modification rights, a user-selected `#modlaw`, and explicit rules for cross-thread visibility.

The lessons cost enough to feel real. I tried to add one syntax to one foreign model and spent the result on exceptions. Then I treated missing enforcement as the whole problem, until the audit made it obvious that a finished checker would still enforce the wrong abstraction. The user's repeated questions already contained the useful split: "who frees?" and "who writes?" were never one question. I needed to stop mapping and listen to the distinction.

This model fits me better, but it is not finished. The 0.3.1 specifications describe the destination. The next honest milestone is a semantic pass in the C host compiler that enforces those rules.

## 当前架构 / Current Architecture (0.3.1)

```text
UltraCPP source (.uc / .upp)
             |
             v
+---------------------------+
| Preprocess / import scan  |  #include, #import, #modlaw
+-------------+-------------+
              |
              v
+---------------------------+
| Lexer                     |
+-------------+-------------+
              |
              v
+---------------------------+
| Parser + AST              |
+-------------+-------------+
              |
              v
+------------------------------------------------------+
| Semantic checks                                      |
| ownership | references | modlaw | lifetime | threads |
| NEW IN THE 0.3.1 DESIGN, NOT YET IMPLEMENTED IN CODE |
+-------------------------+----------------------------+
                          |
                          v
+---------------------------+
| LLVM IR code generation   |
+-------------+-------------+
              |
              v
        LLVM IR -> llc -> system linker -> executable
```

The current `src-c/` pipeline implements the lexer, parser, AST, code generator, and CLI. Reading `#modlaw` and running the semantic box are requirements of the 0.3.1 architecture, not stages already wired into the compiler. This README therefore distinguishes specified safety from implemented behavior.

## 核心设计概念 / Core Design Concepts

### 1. One reference type: `T&`

A `T&` must be initialized, cannot be null, and never owns its referent. It is readable by default. Writing requires `mod()`.

```cpp
int x = 10;
int& r = x;
int copy = r;           // read
mod(r);
*r = 11;                // write
```

### 2. Two independent permissions

Ownership controls disposal. Modification rights control writes. Changing one does not silently change the other.

```cpp
int* owner = alloc(int);
int& r = *owner;
mod(r);                 // changes modification rights only
*r = 7;
free(owner);            // owner still has disposal responsibility
```

### 3. The `#modlaw` directive

Choose `none`, `exclusive`, or `shared`, then choose `global` or `module` scope.

```cpp
#modlaw exclusive module

int value = 0;
int& r = value;
mod(r);                 // exclusive modification right in this scope
*r = 1;
```

### 4. Custom const-pointer semantics

`const T*` locks the pointer. `T* const` is a rebindable, read-only data view. Both forbid write-through, which is the opposite of C++'s placement rules.

```cpp
const int* fixed_pointer = &x;  // cannot rebind
int* const readonly_view = &x;  // may rebind, cannot write data
```

### 5. Thread model

Cross-thread access requires `shared`. Use `__thread` for a per-thread copy. The standard library supplies `mutex<T>` and `atomic<T>`.

```cpp
shared int jobs = 0;
__thread int local_jobs = 0;
mutex<int> jobs_lock;
atomic<int> stop_flag;
```

## 项目结构 / Project Structure

```text
UltraCpp/
├── src-c/       Production C99 host: lexer, parser, AST, codegen, CLI
├── src/         Rust reference implementation, not the production target
├── src-uc/      Future self-hosted compiler written in UltraCPP
├── docs/        Versioned 0.1.0, 0.2.0, and 0.3.1 specifications
├── .dev/        Design plans, audits, drafts, and development records
├── bootstrap/   Bootstrap plan and C/Rust baseline artifacts
├── lib/         UltraCPP standard-library sources
├── test/        Language test programs
└── tools/       Cross-implementation verification scripts
```

## 快速开始 / Quick Start

You need a C99 compiler and GNU Make. Build the current C host compiler:

```bash
make -C src-c build/uc_lexer
src-c/build/uc_lexer --help
```

Tokenize a test program:

```bash
src-c/build/uc_lexer --tokens test/test_t1/main.upp
```

Run the C port's unit tests:

```bash
make -C src-c test
```

`src-c/build/uc_lexer` also supports `--ast`, `--emit-ll` / `-S`, and `--build`. These commands exercise the current compiler. They do not enable the unimplemented 0.3.1 semantic checks.

## 关键文档 / Key Documents

| Document | Purpose |
|---|---|
| [UltraCPP v0.3.1 Chinese Specification](docs/UltraCPP-v0.3.1-spec-zh-CN.md) | Chinese edition of the v0.3.1 language specification |
| [UltraCPP v0.3.1 English Specification](docs/UltraCPP-v0.3.1-spec-en.md) | English edition of the v0.3.1 language specification |
| [0.1.0 borrow-checker audit](.dev/drafts/0.1.0-borrowck-spec-vs-impl.md) | Records the 0/12 enforcement result, contradictions, and later decisions |
| [C port README](src-c/README.md) | Build instructions and capabilities of the production host compiler |
| [Bootstrap Plan](bootstrap/PLAN.md) | Route from the C host to the future compiler in `src-uc/` |
| [.dev README](.dev/README.md) | Index of design plans, drafts, and development records |

## 开发语言 / Development Languages

| Language | Role |
|---|---|
| C99 | Production host compiler in `src-c/` |
| Rust | Reference implementation in `src/` |
| UltraCPP | Future self-hosted compiler implementation in `src-uc/` |

LLVM tools and the system linker remain the compiler backend toolchain.

## 许可证 / License

Apache License 2.0. See [LICENSE](LICENSE).
