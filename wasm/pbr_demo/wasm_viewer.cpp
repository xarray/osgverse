#include <SDL.h>
#include "wasm_viewer.h"
#include <emscripten.h>
#include <osg/LineWidth>
#include <osg/BlendFunc>
#include <osgGA/GUIEventHandler>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <modeling/GeometryMerger.h>
#include <pipeline/IntersectionManager.h>
#include <pipeline/UserInputModule.h>

#define TEST_PIPELINE 1
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)

osg::ref_ptr<Application> g_app = new Application;
void loop() { g_app->frame(); }

namespace
{
    // Initial implemented by @Water_Peach at gitee.com
    osg::ref_ptr<osg::Node> g_highlightNode = nullptr;
    osg::ref_ptr<osg::Group> g_highLightRoot = nullptr;

    void inflateGeometry(osg::Geometry* geom, float inflationFactor)
    {
        osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
        osg::Vec3Array* normals = dynamic_cast<osg::Vec3Array*>(geom->getNormalArray());
        if (!vertices || !normals) return;
        if (geom->getNormalBinding() != osg::Geometry::BIND_PER_VERTEX) return;

        for (size_t i = 0; i < vertices->size(); ++i)
            (*vertices)[i] += (*normals)[i] * inflationFactor;
        vertices->dirty();
    }

    // Create highlighted model for show
    osg::Node* createTransparentHighlightModel(osg::Drawable* drawable, osg::Matrix mat)
    {
        if (!drawable) return nullptr;
        osg::ref_ptr<osg::Drawable> clonedDrawable =
            dynamic_cast<osg::Drawable*>(drawable->clone(osg::CopyOp::DEEP_COPY_ALL));

        osg::Geometry* geometry = dynamic_cast<osg::Geometry*>(clonedDrawable.get());
        if (!geometry) return nullptr;
        else inflateGeometry(geometry, 0.005f);

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(clonedDrawable);

        // Set a suitable transparent stateset
        osg::StateSet* stateSet = geode->getOrCreateStateSet();
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setAttributeAndModes(
            new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA), osg::StateAttribute::ON);
        stateSet->setMode(GL_DEPTH_WRITEMASK, osg::StateAttribute::OFF);
        stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateSet->setRenderBinDetails(10, "TransparentBin");

        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(0.0f, 1.0f, 1.0f, 0.5f));
        geometry->setColorArray(colors, osg::Array::BIND_OVERALL);
        geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

        osg::ref_ptr<osg::MatrixTransform> transform = new osg::MatrixTransform;
        transform->setMatrix(mat);
        transform->addChild(geode);

        // Add transparent nodes to custom input pass
        osgVerse::Pipeline::setPipelineMask(*transform, CUSTOM_INPUT_MASK);
        return transform.release();
    }

    // Update highlighted model
    void updateHighlightDisplay(osg::Drawable* drawable, osg::Matrix mat)
    {
        if (!g_highLightRoot) return;
        {
            g_highLightRoot->removeChild(0, g_highLightRoot->getNumChildren());
            g_highlightNode = nullptr;
        }

        if (drawable)
        {
            g_highlightNode = createTransparentHighlightModel(drawable, mat);
            if (g_highlightNode) g_highLightRoot->addChild(g_highlightNode.get());
        }
    }
}

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

class MouseClickHandler : public osgGA::GUIEventHandler
{
public:
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        // Initial implemented by @Water_Peach at gitee.com
        if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
            (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL))
        {
            float x = ea.getX(), y = ea.getY();
            osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
            if (view)
            {
                // Ray intersection from mouse position
                osgVerse::IntersectionResult result = osgVerse::findNearestIntersection(
                    view->getCamera(), ea.getXnormalized(), ea.getYnormalized());
                if (!result.drawable) return false;

                if (result.drawable)
                {
                    // Get intersection result
                    auto center = result.drawable->getBoundingBox().center();
                    x = center.x(); y = center.y();
                    updateHighlightDisplay(result.drawable.get(), result.matrix);

                    float z = center.z();
                    std::string geoName = result.drawable->getName();
                    EM_ASM(
                        if (typeof handleMouseClick === 'function') {
                            handleMouseClick($0, $1, $2, UTF8ToString($3));
                        }
                        else {
                            console.log('handleMouseClick function not found');
                        }
                    , x, y, z, geoName.c_str());  // sent to JavaScript side
                }
                else
                {
                    g_highLightRoot->removeChild(0, g_highLightRoot->getNumChildren());
                    /*EM_ASM(
                        console.log('no clicked result');
                    );*/
                }
            }
        }
        return false;
    }
};

