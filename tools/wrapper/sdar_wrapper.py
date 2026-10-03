import sys
import subprocess
import os
import shutil

sdar = sys.argv[1].replace('/', '\\')
if sdar.endswith('sdar'):
    sdar += '.exe'

archive = sys.argv[2].replace('/', '\\')
obj = sys.argv[3].replace('/', '\\')
# sys.argv[4] is 'ar' marker, skip it
flags = sys.argv[5:]

# Convert .a -> .lib and .o -> .rel for SDCC
archive_lib = archive[:-2] + '.lib' if archive.lower().endswith('.a') else archive
obj_rel = obj[:-2] + '.rel' if obj.lower().endswith('.o') else obj

# Only copy if needed
created_rel = False
if os.path.exists(obj) and (not os.path.exists(obj_rel) or os.path.getmtime(obj) > os.path.getmtime(obj_rel)):
    shutil.copy2(obj, obj_rel)
    created_rel = True

# Run sdar
result = subprocess.run([sdar] + flags + [archive_lib, obj_rel])

# Sync .lib -> .a
if os.path.exists(archive_lib) and archive_lib != archive:
    shutil.copy2(archive_lib, archive)

# Clean up only if we created the temp .rel
if created_rel and os.path.exists(obj_rel):
    os.remove(obj_rel)

sys.exit(result.returncode)
