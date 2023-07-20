#include <emscripten.h>
#include <SDL2/SDL.h>
#include "ccall_viewer.h"

extern "C"
{
    const char* execute(const char* json)
    {

    }
}

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()

Application* g_app = new Application;;
void loop()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    { if (g_app) g_app->handleEvent(e); }
    if (g_app) g_app->frame();
}

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
    osgVerse::globalInitialize(argc, argv);

    // The scene graph
    osg::ref_ptr<osg::Group> root = new osg::Group;
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

    // Create the application object
    int width = 800, height = 600;
    g_app->setViewer(viewer, width, height);
    viewer->getCamera()->setDrawBuffer(GL_BACK);
    viewer->getCamera()->setReadBuffer(GL_BACK);

    // Start SDL
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
        "osgVerse_ViewerWASM", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
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

    // Start the main loop
    emscripten_set_main_loop(loop, -1, 0);
    return 0;
}
