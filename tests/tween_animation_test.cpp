#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/PagedLOD>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <animation/TweenAnimation.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

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
    // Default method to add a path animation
    osg::ref_ptr<osgVerse::TweenAnimation> tween = new osgVerse::TweenAnimation;
    tween->addAnimation("default", createPath(osg::Vec3(), osg::Z_AXIS, 1.0f));
    tween->play("default", osgVerse::TweenAnimation::PingPong, osgVerse::TweenAnimation::CubicInOut);

    osg::ref_ptr<osg::MatrixTransform> axesMT = new osg::MatrixTransform;
    axesMT->addUpdateCallback(tween.get());
    axesMT->addChild(osgDB::readNodeFile("axes.osgt.10,10,10.scale"));

    // A node for more interactive tests
    osg::ref_ptr<osg::MatrixTransform> playerMT = new osg::MatrixTransform;
    playerMT->addChild(osgDB::readNodeFile("cow.osgt"));

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(axesMT.get());
    root->addChild(playerMT.get());

    // Quick test convenient animation functions
    osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
    handler->addKeyUpCallback('s', [&](int key) {
        osgVerse::doMove(playerMT.get(), osg::Vec3d(0.0, 0.0, -5.0), 1.0, false, true); });
    handler->addKeyUpCallback('w', [&](int key) {
        osgVerse::doMove(playerMT.get(), osg::Vec3d(0.0, 0.0, 5.0), 1.0, false, true); });
    handler->addKeyUpCallback('a', [&](int key) {
        osgVerse::doMove(playerMT.get(), osg::Vec3d(-5.0, 0.0, 0.0), 1.0, false, true); });
    handler->addKeyUpCallback('d', [&](int key) {
        osgVerse::doMove(playerMT.get(), osg::Vec3d(5.0, 0.0, 0.0), 1.0, false, true); });
    handler->addKeyUpCallback('x', [&](int key) {
        osgVerse::doMove(playerMT.get(), osg::Vec3d(0.0, 0.0, 0.0), 1.0, false, false,
        []() { std::cout << "Back to home position." << std::endl; });
    });

    // Start the main loop
    osgViewer::Viewer viewer;
    viewer.addEventHandler(handler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
