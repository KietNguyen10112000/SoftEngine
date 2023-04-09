# import time module, Observer, FileSystemEventHandler
import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

import socket
import os
import subprocess
import traceback
import shutil
import pickle 
import atexit
import sys

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 2222  # Port to listen on (non-privileged ports are > 1023)

class MyWatch:
    # path : datetime
    modifiedFiles = {}

    instance = None
 
    def __init__(self, cfgStr):
        self.observer = Observer()
        self.compileDict = {}
        self.srcPath = ''
        self.destPath = ''
        self.duplicatePaths = []
        self.sign = ''
        self.ProcessArgs(cfgStr)
        self.errorFiles = {}


    def FlushErrorFiles(self):
        for file in self.errorFiles:
            MyWatch.modifiedFiles[file] = self.errorFiles[file]

        self.errorFiles.clear()


    def Contains(self, file):
        for inputExt in self.compileDict:
            if file.endswith(inputExt):
                return True

        return False


    def ProcessArgs(self, arg: str):
        args = arg.split(',')

        for s in args:
            arr = s.strip().split(' ', 1)
            arr2 = arr[0].split('->')

            if len(arr2) > 1:
                self.compileDict[arr2[0]] = (arr2[1], arr[1])
            elif arr[0] == 'Src':
                self.srcPath = arr[1].replace('\\\\', '/').replace('\\', '/')
            elif arr[0] == 'Dest':
                self.destPath = arr[1].replace('\\\\', '/').replace('\\', '/')
            elif arr[0] == 'Dupl':
                self.duplicatePaths.append(arr[1].replace('\\\\', '/').replace('\\', '/'))
            elif arr[0] == 'Sign':
                self.sign =  arr[1]


    def Compile(self, ret, cmd, grapedInputFiles, grapedOutputFiles, grapedDuplFiles):
        for idx, file in enumerate(grapedInputFiles):
            outputFile = grapedOutputFiles[idx]
            duplFiles = grapedDuplFiles[idx]

            os.makedirs(os.path.dirname(outputFile), exist_ok=True)

            _cmd = cmd.replace('<input>', file).replace('<output>', outputFile)
            print(_cmd)

            out = subprocess.run(_cmd, shell=True, capture_output=True, text=True);
            #print(out.stderr)
            #print(out.returncode)
            ret.append(out)

            if out.returncode == 0:
                for dFile in duplFiles:
                    os.makedirs(os.path.dirname(dFile), exist_ok=True)
                    shutil.copyfile(outputFile, dFile)
                    print(f"Copied {outputFile} -> {dFile}")
            else:
                print(f'[ERROR]: failed to compile \'{file}\'')
                self.errorFiles[file] = str(os.path.getmtime(file))

            
    def CompileAll(self):
        ret = []

        for inputExt in self.compileDict:
            opt = self.compileDict[inputExt]

            outputExt = opt[0]
            cmd = opt[1]

            grapedInputFiles = []
            grapedOutputFiles = []
            grapedDuplFiles = []

            for root, dirs, files in os.walk(self.srcPath):
                for file in files:
                    if file.endswith(inputExt):
                        grapedInputFiles.append(os.path.join(root, file).replace("\\","/"))
                        outPathSrc = os.path.join(root, file.replace(inputExt, outputExt)) \
                            .replace("\\","/")
                            
                        outPathDest = outPathSrc.replace(self.srcPath, self.destPath)
                        grapedOutputFiles.append(outPathDest)

                        dupl = []
                        for duplPath in self.duplicatePaths:
                            dupl.append(outPathSrc.replace(self.srcPath, duplPath))

                        grapedDuplFiles.append(dupl)
    
            self.Compile(ret, cmd, grapedInputFiles, grapedOutputFiles, grapedDuplFiles)

        self.FlushErrorFiles()
        
        return ret


    def SaveAllWatchedFiles(self):
        watchedfiles = {}
        for root, dirs, files in os.walk(self.srcPath):
            for file in files:
                path = os.path.join(root, file).replace("\\","/")
                watchedfiles[path] = str(os.path.getmtime(path))

        #print('[LOG]: all watched files saved')
        #print(watchedfiles)

        with open(f'{self.sign}.watchedfiles', 'wb') as f:
            pickle.dump(watchedfiles, f)


    def StartUp(self):
        path = f'{self.sign}.watchedfiles'

        watchedfiles = None
        if os.path.exists(path) and os.stat(path).st_size != 0:
            with open(path, 'rb') as f:
                watchedfiles = pickle.load(f)

            with open(path, 'w'): pass

            for root, dirs, files in os.walk(self.srcPath):
                for file in files:
                    fullPath = os.path.join(root, file).replace("\\","/")

                    if fullPath in watchedfiles:
                        lastModifies = str(os.path.getmtime(fullPath))
                        if lastModifies != watchedfiles[fullPath]:
                            MyWatch.modifiedFiles[fullPath] = lastModifies
                    else:
                        MyWatch.modifiedFiles[fullPath] = lastModifies

            self.CompileChanged()

        else:
            self.CompileAll()


    def CompileChanged(self):
        ret = []

        for inputExt in self.compileDict:
            opt = self.compileDict[inputExt]

            outputExt = opt[0]
            cmd = opt[1]

            grapedInputFiles = []
            grapedOutputFiles = []
            grapedDuplFiles = []

            for file in MyWatch.modifiedFiles:
                if os.path.exists(file) and file.endswith(inputExt):
                    grapedInputFiles.append(file)

                    outPathSrc = file.replace(inputExt, outputExt).replace("\\","/")

                    grapedOutputFiles.append(outPathSrc.replace(self.srcPath, self.destPath))

                    dupl = []
                    for duplPath in self.duplicatePaths:
                        dupl.append(outPathSrc.replace(self.srcPath, duplPath))

                    grapedDuplFiles.append(dupl)

            self.Compile(ret, cmd, grapedInputFiles, grapedOutputFiles, grapedDuplFiles)
 
        MyWatch.modifiedFiles.clear()
        self.FlushErrorFiles()
        return ret


    def run(self):
        event_handler = Handler()
        self.observer.schedule(event_handler, self.srcPath, recursive = True)
        self.observer.start()
        
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.bind((HOST, PORT))
                s.listen()
                s.settimeout(1)

                while True:
                    conn = None
                    try:
                        conn, addr = s.accept()
                    except socket.timeout:
                        time.sleep(5)
                        continue
                    
                    with conn:
                        print(f"Connected by {addr}")
                        
                        data = conn.recv(1024)
                        if not data:
                            continue
                            
                        sign = data.decode()
                        if sign != self.sign:
                            conn.sendall(str.encode(f'[ERROR]: expected \'{self.sign}\', but given \'{sign}\''))
                        else:
                            rets = self.CompileChanged()

                            countErr = 0
                            for ret in rets:
                                if (ret.returncode != 0):
                                    countErr += 1

                                    conn.sendall(str.encode(str(ret.stdout)));
                                    conn.sendall(str.encode(str(ret.stderr)));
                                
                        
                            if (countErr == 0):
                                conn.sendall(b"OK")
                        
                    time.sleep(5)

        except Exception as e:
            traceback.print_exc()
            self.observer.stop()
            print("Observer Stopped")
 
        self.observer.join()
 
 
