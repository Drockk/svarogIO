# Code Formatting Guide

This project uses **clang-format** to maintain consistent C++ code style across the codebase.

## Quick Start

### 1. Install Git Hooks (Recommended)

```bash
./scripts/install-hooks.sh
```

This installs a **pre-commit hook** that automatically checks formatting before each commit.

### 2. Manual Formatting Checks

```bash
# Check all files
./scripts/check-format.sh

# Check only changed files (fast!)
./scripts/check-format.sh --changed

# Check only staged files
./scripts/check-format.sh --staged

# Fix all files automatically
./scripts/check-format.sh --fix
```

### 3. CMake Integration

```bash
# Check all files
cmake --build build/release --target format-check

# Fix all files
cmake --build build/release --target format-fix

# Check only changed files
cmake --build build/release --target format-check-changed
```

## How It Works

### Git Pre-commit Hook

When you run `git commit`, the hook automatically:
1. ✅ Checks formatting of **only staged files** (fast)
2. ❌ Blocks commit if formatting issues found
3. 💡 Shows how to fix the issues

**Bypass the hook:**
```bash
git commit --no-verify
```

### VS Code Integration

The project is configured to **auto-format on save**:
- Settings: `.vscode/settings.json`
- Configuration: `.clang-format`
- Editor config: `.editorconfig`

### CI/CD Enforcement

GitHub Actions **blocks PRs** with formatting violations:
- Workflow: `.github/workflows/build.yml`
- Job: `format-check` (runs before build)
- Checks: All C++ files in `svarog/`, `tests/`, `examples/`

## Configuration Files

| File | Purpose |
|------|---------|
| `.clang-format` | clang-format rules (LLVM-based, 120 chars) |
| `.editorconfig` | Editor settings (4 spaces, UTF-8, LF) |
| `.vscode/settings.json` | VS Code auto-format on save |

## Style Rules

Based on [C++ Best Practices](https://github.com/cpp-best-practices/cppbestpractices/blob/master/03-Style.md):

- **Indentation:** 4 spaces (no tabs)
- **Line length:** 120 characters
- **Pointer alignment:** Left (`int* ptr`, not `int *ptr`)
- **Braces:** K&R style (opening brace on same line)
- **Namespaces:** Commented closing braces (`}  // namespace svarog`)
- **Includes:** Sorted and grouped (C++ std → C → project → external)

## Workflow Examples

### Before Committing

```bash
# Option 1: Check staged files
./scripts/check-format.sh --staged

# Option 2: Fix staged files
./scripts/check-format.sh --staged --fix

# Option 3: Let git hook do it
git commit  # Hook runs automatically
```

### During Development

```bash
# Fast check of what you changed
./scripts/check-format.sh --changed

# VS Code auto-formats on save (Ctrl+S)
# No manual intervention needed!
```

### Before Creating PR

```bash
# Check all project files
cmake --build build/release --target format-check

# Or fix everything
cmake --build build/release --target format-fix
```

## Troubleshooting

### "clang-format not found"

**Ubuntu/Debian:**
```bash
sudo apt-get install clang-format
```

**Verify installation:**
```bash
clang-format --version
# Should show: clang-format version 18.x or higher
```

### "Format check failed"

**Quick fix:**
```bash
./scripts/check-format.sh --fix
```

**Or manually:**
```bash
clang-format -i path/to/file.cpp
```

### Pre-commit Hook Not Running

**Reinstall hooks:**
```bash
./scripts/install-hooks.sh
```

**Verify hook exists:**
```bash
ls -la .git/hooks/pre-commit
# Should be executable
```

## FAQ

**Q: Do I need to run format-check manually?**  
A: No! If you installed git hooks, formatting is checked automatically before each commit.

**Q: Can I disable the pre-commit hook?**  
A: Yes, use `git commit --no-verify`, but CI will still block unformatted code in PRs.

**Q: Does VS Code format automatically?**  
A: Yes, if you have the C++ extension installed. Save any file (Ctrl+S) to auto-format.

**Q: What if I disagree with a formatting rule?**  
A: Discuss in an issue. We follow C++ Best Practices for consistency.

**Q: How do I format only my changes?**  
A: Use `./scripts/check-format.sh --changed` or `--staged`

## Integration Summary

```
Developer → Edits code → VS Code auto-formats on save
                       ↓
                  git add file.cpp
                       ↓
                  git commit
                       ↓
              Pre-commit hook checks staged files
                       ↓
              ✅ Pass → Commit created
              ❌ Fail → Fix with --fix or --no-verify
                       ↓
                  git push
                       ↓
              GitHub Actions format-check job
                       ↓
              ✅ Pass → Build proceeds
              ❌ Fail → PR blocked
```

## See Also

- [CODE_STYLE.md](CODE_STYLE.md) - Full coding standards
- [.clang-format](../.clang-format) - Formatting configuration
- [C++ Best Practices](https://github.com/cpp-best-practices/cppbestpractices)
