#!/usr/bin/env bash
# Run clang-tidy on all source files in the project

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/release"
COMPILE_COMMANDS="${BUILD_DIR}/compile_commands.json"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
FIX=false
JOBS=$(nproc)
HEADER_FILTER='^((?!_deps|build).)*$'
FILE_PATTERN=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --fix)
            FIX=true
            shift
            ;;
        --jobs|-j)
            JOBS="$2"
            shift 2
            ;;
        --file)
            FILE_PATTERN="$2"
            shift 2
            ;;
        --help|-h)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --fix              Apply suggested fixes automatically"
            echo "  --jobs, -j N       Use N parallel jobs (default: $(nproc))"
            echo "  --file PATTERN     Only check files matching PATTERN"
            echo "  --help, -h         Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                           # Check all files"
            echo "  $0 --fix                     # Check and fix all files"
            echo "  $0 --file execution          # Check only files with 'execution' in path"
            echo "  $0 --jobs 4 --fix            # Use 4 parallel jobs and fix issues"
            exit 0
            ;;
        *)
            echo -e "${RED}Unknown option: $1${NC}"
            exit 1
            ;;
    esac
done

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found: $BUILD_DIR${NC}"
    echo "Please run: cmake --preset=release"
    exit 1
fi

# Check if compile_commands.json exists
if [ ! -f "$COMPILE_COMMANDS" ]; then
    echo -e "${RED}Error: compile_commands.json not found${NC}"
    echo "Please run: cmake --preset=release"
    exit 1
fi

# Check if clang-tidy is available
if ! command -v clang-tidy &> /dev/null; then
    echo -e "${RED}Error: clang-tidy not found${NC}"
    echo "Please install: sudo apt install clang-tidy"
    exit 1
fi

echo -e "${GREEN}Running clang-tidy on svarogIO project${NC}"
echo "Build directory: $BUILD_DIR"
echo "Parallel jobs: $JOBS"
if [ "$FIX" = true ]; then
    echo -e "${YELLOW}Fix mode: enabled (will modify files)${NC}"
fi
echo ""

# Find all source files
if [ -n "$FILE_PATTERN" ]; then
    SOURCE_FILES=$(find "$PROJECT_ROOT/svarog/source" -name "*.cpp" | grep "$FILE_PATTERN" || true)
    echo "Filtering files by pattern: $FILE_PATTERN"
else
    SOURCE_FILES=$(find "$PROJECT_ROOT/svarog/source" -name "*.cpp")
fi

if [ -z "$SOURCE_FILES" ]; then
    echo -e "${YELLOW}No source files found${NC}"
    exit 0
fi

FILE_COUNT=$(echo "$SOURCE_FILES" | wc -l)
echo "Found $FILE_COUNT source files to analyze"
echo ""

# Prepare clang-tidy arguments
CLANG_TIDY_ARGS="-p $BUILD_DIR --header-filter=$HEADER_FILTER"
if [ "$FIX" = true ]; then
    CLANG_TIDY_ARGS="$CLANG_TIDY_ARGS --fix"
fi

# Run clang-tidy
if [ "$JOBS" -gt 1 ]; then
    # Parallel execution
    echo "Running in parallel mode..."
    echo "$SOURCE_FILES" | xargs -P "$JOBS" -I {} bash -c "
        echo -e '${GREEN}Analyzing:${NC} {}'
        clang-tidy -p '$BUILD_DIR' --header-filter='$HEADER_FILTER' {} 2>&1 | \
            grep -v 'warnings generated' | \
            grep -v 'Suppressed.*warnings' | \
            grep -v 'Use -header-filter' || true
    "
else
    # Sequential execution
    for file in $SOURCE_FILES; do
        echo -e "${GREEN}Analyzing:${NC} $file"
        clang-tidy $CLANG_TIDY_ARGS "$file" 2>&1 | \
            grep -v 'warnings generated' | \
            grep -v 'Suppressed.*warnings' | \
            grep -v 'Use -header-filter' || true
    done
fi

echo ""
echo -e "${GREEN}clang-tidy analysis complete!${NC}"
