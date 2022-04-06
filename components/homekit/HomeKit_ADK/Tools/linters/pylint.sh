#!/bin/bash -e

ADK_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../../"

usage()
{
    echo "A tool to check and fix code style in python files."
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "OPTIONS:"
    echo "-f  - Lint the provided file. If no option is provided all files will be linted."
    echo "-d  - Lint the files in the specified directory."
    echo "-c  - Correct the style issues in the specified files or directories."
    echo ""
    echo "EXAMPLES: "
    echo "$0 -f <file_name>          # Lint the specified file"
    echo "$0 -f <file_name> -f <file2> # Lint the two specified files"
    echo "$0 -f <file_name> -c       # Lint and fix the specified file"
    echo "$0 -d <dir_name>           # Lint files in the specified directory"
    echo "$0 -d <dir_name> -c        # Lint and fix all files in the specified directory"
    echo "$0                         # Lint all files in this repository"
    echo "$0 -c                      # Fix all files in this repository, in parallel"
    exit 1
}

PYLINT="$(python3 -m site --user-base)/bin/black"
CORRECT=false
SEARCH_PATH=$ADK_ROOT
PARALLEL=8
SOURCE_FILES=()

while getopts "hcf:d:" opt; do
    case ${opt} in
        f ) SOURCE_FILES+=("$OPTARG");;
        d ) SEARCH_PATH="$OPTARG";;
        c ) CORRECT=true;;
        h ) usage;;
        \? ) usage;;
    esac
done

# Install python linter
if ! pip3 show black > /dev/null 2>&1; then
    echo "Installing python linter (black)..."

    if command -v pip3 > /dev/null 2>&1; then
        pip3 -q install black==21.7b0 --user
    else
        echo "***************************************************************************"
        echo "Python3 is required to run this tool. Please install Python3 and try again."
        echo "***************************************************************************"
        exit 1
    fi
fi

if [ ${#SOURCE_FILES[@]} -gt 0 ]; then
    for file in "${SOURCE_FILES[@]}"; do
        if [[ ! -f "$file" ]]; then
            echo "Skipping invalid file '$file' (does not exist)"
            continue
        fi
    done
else
    echo "Looking for files in $SEARCH_PATH directory..."
    while IFS=  read -r -d $'\0'; do
        SOURCE_FILES+=("$REPLY")
    done < <(find "$SEARCH_PATH"  \
        \( \
            -name "*.py" \
        \) \
        -not -path "*Output*" \
        -not -path "*/External/*" \
        -not -path "*/node_modules/*" \
        ! -type l \
        -print0
    )
fi

# Exit early if no files are going to be checked.
if [[ ${#SOURCE_FILES[@]} -eq 0 ]]; then
    echo "No source files to lint!"
    usage
fi

ERROR=false
if ( $CORRECT ); then
    printf "%s\0" "${SOURCE_FILES[@]}" | xargs -t -0 -P ${PARALLEL} -I{} -- "$PYLINT" {}
else
    for file in "${SOURCE_FILES[@]}"; do
        echo "Checking file: $file"
        if ! "$PYLINT" "$file" --check; then
            ERROR=true
        fi
    done
fi

# If user asked to fix the style issues then we are done
if $CORRECT; then
    exit 0
fi

# If no patch has been generated, clean up the file stub and exit
if ! "$ERROR"; then
    echo "No code style issues found"
    exit 0
fi

exit 1
