#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <animation/PlayerAnimation.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);
    osg::ref_ptr<osg::Geode> player = new osg::Geode;

    osg::ref_ptr<osg::MatrixTransform> playerRoot = new osg::MatrixTransform;
    playerRoot->addChild(player.get());

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    root->addChild(playerRoot.get());
    root->addChild(osgDB::readNodeFile("axes.osgt"));

    osg::ref_ptr<osgVerse::PlayerAnimation> animManager = new osgVerse::PlayerAnimation;
    if (!animManager->initialize("ozz/ruby_skeleton.ozz", "ozz/ruby_mesh.ozz")) return 1;
    if (!animManager->loadAnimation("idle", "ozz/ruby_animation.ozz")) return 1;
    animManager->select("idle", 1.0f, true);
    animManager->seek("idle", 0.0f);

    std::vector<osgVerse::PlayerAnimation::ThisAndParent> joints =
        animManager->getSkeletonIndices();
    for (size_t i = 0; i < joints.size(); ++i)
    {
        osgVerse::PlayerAnimation::ThisAndParent p = joints[i];
        OSG_NOTICE << p.first << ": " << animManager->getSkeletonJointName(p.first)
                   << ", parent ID = " << p.second << std::endl;
    }

    osg::ref_ptr<osg::MatrixTransform> skeleton = new osg::MatrixTransform;
    root->addChild(skeleton.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    while (!viewer.done())
    {
        animManager->update(*viewer.getFrameStamp(), false);
        animManager->applyMeshes(*player, true);
        animManager->applyTransforms(*skeleton, true);
        viewer.frame();
    }

    //osgDB::writeNodeFile(*skeleton, "test_skeleton.osg");
    return 0;
}
