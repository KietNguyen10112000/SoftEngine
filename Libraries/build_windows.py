import os
import glob
import shutil

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

def copy_all_files(source_dir, target_dir, regexes=[]):
    #base_path = os.getcwd()

    file_paths = []
    for regex in regexes:
        file_paths.extend(glob.glob(os.path.join(source_dir, regex), recursive=True))
 
    for file_path in file_paths:
        dest_path = os.path.join(target_dir, os.path.relpath(file_path, source_dir)).replace('\\', '/')
        #dest_path = os.path.join(base_path, dest_path)
        
        os.makedirs(os.path.dirname(dest_path), exist_ok=True)
        
        shutil.copyfile(file_path, dest_path)
        print(f"Copied {file_path} -> {dest_path}")


def googletest():
    os.system(r'cmake -DCMAKE_CXX_FLAGS_DEBUG="/MDd" -DCMAKE_CXX_FLAGS_RELEASE="/MD" -DBUILD_SHARED_LIBS=ON -S ./googletest -B ./build/googletest')
    #os.system('cd ./CMakeConfig/googletest')
    #os.chdir('./build/googletest')
    os.system('cmake --build ./build/googletest --config Debug')
    os.system('cmake --build ./build/googletest --config Release')
    
    copy_all_files('./build/googletest/lib/Debug', './build/bin/Debug', ['**/*.lib', '**/*.pdb'])
    copy_all_files('./build/googletest/bin/Debug', '../build/bin/Debug', ['**/*.dll', '**/*.pdb'])
    
    copy_all_files('./build/googletest/lib/Release', './build/bin/Release', ['**/*.lib', '**/*.pdb'])
    copy_all_files('./build/googletest/bin/Release', '../build/bin/Release', ['**/*.dll', '**/*.pdb'])
    
    copy_all_files('./googletest/googletest/include', './build/include/googletest', ['**/*.h'])
    





#=============================RUNNING=================================================
googletest()