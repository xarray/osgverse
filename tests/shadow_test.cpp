#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

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

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR "/models/Sponza/Sponza.gltf");
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv;
    scene->accept(tsv);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setNodeMask(DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());

    // Start the pipeline
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    MyViewer viewer(pipeline.get());
    setupStandardPipeline(pipeline.get(), &viewer, SHADER_DIR, SKYBOX_DIR "barcelona.hdr", 1920, 1080);

    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow)
    {
        osg::ComputeBoundsVisitor cbv; sceneRoot->accept(cbv);
        shadow->addReferenceBound(cbv.getBoundingBox(), true);
        shadow->setLightState(osg::Vec3(), osg::Vec3(0.02f, 0.1f, -1.0f), 5000.0f);  // FIXME
        if (shadow->getFrustumGeode())
        {
            shadow->getFrustumGeode()->setNodeMask(FORWARD_SCENE_MASK);
            root->addChild(shadow->getFrustumGeode());
        }
    }

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());

    // FIXME: how to avoid shadow problem...
    // If renderer->setGraphicsThreadDoesCull(false), which is used by DrawThreadPerContext & ThreadPerCamera,
    // Shadow will go jigger because the output texture is not sync-ed before lighting...
    // For SingleThreaded & CullDrawThreadPerContext it seems OK
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    while (!viewer.done())
    {
        //std::cout << sceneRoot->getBound().center() << "; " << sceneRoot->getBound().radius() << "\n";
        viewer.frame();
    }
    return 0;
}
