#include <osg/io_utils>
#include <osg/Point>
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
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

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
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    int jointToOutput = -1; arguments.read("--joint-skinning", jointToOutput);
    bool withSkinning = !arguments.read("--disable-skinning");

    osg::ref_ptr<osg::MatrixTransform> skeleton = new osg::MatrixTransform;
    osg::ref_ptr<osg::MatrixTransform> playerRoot = new osg::MatrixTransform;

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
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
    osg::ref_ptr<osgDB::Options> opt = new osgDB::Options("DisabledPBR=1");
    osg::ref_ptr<osg::Node> player = (argc > 1) ? osgDB::readNodeFiles(arguments, opt.get())
                                   : osgDB::readNodeFile(BASE_DIR + "/models/Characters/girl.glb", opt.get());
    if (player.valid()) playerRoot->addChild(player.get());
    animManager = findAnimationManager(player.get());
#endif
    playerRoot->addChild(skeleton.get());

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

        // To play/pause animation, or show only rest pose
        //animManager->setPlaying(false, true);
        animManager->setDrawingSkinning(withSkinning);
    }

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);

#if true
    if (animManager.valid() && jointToOutput >= 0)
    {
        std::vector<osgVerse::PlayerAnimation::ThisAndParent> joints = animManager->getSkeletonIndices();
        osgVerse::PlayerAnimation::VertexWeights weights = animManager->getSkeletonVertexWeights();
        for (size_t i = 0; i < joints.size(); ++i)
        {
            osgVerse::PlayerAnimation::ThisAndParent p = joints[i];
            std::string jointName = animManager->getSkeletonJointName(p.first);
            if (jointToOutput != p.first) continue;

            osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
            osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array;
            for (osgVerse::PlayerAnimation::VertexWeights::iterator itr = weights.begin();
                 itr != weights.end(); ++itr)
            {
                const std::vector<osgVerse::PlayerAnimation::JointAndWeight>& jointList = itr->second;
                for (size_t j = 0; j < jointList.size(); ++j)
                {
                    if (jointList[j].first != p.first) continue;
                    float w = jointList[j].second; va->push_back(itr->first);
                    ca->push_back(osg::Vec4(1.0f, 0.0f, w, 1.0f));
                }
            }

            osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
            geom->setVertexArray(va.get());
            geom->setColorArray(ca.get()); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, va->size()));
            geom->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            geom->getOrCreateStateSet()->setAttributeAndModes(new osg::Point(5.0f));

            osg::ref_ptr<osg::Geode> jointGeode = new osg::Geode;
            jointGeode->addDrawable(geom.get());
            skeleton->addChild(jointGeode.get());
            //osgDB::writeNodeFile(*jointGeode, "joint_skinning.osg");
        }
    }
#endif

    osg::ref_ptr<osg::Node> axis = osgDB::readNodeFile("axes.osgt.(0.1,0.1,0.1).scale");
    while (!viewer.done())
    {
        if (animManager.valid())
            animManager->applyTransforms(*skeleton, true, true, axis.get());
        viewer.frame();
    }
    //osgDB::writeNodeFile(*player, "test_skeleton.osg");
    return 0;
}
