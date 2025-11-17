# Clang-Tidy Integration

This project uses clang-tidy for static analysis and code quality checks.

## Setup

### Required Tools

```bash
# Install clang-tidy
sudo apt install clang-tidy

# Verify installation
clang-tidy --version
```

### VS Code Extensions

Install the recommended extensions:
- **C/C++** (ms-vscode.cpptools) - IntelliSense and code analysis
- **clangd** (llvm-vs-code-extensions.vscode-clangd) - Advanced LSP features
- **Clang-Tidy** (notskm.clang-tidy) - Inline diagnostics

## Configuration

### .clang-tidy

The project uses a comprehensive configuration in `.clang-tidy`:

- **Enabled checks**: bugprone, cert, clang-analyzer, concurrency, cppcoreguidelines, misc, modernize, performance, portability, readability
- **Naming conventions**: Enforces project standards (m_ prefix for members, t_ for parameters, snake_case)
- **Modern C++**: Checks for C++23 best practices

### VS Code Integration

Clang-tidy runs automatically in VS Code through:

1. **C/C++ Extension**: `C_Cpp.codeAnalysis.clangTidy.enabled`
2. **clangd**: `--clang-tidy` argument for LSP integration

## Usage

### Automatic Analysis

Files are analyzed automatically when opened or modified in VS Code.

### Manual Analysis

Run clang-tidy from command line:

```bash
# Analyze single file
clang-tidy -p build/release svarog/source/svarog/execution/thread_pool.cpp

# Analyze all source files
find svarog/source -name "*.cpp" -exec clang-tidy -p build/release {} \;

# Fix issues automatically (use with caution)
clang-tidy -p build/release --fix svarog/source/svarog/execution/thread_pool.cpp
```

### CMake Integration

The project uses `compile_commands.json` from the build directory:

```bash
cmake --preset=release
# compile_commands.json is at build/release/compile_commands.json
```

## Common Checks

### Naming Conventions
- Classes/structs: `snake_case` (e.g., `io_context`)
- Functions: `snake_case` (e.g., `run_one()`)
- Member variables: `m_` prefix (e.g., `m_context`)
- Parameters: `t_` prefix (e.g., `t_num_threads`)
- Constants: `UPPER_CASE` or `k_` prefix

### Modern C++ Checks
- Use of auto where appropriate
- Range-based for loops
- nullptr instead of NULL
- override/final keywords
- Move semantics
- Smart pointers

### Performance Checks
- Pass by value vs reference
- Unnecessary copies
- Move opportunities
- Const correctness

## Suppressing Warnings

Use NOLINT comments when necessary:

```cpp
// Single line
void foo() { /* ... */ } // NOLINT(readability-function-cognitive-complexity)

// Block
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
auto* ptr = reinterpret_cast<T*>(data);

// File-wide
// NOLINTBEGIN(cert-err58-cpp)
static const auto global = compute();
// NOLINTEND(cert-err58-cpp)
```

## Disabled Checks

Some checks are disabled to match project style:

- `readability-identifier-length`: Short names allowed in small scopes
- `readability-magic-numbers`: Numbers in tests/examples are acceptable
- `modernize-use-trailing-return-type`: Not enforced
- `cppcoreguidelines-avoid-magic-numbers`: Context-dependent

## See Also

- [Clang-Tidy Documentation](https://clang.llvm.org/extra/clang-tidy/)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- Project coding standards: `docs/CODE_STYLE.md`
