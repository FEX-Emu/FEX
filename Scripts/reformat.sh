#! /bin/bash

# Save current directory
DIR=$(pwd)

# Get the absolute path to the Scripts directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Parse arguments
CHANGED_ONLY=false
TARGET_DIR=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --changed) # Only reformat changed files (staged and unstaged)
            CHANGED_ONLY=true
            shift
            ;;
        *)
            TARGET_DIR=$1
            shift
            ;;
    esac
done

# Change to the directory passed as argument if any
if [ -n "$TARGET_DIR" ]; then
    cd "$TARGET_DIR"
fi

# Reformat files
if [ "$CHANGED_ONLY" = true ]; then
    # Check for unstaged deletions
    if git ls-files -d | head -1 | grep -q .; then
        echo "Error: Unstaged deletions detected. Please stage or discard deletions before formatting."
        exit 1
    fi

    CHANGED_FILES=$(git ls-files -m '*.cpp' '*.h' '*.inl')
    if [ -n "$CHANGED_FILES" ]; then
        echo "$CHANGED_FILES" | xargs -d '\n' -n 1 -P $(nproc) clang-format-19 -i
    else
        echo "No changed files to format."
    fi
else
    # Reformat whole tree (original behavior)
    git ls-files -z '*.cpp' '*.h' '*.inl' | xargs -0 -n 1 -P $(nproc) clang-format-19 -i
fi

cd $DIR
