import os
import fnmatch

patterns = ['*.lib']
dir = 'D:/KEngine/SoftEngine_Archive/downloads/bullet3-lib/bin/Release' #'D:/KEngine/SoftEngine_Archive/downloads/bullet3-3.21/src'

for root, dirs, files in os.walk(dir):
    for pattern in patterns:
        for filename in fnmatch.filter(files, pattern):
            print(filename)