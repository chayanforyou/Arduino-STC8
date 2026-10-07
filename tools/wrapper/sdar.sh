#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if command -v python3 >/dev/null 2>&1; then
    exec python3 "$DIR/sdar_wrapper.py" "$@"
elif command -v python >/dev/null 2>&1; then
    exec python "$DIR/sdar_wrapper.py" "$@"
fi
echo "sdar: Python 3 is required but was not found in PATH" >&2
exit 1
