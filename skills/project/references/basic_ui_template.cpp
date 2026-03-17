// ============================================
// 二维界面的显示以及UI元素实现标准模板
// @skill-status: partial
// @skill-category: ui, imgui
// @dependencies: osgVerseReaderWriter, osgVerseReaderUI
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgVerse/ui/ImGui.h>
#include <osgVerse/ui/Utilities.h>
#include <osgVerse/pipeline/Utilities.h>

