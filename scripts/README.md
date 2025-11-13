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

## See Also

- [docs/FORMATTING.md](../docs/FORMATTING.md) - Complete formatting guide
- [docs/CODE_STYLE.md](../docs/CODE_STYLE.md) - Coding standards
