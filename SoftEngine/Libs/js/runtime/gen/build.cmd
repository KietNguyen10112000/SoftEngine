@echo OFF
call npx webpack


echo ======================Building debug snapshot======================================
call "..\..\..\v8\v8-lib\bin\debug\mksnapshot.exe" --startup-blob=dist/soft_runtime_debug.bin dist/main.js
call xcopy /Y dist\soft_runtime_debug.bin ..\..\..\..\..\out\build\x64-Debug\SoftEngine\snapshot_blob.bin


echo ======================Building release snapshot======================================
call "..\..\..\v8\v8-lib\bin\release\mksnapshot.exe" --startup-blob=dist/soft_runtime_release.bin dist/main.js
call xcopy /Y dist\soft_runtime_release.bin ..\..\..\..\..\out\build\x64-Release\SoftEngine\snapshot_blob.bin