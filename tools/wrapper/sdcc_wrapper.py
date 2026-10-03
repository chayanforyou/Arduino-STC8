import os
import re
import shutil
import subprocess
import sys

# Get arguments
sdcc = sys.argv[1].replace('/', '\\')
if sdcc.endswith('sdcc'):
    sdcc += '.exe'

sdcc_bin_dir = os.path.dirname(sdcc)

# Ensure cc1.exe exists (one-time check via marker file)
marker = os.path.join(sdcc_bin_dir, '.cc1_checked')
if not os.path.exists(marker):
    cc1 = os.path.join(sdcc_bin_dir, 'cc1')
    cc1_exe = cc1 + '.exe'
    if os.path.isfile(cc1) and not os.path.exists(cc1_exe):
        shutil.copy2(cc1, cc1_exe)
    try:
        open(marker, 'w').close()
    except OSError:
        pass

# Ensure SDCC bin dir is in PATH for sdcpp to find cc1.exe
env = os.environ.copy()
if sdcc_bin_dir not in env.get('PATH', ''):
    env['PATH'] = sdcc_bin_dir + os.pathsep + env.get('PATH', '')

src = sys.argv[2]
obj = sys.argv[3]
mark = sys.argv[4]
raw_flags = sys.argv[5:]

# Filter flags that SDCC does not understand or that break on Windows NUL output
# (-MF <file> is added by Arduino IDE for library detection, but SDCC doesn't support -MF)
clean_flags = []
skip_next = False
mf_file = None

for flag in raw_flags:
    if skip_next:
        mf_file = flag
        skip_next = False
        continue
    if flag == '-MF':
        skip_next = True
        continue
    if flag.startswith('-MF'):
        mf_file = flag[3:]
        continue
    if obj.lower() == 'nul' and flag == '-MMD':
        continue
    clean_flags.append(flag)

# SDCC only recognizes .c files natively; all others need -x c
extra_flags = []
if mark in ('cpp', 'preproc') or not src.lower().endswith('.c'):
    extra_flags.extend(['-x', 'c'])
if mark == 'cpp':
    extra_flags.extend(['--include', 'dummy_variable_main.h'])

# Build and execute compiler
cmd = [sdcc] + clean_flags + extra_flags + [src, '-o', obj]
result = subprocess.run(cmd, env=env)


def patch_segment_alignment(rel_path):
    """Patch odd CSEG/GSINIT sizes to even, and fix GSFINAL size 3->4 in main."""
    if not os.path.isfile(rel_path):
        return

    try:
        with open(rel_path, 'r', errors='ignore') as f:
            content = f.read()
    except OSError:
        return

    modified = False

    # Fix odd CSEG and GSINIT sizes to even
    for seg in ('CSEG', 'GSINIT'):
        m = re.search(rf'^(A {seg} size )([0-9A-Fa-f]+)', content, re.MULTILINE)
        if m:
            size_val = int(m.group(2), 16)
            if size_val % 2 == 1:
                content = content.replace(m.group(0), f'{m.group(1)}{size_val + 1:X}')
                modified = True

    # Fix GSFINAL size 3->4 in main.c (ljmp __sdcc_program_startup is 3 bytes)
    if 'main.c' in os.path.basename(rel_path) and 'A GSFINAL size 3' in content:
        content = content.replace('A GSFINAL size 3', 'A GSFINAL size 4')
        modified = True

    if modified:
        try:
            with open(rel_path, 'w') as f:
                f.write(content)
        except OSError:
            pass


# Handle outputs
if obj.lower() == 'nul':
    # If an -MF dependency file was requested during library detection on 'nul', create it
    if mf_file:
        try:
            if not os.path.exists(mf_file):
                open(mf_file, 'w').close()
        except OSError:
            pass

    # Clean up nul.* artifacts created on Windows
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