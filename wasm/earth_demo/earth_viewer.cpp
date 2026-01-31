#include <SDL.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/TileCallback.h>
#include <readerwriter/Utilities.h>
#include <pipeline/ShaderLibrary.h>
#include <pipeline/Predefines.h>
#include "earth_viewer.h"

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_OSGPLUGIN(verse_tiff)
USE_SERIALIZER_WRAPPER(DracoGeometry)
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)

osgVerse::EarthAtmosphereOcean g_earthRenderingUtils;
osg::ref_ptr<Application> g_app = new Application;
void loop() { g_app->frame(); }

extern "C"
{
    const char* EMSCRIPTEN_KEEPALIVE execute(const char* cmd, const char* json)
    {
        std::string type = (cmd == NULL) ? "get" : std::string(cmd);
        std::string input = (json == NULL) ? "" : std::string(json);
        osgVerse::JsonScript* scripter = g_app->scripter();

        picojson::value in, out;
        std::string output = picojson::parse(in, input);
        if (output.empty())
        {
            if (type.find("creat") != type.npos)
                out = scripter->execute(osgVerse::JsonScript::EXE_Creation, in);
            else if (type.find("list") != type.npos)
                out = scripter->execute(osgVerse::JsonScript::EXE_List, in);
            else if (type.find("remove") != type.npos)
                out = scripter->execute(osgVerse::JsonScript::EXE_Remove, in);
            else if (type.find("set") != type.npos)
                out = scripter->execute(osgVerse::JsonScript::EXE_Set, in);
            else
                out = scripter->execute(osgVerse::JsonScript::EXE_Get, in);
            output = out.serialize(false);
        }

        static char* result = NULL; if (result != NULL) free(result);
        int size = output.length(); result = (char*)malloc(size + 1);
        memcpy(result, output.c_str(), size);
        result[size] = '\0'; return result;
    }
}

const char* finalVertCode = {
    "VERSE_VS_OUT vec4 texCoord; \n"
    "void main() {\n"
    "    texCoord = osg_MultiTexCoord0; \n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "}\n"
};

const char* finalFragCode = {
    "uniform sampler2D EarthTexture, OceanTexture;\n"
    "VERSE_FS_IN vec4 texCoord; \n"
    "VERSE_FS_OUT vec4 fragColor;\n"

    "void main() {\n"
    "    vec4 sceneColor = VERSE_TEX2D(EarthTexture, texCoord.st);\n"
    "    vec4 oceanColor = VERSE_TEX2D(OceanTexture, texCoord.st);\n"
    "    fragColor = mix(sceneColor, oceanColor, oceanColor.a); \n"
    "    VERSE_FS_FINAL(fragColor);\n"
    "}\n"
};

static std::string createMixedPath(int type, const std::string& prefix, int x, int y, int z)
{
    if (type == osgVerse::TileCallback::ORTHOPHOTO)
    {
        if (z > 4)
        {
            std::string prefix2 = "https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}";
            return osgVerse::TileCallback::createPath(prefix2, x, pow(2, z) - y - 1, z);
        }
    }
    return osgVerse::TileCallback::createPath(prefix, x, y, z);
}

class EnvironmentHandler : public osgGA::GUIEventHandler
{
public:
    EnvironmentHandler(osgVerse::EarthAtmosphereOcean* e, bool b)
        : _earthData(e), _withOcean(b) {}

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            _earthData->update(view->getCamera());
            if (_withOcean) _earthData->updateOcean(view->getCamera());
        }
        return false;
    }

protected:
    osgVerse::EarthAtmosphereOcean* _earthData;
    bool _withOcean;
};

