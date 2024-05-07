#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#define CUSTOM_INPUT_MASK 0x00010000
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

#ifdef false  // GLDebug requires OpenGL 4.3, enable it by yourselves
class GLDebugOperation : public osg::GraphicsOperation
{
public:
    GLDebugOperation()
        : osg::Referenced(true), osg::GraphicsOperation("GLDebugOperation", false),
          _glDebugMessageCallback(NULL) {}

    virtual void operator () (osg::GraphicsContext* gc)
    {
        _glDebugMessageCallback = (glDebugMessageCallbackPtr)
            osg::getGLExtensionFuncPtr("glDebugMessageCallback");
        if (_glDebugMessageCallback == NULL)
        {
            OSG_WARN << "[GLDebugOperation] Debug callback function not found" << std::endl;
            return;
        }

        glEnable(GL_DEBUG_OUTPUT);
        _glDebugMessageCallback((GLDEBUGPROC)&(GLDebugOperation::debugCallback), NULL);
        //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
    }

    static void debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                              const GLchar* message, const void* userParam)
    {
        std::string srcInfo, typeInfo, idInfo;
        switch (source)
        {
        case GL_DEBUG_SOURCE_API: srcInfo = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: srcInfo = "WINDOW_SYSTEM"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: srcInfo = "SHADER_COMPILER"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: srcInfo = "THIRD_PARTY"; break;
        case GL_DEBUG_SOURCE_APPLICATION: srcInfo = "APPLICATION"; break;
        case GL_DEBUG_SOURCE_OTHER: srcInfo = "OTHER"; break;
        default: srcInfo = "UNKNOWN_SOURCE"; break;
        }

        switch (type)
        {
        case GL_DEBUG_TYPE_ERROR: typeInfo = "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeInfo = "DEPRECATED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: typeInfo = "UNDEFINED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_PORTABILITY: typeInfo = "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: typeInfo = "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_MARKER: typeInfo = "MARKER"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP: typeInfo = "PUSH_GROUP"; break;
        case GL_DEBUG_TYPE_POP_GROUP: typeInfo = "POP_GROUP"; break;
        case GL_DEBUG_TYPE_OTHER: typeInfo = "OTHER"; break;
        default: typeInfo = "UNKNOWN_TYPE"; break;
        }

        switch (id)
        {
        case GL_NO_ERROR: idInfo = "NO_ERROR"; break;
        case GL_INVALID_ENUM: idInfo = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE: idInfo = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: idInfo = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW: idInfo = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW: idInfo = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY: idInfo = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: idInfo = "INVALID_FRAMEBUFFER_OPERATION"; break;
        case GL_CONTEXT_LOST: idInfo = "CONTEXT_LOST"; break;
        //case GL_TABLE_TOO_LARGE: idInfo = "TABLE_TOO_LARGE"; break;
        default: idInfo = "UNKNOWN_ID"; break;
        }

        std::string msgData = "(ID = " + idInfo + ") " + std::string(message)
                            + "\n\t(Source = " + srcInfo + ", Type = " + typeInfo + ")";
        switch (severity)
        {
        case GL_DEBUG_SEVERITY_HIGH: OSG_WARN << "[HIGH] " << msgData << std::endl; break;
        case GL_DEBUG_SEVERITY_MEDIUM: OSG_NOTICE << "[MEDIUM] " << msgData << std::endl; break;
        case GL_DEBUG_SEVERITY_LOW: OSG_INFO << "[LOW] " << msgData << std::endl; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: OSG_INFO << "[NOTIFY] " << msgData << std::endl; break;
        default: OSG_NOTICE << "[DEFAULT] " << msgData << std::endl; break;
        }
    }

protected:
    typedef void (GL_APIENTRY* glDebugMessageCallbackPtr)(GLDEBUGPROC callback, const void* userParam);
    glDebugMessageCallbackPtr _glDebugMessageCallback;
};
#endif

