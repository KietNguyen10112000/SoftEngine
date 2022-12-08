import os
import platform
import sys

WINDOWS = 'Windows'

osName = platform.system()

if osName == WINDOWS:
    os.system('python ./Libraries/build_windows.py')
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