osg::Node* createEarthWithSkyAndOcean(osgViewer::Viewer* viewer, osg::Group* root,
                                      const std::string& baseDir, int w, int h)
{
    std::string mainFolder = baseDir + "/models/Earth";
    std::string earthURLs = " Orthophoto=mbtiles://" + mainFolder + "/DOM_lv4.mbtiles/{z}-{x}-{y}.jpg"
                            " Elevation=mbtiles://" + mainFolder + "/DEM_lv3.mbtiles/{z}-{x}-{y}.tif"
                            " OceanMask=mbtiles://" + mainFolder + "/Mask_lv3.mbtiles/{z}-{x}-{y}.tif"
                            " UseWebMercator=1 UseEarth3D=1 OriginBottomLeft=1 TileElevationScale=3 TileSkirtRatio=0.05";
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs);
    earthOptions->setPluginData("UrlPathFunction", (void*)createMixedPath);

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
    if (!earth) { OSG_WARN << "[EarthWASM] Failed to load earth data\n"; return NULL; }

    // Initialize earth utilities
    g_earthRenderingUtils.create(baseDir + "/textures/transmittance.raw",
                                 baseDir + "/textures/irradiance.raw",
                                 baseDir + "/textures/sunglare.png",
                                 baseDir + "/textures/inscatter.raw");

    // Add earth globe to RTT camera
    osg::ref_ptr<osg::Texture> earthColorBuffer =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
    osg::ref_ptr<osg::Texture> earthMaskBuffer =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::R_INT8, w, h);

    osg::Camera* earthCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, NULL, NULL, false);
    earthCamera->attach(osg::Camera::COLOR_BUFFER0, earthColorBuffer.get(), 0, 0, false, 16, 4);
    earthCamera->attach(osg::Camera::COLOR_BUFFER1, earthMaskBuffer.get());
    earthCamera->setViewport(0, 0, w, h);
    earthCamera->addChild(earth.get());

    osg::Texture* defTex0 = osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    osg::Texture* defTex1 = osgVerse::createDefaultTexture(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    g_earthRenderingUtils.applyToGlobe(earthCamera->getOrCreateStateSet(), defTex0, defTex0, defTex1,
            osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_globe.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_globe.frag.glsl"));
    osgVerse::ShaderLibrary::instance()->updateProgram(*earthCamera->getStateSet());
    root->addChild(earthCamera);

    // Merge atmosphere with earth color and render them on a screen quad
    osg::ref_ptr<osg::Texture> mergedGlobeBuffer = osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
    osg::Camera* skyAndGlobeCamera = osgVerse::createRTTCamera(
        osg::Camera::COLOR_BUFFER0, mergedGlobeBuffer.get(), NULL, true);
    g_earthRenderingUtils.applyToAtmosphere(skyAndGlobeCamera->getOrCreateStateSet(), earthColorBuffer.get(),
            osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_sky.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_sky.frag.glsl"));
    osgVerse::ShaderLibrary::instance()->updateProgram(*skyAndGlobeCamera->getStateSet());
    root->addChild(skyAndGlobeCamera);

    // Add post-processing ocean camera if necessary
    osg::Geometry* grid = g_earthRenderingUtils.createOceanGrid(w, h);
    osg::Geode* grideGeode = new osg::Geode; grideGeode->addDrawable(grid);

    osg::ref_ptr<osg::Texture> oceanColorBuffer = osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
    osg::Camera* oceanCamera = osgVerse::createRTTCamera(
        osg::Camera::COLOR_BUFFER0, oceanColorBuffer.get(), NULL, false);
    oceanCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    oceanCamera->setProjectionMatrix(osg::Matrix::ortho2D(-1.0, 1.0, -1.0, 1.0));
    oceanCamera->setViewMatrix(osg::Matrix::identity());
    oceanCamera->addChild(grideGeode);

    float seaRoughness = 0.0f;
    g_earthRenderingUtils.applyToOcean(oceanCamera->getOrCreateStateSet(),
            earthMaskBuffer.get(), g_earthRenderingUtils.createOceanWaves(seaRoughness),
            osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "global_ocean.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "global_ocean.frag.glsl"));
    osgVerse::ShaderLibrary::instance()->updateProgram(*oceanCamera->getStateSet());
    root->addChild(oceanCamera);

    // Merge results to an HUD camera to show
    osg::Camera* finalCamera = osgVerse::createHUDCamera(NULL, w, h, osg::Vec3(), 1.0f, 1.0f, true);
    finalCamera->getOrCreateStateSet()->setTextureAttributeAndModes(0, mergedGlobeBuffer.get());
    finalCamera->getOrCreateStateSet()->setTextureAttributeAndModes(1, oceanColorBuffer.get());
    finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("EarthTexture", (int)0));
    finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("OceanTexture", (int)1));

    osg::ref_ptr<osg::Program> program = new osg::Program;
    {
        osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, finalVertCode);
        osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, finalFragCode);
        program->addShader(vs); program->addShader(fs);
        osgVerse::ShaderLibrary::instance()->updateProgram(*program);
    }
    finalCamera->getOrCreateStateSet()->setAttributeAndModes(program.get());
    root->addChild(finalCamera);

    viewer->addEventHandler(new EnvironmentHandler(&g_earthRenderingUtils, true));
    return earth.get();
}

// Server structure
/* - <assets>: User resource folder copied from master/assets
   - osgVerse_EarthExplorerWASM.data: preload data (only shaders)
   - osgVerse_EarthExplorerWASM.html: main HTML page
   - osgVerse_EarthExplorerWASM.js: main Javascript file
   - osgVerse_EarthExplorerWASM.wasm: main WASM file
   - osgVerse_EarthExplorerWASM.wasm.map: source-map for debugging
*/
#define SERVER_ADDR "http://127.0.0.1:8000/assets"
int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osgDB::Registry::instance()->addFileExtensionAlias("tif", "verse_tiff");
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    g_app->scripter()->setRootNode(root.get());

    std::string extensions, optData;
    if (arguments.read("--extensions", extensions))
    {
        if (extensions.find("WEBGL_compressed_texture_s3tc") != std::string::npos) optData = "UseDXT=1 UseETC=0";
        else if (extensions.find("webgl_compressed_texture_etc") != std::string::npos) optData = "UseDXT=0 UseETC=1";
        else optData = "UseDXT=0 UseETC=0";
    }

    // Create the viewer
    osgViewer::Viewer* viewer = new osgViewer::Viewer;
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setSceneData(root.get());

    osg::ref_ptr<osgVerse::EarthManipulator> manipulator = new osgVerse::EarthManipulator;
    osg::Node* earth = createEarthWithSkyAndOcean(viewer, root.get(), SERVER_ADDR, 800, 600);
    if (earth) manipulator->setWorldNode(earth);
    viewer->setCameraManipulator(manipulator.get());

    // Create the graphics window
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 0; traits->y = 0; traits->width = 800; traits->height = 600;
    traits->alpha = 8; traits->depth = 24; traits->stencil = 8;
    traits->windowDecoration = true; traits->doubleBuffer = true;
    traits->readDISPLAY(); traits->setUndefinedScreenDetailsToDefaultScreen();
    traits->windowingSystemPreference = "SDL";

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer->getCamera()->setGraphicsContext(gc.get());
    viewer->getCamera()->setViewport(0, 0, traits->width, traits->height);
    viewer->getCamera()->setDrawBuffer(GL_BACK);
    viewer->getCamera()->setReadBuffer(GL_BACK);

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
