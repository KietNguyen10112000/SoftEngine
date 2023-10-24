import os
import glob
import shutil
import sys

from utils import copy_all_files


def googletest():
    #os.system(r'cd ../googletest');
    os.system(( r'cd ../googletest' 
                r' && cmake -DCMAKE_CXX_FLAGS_DEBUG="/MDd" -DCMAKE_CXX_FLAGS_RELEASE="/MD" -DBUILD_SHARED_LIBS=ON -S . -B ../build/vcprojects/googletest'
                r' && cmake --build ../build/vcprojects/googletest --config Debug'
                r' && cmake --build ../build/vcprojects/googletest --config Release'
    ))
    #os.system('cd ./CMakeConfig/googletest')
    #os.chdir('./build/googletest')
    #os.system('cmake --build ./build/googletest --config Debug')
    #os.system('cmake --build ./build/googletest --config Release')
    
    copy_all_files('../build/vcprojects/googletest/lib/Debug', '../../build/bin/Debug', ['**/*.lib', '**/*.pdb'])
    copy_all_files('../build/vcprojects/googletest/bin/Debug', '../../build/bin/Debug', ['**/*.dll', '**/*.pdb'])
    
    copy_all_files('../build/vcprojects/googletest/lib/Release', '../../build/bin/Release', ['**/*.lib', '**/*.pdb'])
    copy_all_files('../build/vcprojects/googletest/bin/Release', '../../build/bin/Release', ['**/*.dll', '**/*.pdb'])
    
    #copy_all_files('./googletest/googletest/include', '../build/googletest/include', ['**/*.h'])


#=============================RUNNING=================================================
googletest();