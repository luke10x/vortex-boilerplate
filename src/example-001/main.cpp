#include "../vtx/ctx.h"

// ***********
//  Constants 
// ***********

typedef struct {
    GLuint VBO1;
    GLuint shader1;
} UserContext;

const char* HELLO_VERTEX_SHADER = 
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
	R"(
	layout(location = 0) in vec3 aPos;

	void main() {
		gl_Position = vec4(aPos, 1.0);
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

int main(int argc, char* argv[]) {
    vtx::openVortex();
}

void vtx::init(vtx::VertexContext* ctx) {

    ctx->triangleProgramId = vtx::createShaderProgram(
        HELLO_VERTEX_SHADER,
        HELLO_FRAGMENT_SHADER
    );
}
void vtx::loop(vtx::VertexContext* ctx) {
#ifdef __USE_SDL
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            exitVortex();
            ctx->shouldContinue = false;
            return;
        }
		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_ESCAPE) {
				exitVortex();
				ctx->shouldContinue = false;
				return;
			}
		}
    }
#elif defined(__USE_GLFW)
    if (glfwWindowShouldClose(ctx.glfwWindow)) {
		exitVortex();
        ctx.shouldContinue = false;
		return;
    }
    if (glfwGetKey(ctx.glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(ctx.glfwWindow, true);  // Mark window to close
    }
#endif
	glClearColor(0.1f, 0.4f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(ctx->triangleProgramId);
	glBindVertexArray(ctx->VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);

	checkOpenGLError();

#ifdef __USE_GLFW 
	glfwPollEvents();
#endif
#ifdef __USE_SDL
    SDL_GL_SwapWindow(ctx->sdlWindow);
#elif defined(__USE_GLFW)
	glfwSwapBuffers(ctx.glfwWindow);
#endif
}