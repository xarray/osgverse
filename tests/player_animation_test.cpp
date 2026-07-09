#include <osg/io_utils>
#include <osg/Point>
#include <osg/PolygonMode>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Global.h>
#include <pipeline/Utilities.h>
#include <animation/PlayerAnimation.h>
#include <animation/BlendShapeAnimation.h>
#include <readerwriter/Utilities.h>
#include <modeling/Utilities.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class FindAnimationVisitor : public osg::NodeVisitor
{
public:
    FindAnimationVisitor() : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}
    std::map<osgVerse::PlayerAnimation*, osg::Geode*> animations;

    virtual void apply(osg::Node& node)
    { traverse(node); }

    virtual void apply(osg::Geode& geode)
    {
        osgVerse::PlayerAnimation* anim = dynamic_cast<osgVerse::PlayerAnimation*>(geode.getUpdateCallback());
        if (anim) animations[anim] = &geode; traverse(geode);
    }
};

std::map<osgVerse::PlayerAnimation*, osg::Geode*> findAnimationManagers(osg::Node* node)
{
    FindAnimationVisitor fav;
    if (node != NULL) node->accept(fav);
    return fav.animations;
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    bool noSkinning = arguments.read("--no-skinning"), noAnimation = arguments.read("--no-animation");
    bool showOriginal = arguments.read("--show-original");
    int jointToOutput = -1, rootBoneId = 0; arguments.read("--root-bone", rootBoneId);
    if (arguments.read("--joint-skinning", jointToOutput)) noAnimation = true;
    
    osg::ref_ptr<osg::MatrixTransform> skeleton = new osg::MatrixTransform;
    osg::ref_ptr<osg::MatrixTransform> playerRoot = new osg::MatrixTransform;
    std::string animToPlay; arguments.read("--animation", animToPlay);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
#if defined(OSG_GLES2_AVAILABLE) || defined(OSG_GLES3_AVAILABLE) || defined(OSG_GL3_AVAILABLE)
    root->getOrCreateStateSet()->setAttribute(osgVerse::createDefaultProgram("baseTexture"));
    root->getOrCreateStateSet()->addUniform(new osg::Uniform("baseTexture", (int)0));
#endif
    root->addChild(playerRoot.get());
    root->addChild(osgDB::readNodeFile("axes.osgt"));

    std::map<osgVerse::PlayerAnimation*, osg::Geode*> animManagers;
#if false
    osg::ref_ptr<osg::Geode> player = new osg::Geode;
    playerRoot->addChild(player.get());

    osg::ref_ptr<osgVerse::PlayerAnimation> animManager = new osgVerse::PlayerAnimation;
    if (!animManager->initialize("ozz/ruby_skeleton.ozz", "ozz/ruby_mesh.ozz")) return 1;
    if (!animManager->loadAnimation("idle", "ozz/ruby_animation.ozz")) return 1;
    animManager->select("idle", 1.0f, true); animManager->seek("idle", 0.0f);
    player->addUpdateCallback(animManager.get()); animManagers[animManager.get()] = player.get();
#else
    osg::ref_ptr<osgDB::Options> opt = new osgDB::Options("DisabledPBR=1");
    osg::ref_ptr<osg::Node> player = (argc > 1) ? osgDB::readNodeFiles(arguments, opt.get())
                                   : osgDB::readNodeFile(BASE_DIR + "/models/Characters/girl.glb", opt.get());
    if (player.valid()) playerRoot->addChild(player.get());
    animManagers = findAnimationManagers(player.get());
#endif
    playerRoot->addChild(skeleton.get());

    if (showOriginal)
    {
        osgVerse::QuickObjectVisitor qov; qov.setNodeMaskOverride(0xffffffff);  // to visit nodes with mask=0
        qov.setNodeFinder([](osg::Object& node)
            {
                bool isOriginal = false; node.getUserValue("OriginalPlayerMesh", isOriginal);
                if (isOriginal)
                {
                    static_cast<osg::Node&>(node).setNodeMask(0xffffffff);
                    static_cast<osg::Node&>(node).getOrCreateStateSet()->setAttributeAndModes(
                        new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE));
                }
                return false;
            });
        root->accept(qov);  // find original player node and display it, for comparing with ozz one
    }

    for (std::map<osgVerse::PlayerAnimation*, osg::Geode*>::iterator it = animManagers.begin();
         it != animManagers.end(); ++it)
    {
        osgVerse::PlayerAnimation* animManager = it->first;
        OSG_NOTICE << "*** Character " << animManager->getName() << " ***" << std::endl;

        // Joints
        std::vector<osgVerse::PlayerAnimation::ThisAndParent> joints = animManager->getSkeletonIndices();
        OSG_NOTICE << "  Joints " << joints.size() << ":" << std::endl;
        for (size_t i = 0; i < joints.size(); ++i)
        {
            osgVerse::PlayerAnimation::ThisAndParent p = joints[i];
            OSG_NOTICE << "    " << p.first << ": " << animManager->getSkeletonJointName(p.first)
                       << ", parent ID = " << p.second << std::endl;
        }

        // Blendshapes
        OSG_NOTICE << "  Blendshapes " << animManager->getNumBlendShapeCallbacks() << ":" << std::endl;
        for (size_t i = 0; i < animManager->getNumBlendShapeCallbacks(); ++i)
        {
            osgVerse::BlendShapeAnimation* bs = animManager->getBlendShapeCallback(i);
            if (!bs) continue;  // It is common to have an empty BS callback... Check it by yourself

            OSG_NOTICE << "    " << bs->getName() << ": ";
            for (size_t j = 0; j < bs->getNumBlendShapes(); ++j)
                OSG_NOTICE << bs->getBlendShapeData(j)->name << ", ";
            OSG_NOTICE << "... Total: " << bs->getNumBlendShapes() << std::endl;
        }
        //animManager->setBlendShape("jawOpen", 1.0f);  // For blendshape test  // TODO

        // Animations
        std::vector<std::string> animations = animManager->getAnimationNames();
        if (animToPlay.empty() && !animations.empty()) animToPlay = animations[0];

        OSG_NOTICE << "  Animations " << animations.size() << ":" << std::endl;
        for (size_t i = 0; i < animations.size(); ++i)
        {
            const std::string& animName = animations[i];
            if (animName.find(animToPlay) != std::string::npos)
            {
                animManager->select(animName, 1.0f, true);
                OSG_NOTICE << "  * " << animName << ": T = " << animManager->getDuration(animName) << std::endl;
            }
            else
                OSG_NOTICE << "    " << animName << ": T = " << animManager->getDuration(animName) << std::endl;
        }

        // To play/pause animation, or show only rest pose
        animManager->setPlaying(!noAnimation, noAnimation);
        animManager->setDrawingSkinning(!noSkinning);

        // Add a validator if your character can't be shown correctly...
        animManager->setSkinningValidator([](osg::Vec3* src, osg::Vec3* dst, int start, int count)
            {
                static int s_validatingCount = 0; unsigned int invalidNum = 0;
                if (s_validatingCount++ > 100) { memcpy(dst + start, src, sizeof(osg::Vec3) * count); return; }

                for (int k = 0; k < count; ++k)
                {
                    osg::Vec3& vec = src[k];
                    if (!vec.valid() || vec.length() > 1e5) invalidNum++;
                    else dst[start + k] = vec;
                }

                if (invalidNum > 0)
                {
                    std::cout << "Skinning has invalid data: " << invalidNum << "/" << count << "\n";
                    s_validatingCount = 0;  // so to always print invalid information
                }
            });
    }

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);

    if (jointToOutput >= 0)
    {
        // Display specified joint weight data (as colors of vertices)
        for (std::map<osgVerse::PlayerAnimation*, osg::Geode*>::iterator it = animManagers.begin();
             it != animManagers.end(); ++it)
        {
            osgVerse::PlayerAnimation* animManager = it->first;
            //(*animManager)(it->second, viewer.getUpdateVisitor());

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
    }

    float axisScale = player.valid() ? player->getBound().radius() * 0.05f : 1.0f;
    osg::ref_ptr<osg::MatrixTransform> axis = new osg::MatrixTransform;
    axis->setMatrix(osg::Matrix::scale(axisScale, axisScale, axisScale));
    axis->addChild(osgDB::readNodeFile("axes.osgt"));
    while (!viewer.done())
    {
        for (std::map<osgVerse::PlayerAnimation*, osg::Geode*>::iterator it = animManagers.begin();
             it != animManagers.end(); ++it) it->first->applyTransforms(*skeleton, true, true, axis.get(), rootBoneId);
        viewer.frame();
    }
    //osgDB::writeNodeFile(*player, "test_skeleton.osg");
    return 0;
}
