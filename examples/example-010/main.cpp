#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

#include "../../src/vtx/ctx.h"
#include "../../src/vtx/gizmo.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"
struct Hud {
    static const char* HUD_VERTEX_SHADER;
    static const char* HUD_FRAGMENT_SHADER;

    GLuint hudVAO;
    GLuint hudShaderId;
    GLuint hudTextureId;

    void initHud() {
        // Define vertices in NDC space (-1.0 to 1.0 range)
        // clang-format off
        static constexpr GLfloat hudVertices[] = {
            // Positions     // TexCoords
            -1.0f, -1.0f,    0.0f, 0.0f, // Bottom-left
            1.0f, -1.0f,     1.0f, 0.0f, // Bottom-right
            1.0f,  1.0f,     1.0f, 1.0f, // Top-right

            -1.0f, -1.0f,    0.0f, 0.0f, // Bottom-left
            1.0f,  1.0f,     1.0f, 1.0f, // Top-right
            -1.0f,  1.0f,    0.0f, 1.0f  // Top-left
        };
        // clang-format on

        glGenVertexArrays(1, &this->hudVAO);
        glBindVertexArray(this->hudVAO);

        // Create a vertex buffer for gizmo
        GLuint hudVBO;
        glGenBuffers(1, &hudVBO);
        glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
        glBufferData(
            GL_ARRAY_BUFFER, sizeof(hudVertices), hudVertices,
            GL_STATIC_DRAW
        );
        // Enable vertex attribute and bind it to shader location
        // clang-format off
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*) 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*) (2 * sizeof(GLfloat)));
        // clang-format on
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);                      // VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);          // VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // EBO

        this->hudShaderId = vtx::createShaderProgram(
            HUD_VERTEX_SHADER, HUD_FRAGMENT_SHADER
        );
    }

    void resizeHud(float screenX, float screenY, float width, float height) {
        glm::vec2 hudPosition(screenX, screenY); // Position in screen coordinates
        glm::vec2 hudSize(width, height);        // Size in screen coordinates

        // Create model matrix for HUD element positioning and scaling
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(hudPosition, 0.0f)); // Position on screen
        model = glm::scale(model, glm::vec3(hudSize, 1.0f));         // Scale to desired size
        
        glUseProgram(this->hudShaderId);
        glUniformMatrix4fv(
            glGetUniformLocation(this->hudShaderId, "u_projection"),
            1,
            GL_FALSE,
            glm::value_ptr(model)
        );
    }

    void drawHud() {
        // Define the HUD element's screen position and size
        glm::vec2 hudPosition(50.0f, 50.0f); // Position in pixels from bottom-left
        glm::vec2 hudSize(100.0f, 100.0f);   // Size in pixels

        // Create the model matrix for this HUD element
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(hudPosition, 0.0f)); // Move to desired screen position
        model = glm::scale(model, glm::vec3(hudSize, 1.0f));         // Scale to desired size

        // Send the model and projection matrices to the shader
        glUseProgram(this->hudShaderId);
        glUniformMatrix4fv(
            glGetUniformLocation(this->hudShaderId, "u_model"),
            1,
            GL_FALSE,
            glm::value_ptr(model)
        );

        // Bind VAO and draw the HUD element
        glBindVertexArray(this->hudVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6); // Draw quad for the HUD element
        glBindVertexArray(0);

    }

    void updateHudTexture(GLuint textureId) {
        glUseProgram(this->hudShaderId);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureId);
        glUniform1i(glGetUniformLocation(this->hudShaderId, "u_hudTexture"), 0); // Texture unit 0
    }

};

GLuint createTexture(const char* texturePath) {
    GLuint hudTexture;
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(1); // Enable vertical flip
    unsigned char *data = stbi_load(texturePath, &width, &height, &nrChannels, 0);
    stbi_set_flip_vertically_on_load(0); // Reset to default after loading

std::cerr << "stbi loaded " << texturePath << " " << width << "x" << height <<  std::endl;
    glGenTextures(1, &hudTexture);
    glBindTexture(GL_TEXTURE_2D, hudTexture);

    // Set texture parameters for wrapping and filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    if (data) {
        
std::cerr << "dataexists " << std::endl;
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data); // Free image data after loading to GPU
    } else {
        std::cerr << "Failed to load texture" << std::endl;
    }

    return hudTexture;
std::cerr << "nr channels " << nrChannels << std::endl;
}

const char* Hud::HUD_VERTEX_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
	precision mediump float;
    layout (location = 0) in vec2 a_position;
    layout (location = 1) in vec2 a_texCoords;

    uniform mat4 u_projection;
    uniform mat4 u_model;

    out vec2 v_texCoords;

    void main()
    {
        v_texCoords = a_texCoords;
        gl_Position = //u_projection * u_model * 
        vec4(a_position, 0.0, 1.0);
    }
    )";


