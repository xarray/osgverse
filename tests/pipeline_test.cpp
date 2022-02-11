#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include "Pipeline.h"

#define SHADER_DIR "../shaders/"
#define DEFERRED_SCENE_MASK 0x00010000

class MyViewer : public osgViewer::Viewer
{
public:
    MyViewer(osgVerse::Pipeline* p) : osgViewer::Viewer(), _pipeline(p) {}
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;

protected:
    virtual osg::GraphicsOperation* createRenderer(osg::Camera* camera)
    {
        if (_pipeline.valid()) return _pipeline->createRenderer(camera);
        else return osgViewer::Viewer::createRenderer(camera);
    }
};

static void setupPipeline(osgVerse::Pipeline* p, osgViewer::View* view,
                          unsigned int originW, unsigned int originH)
{
    p->startStages(originW, originH);

    osgVerse::Pipeline::Stage* gbuffer = p->addInputStage("GBuffer", DEFERRED_SCENE_MASK,
        osgDB::readShaderFile(SHADER_DIR "gbuffer.vert"),
        osgDB::readShaderFile(SHADER_DIR "gbuffer.frag"), 4,
        "PosNormalBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
        "DiffuseBuffer", osgVerse::Pipeline::RGBA_INT8,
        "EmissiveBuffer", osgVerse::Pipeline::RGBA_INT8,
        "DepthBuffer", osgVerse::Pipeline::DEPTH32);

    osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final",
        osgDB::readShaderFile(SHADER_DIR "display.vert"),
        osgDB::readShaderFile(SHADER_DIR "display.frag"), osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    output->applyBuffer(*gbuffer, "PosNormalBuffer", 0);
    output->applyBuffer(*gbuffer, "DiffuseBuffer", 1);
    output->applyBuffer(*gbuffer, "EmissiveBuffer", 2);
    output->applyBuffer(*gbuffer, "DepthBuffer", 3);

    p->applyStagesToView(view, ~DEFERRED_SCENE_MASK);
    p->requireDepthBlit(gbuffer, true);
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFile(argv[1]);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setNodeMask(DEFERRED_SCENE_MASK);
    //sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile(
        "skydome.osgt.(0.005,0.005,0.01).scale.-100,-150,0.trans");
    otherSceneRoot->setNodeMask(~DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());

    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    MyViewer viewer(pipeline.get());
    setupPipeline(pipeline.get(), &viewer, 1920, 1080);

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
