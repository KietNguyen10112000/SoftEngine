import os
import glob
import shutil
import sys

from utils import copy_all_files

def physx():
    copy_all_files('./', '../PhysX/physx/buildtools/presets/public', ['**/*xml']);

    os.system(( r'cd ../PhysX/physx' 
                r' && call generate_projects.bat vc17win64'
                r' && cmake --build ./compiler/vc17win64 --config Debug'
                r' && cmake --build ./compiler/vc17win64 --config Release'
    ))
    copy_all_files('../PhysX/physx/bin/win.x86_64.vc143.md/debug', '../../build/bin/Debug', ['**/*.dll', '**/*.lib', '**/*.pdb'])
    copy_all_files('../PhysX/physx/bin/win.x86_64.vc143.md/release', '../../build/bin/Release', ['**/*.dll', '**/*.lib', '**/*.pdb'])

#=============================RUNNING=================================================
physx();