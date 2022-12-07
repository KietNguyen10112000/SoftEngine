import os
import platform
import sys

osName = platform.system()

if osName == 'Window':
    os.system('python ./Libraries/build_windows.py')
else:
    print(osName + ' is not supported currently!'
    sys.exit(-1)


os.system('cmake -S . -B ./build/SoftEngine')
os.system('cmake --build ./build/SoftEngine --config Debug')
os.system('cmake --build ./build/SoftEngine --config Release')

if osName == 'Window':
    print('Run debug build test...\n')
    os.system('"./build/bin/Debug/test"')
    print('\n=========================================================================================\nRun release build test...\n')
    os.system('"./build/bin/Release/test"')
else:
    print(osName + ' is not supported currently!'
    sys.exit(-1)

