import os
import platform
import sys

WINDOWS = 'Windows'

osName = platform.system()

if osName == WINDOWS:
    cwd = os.getcwd()
    os.chdir(cwd + '/Libraries')
    os.system('python ./build_windows.py')
    os.chdir(cwd)
else:
    print(osName + ' is not supported currently!')
    sys.exit(-1)


os.system('cmake -S . -B ./build/SoftEngine')
os.system('cmake --build ./build/SoftEngine --config Debug')
os.system('cmake --build ./build/SoftEngine --config Release')

if osName == WINDOWS:
    print('\nRun debug build test...')
    os.system('call "./build/bin/Debug/test"')
    print('\n=========================================================================================\nRun release build test...')
    os.system('call "./build/bin/Release/test"')
else:
    print(osName + ' is not supported currently!')
    sys.exit(-1)

