#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../src/vtx/ctx.h"
#include "animation-mixer.h"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl2.h"

struct Gizmo;
struct MyVertex;
struct MyModel;
struct MyImGui;
struct UserContext;
void vtx::init(vtx::VertexContext* ctx);
void vtx::loop(vtx::VertexContext* ctx);
int main(int argc, char* argv[]);

struct Gizmo {
    static const char* GIZMO_VERTEX_SHADER;
    static const char* GIZMO_FRAGMENT_SHADER;

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

    void init();
    void updateProjectionMatrix(const glm::mat4 projectionMatrix) const;
    void updateTransformationMatrix(const glm::mat4 transformationMatrix
    ) const;
    void updateViewMatrix(const glm::mat4 viewMatrix) const;
    void draw() const;
};

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
void Gizmo::init()
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

void Gizmo::updateProjectionMatrix(const glm::mat4 projectionMatrix
) const
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

void Gizmo::updateTransformationMatrix(
    const glm::mat4 transformationMatrix
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

void Gizmo::updateViewMatrix(const glm::mat4 viewMatrix) const
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

void Gizmo::draw() const
{
    glUseProgram(this->gizmoShader);
    glBindVertexArray(this->gizmoVAO);

    glDrawArrays(
        GL_TRIANGLES,            // Only this is supported in GLES
        0,                       // Start from
        sizeof(gizmoVertices) /  // Total size /
            (4 * sizeof(GLfloat)
            )  // DIV size of element = one vertex size
    );

    glBindVertexArray(0);
}

struct MyVertex {
    struct {
        float x, y, z;
    } position;
    struct {
        float r, g, b;
    } color;
    struct {
        float x, y, z;
    } normal;
    // Up to 4 bone indices (use -1 for no bone)
    int bones[4] = {-1, -1, -1, -1};
    // Up to 4 bone weights
    float weights[4] = {0.0f};
};

struct MyModel {
    static const char* MODEL_VERTEX_SHADER;
    static const char* MODEL_FRAGMENT_SHADER;

    // Data structures to hold your VBO data
    std::vector<MyVertex> vertices;
    std::vector<unsigned int> indices;
    GLuint modelVAO;
    GLuint defaultShader;
    const aiScene* scene;
    glm::mat4 boneTransforms[100];

    AnimationMixer ap;

    // importer owns scene, therefore it needs to kept alive,
    // as the scene will be used later in ImGui panels.
    // Normally, in production build, I doubt if keeping
    // importer makes much sense.
    Assimp::Importer importer;

    void init();
    void loadModel(const char* path);
    void updateProjectionMatrix(const glm::mat4 projectionMatrix) const;
    void updateTransformationMatrix(const glm::mat4 transformationMatrix
    ) const;
    void updateViewMatrix(const glm::mat4 viewMatrix) const;
    void updateSelectedJointIndex(GLuint selectedBoneIndex) const;
    // void updateBoneTransform(const glm::mat4& boneTransform) const;
    void updateBoneTransform(const glm::mat4* boneTransform, int count)
        const;
    void draw() const;
};

const char* MyModel::MODEL_VERTEX_SHADER =
#ifdef __EMSCRIPTEN__
    "#version 300 es"
#else
    "#version 330 core"
#endif
    R"(
	precision mediump float;

    layout (location = 0) in vec3  a_pos;
    layout (location = 1) in vec3  a_color;
    layout (location = 2) in vec3  a_normal;
    layout (location = 3) in ivec4 a_joints;
    layout (location = 4) in vec4  a_weights;

    out vec3 v_crntPos;
    out vec4 v_color;
    out vec3 v_normal;
    out vec3 v_lightPos;

    const int MAX_BONES = 200;

    // Converts world-space coordinates to view-space coordinates (camera space).
    // Applied to all objects in the scene to align with the camera's position and orientation.
    uniform mat4 u_worldToView;

    // Converts model-space coordinates to world-space coordinates.
    // This matrix transforms an object's local vertices into the global scene.
    // Applied to each model to position, scale, and rotate it within the world.
    uniform mat4 u_modelToWorld;

    // Projection matrix: transforms view-space coordinates to clip-space coordinates.
    // Determines how objects are projected onto the screen (perspective or orthographic).
    uniform mat4 u_projection;

    // Array of bone transformation matrices for skeletal animation.
    // Each matrix in u_bones adjusts the position and rotation 
    // of a specific bone in model space.
    // MAX_BONES sets the maximum number of bones per model.
    uniform mat4 u_bones[MAX_BONES];

    uniform uint u_selectedJointIndex;

    // Higher weight redder it is, lower weight - bluer
    vec4 calculateBoneHotnessColor(float weight) {
        return vec4(weight, 1.0 - weight, 0.0, 1.0f);
    }

    void main() {

        // Invert the model-to-world matrix to transform
        // the light position from world space to object space.
        // This ensures that the light remains static relative to the object,
        // regardless of the object's transformation by modelToWorld.
        mat4 u_worldToModel = inverse(u_modelToWorld);
        vec3 hardcodedLightPos = vec3(-5.0f, 5.0f, 3.0f);
        
        // v_normal      = a_normal;
        // v_color       = a_color;
        v_crntPos     = vec3(u_modelToWorld * vec4(a_pos, 1.0f));
        v_lightPos    = vec3(u_worldToModel *  vec4(hardcodedLightPos, 1.0f));

        // Default color blue
        v_color = vec4(0.1f, 0.1f, 1.0f, 1.0f);

        for (int i = 0; i < 4; i++) {
            if (a_joints[i] == int(u_selectedJointIndex)) {
                v_color = calculateBoneHotnessColor(a_weights[i]);
            }
        }

        // vec4 posL     = vec4(v_crntPos, 1.0f);

        mat4 boneTransform = u_bones[a_joints[0]] * a_weights[0];
        boneTransform     += u_bones[a_joints[1]] * a_weights[1];
        boneTransform     += u_bones[a_joints[2]] * a_weights[2];
        boneTransform     += u_bones[a_joints[3]] * a_weights[3];

        vec4 animatedPos  = boneTransform * vec4(v_crntPos, 1.0f);
        gl_Position       = u_projection * u_worldToView * u_modelToWorld * animatedPos;

        // TODO animate normals
        v_normal = mat3(transpose(inverse(u_modelToWorld * boneTransform))) * a_normal;
        v_normal = normalize(v_normal);
        // vec4 NormalL = BoneTransform * vec4(Normal, 0.0);
        // Normal0      = (gWorld * NormalL).xyz;
        // WorldPos0    = (gWorld * PosL).xyz;  

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
    in vec4 v_color;
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

        // vec4 surfaceColor = vec4(v_color, 1.0f);
        vec4 surfaceColor = v_color;
        vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);

        FragColor = surfaceColor * vec4(vec3(lightColor * (ambient + diffuse)), 1.0f);
    }
    )";

