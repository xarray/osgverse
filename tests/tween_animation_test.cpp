#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/PagedLOD>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <animation/TweenAnimation.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

osg::AnimationPath* createPath(const osg::Vec3d& pivot, const osg::Vec3d& axis, float angularVelocity)
{
    double time0 = 0.0;
    double time1 = osg::PI * 0.5 / angularVelocity;
    double time2 = osg::PI * 1.0 / angularVelocity;
    double time3 = osg::PI * 1.5 / angularVelocity;
    double time4 = osg::PI * 2.0 / angularVelocity;

    osg::Quat rotation0(0.0, axis);
    osg::Quat rotation1(osg::PI * 0.5, axis);
    osg::Quat rotation2(osg::PI * 1.0, axis);
    osg::Quat rotation3(osg::PI * 1.5, axis);

    osg::AnimationPath* animationPath = new osg::AnimationPath;
    animationPath->setLoopMode(osg::AnimationPath::LOOP);
    animationPath->insert(time0, osg::AnimationPath::ControlPoint(pivot, rotation0));
    animationPath->insert(time1, osg::AnimationPath::ControlPoint(pivot, rotation1));
    animationPath->insert(time2, osg::AnimationPath::ControlPoint(pivot, rotation2));
    animationPath->insert(time3, osg::AnimationPath::ControlPoint(pivot, rotation3));
    animationPath->insert(time4, osg::AnimationPath::ControlPoint(pivot, rotation0));
    return animationPath;
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osgVerse::TweenAnimation> tween = new osgVerse::TweenAnimation;
    tween->addAnimation("default", createPath(osg::Vec3(), osg::Y_AXIS, 1.0f));
    tween->play("default", osgVerse::TweenAnimation::PingPong, osgVerse::TweenAnimation::CubicInOut);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addUpdateCallback(tween.get());
    root->addChild(osgDB::readNodeFile("axes.osgt"));

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
