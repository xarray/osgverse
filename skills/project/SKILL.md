---
name: project
description: 基于osgVerse创建新的CMake工程，建立测试程序或者插件库，并且编译运行
homepage: https://gitee.com/xarray/osgverse
metadata: {"nanobot":{"emoji":"🌤️","requires":{"bins":["cmake", "git"]}}}
---

# Creation

任何情况下，都请在Windows系统的`E:\BotWorkspace`或者Unix系统的`~/BotWorkspace`下工作，如果这个目录不存在，你可以建立它。但是请不要在这个目录之外删除、修改或者创建任何文件与目录。如果这个目录不存在又无法被建立，则直接停止你后续的工作并汇报即可。

默认系统中已有`cmake`，`git`这些指令，不需要再单独搜索它们的路径。如果用户指示查找和阅读某个代码文件，请按照下面的顺序在`BotWorkspace`目录中查找
- 查找`BotWorkspace`根目录
- 查找`BotWorkspace/osgVerse`目录
- 查找`BotWorkspace/OpenSceneGraph`目录
- 如果用户指示查找某个代码仓库，则首先尝试使用git将这个仓库下载到`BotWorkspace`根目录中

创建新工程前，**必须**验证osgVerse编译结果的完整性：
- 默认情况下，检查`BotWorkspace/osgverse/build/sdk/lib/cmake/osgVerse/osgVerseConfig.cmake`文件是否存在
- 如果要求使用CORE模式，则检查`BotWorkspace/osgverse/build/sdk_core/lib/cmake/osgVerse/osgVerseConfig.cmake`文件是否存在
- 如果要求使用GLES2或者GLES3模式，则检查`BotWorkspace/osgverse/build/sdk_es/lib/cmake/osgVerse/osgVerseConfig.cmake`文件是否存在
如果缺少文件，则执行`compiler` Skill中的能力，重新编译OpenSceneGraph和osgVerse。

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
cmake .. -DosgVerse_DIR=<../../osgverse/build/sdk/lib/cmake/osgVerse>
```

如果是针对CORE模式编译的osgVerse，则脚本参数改为：
```bash
cmake .. -DosgVerse_DIR=<../../osgverse/build/sdk_core/lib/cmake/osgVerse>
```

如果是针对GLES2或者GLES3模式编译的osgVerse，则脚本参数改为：
```bash
cmake .. -DosgVerse_DIR=<../../osgverse/build/sdk_es/lib/cmake/osgVerse>
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
代码注释量适中即可，不需要过于频繁。所有的注释都采用英文，以避免编码问题。
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
- 创建图像和纹理对象，请查阅`references/basic_texture_template.cpp`
- 创建GLSL着色器对象，请查阅`references/basic_glsl_template.cpp`
- 创建子场景并将它“渲染到纹理”，请查阅`references/basic_rtt_template.cpp`
- 创建场景对象的路径动画效果，请查阅`references/basic_animation_template.cpp`
- 实现多线程的代码开发，请查阅`references/basic_thread_template.cpp`
- 实现HTTP网络访问，网络数据加载功能，请查阅`references/basic_network_template.cpp`
- 实现三维地球数据瓦片，高斯泼溅数据，3dtiles数据格式的读取，请查阅`references/basic_readers_template.cpp`
- 实现二维界面的显示，UI元素的添加和排列，请查阅`references/basic_ui_template.cpp`
- 实现鼠标键盘等操作事件，场景射线/鼠标点击求交，请查阅`references/basic_interaction_template.cpp`

## File reader-writer plugin

当你需要生成基于osgVerse的插件库代码时，**必须**先查阅对应的references文件：
- Node节点模型，以及Image图片的读取插件，请查阅`references/plugin_template.cpp`

## osgVerse executable

当你需要生成基于osgVerse的可执行程序代码时，**必须**先查阅对应的references文件：
- 用于渲染三维场景的可执行程序，请查阅`references/viewer_template.cpp`