const char* Hud::HUD_FRAGMENT_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
    in vec2 v_texCoords;

    uniform sampler2D u_hudTexture;

    out vec4 fragColor;

    void main()
    {
        fragColor = texture(u_hudTexture, v_texCoords);
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
        float u, v;
    } texCoords;
    struct {
        float x, y, z;
    } normal;
} MyVertex;

struct MyMesh {
    static const char* MODEL_VERTEX_SHADER;
    static const char* MODEL_FRAGMENT_SHADER;

    // Data structures to hold your VBO data
    std::vector<MyVertex> vertices;
    std::vector<unsigned int> indices;
    GLuint modelVAO;
    GLuint defaultShader;
    uint diffuseTextureId;
    glm::mat4 initialTransform;

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
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*) offsetof(MyVertex, texCoords));
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*) offsetof(MyVertex, normal));
        // clang-format on

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);

        // Unbind all to prevent accidentally modifying them
        glBindVertexArray(0);                      // VAO
        glBindBuffer(GL_ARRAY_BUFFER, 0);          // VBO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // EBO

        defaultShader = vtx::createShaderProgram(
            MODEL_VERTEX_SHADER, MODEL_FRAGMENT_SHADER
        );
    }
    uint
    createTextureFromAssimp(const aiScene* scene, aiMaterial* material)
    {
        aiString texturePath;

        if (material->GetTexture(
                aiTextureType_DIFFUSE, 0, &texturePath
            ) == AI_SUCCESS) {
            const aiTexture* texture =
                scene->GetEmbeddedTexture(texturePath.C_Str());

            if (texture) {
                unsigned char* data = nullptr;

                int width, height, channels;
                unsigned char* imageData = stbi_load_from_memory(
                    reinterpret_cast<unsigned char*>(texture->pcData),
                    texture->mWidth,  // mWidth holds the length of the
                                      // compressed data buffer
                    &width, &height, &channels, 0
                );

                uint glChan;
                if (channels == 3) {
                    glChan = GL_RGB;
                } else if (channels == 4) {
                    glChan = GL_RGBA;
                } else {
                    return 0;
                }

                if (imageData) {
                    // Successfully loaded image; you can now use
                    // imageData, width, height, and channels For
                    // example, generate an OpenGL texture:
                    GLuint textureID;
                    glGenTextures(1, &textureID);
                    glBindTexture(GL_TEXTURE_2D, textureID);
                    glTexImage2D(
                        GL_TEXTURE_2D, 0, glChan, width, height, 0,
                        glChan, GL_UNSIGNED_BYTE, imageData
                    );

                    glTexParameteri(
                        GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST
                    );
                    glTexParameteri(
                        GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST
                    );
                    glTexParameterf(
                        GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0
                    );
                    glTexParameteri(
                        GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT
                    );
                    glTexParameteri(
                        GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT
                    );

                    glGenerateMipmap(GL_TEXTURE_2D);

                    // Unbind to make suree something else does not
                    // interfere
                    glBindTexture(GL_TEXTURE_2D, 0);

                    // Free stb_image data after generating texture
                    stbi_image_free(imageData);
                    return textureID;
                } else {
                }
            }
        }
        return 0;
    }

    void updateDiffuseTexture(uint diffuseTexture)
    {
        glUseProgram(this->defaultShader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);

        glUniform1i(
            glGetUniformLocation(
                this->defaultShader, "u_diffuseTexture"
            ),
            0
        );
        checkOpenGLError();
    }

    // Recursive function to find the node containing the mesh index
    const aiNode*
    findNodeForMeshIndex(const aiNode* node, unsigned int meshIndex)
    {
        // Check if this node references the mesh index
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            if (node->mMeshes[i] == meshIndex) {
                return node;  // Found the node
            }
        }

        // Recursively search child nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            const aiNode* foundNode =
                findNodeForMeshIndex(node->mChildren[i], meshIndex);
            if (foundNode) {
                return foundNode;
            }
        }

        return nullptr;  // Node not found
    }
    inline glm::mat4 assimpToGlmMatrix(aiMatrix4x4 mat)
    {
        glm::mat4 m;
        for (int y = 0; y < 4; y++) {
            for (int x = 0; x < 4; x++) {
                m[x][y] = mat[y][x];
            }
        }
        return m;
    }
    void loadMesh(const char* path, const char* meshName)
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
            return;
        }

        aiMesh* mesh           = nullptr;
        unsigned int meshIndex = 0;
        // Iterate through all the meshes in the scene
        for (meshIndex = 0; meshIndex < scene->mNumMeshes;
             meshIndex++) {
            mesh = scene->mMeshes[meshIndex];

            // Print the name of the mesh
            if (mesh->mName.length > 0) {
                std::cout << "Mesh Name: " << mesh->mName.C_Str()
                          << std::endl;
                const char* currentMeshName = mesh->mName.C_Str();
                if (strcmp(currentMeshName, meshName) == 0) {
                    std::cout << "Found matching mesh: " << meshName
                              << std::endl;
                    break;  // Exit the loop
                }
            }
        }
        if (mesh == nullptr) {
            std::cerr << "Error loading mesh: " << meshName
                      << std::endl;  // Fallback name
        }

        glm::mat4 globalTransform(1.0f);
        const aiNode* node =
            this->findNodeForMeshIndex(scene->mRootNode, meshIndex);

        // Traverse up the node hierarchy
        while (node != nullptr) {
            std::cerr << "Mesh" << meshIndex
                      << " node =" << node->mName.C_Str()
                      << std::endl;  // Fallback name

            glm::mat4 localTransform =
                assimpToGlmMatrix(node->mTransformation);
            globalTransform = localTransform * globalTransform;
            node            = node->mParent;
        }
        this->initialTransform = globalTransform;

        aiMaterial* material =
            scene->mMaterials[mesh->mMaterialIndex];  // Get the
                                                      // material
                                                      // assigned to
                                                      // the mesh

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

            if (AI_SUCCESS ==
                aiGetMaterialColor(
                    material, AI_MATKEY_COLOR_DIFFUSE, &baseColor
                )) {
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

            vertex.texCoords.u = mesh->mTextureCoords[0][i].x;
            vertex.texCoords.v = mesh->mTextureCoords[0][i].y;

            vertices.push_back(vertex);
        }

        // Extract indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                indices.push_back(face.mIndices[j]);
            }
        }

        this->diffuseTextureId =
            this->createTextureFromAssimp(scene, material);
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
const char* MyMesh::MODEL_VERTEX_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
	precision mediump float;

    layout (location = 0) in vec3 a_pos;
    layout (location = 1) in vec3 a_color;
    layout (location = 2) in vec2 a_texCoords;
    layout (location = 3) in vec3 a_normal;

    out vec3 v_crntPos;
    out vec3 v_color;
    out vec2 v_texCoords;
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
        vec3 hardcodedLightPos = vec3(3.0f, 3.0f, 1.0f);
        
        v_normal      = a_normal;
        v_color       = a_color;
        v_texCoords   = a_texCoords;
        v_crntPos     = vec3(u_modelToWorld * vec4(a_pos, 1.0f));
        v_lightPos    = vec3(u_worldToModel *  vec4(hardcodedLightPos, 1.0f));

        gl_Position   = u_projection * u_worldToView * vec4(v_crntPos, 1.0f);
	}
	)";

