import os
import glob
import shutil
import sys
import subprocess


'''def move_all_files(source_dir, target_dir, regexes=[]):
    file_paths = []
    for regex in regexes:
        file_paths.extend(glob.glob(os.path.join(source_dir, regex), recursive=True))
 
    for file_path in file_paths:
        dest_path = os.path.join(target_dir, os.path.relpath(file_path, source_dir)).replace('\\', '/')
        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
        shutil.copy(file_path, dest_path)
            
        print(f"Copied {file_path} -> {target_dir}")
'''

#=============================RUNNING=================================================

def build_all_windows():
    print('build_windows');
    
    scripts = [
        r'cd ./build_scripts && python build_googletest.py',
        r'cd ./build_scripts && python build_physx.py',
        r'cd ./build_scripts && python build_assimp.py',
    ]
    
    for script in scripts:
        process = subprocess.Popen(script, shell=True)
        process.wait()
    
    
    
build_all_windows();