#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define GL_VERSION 0x1F02
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
typedef const unsigned char* (*PFNGLGETSTRING)(unsigned int);

int main()
{
    if (!glfwInit()) { printf("Failed to initialize GLFW\n"); return 1; }
#if defined(VERSE_GLCORE)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
#elif defined(VERSE_EMBEDDED_GLES2)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(VERSE_EMBEDDED_GLES3)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
#endif
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(640, 480, "OpenGL_Test", NULL, NULL);
    if (!window) { printf("Failed to create window\n"); glfwTerminate(); return 2; }
    glfwMakeContextCurrent(window);

    PFNGLGETSTRING glGetString = (PFNGLGETSTRING)glfwGetProcAddress("glGetString");
    if (glGetString != NULL)
    {
        const unsigned char* version = glGetString(GL_VERSION);
        const unsigned char* vendor = glGetString(GL_VENDOR);
        const unsigned char* renderer = glGetString(GL_RENDERER);
        if (!version || !vendor || !renderer)
            printf("Failed to get OpenGL information\n");
        else
            printf("Version: %s; %s; %s\n", version, vendor, renderer);
    }
    else
        printf("Failed to obtain glGetString() function\n");
    glfwDestroyWindow(window);
    glfwTerminate(); return 0;
}
