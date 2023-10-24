import os
import glob
import shutil
import sys

from utils import copy_all_files


def assimp():
    os.system(( r'cd ../assimp' 
                r' && cmake -DCMAKE_CXX_FLAGS_DEBUG="/MDd" -DCMAKE_CXX_FLAGS_RELEASE="/MD" -DBUILD_SHARED_LIBS=ON -S . -B ../build/vcprojects/assimp'
                r' && cmake --build ../build/vcprojects/assimp --config Debug'
                r' && cmake --build ../build/vcprojects/assimp --config Release'
    ))
    
    copy_all_files('../build/vcprojects/assimp/lib/Debug', '../../build/bin/Debug', ['**/*.dll', '**/*.lib', '**/*.pdb'])
    copy_all_files('../build/vcprojects/assimp/bin/Debug', '../../build/bin/Debug', ['**/*.dll', '**/*.lib', '**/*.pdb'])
    
    copy_all_files('../build/vcprojects/assimp/lib/Release', '../../build/bin/Release', ['**/*.dll', '**/*.lib', '**/*.pdb'])
    copy_all_files('../build/vcprojects/assimp/bin/Release', '../../build/bin/Release', ['**/*.dll', '**/*.lib', '**/*.pdb'])
    
#=============================RUNNING=================================================
assimp();