// Server structure
/* - <assets>: User resource folder copied from master/assets
   - osgVerse_ViewerWASM.data: preload data (only shaders)
   - osgVerse_ViewerWASM.html: main HTML page
   - osgVerse_ViewerWASM.js: main Javascript file
   - osgVerse_ViewerWASM.wasm: main WASM file
   - osgVerse_ViewerWASM.wasm.map: source-map for debugging
*/
#define SERVER_ADDR "http://127.0.0.1:8000/assets"
int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(SERVER_ADDR "/models/Sponza/Sponza.gltf");

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    if (scene.valid()) sceneRoot->addChild(scene.get());
    //sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    sceneRoot->setName("PbrSceneRoot");
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());

    // 设置全局场景根节点
    osg::ref_ptr<osg::Group> highLightRoot = new osg::Group;
    root->addChild(highLightRoot.get());
    osgVerse::Pipeline::setPipelineMask(*highLightRoot, CUSTOM_INPUT_MASK);
    g_highLightRoot = highLightRoot;

    // Main light
    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(3.0f, 3.0f, 2.8f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());
    root->addChild(lightGeode.get());

    // Create the pipeline
    osgVerse::StandardPipelineParameters params(SHADER_DIR, SERVER_ADDR "/skyboxes/sunset.png");
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;

    // Post-HUD display
    // FIXME: No forward scene can be added for WebGL at present
#if false
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox(pipeline.get());
    {
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }
#endif

    // Create the viewer
#if TEST_PIPELINE
    //osg::setNotifyLevel(osg::INFO);
    MyViewer* viewer = new MyViewer(pipeline.get());
    OSG_NOTICE << "PBR + deferred rendering mode\n";
#else
    //osg::setNotifyLevel(osg::INFO);
    root = new osg::Group;
    root->addChild(postCamera.get());
    root->addChild(sceneRoot.get());

    osgViewer::Viewer* viewer = new osgViewer::Viewer;
#endif
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    viewer->addEventHandler(new MouseClickHandler); // 添加鼠标点击事件处理器
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setSceneData(root.get());

    // Create the graphics window
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0; traits->y = 0; traits->width = 800; traits->height = 600;
    traits->alpha = 8; traits->depth = 24; traits->stencil = 8;
    traits->windowDecoration = true; traits->doubleBuffer = true;
    traits->readDISPLAY(); traits->setUndefinedScreenDetailsToDefaultScreen();
    traits->windowingSystemPreference = "SDL";
    //traits->inheritedWindowData = new osgVerse::GraphicsWindowSDL::WindowData("#canvas");

    osgVerse::Pipeline::Stage* gbuffer = pipeline->addInputStage(
    "GBuffer", DEFERRED_SCENE_MASK, 0,
    osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "std_gbuffer.vert.glsl"),
    osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "std_gbuffer.frag.glsl"), 5,
    "NormalBuffer", osgVerse::Pipeline::RGBA_INT8,
    "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
    "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
    "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
    "DepthBuffer", osgVerse::Pipeline::DEPTH24_STENCIL8);

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer->getCamera()->setGraphicsContext(gc.get());
    viewer->getCamera()->setViewport(0, 0, traits->width, traits->height);
    viewer->getCamera()->setDrawBuffer(GL_BACK);
    viewer->getCamera()->setReadBuffer(GL_BACK);

    osgVerse::UserInputModule* inModule = new osgVerse::UserInputModule("Forward", pipeline.get());
    {
        osgVerse::Pipeline::Stage* customIn = inModule->createStages(
            NULL, NULL,//new osg::Shader(osg::Shader::FRAGMENT, inputFragmentShaderCode),
            NULL, CUSTOM_INPUT_MASK,
            "ColorBuffer", gbuffer->getBufferTexture("DiffuseMetallicBuffer"),
            "DepthBuffer", gbuffer->getBufferTexture(osg::Camera::DEPTH_BUFFER));
    }

    // Setup the pipeline
#if TEST_PIPELINE
    params.enablePostEffects = true; params.enableAO = true; params.enableUserInput = true;
    params.shadowNumber = 3;
    params.addUserInputStage("Forward", CUSTOM_INPUT_MASK,
                             osgVerse::StandardPipelineParameters::BEFORE_FINAL_STAGE);
    params.originWidth = traits->width; params.originHeight = traits->height;

    queryOpenGLVersion(pipeline.get(), true);
    setupStandardPipeline(pipeline.get(), viewer, params);

    // Post pipeline settings
    osgVerse::ShadowModule* shadow = static_cast<osgVerse::ShadowModule*>(pipeline->getModule("Shadow"));
    if (shadow && shadow->getFrustumGeode())
    {
        osgVerse::Pipeline::setPipelineMask(*shadow->getFrustumGeode(), FORWARD_SCENE_MASK);
        root->addChild(shadow->getFrustumGeode());
    }

    osgVerse::LightModule* light = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (light) light->setMainLight(light0.get(), "Shadow");
#endif

    // Start the main loop
    atexit(SDL_Quit);
    g_app->setViewer(viewer);
    emscripten_set_fullscreenchange_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, g_app.get(), 1, Application::fullScreenCallback);
    emscripten_set_webglcontextlost_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, g_app.get(), 1, &Application::contextLostCallback);
    emscripten_set_webglcontextrestored_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, g_app.get(), 1, &Application::contextRestoredCallback);
    emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, g_app.get(), 1, &Application::canvasWindowResized);
    emscripten_set_main_loop(loop, -1, 0);
    return 0;
}