void MyModel::init()
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
        vertices.size() * sizeof(MyVertex),  // all vertices in bytes
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
    // These are the basic
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*) offsetof(MyVertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*) offsetof(MyVertex, color));
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*) offsetof(MyVertex, normal));

    // Required for animations
    glVertexAttribIPointer(3, 4, GL_INT,            sizeof(MyVertex), (void*) offsetof(MyVertex, bones));
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(MyVertex), (void*) offsetof(MyVertex, weights));
    // clang-format on

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    // Unbind all to prevent accidentally modifying them
    glBindVertexArray(0);                      // VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);          // VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // EBO

    defaultShader = vtx::createShaderProgram(
        MODEL_VERTEX_SHADER, MODEL_FRAGMENT_SHADER
    );
}

void MyModel::loadModel(const char* path)
{
    this->scene = importer.ReadFile(
        path,  // path of the file
        aiProcess_Triangulate | aiProcess_FlipUVs |
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

    ap.initBones(scene, mesh);

    // Print the name of the mesh
    if (mesh->mName.length > 0) {
        std::cout << "Mesh Name: " << mesh->mName.C_Str() << std::endl;
    } else {
        // Fallback name
        std::cout << "Mesh Name: Untitled Mesh " << std::endl;
    }

    // Initialize a vertex bone weight map to hold weights for each
    // vertex
    std::vector<std::vector<std::pair<int, float>>> vertexWeights(
        mesh->mNumVertices
    );

    // Process bones
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones;
         ++boneIndex) {
        aiBone* bone = mesh->mBones[boneIndex];

        // Iterate through all the vertices affected by this bone
        for (unsigned int weightIndex = 0;
             weightIndex < bone->mNumWeights; ++weightIndex) {
            aiVertexWeight weight = bone->mWeights[weightIndex];
            int vertexId          = weight.mVertexId;
            float boneWeight      = weight.mWeight;

            // Store the bone index and weight in the vertex weight
            // map
            vertexWeights[vertexId].emplace_back(boneIndex, boneWeight);
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
            // no-op
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

        // Process bone weights and indices for this vertex
        auto& weights = vertexWeights[i];

        // Sort by weight and keep the top 4 weights
        std::sort(
            weights.begin(), weights.end(),
            [](const auto& a, const auto& b) {
                return a.second >
                       b.second;  // Sort by weight descending
            }
        );

        // Store up to 4 bones and their weights
        for (size_t j = 0; j < weights.size() && j < 4; ++j) {
            vertex.bones[j]   = weights[j].first;   // Bone index
            vertex.weights[j] = weights[j].second;  // Bone weight
        }

        // Normalize weights to sum up to 1
        float weightSum = vertex.weights[0] + vertex.weights[1] +
                          vertex.weights[2] + vertex.weights[3];
        if (weightSum > 0.0f) {
            for (int j = 0; j < 4; ++j) {
                vertex.weights[j] /= weightSum;
            }
        }

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

void MyModel::updateProjectionMatrix(const glm::mat4 projectionMatrix
) const
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

void MyModel::updateTransformationMatrix(
    const glm::mat4 transformationMatrix
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

void MyModel::updateViewMatrix(const glm::mat4 viewMatrix) const
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

void MyModel::updateSelectedJointIndex(GLuint selectedBoneIndex) const
{
    glUseProgram(this->defaultShader);
    glUniform1ui(
        glGetUniformLocation(
            this->defaultShader, "u_selectedJointIndex"
        ),                 // Loc
        selectedBoneIndex  // value
    );
}

/*
 * Populates one element to boneTransformation matrices array
 */
void MyModel::updateBoneTransform(
    const glm::mat4* boneTransform,
    int count
) const
{
    if (count >= 200) {
        std::cerr << "Too many bones in file" << std::endl;
        exit(1);
    }
    // TODO if index exceedds max bones then exit
    glUseProgram(this->defaultShader);
    glUniformMatrix4fv(
        glGetUniformLocation(this->defaultShader, "u_bones"),  // Loc
        count,                                                 // count
        GL_TRUE,  // transpose
        glm::value_ptr(boneTransform[0]
        )  // put only one value in specific index
    );
}

void MyModel::draw() const
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

struct MyImGui {
    std::unordered_map<std::string, bool> openBoneNodes;

    GLuint selectedBoneIndex;

    void init(vtx::VertexContext* ctx) const;
    void processEvent(const SDL_Event* event) const;
    void newFrame() const;
    void renderFrame() const;

    void showMatrixEditor(glm::mat4* matrix, const char* title) const;
    int FindBoneIndex(const aiMesh* mesh, const std::string& boneName);
    void ShowOffsetMatrix(const aiMatrix4x4& offsetMatrix);
    void _showBoneHierarchy(
        const aiNode* node,
        const aiMesh* mesh,
        std::unordered_map<std::string, bool>& openNodes,
        int level
    );
    void renderBoneHierarchy(const aiScene* scene);
};

void MyImGui::init(vtx::VertexContext* ctx) const
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(ctx->sdlWindow, ctx->sdlContext);

#ifdef __EMSCRIPTEN__
    ImGui_ImplOpenGL3_Init("#version 300 es");
#else
    ImGui_ImplOpenGL3_Init("#version 330");
#endif
}

void MyImGui::processEvent(const SDL_Event* event) const
{
    ImGui_ImplSDL2_ProcessEvent(&(*event));
}

void MyImGui::newFrame() const
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void MyImGui::renderFrame() const
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void MyImGui::showMatrixEditor(glm::mat4* matrix, const char* title)
    const
{
    if (ImGui::Begin(
            title, nullptr, ImGuiWindowFlags_AlwaysAutoResize
        )) {
        if (ImGui::BeginTable("Matrix", 4, ImGuiTableFlags_Borders)) {
            for (int row = 0; row < 4; row++) {
                ImGui::TableNextRow();
                for (int col = 0; col < 4; col++) {
                    ImGui::TableNextColumn();

                    // Use InputFloat to edit each element in the
                    // matrix
                    char label[32];
                    snprintf(label, sizeof(label), "##%d%d", row, col);

                    // Access the matrix element
                    ImGui::SetNextItemWidth(90);  // Set the input width
                    ImGui::InputFloat(
                        label, &(*matrix)[col][row], 0.01f, 1.0f, "%.2f"
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

// Helper function to find the bone index in the mesh
int MyImGui::FindBoneIndex(
    const aiMesh* mesh,
    const std::string& boneName
)
{
    for (unsigned int i = 0; i < mesh->mNumBones; ++i) {
        if (boneName == mesh->mBones[i]->mName.C_Str()) {
            return i;
        }
    }
    return -1;  // Return -1 if the bone was not found in the mesh
}

// Function to display the offset matrix when a bone is clicked
void MyImGui::ShowOffsetMatrix(const aiMatrix4x4& offsetMatrix)
{
    ImGui::Text("Offset Matrix:");
    for (int row = 0; row < 4; ++row) {
        ImGui::Text(
            "    [%f, %f, %f, %f]", offsetMatrix[row][0],
            offsetMatrix[row][1], offsetMatrix[row][2],
            offsetMatrix[row][3]
        );
    }
}

// Recursive function to show bone hierarchy with click-to-reveal
// details
void MyImGui::_showBoneHierarchy(
    const aiNode* node,
    const aiMesh* mesh,
    std::unordered_map<std::string, bool>& openNodes,
    int level = 0
)
{
    std::string nodeName = node->mName.C_Str();

    int boneIndex = FindBoneIndex(mesh, nodeName);
    bool isBone   = (boneIndex >= 0);

    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);

    std::string buttonLabel =
        "Select##" + std::to_string(boneIndex) + " " + nodeName;
    if (ImGui::Button(buttonLabel.c_str())) {
        std::cout << "Bone selected: " << node->mName.C_Str()
                  << std::endl;
        selectedBoneIndex = boneIndex;
    }

    ImGui::TableSetColumnIndex(1);

    ImGui::Indent(level * 2);
    bool isOpen = openNodes[nodeName];
    if (ImGui::Selectable(nodeName.c_str(), isOpen)) {
        openNodes[nodeName] = !isOpen;
    }
    ImGui::Unindent(level * 2);

    if (isOpen && isBone) {
        std::string littleWindowLabel = "Bone: " + nodeName;

        ImGui::Begin(littleWindowLabel.c_str());
        ImGui::Text("Bone Index: %d", boneIndex);
        ShowOffsetMatrix(mesh->mBones[boneIndex]->mOffsetMatrix);
        ImGui::End();
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        // Recur for each child
        _showBoneHierarchy(
            node->mChildren[i], mesh, openNodes, level + 1
        );
    }
}

// Main function to render the ImGui window with the bone hierarchy
void MyImGui::renderBoneHierarchy(const aiScene* scene)
{
    if (ImGui::Begin(
            "Bone Hierarchy", nullptr, ImGuiWindowFlags_AlwaysAutoResize
        )) {
        // NOTICE: Same assumption is done in MyModel::loadModel()
        aiMesh* mesh = scene->mMeshes[0];

        if (mesh->mNumBones > 0) {
            // Start from the root node of the scene
            if (ImGui::BeginTable(
                    "Bone Table", 2,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg
                )) {
                // Set up the column headers
                ImGui::TableSetupColumn("Bone Name");
                ImGui::TableSetupColumn("Select");

                ImGui::TableHeadersRow();  // Display header row

                _showBoneHierarchy(
                    scene->mRootNode, mesh, openBoneNodes
                );
            }
            ImGui::EndTable();  // End the table

        } else {
            ImGui::Text("No bones in the model.");
        }
    }
    ImGui::End();
}
// == Main program ==

struct UserContext {
    Gizmo gizmo;
    MyModel human;
    MyImGui imgui;
};

UserContext usr;

// Create the camera matrix
glm::vec3 cameraPosition(1.0f, 5.0f, 15.0f);
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
    usr.human.loadModel("./assets/human.glb");
    // usr.human.loadModel("../assets/animated-human/Blend/Animated\ Human.blend");
    usr.human.init();
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

    usr.human.updateProjectionMatrix(projectionMatrix);
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

    usr.human.updateTransformationMatrix(modelToWorld);
    usr.human.updateViewMatrix(cameraMatrix);
    usr.human.updateSelectedJointIndex(usr.imgui.selectedBoneIndex);
    glm::mat4 boneTransforms[100];

    float elapsedTime = 0.1f * (float) SDL_GetTicks() / 1000;

    std::vector<glm::mat4> T;
    usr.human.ap.hydrateBoneTransforms(T, elapsedTime, 0);
    usr.human.updateBoneTransform(T.data(), T.size());
    usr.human.draw();

    usr.gizmo.updateTransformationMatrix(modelToWorld);
    usr.gizmo.updateViewMatrix(cameraMatrix);
    usr.gizmo.draw();

    usr.imgui.newFrame();  // --------------------------

    usr.imgui.showMatrixEditor(
        &modelToWorld, "Model-to-World for human"
    );
    usr.imgui.showMatrixEditor(&cameraMatrix, "Camera matrix");
    usr.imgui.renderBoneHierarchy(usr.human.scene);

    usr.imgui.renderFrame();  // ------------------------

    checkOpenGLError();
    SDL_GL_SwapWindow(ctx->sdlWindow);
}

int main(int argc, char* argv[]) { vtx::openVortex(); }
