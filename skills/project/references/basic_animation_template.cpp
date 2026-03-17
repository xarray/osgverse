// ============================================
// 场景对象的路径动画效果创建标准模板
// @skill-status: partial
// @skill-category: animation, path
// @dependencies: osgVerseAnimation
// 
// 注意，本模板只提供参考的框架，应根据实际需求和输入参数进行修改。标注为TODO的部分是**必须**根据具体情况进行填充的
// 一些输入变量的赋值写作...，仅作示意用途，需要根据实际情况设置正确的值
// 一些条件判断变量在代码中并没有定义，这里仅作示意用途，需要根据实际情况选择正确的分支
// 采用osgVerse命名空间的函数和类，通常是建议**尽可能**使用的，它们比OSG默认的方法更加健壮
// ============================================

// 需要添加在代码文件起始位置的头文件
#include <osgVerse/animation/TweenAnimation.h>
#include <osgVerse/animation/Utilities.h>

// 创建一个路径动画，并设置给节点对象
{
    double t0 = ...;            // 第1个关键帧的时间点
    osg::Vec3 pos0 = ...;       // 第1个关键帧运动的位置
    osg::Quat rotation0 = ...;  // 第1个关键帧运动的旋转值

    double t1 = ...;            // 第2个关键帧的时间点
    osg::Vec3 pos1 = ...;       // 第2个关键帧运动的位置
    osg::Quat rotation1 = ...;  // 第2个关键帧运动的旋转值

    // 创建OSG标准的动画路径
    osg::ref_ptr<osg::AnimationPath> animationPath = new osg::AnimationPath;
    animationPath->insert(t0, osg::AnimationPath::ControlPoint(pos0, rotation0));
    animationPath->insert(t1, osg::AnimationPath::ControlPoint(pos1, rotation1));
    // TODO: 添加更多的关键帧

    // 建立一个osgVerse的动画路径管理类，给动画路径设置名字并纳入管理
    osg::ref_ptr<osgVerse::TweenAnimation> tween = new osgVerse::TweenAnimation;
    tween->addAnimation("default", animationPath.get());

    // 动画路径只作用于osg::MatrixTransform，osg::PositionAttitudeTransform和osg::Camera
    osg::ref_ptr<osg::MatrixTransform> node = ...;
    node->addUpdateCallback(tween.get());

    // TODO: 其它场景设置和管理代码

    // 需要启动之前纳入的动画路径的播放，设置循环模式（Forwarding，Looping等），以及启动和结束时的加减速状态（Tween Motion）
    tween->play("default", osgVerse::TweenAnimation::Looping, osgVerse::TweenAnimation::CubicInOut);

    // 跳转到指定的时刻位置；第二个参数为true的时候，将整个时间线归一化到[0, 1]区间
    tween->seek(0.5, true);

    // 立即结束当前动画路径的播放
    tween->stop();
}

