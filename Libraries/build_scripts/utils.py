import os
import glob
import shutil
import sys

def copy_all_files(source_dir, target_dir, regexes=[]):
    #base_path = os.getcwd()

    file_paths = []
    for regex in regexes:
        file_paths.extend(glob.glob(os.path.join(source_dir, regex), recursive=True))
 
    for file_path in file_paths:
        dest_path = os.path.join(target_dir, os.path.relpath(file_path, source_dir)).replace('//', '/')
        #dest_path = os.path.join(base_path, dest_path)
        
        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
        
        shutil.copyfile(file_path, dest_path)
        print(f"Copied {file_path} -> {dest_path}")