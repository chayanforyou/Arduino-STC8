import os
import shutil
import subprocess
import sys


def prepare_file(path, old_ext, new_ext, cleanup_list):
    """Convert object/archive extension (.o -> .rel, .a -> .lib) if newer."""
    if path.endswith(old_ext):
        target = path[:-len(old_ext)] + new_ext
        if os.path.exists(path) and (not os.path.exists(target) or os.path.getmtime(path) > os.path.getmtime(target)):
            shutil.copy2(path, target)
            cleanup_list.append(target)
        return target if os.path.exists(target) else path
    return path


def parse_rel_file(filepath):
    """Parse a .rel/.lib file in a single pass, returning (defined, referenced) symbol sets."""
    defs = set()
    refs = set()
    try:
        with open(filepath, 'r', errors='ignore') as f:
            for line in f:
                if line.startswith('S '):
                    parts = line.split()
                    if len(parts) >= 3:
                        if parts[2].startswith('Def'):
                            defs.add(parts[1])
                        elif parts[2].startswith('Ref'):
                            refs.add(parts[1])
    except OSError:
        pass
    return defs, refs


# 1. Setup SDCC binary and PATH
sdcc = os.path.normpath(sys.argv[1])
if sys.platform == 'win32' and not sdcc.lower().endswith('.exe'):
    sdcc += '.exe'

sdcc_bin_dir = os.path.dirname(sdcc)
env = os.environ.copy()
if sdcc_bin_dir not in env.get('PATH', ''):
    env['PATH'] = sdcc_bin_dir + os.pathsep + env.get('PATH', '')

# 2. Parse arguments: categorize .o/.a files vs options vs libraries
opt_flags = []
lib_flags = []
rel_files = []
driver_files = []
cleanup = []

for arg in sys.argv[2:]:
    norm = os.path.normpath(arg)
    if norm.endswith('.o'):
        target = prepare_file(norm, '.o', '.rel', cleanup)
        if os.path.join('drivers', 'src') in norm.lower():
            driver_files.append(target)
        else:
            rel_files.append(target)
    elif norm.endswith('.a'):
        rel_files.append(prepare_file(norm, '.a', '.lib', cleanup))
    elif norm.startswith('-l') or norm.startswith('-L'):
        lib_flags.append(norm)
    else:
        opt_flags.append(norm)

# 3. Selective driver resolution (Dead code elimination for MCS-51 flash optimization)
needed_drivers = []
if driver_files:
    # Parse each driver file once; cache both Def and Ref symbols
    driver_cache = {drv: parse_rel_file(drv) for drv in driver_files}

    # Map defined symbol -> driver file
    driver_symbols = {
        sym: drv
        for drv, (defs, _) in driver_cache.items()
        for sym in defs
    }

    # Collect undefined references from sketch & core files
    undefined = set()
    for rf in rel_files:
        _, refs = parse_rel_file(rf)
        undefined |= refs

    # Iteratively resolve chained driver dependencies
    resolved = set()
    to_resolve = set(undefined)
    while to_resolve:
        new_refs = set()
        for sym in to_resolve:
            drv = driver_symbols.get(sym)
            if drv and drv not in resolved:
                resolved.add(drv)
                _, drv_refs = driver_cache[drv]
                new_refs |= drv_refs
        to_resolve = new_refs - undefined
        undefined |= new_refs

    # Link in the order Arduino passed the drivers, not set iteration order
    # (which changes between runs), so identical sources give identical output
    needed_drivers = [drv for drv in driver_files if drv in resolved]

# 4. Execute link (Objects first, then libraries for proper symbol resolution)
cmd = [sdcc] + opt_flags + rel_files + needed_drivers + lib_flags
result = subprocess.run(cmd, env=env)

# 5. Cleanup temporary files
for f in cleanup:
    try:
        os.remove(f)
    except OSError:
        pass

sys.exit(result.returncode)