class Handler(FileSystemEventHandler):
 
    @staticmethod
    def on_any_event(event):
        if event.is_directory:
            return None

        srcPath = event.src_path.replace('\\\\', '/').replace('\\', '/')

        if not MyWatch.instance.Contains(srcPath): return
 
        if event.event_type == 'created' or event.event_type == 'modified':
            MyWatch.modifiedFiles[srcPath] = str(os.path.getmtime(srcPath))
        elif event.event_type == 'deleted' or event.event_type == 'moved':
            if srcPath in MyWatch.modifiedFiles:
                del MyWatch.modifiedFiles[srcPath]
             

def exit_handler():
    # save before exit
    rets = MyWatch.instance.CompileChanged()
    countErr = 0
    for ret in rets:
        if (ret.returncode != 0):
            countErr += 1

    # save watched files if no error
    if countErr == 0:
        MyWatch.instance.SaveAllWatchedFiles()

atexit.register(exit_handler)


def ReadCfg(filename):
    with open(filename) as file:
        strings = [line.rstrip() for line in file]
        lines = [x for x in strings if x]

    ret = ', '.join(lines)
    print(ret)
    return ret

if __name__ == '__main__':
    #s = '.vs.hlsl->.vs.cso "dxc/dxc.exe" -T vs_6_0 -E main <input> -Fo <output>, .ps.hlsl->.ps.cso "dxc/dxc.exe" -T ps_6_0 -E main <input> -Fo <output>, Src D:\\SoftEngine\\Engine\\Modules\\Graphics/API/DX12/Shaders/, Dest D:\\SoftEngine\\build\\bin\\Debug/Shaders/, Dupl D:\\SoftEngine\\build\\bin\\Release/Shaders/, Sign DX12'
    
    watch = MyWatch(ReadCfg(sys.argv[1]))
    MyWatch.instance = watch

    #watch.CompileAll()
    watch.StartUp()
    watch.run()

    #s = '.vs.hlsl->.vs.cso "dxc/dxc.exe" -T vs_6_0 -E main <input> -Fo <output>, .ps.hlsl->.ps.cso "dxc/dxc.exe" -T ps_6_0 -E main <input> -Fo <output>, Src D:\\SoftEngine\\Engine\\Modules\\Graphics/API/DX12/Shaders/, Dest D:\\SoftEngine\\build\\bin\\Debug/Shaders/'
    #ProcessArgs(s);