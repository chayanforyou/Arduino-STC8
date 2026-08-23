import sys
import os

if len(sys.argv) > 1:
    path = sys.argv[1].replace('/', '\\')
    if os.path.exists(path):
        try:
            with open(path, 'r', errors='ignore') as f:
                sys.stdout.write(f.read())
        except Exception:
            pass
