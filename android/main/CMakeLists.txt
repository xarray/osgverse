cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
             main-lib

             SHARED

             # Provides a relative path to your source file(s).
             src/SDL_android_main.c
             src/main.cpp )


find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )



target_link_libraries( # Specifies the target library.
                        main-lib
                        SDL2
                        ${log-lib} )
