#include "../vtx/ctx.h"

// ***********
//  Constants
// ***********

typedef struct {
    GLuint VAO1;
    GLuint shader1;
} UserContext;

UserContext usr;

const char* HELLO_VERTEX_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
	layout(location = 0) in vec3 a_pos;

	void main() {
		gl_Position = vec4(a_pos, 1.0);
	}
	)";

const char* HELLO_FRAGMENT_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
	precision mediump float;

    out vec4 FragColor;

    void main() {
            FragColor = vec4(1.0, 0.5, 0.0, 1.0); // Orange color
    }
    )";

void vtx::init(vtx::VertexContext* ctx)
{
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  // Bottom left
        0.5f,  -0.5f, 0.0f,  // Bottom right
        0.0f,  0.5f,  0.0f   // Top
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW
    );
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*) 0
    );
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // TODO only perhaps vbo setting should be here,
    // and the rest should be moved to the user land.
    usr.VAO1 = VAO;
    // usr.VBO1 = VBO;

    usr.shader1 = vtx::createShaderProgram(
        HELLO_VERTEX_SHADER, HELLO_FRAGMENT_SHADER
    );
}
void vtx::loop(vtx::VertexContext* ctx)
{
#ifdef __USE_SDL
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            exitVortex();
            return;
        }
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                exitVortex();
                return;
            }
        }
    }
#elif defined(__USE_GLFW)
    if (glfwWindowShouldClose(ctx.glfwWindow)) {
        exitVortex();
        return;
    }
    if (glfwGetKey(ctx.glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(
            ctx.glfwWindow, true
        );  // Mark window to close
    }
#endif
    glClearColor(0.1f, 0.4f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(usr.shader1);
    glBindVertexArray(usr.VAO1);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    checkOpenGLError();

#ifdef __USE_GLFW
    glfwPollEvents();
#endif

#ifdef __USE_SDL
    SDL_GL_SwapWindow(ctx->sdlWindow);
#elif defined(__USE_GLFW)
    glfwSwapBuffers(ctx->glfwWindow);
#endif
}

int main(int argc, char* argv[]) { vtx::openVortex(); }