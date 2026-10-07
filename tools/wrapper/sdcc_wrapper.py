import os
import re
import shutil
import subprocess
import sys


def patch_segment_alignment(rel_path):
    """Patch odd CSEG/GSINIT sizes to even, and fix GSFINAL size 3->4 in main.c."""
    if not os.path.isfile(rel_path):
        return

    try:
        with open(rel_path, 'r', errors='ignore') as f:
            content = f.read()
    except OSError:
        return

    def fix_size(m):
        val = int(m.group(2), 16)
        return f"{m.group(1)}{val + 1:X}" if val % 2 == 1 else m.group(0)

    # Fix odd CSEG and GSINIT sizes to even to prevent linker segment overlaps
    new_content = re.sub(r'^(A (?:CSEG|GSINIT) size )([0-9A-Fa-f]+)', fix_size, content, flags=re.MULTILINE)

    # Fix GSFINAL size 3->4 in main.c (ljmp __sdcc_program_startup is 3 bytes)
    if 'main.c' in os.path.basename(rel_path):
        new_content = re.sub(r'^(A GSFINAL size )3\b', r'\g<1>4', new_content, flags=re.MULTILINE)

    if new_content != content:
        try:
            with open(rel_path, 'w') as f:
                f.write(new_content)
        except OSError:
            pass


# 1. Setup SDCC binary and PATH
sdcc = os.path.normpath(sys.argv[1])
if sys.platform == 'win32' and not sdcc.lower().endswith('.exe'):
    sdcc += '.exe'

sdcc_bin_dir = os.path.dirname(sdcc)
env = os.environ.copy()
if sdcc_bin_dir not in env.get('PATH', ''):
    env['PATH'] = sdcc_bin_dir + os.pathsep + env.get('PATH', '')

src = sys.argv[2]
obj = sys.argv[3]
mark = sys.argv[4]
raw_flags = sys.argv[5:]

is_null_out = obj.lower() in ('nul', '/dev/null') or (
    hasattr(os, 'devnull') and os.path.abspath(obj) == os.path.abspath(os.devnull)
)

# 2. Filter flags incompatible with SDCC (e.g., -MF added by Arduino IDE for library scanning)
clean_flags = []
skip_next = False
mf_file = None

for flag in raw_flags:
    if skip_next:
        mf_file = flag
        skip_next = False
    elif flag == '-MF':
        skip_next = True
    elif flag.startswith('-MF'):
        mf_file = flag[3:]
    elif is_null_out and flag == '-MMD':
        continue
    else:
        clean_flags.append(flag)

# 3. SDCC language compatibility flags
extra_flags = []
if mark in ('cpp', 'preproc') or not src.lower().endswith('.c'):
    extra_flags.extend(['-x', 'c'])
if mark == 'cpp':
    extra_flags.extend(['--include', 'dummy_variable_main.h'])

# 4. Execute compiler
cmd = [sdcc] + clean_flags + extra_flags + [src, '-o', obj]
result = subprocess.run(cmd, env=env)
if result.returncode != 0:
    sys.exit(result.returncode)

# 5. Handle output artifacts
if is_null_out:
    # Create empty dependency file expected by Arduino IDE library discovery
    if mf_file:
        try:
            if not os.path.exists(mf_file):
                open(mf_file, 'w').close()
        except OSError:
            pass

    # Clean up stray nul.* artifacts created on Windows
    for ext in ('.d', '.asm', '.lst', '.sym', '.rel', '.rst', '.map'):
        for f in ('nul' + ext, 'NUL' + ext):
            if os.path.isfile(f):
                try:
                    os.remove(f)
                except OSError:
                    pass
else:
    # Synchronize .o <-> .rel and apply segment alignment patch
    if obj.lower().endswith('.o'):
        rel = obj[:-2] + '.rel'
        if os.path.exists(obj):
            shutil.copy2(obj, rel)
        patch_segment_alignment(rel)
        if os.path.exists(rel):
            shutil.copy2(rel, obj)
    elif obj.lower().endswith('.rel'):
        o_file = obj[:-4] + '.o'
        patch_segment_alignment(obj)
        if os.path.exists(obj):
            shutil.copy2(obj, o_file)

sys.exit(result.returncode)