import sys
import subprocess
import os
import shutil

sdcc = sys.argv[1].replace('/', '\\')
if sdcc.endswith('sdcc'):
    sdcc += '.exe'

sdcc_bin_dir = os.path.dirname(sdcc)
env = os.environ.copy()
if sdcc_bin_dir not in env.get('PATH', ''):
    env['PATH'] = sdcc_bin_dir + os.pathsep + env.get('PATH', '')

flags = []
rel_files = []
driver_files = []
cleanup = []

# Parse arguments: categorize .o/.a files vs flags
for arg in sys.argv[2:]:
    norm = arg.replace('/', '\\')

    if norm.endswith('.o'):
        rel = norm[:-2] + '.rel'
        if not os.path.exists(rel) and os.path.exists(norm):
            shutil.copy2(norm, rel)
            cleanup.append(rel)
        target = rel if os.path.exists(rel) else norm
        if 'drivers\\src\\' in norm.lower() or 'drivers/src/' in arg.lower():
            driver_files.append(target)
        else:
            rel_files.append(target)

    elif norm.endswith('.a'):
        lib = norm[:-2] + '.lib'
        if not os.path.exists(lib) and os.path.exists(norm):
            shutil.copy2(norm, lib)
            cleanup.append(lib)
        rel_files.append(lib if os.path.exists(lib) else norm)

    else:
        flags.append(norm)

# Single-pass driver resolution by scanning .rel symbol tables
needed_drivers = []

if driver_files:
    # Build symbol table: which driver defines which symbol
    driver_symbols = {}  # symbol_name -> driver_path
    for drv in driver_files:
        try:
            with open(drv, 'r', errors='ignore') as f:
                for line in f:
                    # Format: "S _clock_init Def000000"
                    if line.startswith('S _'):
                        parts = line.split()
                        if len(parts) >= 3 and parts[2].startswith('Def'):
                            driver_symbols[parts[1]] = drv
        except OSError:
            pass

    # Collect all undefined symbols from non-driver .rel/.lib files
    undefined = set()
    for rf in rel_files:
        try:
            with open(rf, 'r', errors='ignore') as f:
                for line in f:
                    # Format: "S _clock_init Ref000000"
                    if line.startswith('S _'):
                        parts = line.split()
                        if len(parts) >= 3 and parts[2].startswith('Ref'):
                            undefined.add(parts[1])
        except OSError:
            pass

    # Resolve: a needed driver may itself reference other drivers
    resolved = set()
    to_resolve = set(undefined)
    while to_resolve:
        new_refs = set()
        for sym in to_resolve:
            if sym in driver_symbols and driver_symbols[sym] not in resolved:
                drv = driver_symbols[sym]
                resolved.add(drv)
                needed_drivers.append(drv)
                # Scan this driver for its own undefined references
                try:
                    with open(drv, 'r', errors='ignore') as f:
                        for line in f:
                            if line.startswith('S _'):
                                parts = line.split()
                                if len(parts) >= 3 and parts[2].startswith('Ref'):
                                    new_refs.add(parts[1])
                except OSError:
                    pass
        to_resolve = new_refs - undefined
        undefined |= new_refs

# Link once with all resolved drivers
cmd = [sdcc] + flags + rel_files + needed_drivers
result = subprocess.run(cmd, env=env)

# Cleanup temp files
for f in cleanup:
    try:
        os.remove(f)
    except OSError:
        pass

sys.exit(result.returncode)