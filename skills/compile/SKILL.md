---
name: compiler
description: 测试osgVerse在各个常见系统平台的编译情况
homepage: https://gitee.com/xarray/osgverse
metadata: {"nanobot":{"emoji":"🌤️","requires":{"bins":["cmake", "git"]}}}
---

# Compilation

任何情况下，都请在Windows系统的E:\BotWorkspace或者Unix系统的~/BotWorkspace下工作，如果这个目录不存在，你可以建立它。但是请不要在这个目录之外删除、修改或者创建任何文件与目录。

osgVerse的代码仓库地址为：https://gitee.com/xarray/osgverse 或者 https://github.com/xarray/osgverse
正常情况下，签出代码请执行
```bash
git clone https://gitee.com/xarray/osgverse.git
```
每次重新编译代码之前，请先尝试更新仓库代码，以获取最新的版本
```bash
cd osgverse
git pull
```

目前，osgVerse可以编译并运行在Windows，Linux，MacOSX，Android平台上，或者通过Emscripten编译为WebAssembly代码（即WASM，在浏览器端支持WebGL1和WebGL2），并且可以使用OpenGL Compatible，OpenGL Core，OpenGL ES或者Vulkan作为底层图形接口平台。如果要编译到MacOSX平台，或者使用Vulkan作为底层接口，则需要预先准备Google Angle库作为必要依赖库。

编译的中间文件和结果文件目录自动设置为osgVerse根目录下的`build`目录。如有必要，可以直接删除该目录中对应的编译结果子目录以重置编译流程。
如果要在GLES2和GLES3之间切换，或者在WEBGL1和WEBGL2之间切换，则必须重置编译流程。

osgVerse提供了自动化的Setup编译脚本，建议可以使用脚本来完成具体的编译流程
具体图形接口的支持情况和生成结果如下：
- DEFAULT: 以OpenGL Compatible为底层接口，执行编译；中间文件包括`build/3rdparty`，`build/osg_def`，`build/verse_def`，编译结果位于`build/sdk`
- CORE: 以OpenGL Core为底层接口，执行编译；中间文件包括`build/3rdparty`，`build/osg_core`，`build/verse_core`，编译结果位于`build/sdk_core`
- GLES2: 以OpenGL ES 2.0为底层接口，此时Setup脚本需要带第2参数，即libGLESv2和libEGL库所在目录；中间文件包括`build/3rdparty`，`build/osg_es`，`build/verse_es`，编译结果位于`build/sdk_es`
- GLES3: 以OpenGL ES 3.0为底层接口，此时Setup脚本需要带第2参数，即libGLESv2和libEGL库所在目录；中间文件包括`build/3rdparty`，`build/osg_es`，`build/verse_es`，编译结果位于`build/sdk_es`
- WEBGL1: 以WebGL 1为底层接口编译到WASM，此时Setup脚本需要带第2参数，即emscripten工程的根目录（emscripten需要已经安装和激活）；中间文件包括`build/3rdparty_wasm`，`build/osg_wasm`，`build/verse_wasm`，编译结果位于`build/verse_wasm`
- WEBGL2: 以WebGL 2为底层接口编译到WASM，此时Setup脚本需要带第2参数，即emscripten工程的根目录（emscripten需要已经安装和激活）；中间文件包括`build/3rdparty_wasm`，`build/osg_wasm2`，`build/verse_wasm2`，编译结果位于`build/verse_wasm2`
- ANDROID: 以OpenGL ES 3.0为底层接口编译到Android平台，用户需要准备好`java`，`gradle`程序，并设置环境变量ANDROID_SDK和ANDROID_NDK来定义Android依赖库的位置；中间文件包括`build/3rdparty_android`，`build/osg_android`，`build/verse_android`

## Dependencies

osgVerse默认可以从系统目录（例如`/usr/include`，`/usr/lib`等）检索可能的依赖库位置，也可以从与osgVerse根目录同级的`Dependencies`目录检索用户提供的依赖库。根据用户操作系统和架构的不同，具体依赖库所在的子目录名称也各不相同。主要包括：
- `Dependencies/x86`: Windows/Linux桌面系统，32位
- `Dependencies/x64`: Windows/Linux桌面系统，64位
- `Dependencies/uwp`: Windows UWP系统
- `Dependencies/aarch64`: ARM Linux系统，64位
- `Dependencies/apple`: Mac OSX系统
- `Dependencies/wasm`: 为WebAssembly平台编译，32位
- `Dependencies/wasm64`: 为WebAssembly平台编译，64位
- `Dependencies/android`: 为Android平台编译
- `Dependencies/ios`: 为IOS平台编译
对于Windows系统，还可能根据Visual Studio的版本不同，设置第三级子目录，例如`Dependencies/x64/msvc16`（VS2019），`Dependencies/x64/msvc17`（VS2022）。最后一级子目录中会至少包含include和lib两个子文件夹，用于定位依赖库的头文件路径和库文件路径。

