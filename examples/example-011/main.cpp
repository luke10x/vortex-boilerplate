#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_TRUETYPE_IMPLEMENTATION
#include <fstream>
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
#include "stb_truetype.h"

#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <limits>

struct PathSegment {
    std::vector<glm::vec3> points;   // Sampled points along this Catmull-Rom segment
    std::vector<float> arcLengths;   // Cumulative arc lengths for each sampled point
    float totalLength = 0.0f;
};

struct Path {
    std::vector<PathSegment> segments;
    float totalLength;

    Path() : totalLength(0.0f) {}

    void addSegment(const PathSegment& segment) {
        segments.push_back(segment);
        totalLength += segment.totalLength;
    }

    glm::vec3 getPointAtDistance(float distance) const {
        float remainingDist = distance;
        
        for (const auto& segment : segments) {
            if (remainingDist <= segment.totalLength) {
                // Interpolate within this segment
                for (size_t i = 1; i < segment.arcLengths.size(); ++i) {
                    if (remainingDist <= segment.arcLengths[i]) {
                        float t = (remainingDist - segment.arcLengths[i - 1]) / 
                                  (segment.arcLengths[i] - segment.arcLengths[i - 1]);
                        return glm::mix(segment.points[i - 1], segment.points[i], t);
                    }
                }
            }
            remainingDist -= segment.totalLength;
        }
        
        return segments.back().points.back(); // Return the end point if distance exceeds path length
    }

    glm::vec3 findClosestPoint(const glm::vec3& point) const {
        float minDistance = std::numeric_limits<float>::max();
        glm::vec3 closestPoint;

        for (const auto& segment : segments) {
            for (const auto& p : segment.points) {
                float dist = glm::distance(point, p);
                if (dist < minDistance) {
                    minDistance = dist;
                    closestPoint = p;
                }
            }
        }
        return closestPoint;
    }
};
std::vector<glm::vec3> generateSpiralControlPoints(float radius, float heightPerTurn, int numTurns, int pointsPerTurn) {
    std::vector<glm::vec3> controlPoints;
    float angleStep = glm::two_pi<float>() / pointsPerTurn;

    for (int i = 0; i <= numTurns * pointsPerTurn; ++i) {
        float angle = i * angleStep;
        float x = radius * cos(angle);
        float z = radius * sin(angle);
        float y = i * (heightPerTurn / pointsPerTurn);
        controlPoints.emplace_back(x, y, z);
    }

    return controlPoints;
}

