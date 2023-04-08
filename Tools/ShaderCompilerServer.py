# import time module, Observer, FileSystemEventHandler
import time
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

import socket
import os
import subprocess
import traceback

HOST = "127.0.0.1"  # Standard loopback interface address (localhost)
PORT = 2222  # Port to listen on (non-privileged ports are > 1023)

def ProcessArgs(arg: str):
    ret = []

    compileDict = {}

    srcPath = ''
    destPath = ''

    args = arg.split(',')

    for s in args:
        arr = s.strip().split(' ', 1)
        arr2 = arr[0].split('->')

        if len(arr2) > 1:
            compileDict[arr2[0]] = (arr2[1], arr[1])
        elif arr[0] == 'Src':
            srcPath = arr[1].replace('\\\\', '/').replace('\\', '/')
        elif arr[0] == 'Dest':
            destPath = arr[1].replace('\\\\', '/').replace('\\', '/')
            

    for inputExt in compileDict:
        opt = compileDict[inputExt]

        outputExt = opt[0]
        cmd = opt[1]

        grapedInputFiles = []
        grapedOutputFiles = []

        for root, dirs, files in os.walk(srcPath):
            for file in files:
                if file.endswith(inputExt):
                    grapedInputFiles.append(os.path.join(root, file).replace("\\","/"))
                    grapedOutputFiles.append(os.path.join(root, file.replace(inputExt, outputExt)).replace("\\","/"))
    
        for idx, file in enumerate(grapedInputFiles):
            outputFile = grapedOutputFiles[idx]

            _cmd = cmd.replace('<input>', file).replace('<output>', outputFile)

            #print(_cmd)
            out = subprocess.run(_cmd, shell=True, capture_output=True, text=True);
            #print(out.stderr)
            #print(out.returncode)
            ret.append(out)
    
    #print(srcPath)
    #print(destPath)
    #print(compileDict)

    return ret


class OnMyWatch:
    # Set the directory on watch
    watchDirectory = "./"
 
    def __init__(self):
        self.observer = Observer()
 
    def run(self):
        event_handler = Handler()
        self.observer.schedule(event_handler, self.watchDirectory, recursive = True)
        self.observer.start()
        
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.bind((HOST, PORT))
                s.listen()
                while True:
                    conn, addr = s.accept()
                    with conn:
                        print(f"Connected by {addr}")
                        
                        data = conn.recv(1024)
                        if not data:
                            continue
                            
                        rets = ProcessArgs(data.decode())

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
 
        elif event.event_type == 'created':
            # Event is created, you can process it now
            print("Watchdog received created event - % s." % event.src_path)
        elif event.event_type == 'modified':
            # Event is modified, you can process it now
            print("Watchdog received modified event - % s." % event.src_path)
             
 
if __name__ == '__main__':
    watch = OnMyWatch()
    watch.run()

    #s = '.vs.hlsl->.vs.cso "dxc/dxc.exe" -T vs_6_0 -E main <input> -Fo <output>, .ps.hlsl->.ps.cso "dxc/dxc.exe" -T ps_6_0 -E main <input> -Fo <output>, Src D:\\SoftEngine\\Engine\\Modules\\Graphics/API/DX12/Shaders/, Dest D:\\SoftEngine\\build\\bin\\Debug/Shaders/'
    #ProcessArgs(s);