class SetPipelineHandler : public osgGA::GUIEventHandler
{
public:
    SetPipelineHandler(osgVerse::Pipeline* p, osgVerse::LightDrawable* l)
        : _pipeline(p), _mainLight(l) {}

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::KEYUP:
            if (ea.getKey() == 'X')
            {
                OSG_NOTICE << "*** Changing to standard pipeline" << std::endl;
                _pipeline->clearStagesFromView(view);

                osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR "barcelona.hdr");
                setupStandardPipeline(_pipeline.get(), view, params);

                osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(_pipeline->getModule("Light"));
                if (light) light->setMainLight(_mainLight.get(), "Shadow");
            }
            else if (ea.getKey() == 'Z')
            {
                OSG_NOTICE << "*** Changing to fixed pipeline" << std::endl;
                _pipeline->clearStagesFromView(view);
            }
            break;
        }
        return false;
    }

protected:
    osg::observer_ptr<osgVerse::Pipeline> _pipeline;
    osg::observer_ptr<osgVerse::LightDrawable> _mainLight;
};

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
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Node> scene = (argc > 1) ? osgDB::readNodeFiles(arguments)
                                  : osgDB::readNodeFile(BASE_DIR "/models/Sponza.osgb");
    if (!scene) { OSG_WARN << "Failed to load scene model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv;
    scene->accept(tsv);

    if (arguments.read("--save"))
    {
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options;
        options->setPluginStringData("TargetFileVersion", "91");  // the first version

        // Compress and optimize textures (it may take a while)
        // With op: CPU memory = 167.5MB, GPU memory = 0.8GB
        // Without: CPU memory = 401.8MB, GPU memory = 2.1GB
        osgVerse::TextureOptimizer texOp; scene->accept(texOp);
        osgDB::writeNodeFile(*scene, "pbr_scene.osgb", options.get());
        return 0;
    }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->setName("PbrSceneRoot");
    sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.0,0,-250.trans");
    if (otherSceneRoot.valid())
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, CUSTOM_INPUT_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (argc == 1) root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());
    root->setName("Root");

    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(1.5f, 1.5f, 1.2f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Create the pipeline
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR "barcelona.hdr");
#if true
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
#else  // Set different Context and GLSL version on you own risk
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline(100, 330);
#endif

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }

    // Realize the viewer
    MyViewer viewer(pipeline.get());
    viewer.addEventHandler(new SetPipelineHandler(pipeline.get(), light0.get()));
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

#ifdef OSG_GL3_AVAILABLE
#   ifdef VERSE_WINDOWS
    // WGL_CONTEXT_DEBUG_BIT_ARB = 0x0001
    // WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB = 0x0002
    int flags = 0x0001 | 0x0002;
#   else
    int flags = 0;  // TODO
#   endif
    osg::ref_ptr<osg::GraphicsContext> gc = pipeline->createGraphicsContext(1920, 1080, "4.5", NULL, flags);
    viewer.getCamera()->setGraphicsContext(gc.get());
    viewer.getCamera()->setProjectionMatrix(osg::Matrix::perspective(
        30., (double)1920 / (double)1080, 1., 100.));
    viewer.getCamera()->setViewport(new osg::Viewport(0, 0, 1920, 1080));
    viewer.setRealizeOperation(new GLDebugOperation);

    osgVerse::FixedFunctionOptimizer ffo;
    root->accept(ffo);
#else
    // Always call viewer.setUp*() before setupStandardPipeline()!
    viewer.setUpViewOnSingleScreen(0);
#endif

    // Setup the pipeline
#if true
    params.enablePostEffects = true; params.enableAO = true;
    params.userInputMask = CUSTOM_INPUT_MASK; params.enableUserInput = true;
    setupStandardPipeline(pipeline.get(), &viewer, params);
#else
    std::ifstream ppConfig(SHADER_DIR "/standard_pipeline.json");
    pipeline->load(ppConfig, &viewer);
#endif

    // How to use clear color instead of skybox...
    //postCamera->removeChild(skybox.get());
    //pipeline->getStage("GBuffer")->camera->setClearColor(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));

    // Post pipeline settings
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

    // Start the main loop
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}
