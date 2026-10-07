#!/bin/bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if command -v python3 >/dev/null 2>&1; then
    exec python3 "$DIR/sdcc_link_wrapper.py" "$@"
else
    exec python "$DIR/sdcc_link_wrapper.py" "$@"
fi