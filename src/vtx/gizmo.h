#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "./ctx.h"
#include <glm/glm.hpp>

struct Gizmo {
    static const char* GIZMO_VERTEX_SHADER;
    static const char* GIZMO_FRAGMENT_SHADER;

    // Define gizmo vertices (3-axis lines: X, Y, Z)
    // clang-format off
    static constexpr GLfloat gizmoVertices[] = {
        // X-axis
        0.0f,   0.0f,  0.0f, 0.0f,
        3.0f,   0.0f,  0.0f, 0.0f,
        -0.2f, -0.2f, -0.2f, 0.0f,

        0.0f,  0.14f, -0.14f, 0.0f,
        0.0f, -0.14f,  0.14f, 0.0f,
        3.0f,   0.0f,  0.0f,  0.0f,  

        // Y-axis
        0.0f,   0.0f,  0.0f, 1.0f,
        0.0f,   3.0f,  0.0f, 1.0f,
        -0.2f, -0.2f, -0.2f, 1.0f,
    
        0.14f,  0.0f, -0.14f, 1.0f,
        -0.14f, 0.0f,  0.14f, 1.0f,
        0.0f,   3.0f,  0.0f,  1.0f,  

        // Z-axis
        0.0f,   0.0f,  0.0f, 2.0f,
        0.0f,   0.0f,  3.0f, 2.0f,
        -0.2f, -0.2f, -0.2f, 2.0f,

        0.14f, -0.14f, 0.0f, 2.0f,
        -0.14f, 0.14f, 0.0f, 2.0f,
        0.0f,   0.0f,  3.0f, 2.0f,
    };
    // clang-format on

    GLuint gizmoVAO;
    GLuint gizmoShader;

    Gizmo() {}  // End of constructor Gizmo

    void init()
    {
        glGenVertexArrays(1, &gizmoVAO);
        glBindVertexArray(this->gizmoVAO);

        // Create a vertex buffer for gizmo
        GLuint gizmoVBO;
        glGenBuffers(1, &gizmoVBO);
        glBindBuffer(GL_ARRAY_BUFFER, gizmoVBO);
        glBufferData(
            GL_ARRAY_BUFFER, sizeof(gizmoVertices), gizmoVertices,
            GL_STATIC_DRAW
        );
        // Enable vertex attribute and bind it to shader location
        glVertexAttribPointer(
            0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*) 0
        );
        glVertexAttribPointer(
            1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat),
            (void*) (3 * sizeof(GLfloat))
        );
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);                      // VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);          // VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // EBO

        this->gizmoShader = vtx::createShaderProgram(
            GIZMO_VERTEX_SHADER, GIZMO_FRAGMENT_SHADER
        );
    }

    void updateProjectionMatrix(const glm::mat4 projectionMatrix) const
    {
        glUseProgram(this->gizmoShader);

        glUniformMatrix4fv(
            glGetUniformLocation(
                this->gizmoShader, "u_projection"
            ),                                // uniform location
            1,                                // number of matrices
            GL_FALSE,                         // transpose
            glm::value_ptr(projectionMatrix)  // value
        );
    }

    void updateTransformationMatrix(const glm::mat4 transformationMatrix
    ) const
    {
        glUseProgram(this->gizmoShader);

        glUniformMatrix4fv(
            glGetUniformLocation(
                this->gizmoShader, "u_modelToWorld"
            ),                                    // uniform location
            1,                                    // number of matrices
            GL_FALSE,                             // transpose
            glm::value_ptr(transformationMatrix)  // value
        );
    }

    void updateViewMatrix(const glm::mat4 viewMatrix) const
    {
        glUseProgram(this->gizmoShader);

        glUniformMatrix4fv(
            glGetUniformLocation(
                this->gizmoShader, "u_worldToView"
            ),                          // uniform location
            1,                          // number of matrices
            GL_FALSE,                   // transpose
            glm::value_ptr(viewMatrix)  // value
        );
    }

    void draw() const
    {
        glUseProgram(this->gizmoShader);
        glBindVertexArray(this->gizmoVAO);

        glDrawArrays(
            GL_TRIANGLES,  // Only this is supported in GLES
            0,             // Start from
            sizeof(gizmoVertices) /
                (4 * sizeof(GLfloat))  // total size / one vertex size
        );

        glBindVertexArray(0);
    }

};  // end of Gizmo

const char* Gizmo::GIZMO_VERTEX_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
    precision mediump float;

    layout (location = 0) in vec3 a_position;
    layout (location = 1) in float a_direction;

    out float v_direction;

    uniform mat4 u_modelToWorld;
    uniform mat4 u_worldToView;  // World to View matrix
    uniform mat4 u_projection;   // Projection matrix

    void main() {
        v_direction = a_direction;

        // Transform the vertex position from world space to view space
        // and then to clip space
        vec3 crntPos = vec3(u_modelToWorld * vec4(a_position, 1.0f));
        gl_Position  = u_projection * u_worldToView * vec4(crntPos, 1.0f);
    }
    )";

const char* Gizmo::GIZMO_FRAGMENT_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
    precision mediump float;

    in float v_direction;

    out vec4 FragColor;

    void main() {
        // Colour the axis based on the input index
        if (v_direction == 0.0f) {
            FragColor = vec4(1.0, 0.0, 0.0, 1.0);  // Red for X-axis
        } else if (v_direction == 1.0f) {
            FragColor = vec4(0.0, 1.0, 0.0, 1.0);  // Green for Y-axis
        } else {
            FragColor = vec4(0.0, 0.0, 1.0, 1.0);  // Blue for Z-axis
        }
    }
    )";
