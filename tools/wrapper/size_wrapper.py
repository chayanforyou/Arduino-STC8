import os
import sys

if len(sys.argv) > 1:
    path = os.path.normpath(sys.argv[1])
    if os.path.isfile(path):
        try:
            with open(path, 'r', errors='ignore') as f:
                sys.stdout.write(f.read())
        except OSError:
            pass
