# Code Style Guide

This project follows the [C++ Best Practices](https://github.com/cpp-best-practices/cppbestpractices) style guidelines.

## Quick Reference

### Naming Conventions

- **Types**: PascalCase → `MyClass`, `IpAddress`
- **Functions/Methods**: camelCase → `myMethod()`, `getData()`
- **Variables**: camelCase → `myVariable`, `count`
- **Constants**: UPPER_CASE → `const double PI = 3.14159;`
- **Member variables**: `m_` prefix → `m_data`, `m_value`
- **Function parameters**: `t_` prefix → `t_param`, `t_value`
- **Macros**: UPPER_CASE → `SVAROG_EXPECTS`

### Example

```cpp
class MyClass {
public:
    MyClass(int t_data)
        : m_data{t_data}
    {
    }

    int getData() const
    {
        return m_data;
    }

private:
    int m_data;
};
```

## File Extensions

- Headers: `.hpp`
- Sources: `.cpp`

## Formatting

### Automatic Formatting

The project uses `clang-format` for automatic code formatting. The style is defined in `.clang-format`.

**VS Code**: Format on save is enabled by default (see `.vscode/settings.json`)

**Manual formatting**:
```bash
clang-format -i file.cpp
```

### Key Style Rules

1. **Indentation**: 4 spaces (never tabs)
2. **Line length**: 120 characters maximum
3. **Braces**: Required for all blocks (if, for, while, etc.)
   ```cpp
   // Good
   if (condition) {
       doSomething();
   }

   // Bad
   if (condition)
       doSomething();
   ```

4. **Comments**: Use `//` for all comments (not `/* */`)
5. **Include guards**: Use `#pragma once`
6. **Null pointers**: Use `nullptr` (not `NULL` or `0`)

## Best Practices

### Initialize Member Variables

Use member initializer lists and brace initialization:

```cpp
class MyClass {
public:
    MyClass(int t_value)
        : m_value{t_value}  // Preferred
    {
    }

private:
    int m_value{0};  // Default initialization
};
```

### Namespaces

Always use namespaces (never global scope):

```cpp
namespace svarog::execution {

class WorkQueue {
    // ...
};

}  // namespace svarog::execution
```

### Include Order

1. Standard library headers: `<vector>`, `<string>`
2. C headers: `<cstdint>`, `<cassert>`
3. Project headers: `"svarog/core/..."`
4. Other libraries

Enforced automatically by clang-format.

### Type Safety

- Use `explicit` for single-parameter constructors
- Use `= delete` for prohibited operations
- Prefer `std::size_t` for sizes
- Use `auto` when type is obvious

### Modern C++ Features

This project uses **C++23**. Prefer modern features:

- `std::move_only_function` over function pointers
- `std::expected` for error handling
- `std::jthread` over `std::thread`
- Range-based for loops
- Structured bindings
- Concepts (where appropriate)

## Tools

### Required

- **clang-format**: Automatic code formatting
- **EditorConfig**: Editor settings (`.editorconfig`)

### Recommended

- **clang-tidy**: Static analysis
- **cppcheck**: Additional static analysis

## Compiler Warnings

The project is compiled with extensive warning flags (see `CMakeLists.txt`):

- `-Wall -Wextra -Wpedantic`
- `-Wshadow -Wconversion -Wsign-conversion`
- And many more...

**All warnings must be addressed before merging.**

## References

- [C++ Best Practices](https://github.com/cpp-best-practices/cppbestpractices/blob/master/03-Style.md)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- Project ADRs in `docs/adr/`
