## 1. Build

***The Engine is currently only supported on Windows platform.***

### 1.1. Build the Engine

Step 1: Install Visual Studio with C/C++, CMake toolchain

Step 2: Install Python 3, Watchman

Step 3: Clone Engine's repository and all sub-repositories

Step 4: Set System Path`SOFT_ENGINE_HOME` to the cloned repository's directory above

Step 5: Open terminal inside `SOFT_ENGINE_HOME` and run the following command:

```batch
python build.py
```

Step 6: Copy directory `SOFT_ENGINE_HOME/Resources` to `SOFT_ENGINE_HOME/build/bin/Debug` and `SOFT_ENGINE_HOME/build/bin/Release`

### 1.2. Build Editor

Open `SOFT_ENGINE_HOME/Tools/Editor/Editor.sln` in Visual Studio and build it as a normal Visual Studio solution.

### 1.3. Build Sample project

Open `SOFT_ENGINE_HOME/SampleProjects/Sample.sln` in Visual Studio and build it as a normal Visual Studio solution.



### 2. Run

To run the Engine, run ShaderCompilerServer at `SOFT_ENGINE_HOME/DevTools/ShaderCompilerServer/DX12ShaderCompilerServer.bat` first.

Then run the Engine with a Project from a Visual Studio project, such as `Sample`, or run the Engine without any project with `Engine.exe` in `SOFT_ENGINE_HOME/build/bin/Debug`.

To set default Project to run when executing `Engine.exe`, create a file named `Default.Soft` in `SOFT_ENGINE_HOME/build/bin/Debug` with the following content

```json
{
    "ForwardProject": "../../../SampleProjects/Sample/Sample.Soft"
}
```

`ForwardProject` is the path to a SoftEngine Project.