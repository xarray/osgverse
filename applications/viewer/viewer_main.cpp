#include PREPENDED_HEADER
#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <pipeline/SkyBox.h>
#include <pipeline/ShaderLibrary.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <wrappers/Export.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

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

                osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR + "barcelona.hdr");
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

class DebugEventCallback : public osg::NodeCallback
{
public:
    DebugEventCallback(osgVerse::Pipeline* p, osg::StateSet* ss, osgText::Text* t)
        : _pipeline(p), _stateset(ss), _text(t) { applyStageWindow(0); }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osgGA::EventVisitor* ev = static_cast<osgGA::EventVisitor*>(nv);
        if (!ev) { traverse(node, nv); return; }
        
        osgGA::EventQueue::Events& events = ev->getEvents();
        for (osgGA::EventQueue::Events::iterator itr = events.begin(); itr != events.end(); ++itr)
        {
            osgGA::GUIEventAdapter* ea = dynamic_cast<osgGA::GUIEventAdapter*>(itr->get());
            if (!ea || (ea && ea->getEventType() != osgGA::GUIEventAdapter::RELEASE)) continue;

            float xx = ea->getXnormalized() * 0.5f + 0.5f;
            float yy = ea->getYnormalized() * 0.5f + 0.5f;
            if (xx > 0.2f || yy > 0.2f) continue;

            if (xx < 0.05f) applyStageWindow(_currentStage - 1, 0);
            else if (xx > 0.15f) applyStageWindow(_currentStage + 1, 0);
            else applyStageWindow(_currentStage, _currentOutput + 1);
        }
        traverse(node, nv);
    }

    void applyStageWindow(int stage, int output = 0)
    {
        int numStages = (int)_pipeline->getNumStages();
        if (stage < 0) stage = numStages - 1;
        else if (numStages <= stage) stage = 0;

        osgVerse::Pipeline::Stage* s = _pipeline->getStage(stage);
        int numOutputs = (int)s->outputs.size(), ptr = 0;
        if (output < 0) output = numOutputs - 1;
        else if (numOutputs <= output) output = 0;

        for (std::map<std::string, osg::observer_ptr<osg::Texture>>::iterator
             it = s->outputs.begin(); it != s->outputs.end(); ++it, ++ptr)
        {
            if (ptr != output) continue;
            _stateset->setTextureAttributeAndModes(0, it->second.get());
            _text->setText(s->name + "::" + it->first); break;
        }
        _currentStage = stage;
        _currentOutput = output;
    }

protected:
    osg::observer_ptr<osgVerse::Pipeline> _pipeline;
    osg::observer_ptr<osg::StateSet> _stateset;
    osg::observer_ptr<osgText::Text> _text;
    int _currentStage, _currentOutput;
};

void addStagesToHUD(osgVerse::Pipeline* pipeline, osg::Camera* camera)
{
    osgText::Text* text = new osgText::Text;
    text->setPosition(osg::Vec3(0.1f, 0.205f, 0.01f));
    text->setAlignment(osgText::Text::CENTER_BOTTOM);
    text->setCharacterSize(0.01f, 1.0f);
    text->setFont(MISC_DIR + "LXGWFasmartGothic.ttf");

    osg::Geode* textGeode = new osg::Geode;
    textGeode->addDrawable(text);
    camera->addChild(textGeode);

    osg::Node* quad = osgVerse::createScreenQuad(
        osg::Vec3(0.0f, 0.0f, 0.0f), 0.2f, 0.2f, osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    camera->setEventCallback(new DebugEventCallback(pipeline, quad->getOrCreateStateSet(), text));
    camera->addChild(quad);
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);
    osgVerse::updateOsgBinaryWrappers();

    std::string optString, optAll;
    while (arguments.read("-O", optString)) optAll += optString + " ";
    osg::ref_ptr<osgDB::Options> options = optAll.empty() ? NULL : new osgDB::Options(optAll);

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments, options.get());
    if (!scene) scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza.osgb", options.get());
    if (!scene) { OSG_WARN << "Failed to load scene model" << std::endl; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
    osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);

    if (arguments.read("--save"))
    {
        osg::ref_ptr<osgDB::Options> options = new osgDB::Options;
        options->setPluginStringData("TargetFileVersion", "91");  // the first version
        options->setPluginStringData("WriteImageHint", "IncludeFile");
        options->setPluginStringData("UseBASISU", "1");

        // Compress and optimize textures (it may take a while)
        // With op: CPU memory = 167.5MB, GPU memory = 0.8GB
        // Without: CPU memory = 401.8MB, GPU memory = 2.1GB
        scene = osgDB::readNodeFile(BASE_DIR + "/models/Sponza/Sponza.gltf.125,125,125.scale");
        if (scene.valid())
        {
            osgVerse::TextureOptimizer texOp(true);
            texOp.setGeneratingMipmaps(true); scene->accept(texOp);
            osgDB::writeNodeFile(*scene, "pbr_scene.osgb", options.get());
        }
        return 0;
    }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->setName("PbrSceneRoot");
    sceneRoot->addChild(scene.get());
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.0,0,-250.trans");
    if (otherSceneRoot.valid())
    {
        osgVerse::FixedFunctionOptimizer ffo; otherSceneRoot->accept(ffo);
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, CUSTOM_INPUT_MASK);
    }

    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (otherSceneRoot.valid()) root->addChild(otherSceneRoot.get());
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
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SKYBOX_DIR + "barcelona.hdr");
#if true
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
#else  // Set different Context and GLSL version on you own risk
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline(100, 330);
#endif

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    osgVerse::Pipeline::setPipelineMask(*postCamera, FORWARD_SCENE_MASK);
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
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

    float lodScale = 1.0f;
    if (arguments.read("--lod-scale", lodScale))
        viewer.getCamera()->setLODScale(lodScale);

    // FIXME: how to avoid shadow problem...
    // If renderer->setGraphicsThreadDoesCull(false), which is used by DrawThreadPerContext & ThreadPerCamera,
    // Shadow will go jigger because the output texture is not sync-ed before lighting...
    // For SingleThreaded & CullDrawThreadPerContext it seems OK
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

