#!/bin/bash
# Install git hooks for the project

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
GIT_HOOKS_DIR="$PROJECT_ROOT/.git/hooks"

cd "$PROJECT_ROOT"

# Check if we're in a git repository
if [ ! -d ".git" ]; then
    echo "❌ Error: Not a git repository"
    exit 1
fi

echo "📦 Installing git hooks..."

# Create pre-commit hook
cat > "$GIT_HOOKS_DIR/pre-commit" << 'EOF'
#!/bin/bash
# Pre-commit hook to check C++ code formatting

# Colors
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Run format check on staged files
if ! ./scripts/check-format.sh --staged; then
    echo ""
    echo -e "${RED}╔═══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${RED}║  ❌ Code formatting check failed!                        ║${NC}"
    echo -e "${RED}╚═══════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${YELLOW}Options:${NC}"
    echo "  1. Fix automatically:  ./scripts/check-format.sh --staged --fix"
    echo "  2. Skip this check:    git commit --no-verify"
    echo ""
    exit 1
fi

exit 0
EOF

chmod +x "$GIT_HOOKS_DIR/pre-commit"

echo "✅ Pre-commit hook installed successfully!"
echo ""
echo "The hook will automatically check C++ formatting before each commit."
echo "To bypass the hook, use: git commit --no-verify"
