// ============================================
// 多线程代码开发标准模板
// @skill-status: partial
// @skill-category: thread
// @dependencies: osgVerseReaderWriter, OpenThreads
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgVerse/pipeline/Utilities.h>

// 使用lambda方式快速建立一个线程类
{
    osgVerse::QuickThread thread;
    thread.setPreProcessor([]
        {
            // TODO: 在线程开始之前执行的代码
        });
    thread.setProcessor([]
        {
            // TODO: 在线程循环执行时的代码
        });
    thread.setPostProcessor([]
        {
            // TODO: 在线程结束之后执行的代码
        });

    // 启动线程
    thread.start();

    // TODO: 后续主进程的操作

    // 销毁线程
    thread.quit();
}
