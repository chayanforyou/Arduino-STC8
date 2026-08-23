import sys
import subprocess
import os
import shutil
import re

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
# (e.g., -MF <file> is added by Arduino IDE for library detection, but SDCC doesn't support -MF
# and tries to compile <file> as a source file. -MMD when obj='nul' attempts to create 'nul.d')
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
    extra_flags.append('-x')
    extra_flags.append('c')
if mark == 'cpp':
    extra_flags.append('--include')
    extra_flags.append('dummy_variable_main.h')

# Build and execute
cmd = [sdcc] + clean_flags + extra_flags + [src, '-o', obj]
result = subprocess.run(cmd, env=env)

# If an -MF dependency file was requested during library detection on 'nul', create it
if mf_file and obj.lower() == 'nul':
    try:
        if not os.path.exists(mf_file):
            open(mf_file, 'w').close()
    except OSError:
        pass

# Clean up nul.* artifacts when output is the Windows NUL device
if obj.lower() == 'nul':
    for ext in ('.d', '.asm', '.lst', '.sym', '.rel', '.rst', '.map'):
        for f in ('nul' + ext, 'NUL' + ext):
            if os.path.isfile(f):
                try:
                    os.remove(f)
                except OSError:
                    pass
# Sync .o <-> .rel for SDCC/Arduino compatibility
elif obj.lower().endswith('.o'):
    rel = obj[:-2] + '.rel'
    if os.path.exists(obj) and not os.path.exists(rel):
        shutil.copy2(obj, rel)
    elif os.path.exists(rel) and not os.path.exists(obj):
        shutil.copy2(rel, obj)

# CSEG/GSINIT/GSFINAL alignment patching
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

    # Fix CSEG odd size
    m = re.search(r'^(A CSEG size )([0-9A-Fa-f]+)', content, re.MULTILINE)
    if m:
        size_val = int(m.group(2), 16)
        if size_val % 2 == 1:
            new_val = format(size_val + 1, 'X')
            content = content.replace(m.group(0), m.group(1) + new_val)
            modified = True

    # Fix GSINIT odd size
    m = re.search(r'^(A GSINIT size )([0-9A-Fa-f]+)', content, re.MULTILINE)
    if m:
        size_val = int(m.group(2), 16)
        if size_val % 2 == 1:
            new_val = format(size_val + 1, 'X')
            content = content.replace(m.group(0), m.group(1) + new_val)
            modified = True

    # Fix GSFINAL size 3->4 in main.c (ljmp __sdcc_program_startup is 3 bytes)
    if 'main.c' in os.path.basename(rel_path):
        if 'A GSFINAL size 3' in content:
            content = content.replace('A GSFINAL size 3', 'A GSFINAL size 4')
            modified = True

    if modified:
        try:
            with open(rel_path, 'w') as f:
                f.write(content)
        except OSError:
            pass

# Apply alignment patching to the .rel file
if obj.lower() != 'nul':
    if obj.lower().endswith('.o'):
        patch_segment_alignment(obj[:-2] + '.rel')
    elif obj.lower().endswith('.rel'):
        patch_segment_alignment(obj)

sys.exit(result.returncode)