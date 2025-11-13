#!/bin/bash
# Script to check C++ code formatting
# Usage: 
#   ./scripts/check-format.sh           # Check all files
#   ./scripts/check-format.sh --fix     # Fix all files
#   ./scripts/check-format.sh --changed # Check only git changed files
#   ./scripts/check-format.sh --staged  # Check only git staged files

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_ROOT"

FIX_MODE=0
CHANGED_ONLY=0
STAGED_ONLY=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --fix|-f)
            FIX_MODE=1
            shift
            ;;
        --changed|-c)
            CHANGED_ONLY=1
            shift
            ;;
        --staged|-s)
            STAGED_ONLY=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--fix] [--changed|--staged]"
            exit 1
            ;;
    esac
done

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get list of files to check
FILES=()

if [ $STAGED_ONLY -eq 1 ]; then
    echo -e "${BLUE}🔍 Checking staged C++ files...${NC}"
    while IFS= read -r file; do
        if [[ "$file" =~ \.(cpp|hpp|h)$ ]] && [ -f "$file" ]; then
            FILES+=("$file")
        fi
    done < <(git diff --cached --name-only --diff-filter=ACM)
    
elif [ $CHANGED_ONLY -eq 1 ]; then
    echo -e "${BLUE}🔍 Checking changed C++ files...${NC}"
    while IFS= read -r file; do
        if [[ "$file" =~ \.(cpp|hpp|h)$ ]] && [ -f "$file" ]; then
            FILES+=("$file")
        fi
    done < <(git diff --name-only --diff-filter=ACM HEAD)
    
else
    echo -e "${BLUE}🔍 Checking all C++ files...${NC}"
    while IFS= read -r file; do
        FILES+=("$file")
    done < <(find svarog tests examples -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \) 2>/dev/null)
fi

# Check if we have any files to process
if [ ${#FILES[@]} -eq 0 ]; then
    echo -e "${YELLOW}⚠️  No C++ files to check${NC}"
    exit 0
fi

echo ""
FAILED=0
TOTAL=${#FILES[@]}

for file in "${FILES[@]}"; do
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
done

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
