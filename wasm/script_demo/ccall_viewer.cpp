#include <SDL2/SDL.h>
#include <pipeline/ShaderLibrary.h>
#include <pipeline/Predefines.h>
#include "ccall_viewer.h"

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)

osg::ref_ptr<Application> g_app = new Application;
void loop() { g_app->frame(); }

const char* vertCode = {
    "VERSE_VS_OUT vec4 texCoord;\n"
    "void main() {\n"
    "    texCoord = osg_MultiTexCoord0;\n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;\n"
    "}\n"
};

const char* fragCode = {
    "uniform sampler2D DiffuseMap;\n"
    "VERSE_FS_IN vec4 texCoord;\n"
    "VERSE_FS_OUT vec4 fragData;\n"
    "void main() {\n"
    "    fragData = VERSE_TEX2D(DiffuseMap, texCoord.st);\n"
    "    VERSE_FS_FINAL(fragData);\n"
    "}\n"
};

void applyProgram(osg::Node* scene)
{
    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, vertCode));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragCode));
    osgVerse::ShaderLibrary::instance()->updateProgram(*program);

    osg::StateSet* stateSet = scene->getOrCreateStateSet();
    stateSet->setAttribute(program.get());
    stateSet->addUniform(new osg::Uniform("DiffuseMap", (int)0));
}

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

// Server structure
/* - <assets>: User resource folder copied from master/assets
   - osgVerse_JsCallerWASM.data: preload data (only shaders)
   - osgVerse_JsCallerWASM.html: main HTML page
   - osgVerse_JsCallerWASM.js: main Javascript file
   - osgVerse_JsCallerWASM.wasm: main WASM file
   - osgVerse_JsCallerWASM.wasm.map: source-map for debugging
*/
#define SERVER_ADDR "http://127.0.0.1:8000/assets"
int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Group> root = new osg::Group;
    g_app->scripter()->setRootNode(root.get());

    std::string extensions, optData;
    if (arguments.read("--extensions", extensions))
    {
        if (extensions.find("WEBGL_compressed_texture_s3tc") != std::string::npos) optData = "UseDXT=1 UseETC=0";
        else if (extensions.find("webgl_compressed_texture_etc") != std::string::npos) optData = "UseDXT=0 UseETC=1";
        else optData = "UseDXT=0 UseETC=0";
    }

    // FIXME: These will be called in caller.js for testing osgVerse scripting module
#if false
    osgDB::Options* options = new osgDB::Options(optData);
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+958_+8053/Tile_+958_+8053.osgb", options));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+958_+8054/Tile_+958_+8054.osgb", options));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+958_+8055/Tile_+958_+8055.osgb", options));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+959_+8053/Tile_+959_+8053.osgb", options));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+959_+8054/Tile_+959_+8054.osgb", options));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+959_+8055/Tile_+959_+8055.osgb", options));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+960_+8053/Tile_+960_+8053.osgb", options));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+960_+8054/Tile_+960_+8054.osgb", options));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+960_+8055/Tile_+960_+8055.osgb", options));
#endif

    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());
    applyProgram(root.get());

    // Post-HUD display
    osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox;
    {
        osgVerse::StandardPipelineParameters params(SHADER_DIR, SERVER_ADDR "/skyboxes/sunset.png");
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
        skybox->setEnvironmentMap(params.skyboxMap.get(), false);
        osgVerse::Pipeline::setPipelineMask(*skybox, FORWARD_SCENE_MASK);
        postCamera->addChild(skybox.get());
    }

    // Create the viewer
    osgViewer::Viewer* viewer = new osgViewer::Viewer;
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgGA::StateSetManipulator(viewer->getCamera()->getOrCreateStateSet()));
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