const char* MyMesh::MODEL_FRAGMENT_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
	precision mediump float;

    in vec3 v_crntPos;
    in vec3 v_color;
    in vec2 v_texCoords;
    in vec3 v_normal;
    in vec3 v_lightPos;

	uniform sampler2D u_diffuseTexture;

    out vec4 FragColor;

    void main() {

        vec3 lightPos = v_lightPos;

        // ambient lighting
        float ambient = 0.4f;

        // diffuse lighting
        vec3 normal = normalize(v_normal);
        vec3 lightDirection = normalize(lightPos - v_crntPos);
        float diffuse = 0.6f * max(dot(normal, lightDirection), 0.0f);

        // vec4 surfaceColor = vec4(v_color, 1.0f);
        vec4 surfaceColor = texture(u_diffuseTexture, v_texCoords).rgba; ;
        vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

        FragColor = surfaceColor * vec4(vec3(lightColor * (ambient + diffuse)), 1.0f);
    }
    )";

// == ImGui object ==
struct MyImGui {
    // Initialize ImGui
    void init(vtx::VertexContext* ctx) const
    {
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

    void processEvent(const SDL_Event* event)
    {
        ImGui_ImplSDL2_ProcessEvent(&(*event));
    }

    void newFrame()
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    void renderFrame()
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void showMatrixEditor(glm::mat4* matrix, const char* title) const
    {
        if (ImGui::Begin(
                title, nullptr, ImGuiWindowFlags_AlwaysAutoResize
            )) {
            if (ImGui::BeginTable(
                    "Matrix", 4, ImGuiTableFlags_Borders
                )) {
                for (int row = 0; row < 4; row++) {
                    ImGui::TableNextRow();
                    for (int col = 0; col < 4; col++) {
                        ImGui::TableNextColumn();

                        // Use InputFloat to edit each element in
                        // the matrix
                        char label[32];
                        snprintf(
                            label, sizeof(label), "##%d%d", row, col
                        );

                        // Access the matrix element
                        ImGui::SetNextItemWidth(90
                        );  // Set the input width
                        ImGui::InputFloat(
                            label, &(*matrix)[col][row], 0.01f, 1.0f,
                            "%.2f"
                        );
                        // Note: glm::mat4 is column-major, so we
                        // use modelToWorld[col][row]
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
    MyMesh plant;
    MyMesh cubeTop;
    MyMesh cubeBody;
    MyImGui imgui;
    Hud hud;
} UserContext;

UserContext usr;

// Create the camera matrix
glm::vec3 cameraPosition(-0.25f, 3.0f, 0.25f);
glm::vec3 targetPosition(1.5f, 1.6f, -1.5f);
glm::vec3 upDirection(0.0f, 1.0f, 0.0f);
glm::mat4 cameraMatrix = glm::lookAt(
    cameraPosition,
    targetPosition,  // The center of the pyramid
    upDirection
);

glm::mat4 modelToWorld = glm::mat4(1.0f);  // Identity matrix

void vtx::init(vtx::VertexContext* ctx)
{
    usr.cubeTop.loadMesh(
        "./assets/texture-test.glb", "big-cube-mesh-0"
    );
    usr.cubeTop.init();

    usr.cubeBody.loadMesh(
        "./assets/texture-test.glb", "big-cube-mesh-1"
    );
    usr.cubeBody.init();

    usr.imgui.init(ctx);

    GLuint heartTextureId = createTexture("./assets/heart.png");
    usr.hud.hudTextureId = heartTextureId;
    usr.hud.initHud();
    usr.hud.resizeHud(0, 0, ctx->screenWidth, ctx->screenHeight);
    usr.hud.updateHudTexture(heartTextureId);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // Figure projection matrix
    float fov       = glm::radians(45.0f);  // Field of view in radians
    float nearPlane = 0.1f;    // Distance to the near clipping plane
    float farPlane  = 100.0f;  // Distance to the far clipping plane
    // Actually, this needs to be recalculated in the loop,
    // as window could be resized at any time by user
    float aspectRatio =
        (float) ctx->screenWidth / (float) ctx->screenHeight;

    // But, here is the initials perspective matrix
    glm::mat4 projectionMatrix =
        glm::perspective(fov, aspectRatio, nearPlane, farPlane);

    usr.cubeTop.updateProjectionMatrix(projectionMatrix);
    usr.cubeBody.updateProjectionMatrix(projectionMatrix);
}

void vtx::loop(vtx::VertexContext* ctx)
{
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            vtx::exitVortex();
            return;
        }
        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
            // screenWidth = event.window.data1;
            // screenHeight = event.window.data2;
            // orthoProjection = glm::ortho(0.0f, screenWidth, screenHeight, 0.0f, -1.0f, 1.0f);
            // glViewport(0, 0, screenWidth, screenHeight); // Update viewport
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

    static Uint32 lastTime = 0;
    Uint32 currentTime     = SDL_GetTicks();
    float deltaTime        = (currentTime - lastTime) / 1000.0f;
    lastTime               = currentTime;

    float rotationSpeed =
        glm::radians(45.0f);  // Rotation speed in radians per second
    float angle = rotationSpeed * SDL_GetTicks() /
                  1000.0f;  // Total angle based on elapsed time

    modelToWorld = glm::mat4(1.0f);  // Start with an identity matrix
    modelToWorld =
        glm::rotate(modelToWorld, angle, glm::vec3(0.0f, 1.0f, 0.0f));

    usr.cubeTop.updateTransformationMatrix(
        usr.cubeTop.initialTransform * modelToWorld
    );
    usr.cubeTop.updateViewMatrix(cameraMatrix);
    usr.cubeTop.updateDiffuseTexture(usr.cubeTop.diffuseTextureId);
    usr.cubeTop.draw();

    usr.cubeBody.updateTransformationMatrix(
        usr.cubeBody.initialTransform * modelToWorld
    );
    usr.cubeBody.updateViewMatrix(cameraMatrix);
    usr.cubeBody.updateDiffuseTexture(usr.cubeBody.diffuseTextureId
    );  // ok it is time to exract shader
    usr.cubeBody.draw();

    usr.hud.updateHudTexture(usr.hud.hudTextureId);
    usr.hud.drawHud();

    usr.imgui.newFrame();
    usr.imgui.showMatrixEditor(
        &modelToWorld, "Model-to-World for mesh"
    );
    usr.imgui.showMatrixEditor(&cameraMatrix, "Camera matrix");
    usr.imgui.renderFrame();

    checkOpenGLError();
    SDL_GL_SwapWindow(ctx->sdlWindow);
}

int main(int argc, char* argv[]) { vtx::openVortex(); }
