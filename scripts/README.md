# Development Scripts

This directory contains utility scripts for development workflow automation.

## Available Scripts

### 🎨 check-format.sh

Checks and fixes C++ code formatting using clang-format.

**Usage:**
```bash
# Check all files
./scripts/check-format.sh

# Check only changed files (fast!)
./scripts/check-format.sh --changed

# Check only staged files
./scripts/check-format.sh --staged

# Fix all files
./scripts/check-format.sh --fix

# Fix only staged files
./scripts/check-format.sh --staged --fix
```

**Integration:**
- Used by git pre-commit hook
- Used by GitHub Actions CI
- Available as CMake targets (`format-check`, `format-fix`, `format-check-changed`)

### 🪝 install-hooks.sh

Installs git hooks for the project.

**Usage:**
```bash
./scripts/install-hooks.sh
```

**Installed Hooks:**
- **pre-commit**: Automatically checks formatting of staged files before each commit

**Note:** Hooks are local to your git repository and need to be installed once per clone.

### 🔍 run-clang-tidy.sh

Runs clang-tidy static analysis on all source files.

**Usage:**
```bash
# Analyze all files
./scripts/run-clang-tidy.sh

# Analyze and fix issues automatically
./scripts/run-clang-tidy.sh --fix

# Use 8 parallel jobs
./scripts/run-clang-tidy.sh --jobs 8

# Analyze only files matching pattern
./scripts/run-clang-tidy.sh --file execution

# Combine options
./scripts/run-clang-tidy.sh --fix --jobs 4 --file io_context
```

**Options:**
- `--fix` - Apply suggested fixes automatically (modifies files)
- `--jobs N`, `-j N` - Use N parallel jobs (default: all CPU cores)
- `--file PATTERN` - Only check files matching PATTERN
- `--help`, `-h` - Show help message

**Requirements:**
```bash
sudo apt install clang-tidy
```

## See Also

- [docs/FORMATTING.md](../docs/FORMATTING.md) - Complete formatting guide
- [docs/CODE_STYLE.md](../docs/CODE_STYLE.md) - Coding standards
- [docs/CLANG_TIDY.md](../docs/CLANG_TIDY.md) - Static analysis guide
