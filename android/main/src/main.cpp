#include "SDL.h"

int SDL_main(int argc, char* argv[])
{
    bool quit = false;
    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL2

    // Create an application window with the following settings:
    SDL_Window* window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        640,                               // width, in pixels
        480,                               // height, in pixels
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

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