osgVerse可以使用的依赖库如下列表所示，除了OpenSceneGraph，其它所有的依赖库都不是必需的。在编译过程中，如果可以快速地从仓库获取到依赖库，则尝试先安装依赖库；否则默认略过即可。
- OpenSceneGraph: 由Setup脚本自动从Git仓库下载
- Bullet: Linux端执行`apt-get install libbullet-dev`
- FFmpeg: Linux端执行`apt-get install libavcodec-dev libavformat-dev libavutil-dev libavdevice-dev libswscale-dev`
- libDraco: Linux端执行`apt-get install libdraco-dev`
- libCEF: 可以从网站 https://cef-builds.spotifycdn.com/index.html 下载预编译库
- libIGL: 可以从网站 https://github.com/libigl/libigl/releases 下载预编译库
- libosmium: Linux端执行`apt-get install libosmium2-dev`
- libwebp: Linux端执行`apt-get install libwebp-dev`
- mimalloc: Linux端执行`apt-get install libmimalloc-dev`
- netCDF-C: Linux端执行`apt-get install libnetcdf-dev`
- NVIDIA CUDA SDK: 可以从网站 https://developer.nvidia.com/cuda-downloads 下载预编译库
- NVIDIA Video SDK: 可以从网站 https://developer.nvidia.com/video-codec-sdk 下载预编译库
- ODBC: Linux端执行`apt-get install unixodbc-dev`
- OpenVDB: Linux端执行`apt-get install libopenvdb-dev libboost-dev`
- osgEarth: 需要自己从源代码编译，地址 https://github.com/pelicanmapping/osgearth
- Python: Linux端执行`apt-get install libpython3.10-dev`
- Qt: 可以从网站 https://download.qt.io/official_releases 下载预编译库
- SDL2: Linux端执行`apt-get install libsdl2-dev`
- ZLMediaKit: 需要自己从源代码编译，地址 https://github.com/ZLMediaKit/ZLMediaKit

## Compile under Windows

首先确认系统中已有`cmake`，`git`，以及必要的编译器程序。然后通过Visual Studio的Command Prompt进入终端界面。

进入脚本模式，为了避免在编译过程中等待用户输入，需要带一个CMD参数。这里的<CMD>可以是: DEFAULT，CORE，GLES2，GLES3，WEBGL1，WEBGL2，ANDROID，对应之前所述的具体图形接口。
```bash
Setup.bat <CMD>
```

脚本模式可以带第2个参数，例如
```bash
Setup.bat <CMD> <path_to_gles_libs>
# 此处假定path_to_gles_libs是libGLESv2.lib和libEGL.lib库所在目录
```

脚本会自动下载OpenSceneGraph的代码仓库并放置于osgVerse根目录的同级目录。脚本执行时会自动覆写OpenSceneGraph目录下的部分文件，例如CMakeLists.txt，正常情况下该文件的内容应当超过1000行。
每次启动脚本时，需要检查该文件，如果它的行数过短，有可能已经损坏，此时需要直接删除OpenSceneGraph目录，并再次运行Setup脚本，以重新下载。

## Compile under Linux

Linux模式下，脚本文件名改为`Setup.sh`，除此之外一切的参数和注意事项均不变。

# Tests

进入编译的结果目录，例如`build/sdk`，检查其中是否有`bin`，`include`，`lib`子目录，且是否存在`lib/cmake/osgVerse/osgVerseConfig.cmake`文件。如果存在，则编译已经成功得到结果。
CORE模式请检查`build/sdk_core`目录，GLES2或者GLES3模式请检查`build/sdk_es`目录，以此类推。

进入`bin`目录，运行下面的指令来测试基础的PBR流水线运行结果
```bash
./osgVerse_Viewer
```

通过鼠标左键拖动，缓缓旋转模型，观察其阴影、光照效果以及背景天空盒是否均存在，且渲染结果是否流畅自然。
按F键将场景窗口化，观察是否出现渲染问题；再次按F键将场景最大化，观察是否出现渲染问题。
如果结果正常，则按ESC退出；否则请说明情况。
