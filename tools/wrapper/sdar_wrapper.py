import os
import shutil
import subprocess
import sys

# 1. Setup SDAR binary
sdar = os.path.normpath(sys.argv[1])
if sys.platform == 'win32' and not sdar.lower().endswith('.exe'):
    sdar += '.exe'

archive = os.path.normpath(sys.argv[2])
obj = os.path.normpath(sys.argv[3])
# sys.argv[4] is 'ar' mode marker, skip it
flags = sys.argv[5:]

# 2. Convert .a -> .lib and .o -> .rel for SDCC
archive_lib = archive[:-2] + '.lib' if archive.lower().endswith('.a') else archive
obj_rel = obj[:-2] + '.rel' if obj.lower().endswith('.o') else obj

# 3. Prepare .rel file if needed
created_rel = False
if os.path.exists(obj) and (not os.path.exists(obj_rel) or os.path.getmtime(obj) > os.path.getmtime(obj_rel)):
    shutil.copy2(obj, obj_rel)
    created_rel = True

# 4. Run sdar
result = subprocess.run([sdar] + flags + [archive_lib, obj_rel])

# 5. Synchronize .lib back to .a for Arduino
if os.path.exists(archive_lib) and archive_lib != archive:
    shutil.copy2(archive_lib, archive)

# 6. Cleanup temporary .rel file
if created_rel and os.path.exists(obj_rel):
    try:
        os.remove(obj_rel)
    except OSError:
        pass

sys.exit(result.returncode)
