#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Global.h>
#include <animation/PlayerAnimation.h>
#include <animation/BlendShapeAnimation.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class FindAnimationVisitor : public osg::NodeVisitor
{
public:
    FindAnimationVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), pAnim(NULL) {}
    osgVerse::PlayerAnimation* pAnim;

    virtual void apply(osg::Node& node)
    { traverse(node); }

    virtual void apply(osg::Geode& geode)
    {
        if (!pAnim) pAnim = dynamic_cast<osgVerse::PlayerAnimation*>(geode.getUpdateCallback());
        traverse(geode);
    }
};

osgVerse::PlayerAnimation* findAnimationManager(osg::Node* node)
{
    FindAnimationVisitor fav;
    if (node != NULL) node->accept(fav);
    return fav.pAnim;
}

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);

    osg::ref_ptr<osg::MatrixTransform> skeleton = new osg::MatrixTransform;
    osg::ref_ptr<osg::MatrixTransform> playerRoot = new osg::MatrixTransform;
    playerRoot->addChild(skeleton.get());

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    root->addChild(playerRoot.get());
    root->addChild(osgDB::readNodeFile("axes.osgt"));

    osg::ref_ptr<osgVerse::PlayerAnimation> animManager;
#if false
    osg::ref_ptr<osg::Geode> player = new osg::Geode;
    playerRoot->addChild(player.get());

    animManager = new osgVerse::PlayerAnimation;
    if (!animManager->initialize("ozz/ruby_skeleton.ozz", "ozz/ruby_mesh.ozz")) return 1;
    if (!animManager->loadAnimation("idle", "ozz/ruby_animation.ozz")) return 1;
    animManager->select("idle", 1.0f, true);
    animManager->seek("idle", 0.0f);
    player->addUpdateCallback(animManager.get());
#else
    osg::ref_ptr<osg::Node> player = (argc > 1) ? osgDB::readNodeFile(argv[1])
                                   : osgDB::readNodeFile(BASE_DIR "/models/Characters/girl.glb");
    if (player.valid()) playerRoot->addChild(player.get());
    animManager = findAnimationManager(player.get());
#endif

    if (animManager.valid())
    {
        std::vector<osgVerse::PlayerAnimation::ThisAndParent> joints = animManager->getSkeletonIndices();
        for (size_t i = 0; i < joints.size(); ++i)
        {
            osgVerse::PlayerAnimation::ThisAndParent p = joints[i];
            OSG_NOTICE << p.first << ": " << animManager->getSkeletonJointName(p.first)
                       << ", parent ID = " << p.second << std::endl;
        }

        for (size_t i = 0; i < animManager->getNumBlendShapeCallbacks(); ++i)
        {
            osgVerse::BlendShapeAnimation* bs = animManager->getBlendShapeCallback(i);
            if (!bs) continue;  // It is common to have an empty BS callback... Check it by yourself

            OSG_NOTICE << "BlendshapeCB " << bs->getName() << ": ";
            for (size_t j = 0; j < bs->getNumBlendShapes(); ++j)
                OSG_NOTICE << bs->getBlendShapeData(j)->name << ", ";
            OSG_NOTICE << "... Total: " << bs->getNumBlendShapes() << std::endl;
        }

        // For blendshape test
        animManager->setBlendShape("jawOpen", 1.0f);
    }

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);

    while (!viewer.done())
    {
        if (animManager.valid()) animManager->applyTransforms(*skeleton, true, true);
        viewer.frame();
    }
    //osgDB::writeNodeFile(*skeleton, "test_skeleton.osg");
    return 0;
}
