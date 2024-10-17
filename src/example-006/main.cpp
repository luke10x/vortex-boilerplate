#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "../vtx/ctx.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

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

struct MyModel {
    static const char* MODEL_VERTEX_SHADER;
    static const char* MODEL_FRAGMENT_SHADER;

    // Data structures to hold your VBO data
    std::vector<MyVertex> vertices;
    std::vector<unsigned int> indices;
    GLuint modelVAO;
    GLuint defaultShader;

    void init()
    {
        // Create VAO
        glGenVertexArrays(1, &modelVAO);
        glBindVertexArray(modelVAO);

        // Create VBO with vertices
        GLuint VBO;
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(
            GL_ARRAY_BUFFER,
            vertices.size() *
                sizeof(MyVertex),  // all vertices in bytes
            vertices.data(), GL_STATIC_DRAW
        );

        // Create EBO with indexes
        GLuint EBO;
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,  // This is used for EBO
            indices.size() * sizeof(unsigned int), indices.data(),
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

        // Unbind all to prevent accidentally modifying them
        glBindVertexArray(0);                      // VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);          // VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // EBO

        defaultShader = vtx::createShaderProgram(
            MODEL_VERTEX_SHADER, MODEL_FRAGMENT_SHADER
        );
    }

    void loadModel(const char* path)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,  // path of the file
            aiProcess_Triangulate | aiProcess_FlipUVs |
                // aiProcess_GenNormals |
                aiProcess_CalcTangentSpace
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
            !scene->mRootNode) {
            std::cerr << "Error loading model: "
                      << importer.GetErrorString() << std::endl;
            return;
        }

        aiMesh* mesh = scene->mMeshes[0];  // Assuming the model has at
                                           // least one mesh

        // Iterate through all the meshes in the scene
        for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[i];

            // Print the name of the mesh
            if (mesh->mName.length > 0) {
                std::cout << "Mesh Name: " << mesh->mName.C_Str()
                          << std::endl;
            } else {
                std::cout << "Mesh Name: Untitled Mesh " << i
                          << std::endl;  // Fallback name
            }
        }

        // Extract vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            MyVertex vertex;

            // Position
            vertex.position.x = mesh->mVertices[i].x;
            vertex.position.y = mesh->mVertices[i].y;
            vertex.position.z = mesh->mVertices[i].z;

            // Try to get the base color from the material
            aiColor4D baseColor(
                1.0f, 1.0f, 1.0f, 1.0f
            );  // Default color is white
            aiMaterial* material =
                scene->mMaterials[mesh->mMaterialIndex];  // Get the
                                                          // material
                                                          // assigned to
                                                          // the mesh
            if (AI_SUCCESS ==
                aiGetMaterialColor(
                    material, AI_MATKEY_COLOR_DIFFUSE, &baseColor
                )) {
                // std::cerr << "Material Base Color: "
                //           << baseColor.r << ", "
                //           << baseColor.g << ", "
                //           << baseColor.b << "\n";
            }

            // Colour (if available, otherwise default white)
            if (mesh->HasVertexColors(0)) {
                vertex.color.r = mesh->mColors[0][i].r;
                vertex.color.g = mesh->mColors[0][i].g;
                vertex.color.b = mesh->mColors[0][i].b;
            } else {
                vertex.color.r = baseColor.r;
                vertex.color.g = baseColor.g;
                vertex.color.b = baseColor.b;
            }

            // Normal (if available)
            if (mesh->HasNormals()) {
                vertex.normal.x = mesh->mNormals[i].x;
                vertex.normal.y = mesh->mNormals[i].y;
                vertex.normal.z = mesh->mNormals[i].z;
            }
            // std::cerr << "Vertex details\n"
            //           << "  Position: (" << vertex.position.x << ", "
            //           << vertex.position.y << ", " <<
            //           vertex.position.z
            //           << ")\n"
            //           << "  Color: (" << vertex.color.r << ", "
            //           << vertex.color.g << ", " << vertex.color.b <<
            //           ")\n"
            //           << "  Normal: (" << vertex.normal.x << ", "
            //           << vertex.normal.y << ", " << vertex.normal.z
            //           << ")\n";

            vertices.push_back(vertex);
        }

        // Extract indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }
        std::cerr << "vertices: " << vertices.size() << std::endl;
        std::cerr << "indices: " << indices.size() << std::endl;
    }

    void updateProjectionMatrix(const glm::mat4 projectionMatrix) const
    {
        glUseProgram(this->defaultShader);

        glUniformMatrix4fv(
            glGetUniformLocation(
                this->defaultShader, "u_projection"
            ),                                // uniform location
            1,                                // number of matrices
            GL_FALSE,                         // transpose
            glm::value_ptr(projectionMatrix)  // value
        );
    }

    void updateTransformationMatrix(const glm::mat4 transformationMatrix
    ) const
    {
        glUseProgram(this->defaultShader);

        glUniformMatrix4fv(
            glGetUniformLocation(
                this->defaultShader, "u_modelToWorld"
            ),                                    // uniform location
            1,                                    // number of matrices
            GL_FALSE,                             // transpose
            glm::value_ptr(transformationMatrix)  // value
        );
    }

    void updateViewMatrix(const glm::mat4 viewMatrix) const
    {
        glUseProgram(this->defaultShader);

        glUniformMatrix4fv(
            glGetUniformLocation(
                this->defaultShader, "u_worldToView"
            ),                          // uniform location
            1,                          // number of matrices
            GL_FALSE,                   // transpose
            glm::value_ptr(viewMatrix)  // value
        );
    }
    void draw() const
    {
        // Draw using default shader
        glUseProgram(this->defaultShader);
        glBindVertexArray(this->modelVAO);
        glDrawElements(
            GL_TRIANGLES,     // Mode
            indices.size(),   // Index count
            GL_UNSIGNED_INT,  // Data type of indices array
            (void*) (0 * sizeof(unsigned int))  // Indices pointer
        );
        glBindVertexArray(0);
    }
};
// == MyModel impl ==
const char* MyModel::MODEL_VERTEX_SHADER =
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
    out vec3 v_lightPos;

    uniform mat4 u_worldToView;
    uniform mat4 u_modelToWorld;
    uniform mat4 u_projection;

	void main() {
        // Invert the model-to-world matrix to transform
        // the light position from world space to object space.
        // This ensures that the light remains static relative to the object,
        // regardless of the object's transformation by modelToWorld.
        mat4 u_worldToModel = inverse(u_modelToWorld);
        vec3 hardcodedLightPos = vec3(1.0f, 1.0f, 3.0f);
        
        v_normal      = a_normal;
        v_color       = a_color;
        v_crntPos     = vec3(u_modelToWorld * vec4(a_pos, 1.0f));
        v_lightPos    = vec3(u_worldToModel *  vec4(hardcodedLightPos, 1.0f));

        gl_Position   = u_projection * u_worldToView * vec4(v_crntPos, 1.0f);
	}
	)";

