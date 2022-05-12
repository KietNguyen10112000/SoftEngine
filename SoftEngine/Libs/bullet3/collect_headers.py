import os
import shutil
import fnmatch

dest = '<bullet3-lib>' #'D:/KEngine/SoftEngine_Archive/downloads/bullet3-lib/include/'
patterns = ['*.h']
dir = '<bullet3-src>' #'D:/KEngine/SoftEngine_Archive/downloads/bullet3-3.21/src'

for root, dirs, files in os.walk(dir):
    for pattern in patterns:
        for filename in fnmatch.filter(files, pattern):
            source = os.path.join(root, filename)
            destRelDir = os.path.relpath(root, dir)
            destFullDir = dest + destRelDir
            
            if not os.path.exists(destFullDir):
                os.makedirs(destFullDir)
                
            shutil.copyfile(source, os.path.join(destFullDir, filename))