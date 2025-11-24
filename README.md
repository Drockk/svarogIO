# SvarogIO

[![CodeFactor](https://www.codefactor.io/repository/github/drockk/svarogio/badge)](https://www.codefactor.io/repository/github/drockk/svarogio)

A modern, high-performance C++23 asynchronous I/O library inspired by Boost.Asio.

> **Note:** As of November 2025 the codebase was simplified around `io_context`, `work_queue`, and `thread_pool`. Template-based service registries and coroutine helpers (`awaitable_task`, `co_spawn`, `execution_context`) were removed to keep the API minimal.

## Features

- **C++23 Standard**: Leveraging modern C++ features for better performance and safety
- **Contract Programming**: Built-in precondition/postcondition checking using GSL
- **Lock-Free Architecture**: High-performance concurrent data structures
- **Multiple Build Configurations**: Optimized Debug and Release builds

## Requirements

- **Compiler**: GCC 13+ or Clang 16+ with C++23 support
- **Build System**: CMake 3.23+, Ninja
- **Dependencies**: Automatically managed via CPM
  - Microsoft GSL 4.2.0+

## Building the Project

### Quick Start

The project uses CMake presets for easy configuration:

```bash
# Configure and build Debug version
cmake --preset debug
cmake --build --preset debug

# Configure and build Release version
cmake --preset release
cmake --build --preset release
```

### Build Directory Structure

- `build/debug/` - Debug build with full contract checking
- `build/release/` - Release build with optimizations

### Running Tests

```bash
# Run tests for Debug build
ctest --test-dir build/debug --output-on-failure

# Run tests for Release build
ctest --test-dir build/release --output-on-failure

# Or use the test preset
cmake --build --preset debug --target test
```

### Running Examples

```bash
# Build examples (Debug)
cmake --build build/debug --target examples

# Build examples (Release)
cmake --build build/release --target examples

# Run a specific example
./build/debug/examples/simple_task/simple_task
./build/release/examples/simple_tcp_server/simple_tcp_server
```

### Complete Workflow (Configure + Build + Test)

```bash
# Debug workflow
cmake --workflow --preset debug

# Release workflow
cmake --workflow --preset release
```

## Project Structure

```
svarogIO/
├── svarog/          # Core library source
│   ├── include/     # Public headers
│   └── source/      # Implementation files
├── tests/           # Unit tests
├── examples/        # Usage examples
├── docs/            # Documentation and ADRs
└── cmake/           # CMake modules
```

## Development

### Contract Programming

This project uses contract programming for better safety:

- **Debug builds**: All contracts (`SVAROG_EXPECTS`, `SVAROG_ENSURES`) are checked
- **Release builds**: Contracts are compiled out by default for zero overhead
- **Optional**: Enable contracts in Release with `-DSVAROG_ENABLE_CONTRACTS_IN_RELEASE=ON`

```bash
# Release build with contract validation
cmake --preset release -DSVAROG_ENABLE_CONTRACTS_IN_RELEASE=ON
cmake --build --preset release
```

See [docs/CODING_STANDARDS.md](docs/CODING_STANDARDS.md) for contract usage guidelines.

### Architecture Decision Records

See [docs/adr/](docs/adr/) for architectural decisions and rationale.

## Documentation

- [Code Formatting Guide](docs/FORMATTING.md) - Automated formatting with git hooks and CMake
- [Code Style](docs/CODE_STYLE.md) - Comprehensive style guidelines
- [Phase 1 Tasks](docs/PHASE_1_TASKS.md) - Current implementation progress
- [Coding Standards](docs/CODING_STANDARDS.md) - Code style and contract guidelines
- [Refactoring Plan](docs/REFACTORING_PLAN.md) - Overall project roadmap
- [Architecture Decision Records](docs/adr/) - Design decisions and rationale

## License

See [LICENSE.md](LICENSE.md) for details.