// Function to initialize Path from control points
Path initializeSpiralPath(float radius, float heightPerTurn, int numTurns, int pointsPerTurn) {
    Path path;
    std::vector<glm::vec3> controlPoints = generateSpiralControlPoints(radius, heightPerTurn, numTurns, pointsPerTurn);

    // Construct PathSegments from control points using Catmull-Rom spline interpolation
    for (size_t i = 1; i < controlPoints.size() - 2; ++i) {
        PathSegment segment;
        glm::vec3 p0 = controlPoints[i - 1];
        glm::vec3 p1 = controlPoints[i];
        glm::vec3 p2 = controlPoints[i + 1];
        glm::vec3 p3 = controlPoints[i + 2];

        // Sample points along this Catmull-Rom segment
        for (float t = 0.0f; t <= 1.0f; t += 0.05f) {
            glm::vec3 point = 0.5f * ((2.0f * p1) +
                                      (-p0 + p2) * t +
                                      (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t * t +
                                      (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t * t * t);
            
            if (!segment.points.empty()) {
                float arcLength = glm::distance(segment.points.back(), point);
                segment.arcLengths.push_back(segment.totalLength + arcLength);
                segment.totalLength += arcLength;
            } else {
                segment.arcLengths.push_back(0.0f);
            }
            segment.points.push_back(point);
        }

        // path.totalLength += segment.totalLength;
        path.addSegment(segment);
    }

    return path;
}

struct Line {
    static const char* LINE_VERTEX_SHADER;
    static const char* LINE_FRAGMENT_SHADER;

    GLuint lineVAO;
    GLuint lineShaderId;

    void initLine() {
        float lineVertices[] = {
            // Line 1
            -10.5f, 0.0f, 0.0f,  // Start point
            0.0f, 0.0f, 0.0f,  // End point
            // Line 2
            0.0f, -10.5f, 0.0f,
            0.0f, -0.0f, 0.0f,
            // Add more lines as needed
        };
        GLuint VAO, VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);

        // Enable the vertex attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0); 
        glBindVertexArray(0);
        this->lineVAO = VAO;
        this->lineShaderId = vtx::createShaderProgram(
            LINE_VERTEX_SHADER, LINE_FRAGMENT_SHADER
        );
        // glLineWidth(5.0f);
    }

    void renderTheLines(const glm::mat4 view, const glm::mat4 projection, const glm::mat4 model) {
        glUseProgram(this->lineShaderId); // Use your shader program

        // Set your transformation uniforms
        glUniformMatrix4fv(glGetUniformLocation(this->lineShaderId, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(this->lineShaderId, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(this->lineShaderId, "uModel"), 1, GL_FALSE, glm::value_ptr(model));

        // Set line colour
        glUniform4f(glGetUniformLocation(this->lineShaderId, "uColor"), 1.0f, 0.0f, 0.0f, 1.0f); // Red

        glBindVertexArray(this->lineVAO);

        glDrawArrays(GL_LINES, 0, 4); // Number of vertices in your lines
        glBindVertexArray(0);
    }
};

const char* Line::LINE_VERTEX_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
    precision mediump float;

    layout(location = 0) in vec3 aPos; // Vertex position
    uniform mat4 uProjection;
    uniform mat4 uView;
    uniform mat4 uModel;

    void main() {
        gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    }
    )";

const char* Line::LINE_FRAGMENT_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
    precision mediump float;

    out vec4 FragColor;
    uniform vec4 uColor;

    void main() {
        FragColor = uColor; // Colour for the line
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
    Line redLine;
    Path spiralPath;
    Gizmo gizmo;
} UserContext;

UserContext usr;

// Create the camera matrix
glm::vec3 cameraPosition(30.0f, 15.0f, 30.0f);
glm::vec3 targetPosition(0.0f, 1.6f, 0.0f);
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

    usr.redLine.initLine();

    float radius = 3.5f;
    float heightPerTurn = 4.0f;
    int numTurns = 3;
    int pointsPerTurn = 4;

    usr.spiralPath = initializeSpiralPath(radius, heightPerTurn, numTurns, pointsPerTurn);

    // Output the total length of the spiral path
    std::cout << "Total Length of Spiral Path: " << usr.spiralPath.totalLength << std::endl;

    // Optionally print some sample points along the path
    for (const auto& segment : usr.spiralPath.segments) {
        for (const auto& point : segment.points) {
            // std::cout << "Point: (" << point.x << ", " << point.y << ", " << point.z << ")" << std::endl;
        }
    }

    usr.gizmo.init();
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
        if (event.type == SDL_WINDOWEVENT &&
            event.window.event == SDL_WINDOWEVENT_RESIZED) {
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

    usr.gizmo.updateViewMatrix(cameraMatrix);
    
    // THIS IS ALSO IMPORTANT FOR THIS EXAMPLE
    // TODO make it run too, but it kind of dublicates the visual impact of the next 
    // code block
    // for (const auto& seg : usr.spiralPath.segments) {
    //     for (const auto point : seg.points) {

    //         glm::mat4 gizmoMat = glm::mat4(1.0f); // Start with the identity matrix
    //         gizmoMat = glm::translate(gizmoMat, point);

    //         usr.gizmo.updateTransformationMatrix(gizmoMat);
    //         usr.gizmo.draw();
    //     }
    // }

    for (float cursor = 0.0f; cursor < usr.spiralPath.totalLength; cursor += 2.3) {
        const glm::vec3 point = usr.spiralPath.getPointAtDistance(cursor);

            glm::mat4 gizmoMat = glm::mat4(1.0f); // Start with the identity matrix
            gizmoMat = glm::translate(gizmoMat, point);
            usr.gizmo.updateTransformationMatrix(gizmoMat);
            usr.gizmo.draw();
    }

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
    usr.redLine.renderTheLines(
        cameraMatrix,
        projectionMatrix,
        glm::mat4(1.0f)
    );
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
