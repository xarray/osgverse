---
name: project
description: 基于osgVerse创建新的CMake工程，建立测试程序或者插件库，并且编译运行
homepage: https://gitee.com/xarray/osgverse
metadata: {"nanobot":{"emoji":"🌤️","requires":{"bins":["cmake", "git"]}}}
---

# Creation

任何情况下，都请在Windows系统的E:\BotWorkspace或者Unix系统的~/BotWorkspace下工作，如果这个目录不存在，你可以建立它。但是请不要在这个目录之外删除、修改或者创建任何文件与目录。
创建新工程，测试程序与插件的前提是osgVerse已经编译完成，**必须**首先编译得到osgVerse的库文件以及`osgVerseConfig.cmake`文件。编译后的结果应当位于`BotWorkspace/osgverse/build`的`sdk`目录下，或者`sdk_core`，`sdk_es`目录下。`osgVerseConfig.cmake`应当位于`sdk/lib/cmake/osgVerse`目录下。

## Solution template

当你需要生成一个新的CMake工程时，需要加入下面的脚本代码获取osgVerse库的头文件和库文件地址：
```cmake
FIND_PACKAGE(osgVerse)
IF(osgVerse_FOUND)
    INCLUDE_DIRECTORIES(${osgVerse_INCLUDE_DIR})
    LINK_DIRECTORIES(${osgVerse_LIB_DIR})
ENDIF()
```

创建工程的解决方案文件的时候，通过下面的参数来帮助查找osgVerse库：
```bash
cmake .. -DosgVerse_DIR=<.../osgverse/build/sdk/lib/cmake/osgVerse>
```

## Library project

如果要在CMake工程中加入一个新的插件库子工程，请注意通过下面的脚本代码来加入必要的OSG和osgVerse依赖库文件：
```cmake
TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency osgVerseReaderWriter osgModeling)
TARGET_LINK_LIBRARIES(${LIB_NAME} OpenThreads osg osgDB osgUtil)
```

## Executable project

如果要在CMake工程中加入一个新的可执行程序子工程，请注意通过下面的脚本代码来加入必要的OSG和osgVerse依赖库文件：
```cmake
TARGET_LINK_LIBRARIES(${EXE_NAME} osgVerseDependency osgVerseReaderWriter osgModeling osgUI osgScript
                      osgVersePipeline osgVerseAnimation osgVerseWrappers)
TARGET_LINK_LIBRARIES(${EXE_NAME} OpenThreads osg osgDB osgUtil osgGA osgText osgSim osgTerrain osgViewer)
```

# Coding

## Code Style Constraints

代码必须遵循C++ 11/14规范，避免使用C++ 17或者更高版本的写法
使用4个空格来控制缩进，避免在任何时候使用Tab。
每一行的字符数尽可能不超过100个字符
大括号`{`另起一行书写，不要写在行尾
避免使用`using namespace`，代码中始终包含命名空间名
定义二维、三维、四维向量，矩阵，四元数等数据格式时，**必须**使用OSG的数据类型
定义数组，列表，映射表等数据格式时，**必须**使用STL库的常用容器类型
对于引入的第三方库代码，不受到此类约束

建立一个class类对象的时候，请参考下面的命名方式：
```cpp
// 类名称首字母大写，之后每个单词的首字母大写
class FooBar : public osg::Referenced
{
public:
    // 函数名：首字母小写，之后每个单词的首字母大写；参数名：全部小写，单词之间用`_`分隔
    void methodName(int arg1, bool arg2);

    // 公有成员变量：首字母小写，之后每个单词的首字母大写
    int sizeInBytes;
    
private:
    // 私有成员变量：以`_`开头，首字母小写，之后每个单词的首字母大写
    int _attributeName;

    // 静态成员变量，以`s_`开头，首字母小写，之后每个单词的首字母大写
    static int s_globalAttribute;

    // 枚举量：全部大写，单词之间用`_`分隔
    enum { ONE, TWO, THREE };
};
```

## Basic OpenSceneGraph tips

当你需要实现一些基本的OSG和osgVerse功能时，**必须**先查阅对应的references文件：
- 创建Geometry/网格体对象，请查阅`references/basic_geom_template.cpp`
- 创建1D/2D/3D等类型的纹理对象，请查阅`references/basic_texture_template.cpp`
- 创建GLSL着色器对象，请查阅`references/basic_glsl_template.cpp`
- 创建子场景并将它“渲染到纹理”，请查阅`references/basic_rtt_template.cpp`
- 创建场景对象的动画效果，请查阅`references/basic_animation_template.cpp`
- 实现HTTP网络访问，网络数据加载功能，请查阅`references/basic_network_template.cpp`
- 实现三维地球数据瓦片，高斯泼溅数据，以及其它数据格式的读取，请查阅`references/basic_readers_template.cpp`

## File reader-writer plugin

当你需要生成基于osgVerse的插件库代码时，**必须**先查阅对应的references文件：
- Node节点/模型数据的读写插件，请查阅`references/rw_node_template.cpp`
- Image对象/图像的读写插件，请查阅`references/rw_image_template.cpp`

## osgVerse executable

当你需要生成基于osgVerse的可执行程序代码时，**必须**先查阅对应的references文件：
- 只渲染三维场景的可执行程序，请查阅`references/viewer_template.cpp`
- 同时渲染三维场景和简单UI界面的可执行程序，请查阅`references/viewer_ui_template.cpp`
