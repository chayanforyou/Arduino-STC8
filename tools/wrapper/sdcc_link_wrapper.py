import os
import shutil
import subprocess
import sys

# Setup SDCC binary and PATH
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


def prepare_file(path, old_ext, new_ext):
    """Convert object/archive extension (.o -> .rel, .a -> .lib) if newer."""
    if path.endswith(old_ext):
        target = path[:-len(old_ext)] + new_ext
        if os.path.exists(path) and (not os.path.exists(target) or os.path.getmtime(path) > os.path.getmtime(target)):
            shutil.copy2(path, target)
            cleanup.append(target)
        return target if os.path.exists(target) else path
    return path


def parse_symbols(filepath, sym_type):
    """Extract defined ('Def') or referenced ('Ref') symbols from a .rel/.lib file."""
    symbols = set()
    try:
        with open(filepath, 'r', errors='ignore') as f:
            for line in f:
                if line.startswith('S _'):
                    parts = line.split()
                    if len(parts) >= 3 and parts[2].startswith(sym_type):
                        symbols.add(parts[1])
    except OSError:
        pass
    return symbols


# Parse arguments: categorize .o/.a files vs flags
for arg in sys.argv[2:]:
    norm = arg.replace('/', '\\')
    if norm.endswith('.o'):
        target = prepare_file(norm, '.o', '.rel')
        if 'drivers\\src\\' in norm.lower() or 'drivers/src/' in arg.lower():
            driver_files.append(target)
        else:
            rel_files.append(target)
    elif norm.endswith('.a'):
        rel_files.append(prepare_file(norm, '.a', '.lib'))
    else:
        flags.append(norm)

# Single-pass driver resolution: only link drivers that supply referenced symbols
needed_drivers = []
if driver_files:
    # Map defined symbol -> driver file
    driver_symbols = {
        sym: drv
        for drv in driver_files
        for sym in parse_symbols(drv, 'Def')
    }

    # Collect undefined references from sketch & core files
    undefined = {sym for rf in rel_files for sym in parse_symbols(rf, 'Ref')}

    # Iteratively resolve chained driver dependencies
    resolved = set()
    to_resolve = set(undefined)
    while to_resolve:
        new_refs = set()
        for sym in to_resolve:
            drv = driver_symbols.get(sym)
            if drv and drv not in resolved:
                resolved.add(drv)
                needed_drivers.append(drv)
                new_refs |= parse_symbols(drv, 'Ref')
        to_resolve = new_refs - undefined
        undefined |= new_refs

# Execute link
cmd = [sdcc] + flags + rel_files + needed_drivers
result = subprocess.run(cmd, env=env)

# Cleanup temporary files
for f in cleanup:
    try:
        os.remove(f)
    except OSError:
        pass

sys.exit(result.returncode)