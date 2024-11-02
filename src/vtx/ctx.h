#pragma once

#include <GL/glew.h>
#ifdef __USE_SDL
#include <SDL2/SDL.h>
#elif defined(__USE_GLFW)
#include <GLFW/glfw3.h>
#endif

#include <iostream>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// ***********
//  Constants
// ***********

// Screen dimensions
// const int SCREEN_WIDTH  = 400;
// const int SCREEN_HEIGHT = 300;
// const int SCREEN_WIDTH  = 854;
// const int SCREEN_HEIGHT = 480;
const int SCREEN_HEIGHT  = 854;
const int SCREEN_WIDTH = 480;

// *******************************
//  Declarations of all functions
// *******************************

// 1. OpenGL init subsystem

static bool initVideo();

// 2. OpenGL shader subsystem
namespace vtx {
GLuint createShaderProgram(
    const char* vertexShader,
    const char* fragmentShader
);
}
static GLuint compileShader(GLenum type, const char* source);

// 3. OpenGL diagnostic utils

static void printShaderVersions();
static void checkOpenGLError(const char* optionalTag);

// 4. Main loop subsystem

static void performOneCycle();
namespace vtx {
void openVortex();
void exitVortex();
}

// **********************
//  Global state context
// **********************

namespace vtx {
typedef struct {
    bool shouldContinue;
#ifdef __USE_SDL
    SDL_Window* sdlWindow;
    SDL_GLContext sdlContext;
#elif defined(__USE_GLFW)
    GLFWwindow* glfwWindow;
#endif
    int screenWidth, screenHeight;
} VertexContext;

void init(vtx::VertexContext* ctx);
void loop(vtx::VertexContext* ctx);
}  // namespace vtx

// *********************************
//  Start with initializing context
// *********************************

static vtx::VertexContext ctx;

// **************************
//  1. OpenGL init subsystem
// **************************
bool initVideo()
{
#ifdef __USE_SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: "
                  << SDL_GetError() << std::endl;
        return false;
    }

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES
    );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(
        SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE
    );
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow(
        "SDL2 OpenGL Home", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,   // ignored in fullscreen
        SCREEN_HEIGHT,  // ignored in fullscreen
        SDL_WINDOW_OPENGL |
            SDL_WINDOW_SHOWN  // | SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL Error: "
                  << SDL_GetError() << std::endl;
        return false;
    }

    std::cerr << " will create cont now " << std::endl;
    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr) {
        std::cerr << "OpenGL context could not be created! SDL Error: "
                  << SDL_GetError() << std::endl;
        return false;
    }

    std::cerr << " will make  cont active now " << std::endl;
    if (SDL_GL_MakeCurrent(window, context) < 0) {
        std::cerr << "Could not make OpenGL context current: "
                  << SDL_GetError() << std::endl;
        SDL_Quit();
    }

    std::cerr << " will create swap " << std::endl;
    SDL_GL_SetSwapInterval(1);  // Use V-Sync

#elif defined(__USE_GLFW)
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);  // enable multisampling

    GLFWwindow* window = glfwCreateWindow(
        SCREEN_WIDTH, SCREEN_HEIGHT, "GLFW OpenGL program", NULL, NULL
    );
    if (window == nullptr) {
        printf("Failed to create window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);  // V-Sync
#endif

    // Loading Glew is necessary no matter which graphics library you
    // use
    glewExperimental = GL_TRUE;
    GLenum err       = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error initializing GLEW! "
                  << glewGetErrorString(err) << std::endl;
        return false;
    }

    int width, height;
#ifdef __USE_SDL
    SDL_GL_GetDrawableSize(window, &width, &height);
    ctx.sdlContext = context;
    ctx.sdlWindow  = window;
    ctx.screenWidth = width;
    ctx.screenHeight = height;

    // SDL_SetWindowGrab(window, SDL_TRUE); // Forces the window to capture all input
    // SDL_SetRelativeMouseMode(SDL_FALSE); // Optional: Controls how the mouse pointer behaves
#elif defined(__USE_GLFW)
    glfwGetFramebufferSize(window, &width, &height);
    ctx.glfwWindow = window;
#endif

    glViewport(0, 0, width, height);

    std::cerr << "Status: Using GLEW" << glewGetString(GLEW_VERSION)
              << '\n';
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf(
        "GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION)
    );
    printShaderVersions();

    return true;
}

// ****************************
//  2. OpenGL shader subsystem
// ****************************

GLuint vtx::createShaderProgram(
    const char* vertexShaderSource,
    const char* fragmentShaderSource
)
{
    GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

static GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);

        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n"
                  << "SHADER SOURCE:\n"
                  << source << "\nSHADER TYPE: "
                  << (type == GL_VERTEX_SHADER ? "Vertex Shader" : "")
                  << (type == GL_FRAGMENT_SHADER ? "Fragment Shader"
                                                 : "")
                  << "\nSHADER COMPILATION ERROR:\n"
                  << infoLog << std::endl;
        exit(1);
    }
    return shader;
}

// ****************************
//  3. OpenGL diagnostic utils
// ****************************

static void printShaderVersions()
{
    // Get OpenGL version
    const GLubyte* glVersion = glGetString(GL_VERSION);
    printf("OpenGL Version: %s\n", glVersion);

    // Get GLSL (shader language) version
    const GLubyte* glslVersion =
        glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("GLSL (Shader) Version: %s\n", glslVersion);

    // You can also get OpenGL vendor and renderer information
    const GLubyte* vendor   = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    printf("Vendor: %s\n", vendor);
    printf("Renderer: %s\n", renderer);
}

static void checkOpenGLError(const char* optionalTag = "")
{
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << optionalTag << "OpenGL error: " << err << std::endl;
        exit(1);
    }
}

// ************************
//  4. Main loop subsystem
// ************************

static void performOneCycle()
{
    ctx.shouldContinue = true;
    vtx::loop(&ctx);
}

void vtx::exitVortex()
{
    ctx.shouldContinue = false;
#ifdef __USE_SDL
    SDL_GL_DeleteContext(ctx.sdlContext);
    SDL_DestroyWindow(ctx.sdlWindow);
    SDL_Quit();
#elif defined(__USE_GLFW)
    glfwDestroyWindow(ctx.glfwWindow);
    glfwTerminate();
#endif
}

void vtx::openVortex()
{
    ctx.shouldContinue = true;

    if (!initVideo()) {
        std::cerr << "Failed to initialize!" << std::endl;
        exitVortex();
        exit(1);
    }

    init(&ctx);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(performOneCycle, 0, 1);
#else
    while (ctx.shouldContinue) performOneCycle();
#endif

    return;

    // Should be no code beyond this point!
    // singleLoopCycle() performs the cleanup
    // when it detects that it should quit.
    // The reason for this is that in Emscripten build
    // this coude would be reached before
    // singleLoopCycle() is even called,
    // and the native code would reach here only
    // after the main loop.
    //
    // The End.
}
