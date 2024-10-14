#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../vtx/ctx.h"

const char* HELLO_VERTEX_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
	precision mediump float;

    layout (location = 0) in vec3 a_pos;
    layout (location = 1) in vec3 a_color;
    layout (location = 2) in vec3 a_normal;

    out vec3 v_crntPos;
    out vec3 v_color;
    out vec3 v_normal;

    uniform mat4 u_worldToView;
    uniform mat4 u_modelToWorld;

	void main() {
      v_normal      = a_normal;
      v_color       = a_color;
      v_crntPos     = vec3(u_modelToWorld * vec4(a_pos, 1.0f));
      gl_Position   = u_worldToView * vec4(v_crntPos, 1.0f);
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

    in vec3 v_crntPos;
    in vec3 v_color;
    in vec3 v_ormal;

    out vec4 FragColor;

    void main() {
        FragColor = vec4(v_color, 1.0);
    }
    )";

typedef struct {
    struct {
        float x, y, z;
    } position;
    struct {
        float r, g, b;
    } color;
    struct {
        float x, y, z;
    } normal;
} MyVertex;

// clang-format off
MyVertex vertices[] =
{
    { .position = {-0.5f, -0.4f,  0.5f}, .color = { 0.99f, 0.99f, 0.99f }, .normal = {  0.0f, -1.0f, 0.0f }}, // Bottom side
    { .position = {-0.5f, -0.4f, -0.5f}, .color = { 0.99f, 0.99f, 0.99f }, .normal = {  0.0f, -1.0f, 0.0f }}, // Bottom side
    { .position = {0.5f,  -0.4f, -0.5f}, .color = { 0.99f, 0.99f, 0.99f }, .normal = {  0.0f, -1.0f, 0.0f }}, // Bottom side
    { .position = {0.5f,  -0.4f,  0.5f}, .color = { 0.99f, 0.99f, 0.99f }, .normal = {  0.0f, -1.0f, 0.0f }}, // Bottom side
    { .position = {-0.5f, -0.4f,  0.5f}, .color = { 0.99f, 0.00f, 0.00f }, .normal = { -0.8f, 0.5f,  0.0f }}, // Left Side
    { .position = {-0.5f, -0.4f, -0.5f}, .color = { 0.99f, 0.00f, 0.00f }, .normal = { -0.8f, 0.5f,  0.0f }}, // Left Side
    { .position = {0.0f,  +0.4f,  0.0f}, .color = { 0.99f, 0.00f, 0.00f }, .normal = { -0.8f, 0.5f,  0.0f }}, // Left Side
    { .position = {-0.5f, -0.4f, -0.5f}, .color = { 0.00f, 0.00f, 0.99f }, .normal = {  0.0f, 0.5f, -0.8f }}, // Non-facing side
    { .position = {0.5f,  -0.4f, -0.5f}, .color = { 0.00f, 0.00f, 0.99f }, .normal = {  0.0f, 0.5f, -0.8f }}, // Non-facing side
    { .position = {0.0f,  +0.4f,  0.0f}, .color = { 0.00f, 0.00f, 0.99f }, .normal = {  0.0f, 0.5f, -0.8f }}, // Non-facing side
    { .position = {0.5f,  -0.4f, -0.5f}, .color = { 0.99f, 0.99f, 0.00f }, .normal = {  0.8f, 0.5f,  0.0f }}, // Right side
    { .position = {0.5f,  -0.4f,  0.5f}, .color = { 0.99f, 0.99f, 0.00f }, .normal = {  0.8f, 0.5f,  0.0f }}, // Right side
    { .position = {0.0f,  +0.4f,  0.0f}, .color = { 0.99f, 0.99f, 0.00f }, .normal = {  0.8f, 0.5f,  0.0f }}, // Right side
    { .position = {0.5f,  -0.4f,  0.5f}, .color = { 0.00f, 0.99f, 0.00f }, .normal = {  0.0f, 0.5f,  0.8f }}, // Facing side
    { .position = {-0.5f, -0.4f,  0.5f}, .color = { 0.00f, 0.99f, 0.00f }, .normal = {  0.0f, 0.5f,  0.8f }}, // Facing side
    { .position = {0.0f,  +0.4f,  0.0f}, .color = { 0.00f, 0.99f, 0.00f }, .normal = {  0.0f, 0.5f,  0.8f }}  // Facing side
};
GLuint indices[] =
{
    0, 1, 2,    // Bottom side
    0, 2, 3,    // Bottom side
    4, 6, 5,    // Left side
    7, 9, 8,    // Non-facing side
    10, 12, 11, // Right side
    13, 15, 14  // Facing side
};
// clang-format on

typedef struct {
    GLuint pyramidVAO;
    GLuint shader1;
} UserContext;

UserContext usr;

// Create the camera matrix
glm::vec3 cameraPosition(0.0f, 0.0f, 0.4f);
glm::vec3 targetPosition(0.0f, 0.8f, 0.0f);
glm::vec3 upDirection(0.0f, 1.0f, 1.0f);
glm::mat4 cameraMatrix = glm::lookAt(
    cameraPosition,
    targetPosition,  // The center of the pyramid
    upDirection
);

glm::mat4 modelToWorld = glm::mat4(1.0f);  // Identity matrix

GLsizei vertex_count = sizeof(vertices) / sizeof(MyVertex);
GLsizei index_count  = sizeof(indices) / sizeof(GLuint);

void vtx::init(vtx::VertexContext* ctx)
{
    // Create VAO
    glGenVertexArrays(1, &(usr.pyramidVAO));
    glBindVertexArray(usr.pyramidVAO);

    // Create VBO with vertices
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW
    );

    // Create EBO with indexes
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
        GL_STATIC_DRAW
    );

    // Links VBO attributes such as coordinates and colors to VAO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // clang-format off
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*) offsetof(MyVertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*) offsetof(MyVertex, color));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*) offsetof(MyVertex, normal));
    // clang-format on

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Unbind all to prevent accidentally modifying them
    glBindVertexArray(0);                      // VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);          // VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // EBO

    usr.shader1 = vtx::createShaderProgram(
        HELLO_VERTEX_SHADER, HELLO_FRAGMENT_SHADER
    );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
}

void vtx::loop(vtx::VertexContext* ctx)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            vtx::exitVortex();
            return;
        }
        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                vtx::exitVortex();
                return;
            }
        }
    }

    glClearColor(0.1f, 0.4f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(usr.shader1);

    glUniformMatrix4fv(
        glGetUniformLocation(
            usr.shader1, "u_worldToView"
        ),                            // uniform location
        1,                            // number of matrices
        GL_FALSE,                     // transpose
        glm::value_ptr(cameraMatrix)  // value
    );

    glUniformMatrix4fv(
        glGetUniformLocation(
            usr.shader1, "u_modelToWorld"
        ),                            // uniform location
        1,                            // number of matrices
        GL_FALSE,                     // transpose
        glm::value_ptr(modelToWorld)  // value
    );
    ;

    glBindVertexArray(usr.pyramidVAO);

    glDrawElements(
        GL_TRIANGLES,     // Mode
        index_count,      // Index count
        GL_UNSIGNED_INT,  // Data type of indices array
        (void*) (0 * sizeof(unsigned int))  // Indices pointer
    );

    glBindVertexArray(0);

    checkOpenGLError();

    SDL_GL_SwapWindow(ctx->sdlWindow);
}

int main(int argc, char* argv[]) { vtx::openVortex(); }
