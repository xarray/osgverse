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
#include <readerwriter/LoadSceneGLTF.h>
#include <pipeline/ShadowModule.h>

#define SHADER_DIR "../shaders/"
#define FORWARD_SCENE_MASK  0x0000ff00
#define DEFERRED_SCENE_MASK 0x00ff0000
#define SHADOW_CASTER_MASK  0x01000000

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

static void setupPipeline(osgVerse::Pipeline* p, osgViewer::View* view, osg::Group* root,
                          unsigned int originW, unsigned int originH)
{
    p->startStages(originW, originH);

    osg::ref_ptr<osgVerse::ShadowModule> shadow = new osgVerse::ShadowModule(p, true);
    shadow->setLightState(osg::Vec3(), osg::Vec3(1.0f, 0.0f, -1.0f), 1500.0f);
    shadow->createStages(2048, 1,
        osgDB::readShaderFile(SHADER_DIR "shadow_cast.vert"), 
        osgDB::readShaderFile(SHADER_DIR "shadow_cast.frag"), SHADOW_CASTER_MASK);
    view->getCamera()->addUpdateCallback(shadow.get());

    osgVerse::Pipeline::Stage* gbuffer = p->addInputStage("Merge", DEFERRED_SCENE_MASK,
        osgDB::readShaderFile(SHADER_DIR "shadow_merge.vert"),
        osgDB::readShaderFile(SHADER_DIR "shadow_merge.frag"), 3,
        "PosNormalBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
        "ColorBuffer", osgVerse::Pipeline::RGBA_INT8,
        "DepthBuffer", osgVerse::Pipeline::DEPTH32);
    gbuffer->applyDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), "BaseMap", 0);
    gbuffer->applyTexture(shadow->getTextureArray(), "ShadowMap", 1);
    gbuffer->applyUniform("lightMatrices", shadow->getLightMatrices());

    osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final",
        osgDB::readShaderFile(SHADER_DIR "shadow_display.vert"),
        osgDB::readShaderFile(SHADER_DIR "shadow_display.frag"), osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    output->applyBuffer(*gbuffer, "PosNormalBuffer", 0);
    output->applyBuffer(*gbuffer, "ColorBuffer", 1);

    p->applyStagesToView(view, FORWARD_SCENE_MASK);
    p->requireDepthBlit(gbuffer, true);
    
    shadow->getFrustumGeode()->setNodeMask(FORWARD_SCENE_MASK);
    root->addChild(shadow->getFrustumGeode());
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> scene = osgVerse::loadGltf(
        "../models/DamagedHelmet/DamagedHelmet.gltf", false);
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setNodeMask(DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);
    //sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osgt.0,0,-150.trans");
    otherSceneRoot->setNodeMask(DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    //root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());

    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    MyViewer viewer(pipeline.get());
    //setupPipeline(pipeline.get(), &viewer, root.get(), 1920, 1080);

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    
    //return viewer.run();
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}
