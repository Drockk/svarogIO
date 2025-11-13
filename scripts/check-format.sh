#!/bin/bash
# Script to check C++ code formatting
# Usage: ./scripts/check-format.sh [--fix]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

FIX_MODE=0
if [ "$1" = "--fix" ] || [ "$1" = "-f" ]; then
    FIX_MODE=1
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "🔍 Checking C++ code formatting..."
echo ""

FAILED=0
TOTAL=0

while IFS= read -r file; do
    TOTAL=$((TOTAL + 1))
    
    if [ $FIX_MODE -eq 1 ]; then
        # Fix mode - format the file
        clang-format -i "$file"
        echo "✅ Formatted: $file"
    else
        # Check mode - verify formatting
        if ! clang-format --dry-run --Werror "$file" 2>&1 > /dev/null; then
            echo -e "${RED}❌ Format check failed:${NC} $file"
            FAILED=$((FAILED + 1))
        fi
    fi
done < <(find svarog tests examples -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) 2>/dev/null)

echo ""
echo "================================================"

if [ $FIX_MODE -eq 1 ]; then
    echo -e "${GREEN}✅ Formatted $TOTAL files${NC}"
    exit 0
fi

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}✅ All $TOTAL files are properly formatted!${NC}"
    exit 0
else
    echo -e "${RED}❌ Format check failed for $FAILED out of $TOTAL files${NC}"
    echo ""
    echo "To fix formatting issues, run:"
    echo -e "${YELLOW}  ./scripts/check-format.sh --fix${NC}"
    echo ""
    echo "Or manually:"
    echo -e "${YELLOW}  find svarog tests examples -type f \\( -name \"*.cpp\" -o -name \"*.hpp\" \\) -exec clang-format -i {} \\;${NC}"
    exit 1
fi
