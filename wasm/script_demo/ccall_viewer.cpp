#include <emscripten.h>
#include <SDL2/SDL.h>
#include "ccall_viewer.h"

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

osg::ref_ptr<Application> g_app = new Application;
void loop()
{
    SDL_Event e;
    while (SDL_PollEvent(&e)) { g_app->handleEvent(e); }
    g_app->frame();
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
    osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Group> root = new osg::Group;
    g_app->scripter()->setRootNode(root.get());

    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+958_+8053/Tile_+958_+8053.osgb"));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+958_+8054/Tile_+958_+8054.osgb"));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+958_+8055/Tile_+958_+8055.osgb"));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+959_+8053/Tile_+959_+8053.osgb"));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+959_+8054/Tile_+959_+8054.osgb"));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+959_+8055/Tile_+959_+8055.osgb"));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+960_+8053/Tile_+960_+8053.osgb"));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+960_+8054/Tile_+960_+8054.osgb"));
    root->addChild(osgDB::readNodeFile(SERVER_ADDR "/Data/Tile_+960_+8055/Tile_+960_+8055.osgb"));

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    root->addChild(postCamera.get());

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

    // Start SDL
    int width = 800, height = 600;
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("[osgVerse] Could not init SDL: '%s'\n", SDL_GetError());
        return 1;
    }

    atexit(SDL_Quit);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_Window* window = SDL_CreateWindow(
        "osgVerse_JsCallerWASM", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_OPENGL);
    if (!window)
    {
        printf("[osgVerse] Could not create window: '%s'\n", SDL_GetError());
        return 1;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == NULL)
    {
        printf("[osgVerse] Could not create SDL context: '%s'\n", SDL_GetError());
        return 1;
    }

    // Create the application object
    g_app->setViewer(viewer, width, height);
    viewer->getCamera()->setDrawBuffer(GL_BACK);
    viewer->getCamera()->setReadBuffer(GL_BACK);

    // Start the main loop
    emscripten_set_main_loop(loop, -1, 0);
    return 0;
}
