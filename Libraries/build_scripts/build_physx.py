import os
import glob
import shutil
import sys

from utils import copy_all_files


def physx():
    os.system(( r'cd ../PhysX/physx' 
                r' && call generate_projects.bat < ../../build_scripts/build_physx.txt'
                r' && cmake --build ./compiler/vc17win64 --config Debug'
                r' && cmake --build ./compiler/vc17win64 --config Release'
    ))
    copy_all_files('../PhysX/physx/bin/win.x86_64.vc143.mt/debug', '../../build/bin/Debug', ['**/*.dll', '**/*.lib', '**/*.pdb'])
    copy_all_files('../PhysX/physx/bin/win.x86_64.vc143.mt/release', '../../build/bin/Release', ['**/*.dll', '**/*.lib', '**/*.pdb'])

#=============================RUNNING=================================================
physx();