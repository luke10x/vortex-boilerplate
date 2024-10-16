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

// Data structures to hold your VBO data
std::vector<MyVertex> vertices;
std::vector<unsigned int> indices;

void loadModel(const char* path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,  // path of the file
        aiProcess_Triangulate | 
        aiProcess_FlipUVs |
        // aiProcess_GenNormals |
        aiProcess_CalcTangentSpace
    );

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        std::cerr << "Error loading model: "
                  << importer.GetErrorString() << std::endl;
        return;
    }

    aiMesh* mesh =
        scene->mMeshes[0];  // Assuming the model has at least one mesh

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
        aiColor4D baseColor(1.0f, 1.0f, 1.0f, 1.0f); // Default color is white
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex]; // Get the material assigned to the mesh
        if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &baseColor)) {
            std::cerr << "Material Base Color: "
                      << baseColor.r << ", "
                      << baseColor.g << ", "
                      << baseColor.b << "\n";
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
        std::cerr << "Vertex details\n"
                  << "  Position: (" << vertex.position.x << ", "
                  << vertex.position.y << ", " << vertex.position.z
                  << ")\n"
                  << "  Color: (" << vertex.color.r << ", "
                  << vertex.color.g << ", " << vertex.color.b << ")\n"
                  << "  Normal: (" << vertex.normal.x << ", "
                  << vertex.normal.y << ", " << vertex.normal.z
                  << ")\n";

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

typedef struct {
    GLuint pyramidVAO;
    GLuint defaultShader;
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
    // Create VAO
    glGenVertexArrays(1, &(usr.pyramidVAO));
    glBindVertexArray(usr.pyramidVAO);

    // GLB file contains normals, but Blender not
    loadModel("./assets/red-monkey.glb");

    // Create VBO with vertices
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size() * sizeof(MyVertex), // all vertices in bytes
        vertices.data(), 
        GL_STATIC_DRAW
    );

    // Create EBO with indexes
    GLuint EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, // This is used for EBO
        indices.size() * sizeof(unsigned int),
        indices.data(),
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

    usr.defaultShader = vtx::createShaderProgram(
        HELLO_VERTEX_SHADER, HELLO_FRAGMENT_SHADER
    );

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // Figure projection matrix
    float fov = glm::radians(45.0f);  // Field of view in radians
    float nearPlane = 0.1f;    // Distance to the near clipping plane
    float farPlane  = 100.0f;  // Distance to the far clipping plane
    // Actually, this needs to be recalculated in the loop,
    // as window could be resized at any time by user
    float aspectRatio = ctx->screenWidth / ctx->screenHeight;     

    // But, here is the initials perspective matrix
    glm::mat4 projectionMatrix =
        glm::perspective(fov, aspectRatio, nearPlane, farPlane);

    // Load it to shader
    glUseProgram(usr.defaultShader);

    glUniformMatrix4fv(
        glGetUniformLocation(
            usr.defaultShader, "u_projection"
        ),                                // uniform location
        1,                                // number of matrices
        GL_FALSE,                         // transpose
        glm::value_ptr(projectionMatrix)  // value
    );
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
    glUseProgram(usr.defaultShader);

    glUniformMatrix4fv(
        glGetUniformLocation(
            usr.defaultShader, "u_worldToView"
        ),                            // uniform location
        1,                            // number of matrices
        GL_FALSE,                     // transpose
        glm::value_ptr(cameraMatrix)  // value
    );

    // Spin modelToVorld
    static Uint32 lastTime = 0;
    Uint32 currentTime = SDL_GetTicks();
    float deltaTime = (currentTime - lastTime) / 1000.0f;
    lastTime = currentTime;
    float rotationSpeed = glm::radians(45.0f);
    float angle = rotationSpeed * deltaTime;
    modelToWorld = glm::rotate(modelToWorld, angle, glm::vec3(0.0f, 1.0f, 0.0f));

    glUniformMatrix4fv(
        glGetUniformLocation(
            usr.defaultShader, "u_modelToWorld"
        ),                            // uniform location
        1,                            // number of matrices
        GL_FALSE,                     // transpose
        glm::value_ptr(modelToWorld)  // value
    );
    ;

    glBindVertexArray(usr.pyramidVAO);

    glDrawElements(
        GL_TRIANGLES,     // Mode
        indices.size(),      // Index count
        GL_UNSIGNED_INT,  // Data type of indices array
        (void*) (0 * sizeof(unsigned int))  // Indices pointer
    );

    glBindVertexArray(0);

    checkOpenGLError();

    SDL_GL_SwapWindow(ctx->sdlWindow);
}

int main(int argc, char* argv[]) { vtx::openVortex(); }
