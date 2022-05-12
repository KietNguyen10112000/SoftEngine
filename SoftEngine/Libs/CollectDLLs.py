import glob, os, shutil

def Copy(source_dir, dest_dir, ext):
    files = glob.iglob(os.path.join(source_dir, ext))
    for file in files:
        if os.path.isfile(file):
            shutil.copy2(file, dest_dir)

Copy('./v8/v8-lib/bin/debug/', '../../out/build/x64-Debug/SoftEngine/', '*.dll')
Copy('./v8/v8-lib/bin/release/', '../../out/build/x64-Release/SoftEngine/', '*.dll')

Copy('./assimp/bin/', '../../out/build/x64-Debug/SoftEngine/', '*.dll')
Copy('./assimp/bin/', '../../out/build/x64-Release/SoftEngine/', '*.dll')
