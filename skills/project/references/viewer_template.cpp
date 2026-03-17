// ============================================
// 功能测试用的可执行程序标准模板
// @skill-status: partial
// @skill-category: executable, test
// @dependencies: osgVerseReaderWriter, osgVerseReaderPipeline
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgVerse/pipeline/Pipeline.h>
#include <osgVerse/pipeline/Utilities.h>
#include <osgVerse/pipeline/Global.h>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

// 在程序主函数main开始之前，**必须**全局声明一些宏定义，以便在静态编译模式下找到必要的窗口实现方案，并预先加载插件
#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
USE_GRAPICSWINDOW_IMPLEMENTATION(GLFW)

// 在程序主函数main开始的时候，**必须**先全局初始化osgVerse相关的类和工具，并且通过updateOsgBinaryWrappers()更新osgVerse的脚本系统
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osgVerse::updateOsgBinaryWrappers();

    // TODO: 后续的初始化代码，场景构建代码等
}

// 如果只做前向渲染，构建场景Viewer的时候，请设置必要的调试工具（帧速显示，窗口大小调整等）
{
    osg::ref_ptr<osg::Node> root = ...;  // 要加入到Viewer中的场景主根节点
    osg::ref_ptr<osgGA::GUIEventHandler> handler = ...;  // 如有必要，加入到Viewer中的交互事件处理器

    // TODO: 其它初始化相关代码

    osgViewer::Viewer viewer;
    viewer.addEventHandler(handler.get());  // 如果确实存在交互事件处理的需求，则加入到viewer中
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);  // 设置单线程工作模式
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);   // 自动管理底层图形接口的加载

    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer.setUpViewOnSingleScreen(screenNo);                     // 考虑多屏幕的情况，默认只渲染到主屏
    return viewer.run();
}

// 如果要做延迟渲染和PBR材质的支持，构建场景Viewer的时候，需要使用osgVerse::StandardPipelineViewer
{
    // 要加入到延迟渲染过程中的节点，需要指定对应的流水线Mask值
    osg::ref_ptr<osg::Node> scene = ...;
    osgVerse::Pipeline::setPipelineMask(*scene, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    // 不加入延迟渲染过程的节点，需要指定对应的流水线Mask值，并且自己管理着色器或者使用默认的前向渲染方式
    osg::ref_ptr<osg::Node> scene2 = ...;
    osgVerse::Pipeline::setPipelineMask(*scene2, CUSTOM_INPUT_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;  // 要加入到Viewer中的场景主根节点
    root->addChild(scene.get());
    root->addChild(scene2.get());

    osg::ref_ptr<osgGA::GUIEventHandler> handler = ...;  // 如有必要，加入到Viewer中的交互事件处理器

    // TODO: 其它初始化相关代码

    osg::ref_ptr<osgVerse::StandardPipelineViewer> viewer = new osgVerse::StandardPipelineViewer(true, false, false);
    viewer->getParameters().enableAO = true;             // 允许计算场景SSAO
    viewer->getParameters().enablePostEffects = true;    // 允许计算场景后处理效果
    viewer->getParameters().enableUserInput = true;      // 允许添加前向渲染/自定义渲染方式的对象

    // 添加一个独立的输入阶段，用于渲染前向渲染/自定义渲染方式的对象，并且在最终阶段开始前与延迟管线的场景融合
    viewer->getParameters().addUserInputStage("Forward", CUSTOM_INPUT_MASK,
                                              osgVerse::StandardPipelineParameters::BEFORE_FINAL_STAGE);

    viewer->addEventHandler(handler.get());  // 如果确实存在交互事件处理的需求，则加入到viewer中
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgViewer::WindowSizeHandler);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setSceneData(root.get());

    int screenNo = 0; arguments.read("--screen", screenNo);
    viewer->setUpViewOnSingleScreen(screenNo);           // 考虑多屏幕的情况，默认只渲染到主屏

    // 此时没必要再设置单线程模式和管理底层图形接口的加载，它们已经在内部实现了
    return viewer->run();
}
