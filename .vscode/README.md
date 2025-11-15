# VS Code Configuration

## Required Extensions

This project requires specific VS Code extensions for consistent code formatting. When you open the project, VS Code should prompt you to install recommended extensions.

### Manual Installation

If not prompted automatically, install these extensions:

1. **Clang-Format** (`xaver.clang-format`)
   - Press `Ctrl+Shift+X` (View → Extensions)
   - Search for "Clang-Format"
   - Install extension by **xaver**
   - This ensures formatting matches `./scripts/check-format.sh`

2. **C/C++** (`ms-vscode.cpptools`)
   - Already installed if you're developing C++

## Formatting Setup

The project uses `.clang-format` configuration file located in the project root.

### Settings Applied:
- **Format on Save**: Enabled (automatic formatting when saving files)
- **Tab Size**: 4 spaces
- **Column Limit**: 120 characters
- **Style**: Defined in `.clang-format` file

### Verify Formatting

Before committing, always run:

```bash
./scripts/check-format.sh
```

To auto-fix formatting issues:

```bash
./scripts/check-format.sh --fix
```

## Troubleshooting

### VS Code formats differently than the script

1. Ensure you have **Clang-Format extension** (`xaver.clang-format`) installed
2. Check that `settings.json` has:
   ```json
   "[cpp]": {
       "editor.defaultFormatter": "xaver.clang-format"
   }
   ```
3. Reload VS Code: `Ctrl+Shift+P` → "Reload Window"
4. Run `./scripts/check-format.sh --fix` to align with project standards

### Format on Save not working

1. Check that `.vscode/settings.json` has:
   ```json
   "editor.formatOnSave": true
   ```
2. Verify the formatter is configured correctly
3. Try manual format: `Ctrl+Shift+I` or right-click → "Format Document"

## clang-format Version

This project requires **clang-format version 14+**. Check your version:

```bash
clang-format --version
```

If you have an older version, formatting may differ from CI checks.