const char* MyModel::MODEL_FRAGMENT_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
	precision mediump float;

    in vec3 v_crntPos;
    in vec3 v_color;
    in vec3 v_normal;
    in vec3 v_lightPos;

    out vec4 FragColor;

    void main() {

        vec3 lightPos = v_lightPos;

        // ambient lighting
        float ambient = 0.4f;

        // diffuse lighting
        vec3 normal = normalize(v_normal);
        vec3 lightDirection = normalize(lightPos - v_crntPos);
        float diffuse = 0.6f * max(dot(normal, lightDirection), 0.0f);

        vec4 surfaceColor = vec4(v_color, 1.0f);
        vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

        FragColor = surfaceColor * vec4(vec3(lightColor * (ambient + diffuse)), 1.0f);
    }
    )";

// == ImGui object ==
struct MyImGui {
    // Initialize ImGui
    void init(vtx::VertexContext* ctx) const {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void) io;
        ImGui::StyleColorsDark();
        // ImGui_ImplGlfw_InitForOpenGL(ctx->window, true);
        ImGui_ImplSDL2_InitForOpenGL(ctx->sdlWindow, ctx->sdlContext);
        // ImplGlfw_InitForOpenGL(ctx->window, true);

#ifdef __EMSCRIPTEN__
        ImGui_ImplOpenGL3_Init("#version 300 es");
#else
        ImGui_ImplOpenGL3_Init("#version 330");
#endif
    }

    void processEvent(const SDL_Event* event) {
        ImGui_ImplSDL2_ProcessEvent(&(*event));
    }

    void newFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    void renderFrame() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void showMatrixEditor(glm::mat4* matrix, const char *title) const 
    {
        if (ImGui::Begin(
                title, nullptr, ImGuiWindowFlags_AlwaysAutoResize
            )) {
            if (ImGui::BeginTable("Matrix", 4, ImGuiTableFlags_Borders)) {
                for (int row = 0; row < 4; row++) {
                    ImGui::TableNextRow();
                    for (int col = 0; col < 4; col++) {
                        ImGui::TableNextColumn();

                        // Use InputFloat to edit each element in the matrix
                        char label[32];
                        snprintf(label, sizeof(label), "##%d%d", row, col);

                        // Access the matrix element
                        ImGui::SetNextItemWidth(90);  // Set the input width
                        ImGui::InputFloat(
                            label, &(*matrix)[col][row], 0.01f, 1.0f,
                            "%.2f"
                        );
                        // Note: glm::mat4 is column-major, so we use
                        // modelToWorld[col][row]
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();
    }
};
// == Main program ==

typedef struct {
    Gizmo gizmo;
    MyModel monkey;
    MyImGui imgui;
} UserContext;

UserContext usr;

// Create the camera matrix
glm::vec3 cameraPosition(5.0f, 8.0f, 0.0f);
glm::vec3 targetPosition(0.0f, 0.0f, 0.0f);
glm::vec3 upDirection(0.0f, 1.0f, 0.0f);
glm::mat4 cameraMatrix = glm::lookAt(
    cameraPosition,
    targetPosition,  // The center of the pyramid
    upDirection
);

glm::mat4 modelToWorld = glm::mat4(1.0f);  // Identity matrix

void vtx::init(vtx::VertexContext* ctx)
{
    // GLB file contains normals, but Blender not
    usr.monkey.loadModel("./assets/red-monkey.glb");
    usr.monkey.init();
    usr.gizmo.init();

    usr.imgui.init(ctx);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // Figure projection matrix
    float fov       = glm::radians(45.0f);  // Field of view in radians
    float nearPlane = 0.1f;    // Distance to the near clipping plane
    float farPlane  = 100.0f;  // Distance to the far clipping plane
    // Actually, this needs to be recalculated in the loop,
    // as window could be resized at any time by user
    float aspectRatio = ctx->screenWidth / ctx->screenHeight;

    // But, here is the initials perspective matrix
    glm::mat4 projectionMatrix =
        glm::perspective(fov, aspectRatio, nearPlane, farPlane);

    usr.monkey.updateProjectionMatrix(projectionMatrix);
    usr.gizmo.updateProjectionMatrix(projectionMatrix);
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

        usr.imgui.processEvent(&event);
    }

    glClearColor(0.1f, 0.4f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    usr.monkey.updateTransformationMatrix(modelToWorld);
    usr.monkey.updateViewMatrix(cameraMatrix);
    usr.monkey.draw();

    usr.gizmo.updateTransformationMatrix(modelToWorld);
    usr.gizmo.updateViewMatrix(cameraMatrix);
    usr.gizmo.draw();

    usr.imgui.newFrame();
    usr.imgui.showMatrixEditor(&modelToWorld, "Model-to-World for monkey");
    usr.imgui.showMatrixEditor(&cameraMatrix, "Camera matrix");
    usr.imgui.renderFrame();

    checkOpenGLError();
    SDL_GL_SwapWindow(ctx->sdlWindow);
}

int main(int argc, char* argv[]) { vtx::openVortex(); }
