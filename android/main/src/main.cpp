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
#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>
#include <android/log.h>
#include <SDL.h>

USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)

int SDL_main(int argc, char* argv[])
{
    osgVerse::globalInitialize(argc, argv);
    osg::setNotifyHandler(new osgVerse::ConsoleHandler);

    bool quit = false;
    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2
    __android_log_print(ANDROID_LOG_VERBOSE, "osgVerse", "SDL_main() started");

    // Create an application window with the following settings:
    SDL_Window* window = SDL_CreateWindow(
        "PBR Example",                     // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        480,                               // width, in pixels
        800,                               // height, in pixels
        SDL_WINDOW_OPENGL                  // flags - see below
    );

    // TODO

    // Event loop
    SDL_Event event;
    while(!quit && SDL_WaitEvent(&event))
    {
        switch (event.type)
        {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYDOWN:
                if ((event.key.keysym.sym == SDLK_AC_BACK) || (event.key.keysym.sym == SDLK_ESCAPE))
                    quit = true;
                break;
            default:
                break;
        }
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "osgVerse", "SDL_main() finished");
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
