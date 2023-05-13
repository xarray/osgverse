#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

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
    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFile(argv[1]);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile(
        "skydome.osgt.(0.005,0.005,0.01).scale.-100,-150,0.trans");
    osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, FORWARD_SCENE_MASK);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());

    //osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    //MyViewer viewer(pipeline.get());
    //setupPipeline(pipeline.get(), &viewer, 1920, 1080);
    osgViewer::Viewer viewer;

    osg::ref_ptr<osg::Texture> tex2D_0 =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGB_INT8, 1024, 1024);
    osg::ref_ptr<osg::Texture> tex2D_1 =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGB_INT8, 1024, 1024);
    otherSceneRoot->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, tex2D_1.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    osg::ref_ptr<osgVerse::DeferredRenderCallback> drcb = new osgVerse::DeferredRenderCallback(false);
    {
        osg::ref_ptr<osgVerse::DeferredRenderCallback::RttGeometryRunner> gr0 =
            new osgVerse::DeferredRenderCallback::RttGeometryRunner("Test0");
        gr0->projection = osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0);
        gr0->geometry = osg::createTexturedQuadGeometry(
            osg::Vec3(0.2f, 0.2f, 0.0f), osg::Vec3(0.6f, 0.0f, 0.0f), osg::Vec3(0.0f, 0.6f, 0.0f));
        {
            osg::ref_ptr<osg::Texture2D> osg128 = new osg::Texture2D;
            osg128->setImage(osgDB::readImageFile("Images/osg128.png"));
            gr0->geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, osg128.get());
            gr0->geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            gr0->geometry->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        }
        gr0->attach(osg::Camera::COLOR_BUFFER, tex2D_0.get());
        drcb->addRunner(gr0.get());

        osg::ref_ptr<osgVerse::DeferredRenderCallback::RttGeometryRunner> gr1 =
            new osgVerse::DeferredRenderCallback::RttGeometryRunner("Test1");
        gr1->setUseScreenQuad(0, tex2D_0.get());
        gr1->attach(osg::Camera::COLOR_BUFFER, tex2D_1.get());
        {
            const char* testFrag = {
                "uniform sampler2D MainTex;\n"
                "varying vec4 gl_TexCoord[gl_MaxTextureCoords];\n"
                "void main() {\n"
                "  if (gl_FragCoord.x < 512) gl_FragColor = texture2D(MainTex, gl_TexCoord[0].st);\n"
                "  else gl_FragColor = texture2D(MainTex, gl_TexCoord[0].st).bgra;\n"
                "}\n"
            };

            osg::ref_ptr<osg::Program> program = new osg::Program;
            program->addShader(new osg::Shader(osg::Shader::FRAGMENT, testFrag));
            gr1->geometry->getOrCreateStateSet()->setAttributeAndModes(program.get());
            gr1->geometry->getOrCreateStateSet()->addUniform(new osg::Uniform("MainTex", (int)0));
        }
        drcb->addRunner(gr1.get());
    }
    drcb->setup(viewer.getCamera(), PRE_DRAW);

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
