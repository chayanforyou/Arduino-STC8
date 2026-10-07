#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if command -v python3 >/dev/null 2>&1; then
    exec python3 "$DIR/sdcc_wrapper.py" "$@"
elif command -v python >/dev/null 2>&1; then
    exec python "$DIR/sdcc_wrapper.py" "$@"
fi
echo "sdcc: Python 3 is required but was not found in PATH" >&2
exit 1