#ifdef false  // GLDebug requires OpenGL 4.3, enable it by yourselves
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
    if (arguments.read("--msaa-4x")) params.coverageSamples = osgVerse::Pipeline::COVERAGE_SAMPLES_4X;
    else if (arguments.read("--msaa-16x")) params.coverageSamples = osgVerse::Pipeline::COVERAGE_SAMPLES_16X;
    params.enablePostEffects = true; params.enableAO = true; params.enableUserInput = true;
    params.addUserInputStage("Forward", CUSTOM_INPUT_MASK,
                             osgVerse::StandardPipelineParameters::BEFORE_FINAL_STAGE);
    
    if (arguments.read("--no-ssao")) params.enableAO = false;
    if (arguments.read("--no-posteffects")) params.enablePostEffects = false;
    if (arguments.read("--no-shadows")) params.shadowNumber = 0;
    setupStandardPipeline(pipeline.get(), &viewer, params);
#else
    std::ifstream ppConfig(SHADER_DIR "/standard_pipeline.json");
    pipeline->load(ppConfig, &viewer);
#endif

    if (arguments.read("--gbuffer-variant-test"))
    {   // How to inherit and set custom GBuffer shaders to specified object
        osg::ref_ptr<osgVerse::ScriptableProgram> gbufferProg = static_cast<osgVerse::ScriptableProgram*>(
            pipeline->getStage("GBuffer")->getProgram()->clone(osg::CopyOp::DEEP_COPY_ALL));
        gbufferProg->setName("Custom_GBuffer_PROGRAM");
        gbufferProg->addSegment(osg::Shader::FRAGMENT, 1, "diffuse.rgb = vec3(1.0, 0.0, 0.0);");
        scene->getOrCreateStateSet()->setAttribute(
            gbufferProg.get(), osg::StateAttribute::ON | osg::StateAttribute::PROTECTED);
    }

    osg::Vec3 bkColor(0.2f, 0.2f, 0.4f);
    if (arguments.read("--background-color", bkColor[0], bkColor[1], bkColor[2]))
    {   // How to use clear color instead of skybox...
        postCamera->removeChild(skybox.get());
        pipeline->getStage("GBuffer")->camera->setClearColor(osg::Vec4(bkColor, 1.0f));
    }

    // Post pipeline settings
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");

    if (arguments.read("--display-variant-test"))
    {   // How to add custom code to ScriptableProgram
        osgVerse::ScriptableProgram* display = pipeline->getStage("Final")->getProgram();
        if (display) display->addSegment(osg::Shader::FRAGMENT, 0,
                                         "if (gl_FragCoord.x < 960.0) fragData = vec4(depthValue);");
    }

    if (arguments.read("--debug-window"))
    {
        osg::ref_ptr<osg::Camera> debugCamera = new osg::Camera;
        debugCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
        debugCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        debugCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
        debugCamera->setViewMatrix(osg::Matrix::identity());
        debugCamera->setRenderOrder(osg::Camera::POST_RENDER, 10001);
        debugCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
        osgVerse::Pipeline::setPipelineMask(*debugCamera, FORWARD_SCENE_MASK);

        addStagesToHUD(pipeline.get(), debugCamera.get());
        root->addChild(debugCamera.get());

        postCamera->setName("PostSkyCamera");
        debugCamera->setName("DebugCamera");
    }

    // Start the main loop
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}
