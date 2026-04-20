# UltraCPP

**A compiled language combining C++ syntax with Rust-like memory safety**

[![Version](https://img.shields.io/badge/version-0.1.0--alpha-orange.svg)](Cargo.toml)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)

## Project Goal

UltraCPP is a research project exploring whether C++ developers can work with a language that has:

- **Familiar C++ syntax** - minimal learning curve
- **Compile-time memory safety** - no garbage collection required
- **Move semantics by default** - ownership transfer without `std::move`
- **Modern module system** - `#import` and `#include` directives

**Core Question**: Can we build a language that C++ developers accept naturally, while providing Rust-like safety guarantees?

## Vision

```
C++ syntax × Rust safety = UltraCPP
```

We believe the success of any language depends on:
1. **Syntax familiarity** - developers shouldn't need to learn a new syntax to try safety
2. **Tooling compatibility** - works with existing debuggers, linkers, build systems
3. **Incremental adoption** - can mix safe and unsafe code

## Current Status (v0.1.0)

The compiler can now:

- ✅ Compile basic programs with functions, variables, control flow
- ✅ Generate LLVM IR and assemble to executables
- ✅ Support string literals and global constants
- ✅ Provide I/O library (`io.print_str()`, `io.getValue()`)
- ✅ Handle module imports (`#import`) and includes (`#include`)
- ✅ Manage local variable load/store with proper type conversions

### What's Working

| Feature | Status | Example |
|---------|--------|---------|
| Basic types (int, char, float) | ✅ | `int x = 42;` |
| Functions | ✅ | `int add(int a, int b) { return a + b; }` |
| Control flow | ✅ | `if/else`, `while` |
| String literals | ✅ | `"Hello, World!\n"` |
| Module import | ✅ | `#import "lib/io"` |
| Module include | ✅ | `#include "math.uc"` |
| sys$* builtins | ✅ | `sys$strlen()`, `sys$write()` |

### What's Next

- [ ] Complete standard library implementation
- [ ] Memory ownership and borrowing system
- [ ] Error handling (`Result<T, E>`)
- [ ] Cargo-like build tool integration

## Quick Start

### Prerequisites

- Rust (latest stable)
- LLVM (via `llvm-tools` or system LLVM)
- GCC or Clang (for linking)

### Build

```bash
git clone https://github.com/your_username/ultracpp.git
cd ultracpp
cargo build --release
./target/release/ultracpp --help
```

### Your First Program

Create `hello.upp`:

```c
#import "lib/io"

int main() {
    io.print_str("Hello, World!\n");
    return 0;
}
```

### Compile and Run

```bash
# Build the I/O library first
./scripts/uc-build lib lib/io.uc

# Compile your program
./scripts/uc-build compile test/test_hello_world/main.upp

# Link and run (manual)
llc build/uc/test_test_hello_world_main/*.ll -o /tmp/hello.s
gcc -c /tmp/hello.s -o /tmp/hello.o
gcc build/uc/lib_io/lib_io.o /tmp/hello.o -no-pie -o /tmp/hello
/tmp/hello
# Output: Hello, World!
```

## Architecture

```
Source (.uc/.upp)
       │
       ▼
┌──────────────────┐
│   Preprocessor   │  Handles #import, #include, export
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│      Lexer       │  Tokenizes source into tokens
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│      Parser      │  Builds AST from tokens
└────────┬─────────┘
         │
         ▼
┌──────────────────┐
│   Code Generator │  Emits LLVM IR
└────────┬─────────┘
         │
         ▼
    LLVM IR (.ll)
         │
         ▼
       LLC  (assemble to .s)
         │
         ▼
       GCC  (link to executable)
```

## Project Structure

```
ultracpp/
├── src/                    # Compiler source (Rust)
│   ├── main.rs            # Entry point
│   ├── frontend/          # Lexer, Parser, AST
│   ├── codegen/           # LLVM IR generation
│   ├── semantic/           # Type checking, resolution
│   └── preprocessor.rs     # #import, #include
├── lib/                    # Standard library (UltraCPP source)
│   ├── io.uc              # I/O library
│   └── math.uc           # Math library
├── test/                   # Test suites
│   ├── test_t1/           # Basic tests
│   ├── test_t2/           # Variable tests
│   ├── test_hello_world/  # Hello World test
│   └── README.md          # Test documentation
├── scripts/               # Build tools
│   └── uc-build           # Build script
├── docs/                  # Project documentation
├── .sisyphus/             # Agent working files
│   ├── plans/             # Design specifications
│   └── drafts/            # Research notes
└── Cargo.toml            # Rust project config
```

## Language Syntax

### Hello World

```c
#import "lib/io"

int main() {
    io.print_str("Hello, World!\n");
    return 0;
}
```

### Variables and Functions

```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 10;
    int y = add(x, 5);
    return y;
}
```

### Module Import

```c
#import "lib/io"

int main() {
    int val = io.getValue();
    return val;
}
```

### Module Include

```c
#include "lib/math.uc"

int main() {
    int result = add(5, 3);  // add from math.uc
    return result;
}
```

## Symbol Naming Convention

UltraCPP uses `$` as the module-function separator to avoid conflicts with C naming conventions:

| Expression | LLVM Symbol |
|-----------|-------------|
| `io.getValue()` | `@io$getValue` |
| `sys$strlen(s)` | `@strlen` (builtin) |
| `main` | `@main` |

## Testing

### Test Categories

| Directory | Purpose |
|-----------|---------|
| `test/test_t1/` | Basic empty program |
| `test/test_t2/` | Variable declaration |
| `test/test_io/` | I/O library via `#import` |
| `test/test_hello_world/` | String literals + output |
| `test/test_import/` | `#include` directive |

### Run Tests

```bash
# Build all libraries first
./scripts/uc-build lib lib/io.uc

# Compile a test
./scripts/uc-build compile test/test_hello_world/main.upp

# Run manually
llc build/uc/*/test_*.ll -o /tmp/test.s
gcc build/uc/lib_io/lib_io.o /tmp/test.o -no-pie -o /tmp/test
/tmp/test
```

## Documentation

### Project Documentation

- [docs/UltraCPP-v0.1.0-spec-en.md](docs/UltraCPP-v0.1.0-spec-en.md) - Language Specification (English)
- [docs/UltraCPP-v0.1.0-spec-zh-CN.md](docs/UltraCPP-v0.1.0-spec-zh-CN.md) - 语言规范（中文）

### Agent Documentation (Internal)

- `.sisyphus/plans/` - Design specifications and RFCs
- `.sisyphus/drafts/` - Research notes and explorations

### Key Documents

| Document | Description |
|---------|-------------|
| `AGENTS.md` | Agent system prompt for project |
| `CONTRIBUTING.md` | Contribution guidelines |
| `Cargo.toml` | Rust dependencies and project config |

## Development Language

**Primary Language**: Rust

The compiler is implemented in Rust for:
- Memory safety during compiler development
- Strong type system for AST manipulation
- Excellent LLVM bindings via `inkwell` or `llvm-sys`

**Target Language**: Any architecture supported by LLVM

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Version History

### v0.1.0 (Current)
- Working compiler frontend (lexer, parser)
- LLVM IR code generation
- Basic I/O library
- Module import/export system
- String literal support

## License

Apache License 2.0 - see [LICENSE](LICENSE)

---

*"C++ syntax × Rust safety = UltraCPP"*
