#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "FastNoise/Utility/SmartNode.h"
#include "GLDebug.h"
#include "GLDebug.cpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include "stb_image.h"
#include "stb_image.cpp"
#include "ShaderReader.h"
#include "textureMap.cpp"
#include "noiseGeneration.cpp"


static int WIDTH = 800;
static int HEIGHT = 600;
static float deltaTime;
static float lastFrame;
static glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);



float cubeVertices[] = {
// Front face (-Z)
// position             // UV
0.0f, 0.0f, 0.0f,       0.0f, 0.0f,
1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
1.0f, 1.0f, 0.0f,       1.0f, 1.0f,

1.0f, 1.0f, 0.0f,       1.0f, 1.0f,
0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
0.0f, 0.0f, 0.0f,       0.0f, 0.0f,

// Back face (+Z)
0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
1.0f, 0.0f, 1.0f,       1.0f, 0.0f,
1.0f, 1.0f, 1.0f,       1.0f, 1.0f,

1.0f, 1.0f, 1.0f,       1.0f, 1.0f,
0.0f, 1.0f, 1.0f,       0.0f, 1.0f,
0.0f, 0.0f, 1.0f,       0.0f, 0.0f,

// Left face (-X)
0.0f, 1.0f, 1.0f,       1.0f, 1.0f,
0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
0.0f, 0.0f, 0.0f,       0.0f, 0.0f,

0.0f, 0.0f, 0.0f,       0.0f, 0.0f,
0.0f, 0.0f, 1.0f,       1.0f, 0.0f,
0.0f, 1.0f, 1.0f,       1.0f, 1.0f,

// Right face (+X)
1.0f, 1.0f, 1.0f,       1.0f, 1.0f,
1.0f, 1.0f, 0.0f,       0.0f, 1.0f,
1.0f, 0.0f, 0.0f,       0.0f, 0.0f,

1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
1.0f, 0.0f, 1.0f,       1.0f, 0.0f,
1.0f, 1.0f, 1.0f,       1.0f, 1.0f,

// Bottom face (-Y)
0.0f, 0.0f, 0.0f,       0.0f, 1.0f,
1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
1.0f, 0.0f, 1.0f,       1.0f, 0.0f,

1.0f, 0.0f, 1.0f,       1.0f, 0.0f,
0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
0.0f, 0.0f, 0.0f,       0.0f, 1.0f,

// Top face (+Y)
0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
1.0f, 1.0f, 0.0f,       1.0f, 1.0f,
1.0f, 1.0f, 1.0f,       1.0f, 0.0f,

1.0f, 1.0f, 1.0f,       1.0f, 0.0f,
0.0f, 1.0f, 1.0f,       0.0f, 0.0f,
0.0f, 1.0f, 0.0f,       0.0f, 1.0f
};

enum Block
{
    AIR = 0,
    GRASS,
    DIRT,
    STONE,
    SAND
};

const int CHUNK_X = 16;
const int CHUNK_Y = 64;
const int CHUNK_Z = 16;
const int BLOCK_COUNT = CHUNK_X * CHUNK_Y * CHUNK_Z;

//Chunk key in the form of a tuple of x and z coordinates
struct ChunkKey
{
    int x;
    int z;

    bool operator== (const ChunkKey& other) const
    {
        return x == other.x && z == other.z;
    }
};

//Hash function to not repeat keys as much
struct ChunkKeyHash
{
    std::size_t operator()(const ChunkKey& key) const
    {
        std::size_t h1 = std::hash<int>{}(key.x);
        std::size_t h2 = std::hash<int>{}(key.z);

        return h1 ^ (h2 << 1);
    }
};

struct Chunk
{
    int chunkX;
    int chunkZ;
    //Block blocks [CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    std::array<Block, BLOCK_COUNT> blocks;

    //Rendering fields
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint vertexCount = 0;

    //rendering flags
    bool needsMeshRebuild = false;
    bool generated = false; //for ram generation False -> in disk true -> ram
};

struct Vertex
{
    float x, y, z; //position
    //float nx, ny, nz; // normals
    float u, v; //textures
};

enum Face {
    Front = 0,  // -Z
    Back   = 1, // +Z
    Left   = 2, // -X
    Right  = 3, // +X
    Bottom = 4, // -Y
    Top    = 5  // +Y
};

struct Camera
{
    //Positions
    glm::vec3 cameraPos = glm::vec3(0.0f, 64.0f, 32.0f);
    glm::vec3 DirectionFront = glm::vec3(0.0f, -0.2f, -1.0f);
    glm::vec3 DirectionUp = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 DirectionRight = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 MovementFront = glm::vec3(0.0f, 0.0f, -1.0f);
    //Mouse directions
    float yaw_angle = -90.0f;
    float pitch_angle = 0.0f;
    float lastX = WIDTH/2.0f, lastY = HEIGHT/2.0f;
    float zoom = 45.0f;
    bool firstMouse = true;
};

// Global objects
static Camera camera {};
//NOTE(ullr): World (chuck container)
using World = std::unordered_map<ChunkKey, Chunk, ChunkKeyHash>;
static World worldChunks;


unsigned int loadTexture(char const* path);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
constexpr int blockIndex(int x, int y, int z);
Block& getBlock(Chunk& chunk, int x, int y, int z);
const Block& getBlock(const Chunk& chunk, int x, int y, int z);
void appendFace( std::vector<Vertex> &meshVertices, const float cubeVertices[], int faceIndex, float blockX, float blockY, float blockZ, const TextureRegion &region);
void renderWorld(const World &world);
void uploadMesh(Chunk &chunk, const std::vector<Vertex> &vertices);
void buildChunkMesh(const World &world, const Chunk &chunk, std::vector<Vertex> &vertices);
bool isTransparent(Block block);
Block getWorldBlock(const World &world, int worldX, int worldY, int worldZ);
void generateChunk(Chunk &chunk);
float terrainHeight(int worldX, int worldZ);
TextureRegion getFaceTexture(Block block, Face face);


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Minecraft Clone", 0, 0);

    if(!window)
    {
        std::cout << "Failed to create GLFW window" << '\n';
        return -1;
    }

    stbi_set_flip_vertically_on_load(true);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << '\n';
        return -1;
    }

    initNoise();

    const int NUM_CHUNKS = 2;

    for (int chunkZ = 0; chunkZ < NUM_CHUNKS; ++chunkZ)
    {
        for (int chunkX = 0; chunkX < NUM_CHUNKS; ++chunkX)
        {
            ChunkKey key{chunkX, chunkZ};

            Chunk &chunk = worldChunks[key];
            chunk.chunkX = chunkX;
            chunk.chunkZ = chunkZ;

            generateChunk(chunk);
        }
    }

    std::vector<Vertex> meshVertices;

    for (auto& [key, chunk] : worldChunks)
    {
        buildChunkMesh(worldChunks, chunk, meshVertices);
        uploadMesh(chunk, meshVertices);

        chunk.needsMeshRebuild = false;
    }


    Shader shader(SHADER_DIR"shader.vs", SHADER_DIR"shader.fs");

    /*
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(myVertices), myVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    */


    GLuint texture = loadTexture(TEXTURE_DIR"textureAtlas.png");
    shader.setInt("texture1", 0);
    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    while (!glfwWindowShouldClose(window))
    {
        //Deltatime calculation
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //Input calculation
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glm::mat4 projection = glm::perspective(
                glm::radians(camera.zoom),
                (float)WIDTH/(float)HEIGHT,
                 0.1f,
                 100.0f);
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::lookAt(camera.cameraPos, camera.cameraPos + camera.DirectionFront, WorldUp);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        renderWorld(worldChunks);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport( 0, 0, width, height);
}

unsigned int loadTexture(char const* path)
{
    unsigned int texture;
    glGenTextures(1, &texture);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels,0);
    if (data)
    {
        GLenum format;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        //parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    stbi_image_free(data);
    return texture;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {glfwSetWindowShouldClose(window, true);}

    float cameraSpeed = 8 * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.cameraPos += camera.MovementFront * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.cameraPos -= camera.MovementFront * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.cameraPos -= camera.DirectionRight * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.cameraPos += camera.DirectionRight * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.cameraPos.y += 1.0f * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.cameraPos.y -= 1.0f * cameraSpeed;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (camera.firstMouse)
    {
        camera.lastX = xpos;
        camera.lastY = ypos;
        camera.firstMouse = false;
    }

    float xoffset = xpos - camera.lastX;
    float yoffset = camera.lastY - ypos;
    camera.lastX = xpos;
    camera.lastY = ypos;

    const float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    camera.yaw_angle += xoffset;
    camera.pitch_angle += yoffset;

    if (camera.pitch_angle > 89.0f)
        camera.pitch_angle = 89.0f;
    if (camera.pitch_angle < -89.0f)
        camera.pitch_angle = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(camera.yaw_angle)) * cos(glm::radians(camera.pitch_angle));
    direction.y = sin(glm::radians(camera.pitch_angle));
    direction.z = sin(glm::radians(camera.yaw_angle)) * cos(glm::radians(camera.pitch_angle));

    camera.DirectionFront = glm::normalize(direction);
    camera.DirectionRight = glm::normalize(glm::cross(camera.DirectionFront, WorldUp));
    camera.DirectionUp = glm::normalize(glm::cross(camera.DirectionRight, camera.DirectionFront));
    camera.MovementFront = glm::normalize(glm::vec3(direction.x, 0.0f, direction.z));
}

//NOTE(ullr): Chunk and Terrain system utility functions
constexpr int blockIndex(int x, int y, int z)
{
    return x + CHUNK_X * (z + CHUNK_Z * y);
}

Block& getBlock(Chunk& chunk, int x, int y, int z)
{
    return chunk.blocks[blockIndex(x, y, z)];
}

const Block& getBlock(const Chunk& chunk, int x, int y, int z)
{
    return chunk.blocks[blockIndex(x, y, z)];
}

float terrainHeight(int worldX, int worldZ)
{
    /*
    float nx = worldX * 0.01f;
    float nz = worldZ * 0.01f;

    float value =
        std::sin(nx) * 8.0f +
        std::cos(nz * 0.8f) * 6.0f +
        std::sin((nx + nz) * 0.4f) + 4.0f;

    return 64.0f + value;
    */
    float nx = worldX * 0.005f;
    float nz = worldZ * 0.005f;

    float value = NoiseGenerator::terrainNoise->GenSingle2D(nx, nz, 2);
    value = (value + 1.f) * 0.5f;

    const float baseHeight = 64.0f;
    const float amplitude = 40.0f;
    return baseHeight + value * amplitude;

}

//TODO(ullr): Implement real noise implementation
void generateChunk(Chunk &chunk)
{
    for (int z = 0; z < CHUNK_Z; ++z)
    {
        for (int x = 0; x < CHUNK_X; ++x)
        {
            int worldX = chunk.chunkX * CHUNK_X + x;
            int worldZ = chunk.chunkZ * CHUNK_Z + z;

            int height = static_cast<int>(terrainHeight(worldX, worldZ));
            height = std::clamp(height, 1, CHUNK_Y - 1);

            for (int y = 0; y < CHUNK_Y; ++y)
            {
                Block block = AIR;
                if (y < height - 4) {block = STONE;}
                else if (y < height - 1) {block = DIRT;}
                else if (y == height - 1) {block = GRASS;}

                //carving caves only into solid blocks below surface
                if (block != AIR &&
                    y >= 8 &&
                    y < height - 5 &&
                    isCave(worldX, y, worldZ))
                {
                    block = AIR;
                }

                getBlock(chunk, x, y, z) = block;
            }
        }
        chunk.generated = true;
        chunk.needsMeshRebuild = true;
    }
}

Block getWorldBlock(const World &world, int worldX, int worldY, int worldZ)
{
    if(worldY < 0 || worldY >= CHUNK_Y)
        return AIR;

    //We use std::floor because the world coordinates can be negative
    int chunkX = static_cast<int>(std::floor(static_cast<float>(worldX) / CHUNK_X));
    int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldZ) / CHUNK_Z));

    int localX = worldX - chunkX * CHUNK_X;
    int localZ = worldZ - chunkZ * CHUNK_Z;

    ChunkKey key{chunkX, chunkZ};
    auto it = world.find(key);

    if (it == world.end() || !it->second.generated)
        return AIR;

    return getBlock(it->second, localX, worldY, localZ);
}

//Render and Meshing functions
bool isTransparent(Block block)
{
    return block == AIR;
}

//NOTE(ullr): Can create a Struct of vertex to store position, normal, texture and blocktype instead of flat array
void buildChunkMesh(const World &world, const Chunk &chunk, std::vector<Vertex> &vertices)
{
    vertices.clear();

    static bool debugRenderAllFaces = false;

    for (int y = 0; y < CHUNK_Y; ++y)
    {
        for (int z = 0; z < CHUNK_Z; ++z)
        {
            for (int x = 0; x < CHUNK_X; ++x)
            {
                Block block = getBlock(const_cast<Chunk&>(chunk), x, y, z);

                if (block == AIR)
                    continue;

                int worldX = chunk.chunkX * CHUNK_X + x;
                int worldZ = chunk.chunkZ * CHUNK_Z + z;

                if (debugRenderAllFaces)
                {
                    appendFace(vertices, cubeVertices, Front,
                               worldX, y, worldZ,
                               getFaceTexture(block, Front));

                    appendFace(vertices, cubeVertices, Back,
                               worldX, y, worldZ,
                               getFaceTexture(block, Back));

                    appendFace(vertices, cubeVertices, Left,
                               worldX, y, worldZ,
                               getFaceTexture(block, Left));

                    appendFace(vertices, cubeVertices, Right,
                               worldX, y, worldZ,
                               getFaceTexture(block, Right));

                    appendFace(vertices, cubeVertices, Bottom,
                               worldX, y, worldZ,
                               getFaceTexture(block, Bottom));

                    appendFace(vertices, cubeVertices, Top,
                               worldX, y, worldZ,
                               getFaceTexture(block, Top));
                }
                else {

                 if (isTransparent(
                        getWorldBlock(world, worldX, y, worldZ - 1)))
                {
                    appendFace(
                        vertices, cubeVertices, Front,
                        worldX, y, worldZ, getFaceTexture(block, Front));
                }

                if (isTransparent(
                        getWorldBlock(world, worldX, y, worldZ + 1)))
                {
                    appendFace(
                        vertices, cubeVertices, Back,
                        worldX, y, worldZ, getFaceTexture(block, Back));
                }

                if (isTransparent(
                        getWorldBlock(world, worldX - 1, y, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Left,
                        worldX, y, worldZ, getFaceTexture(block, Left));
                }

                if (isTransparent(
                        getWorldBlock(world, worldX + 1, y, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Right,
                        worldX, y, worldZ, getFaceTexture(block, Right));
                }

                if (isTransparent(
                        getWorldBlock(world, worldX, y - 1, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Bottom,
                        worldX, y, worldZ, getFaceTexture(block, Bottom));
                }

                if (isTransparent(
                        getWorldBlock(world, worldX, y + 1, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Top,
                        worldX, y, worldZ, getFaceTexture(block, Top));
                }
                }
            }
        }
    }
}

void uploadMesh(Chunk &chunk, const std::vector<Vertex> &vertices)
{
    if (chunk.VAO == 0)
    {
        glGenVertexArrays(1, &chunk.VAO);
        glGenBuffers(1, &chunk.VBO);
    }

    glBindVertexArray(chunk.VAO);
    glBindBuffer(GL_ARRAY_BUFFER,chunk.VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, x)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, u)));

    chunk.vertexCount = static_cast<int>(vertices.size());

    glBindVertexArray(0);
}

TextureRegion getFaceTexture(Block block, Face face)
{
    switch (block)
    {
        case GRASS:
            switch (face)
            {
                case Top: return renderRegion(30, 3);
                case Bottom: return renderRegion(30, 1);
                default: return renderRegion(30, 2);
            }
        case DIRT:
            return renderRegion(30, 2);
        case STONE:
            return renderRegion(32, 22);
        case SAND:
            return renderRegion(34, 2);
        default:
            return renderRegion(0, 0);
    }
}

void appendFace(
        std::vector<Vertex> &meshVertices,
        const float cubeVertices[],
        int faceIndex,
        float blockX,
        float blockY,
        float blockZ,
        const TextureRegion &region
){
    constexpr int floatsPerVertex = 5;
    constexpr int verticesPerFace = 6;

    int faceOffset = faceIndex * verticesPerFace * floatsPerVertex;

    for (int i = 0; i < verticesPerFace; ++i)
    {
        int offset = faceOffset + i * floatsPerVertex;

        float localU = cubeVertices[offset + 3];
        float localV = cubeVertices[offset + 4];

        Vertex vertex{
            cubeVertices[offset + 0] + blockX,
            cubeVertices[offset + 1] + blockY,
            cubeVertices[offset + 2] + blockZ,
            region.u0 + localU * (region.u1 - region.u0),
            region.v0 + localV * (region.v1 - region.v0),
        };

        meshVertices.push_back(vertex);
    }
}

void renderWorld(const World &world)
{
    for (const auto& [key, chunk] : world)
    {
        if (chunk.vertexCount == 0)
            continue;

        glBindVertexArray(chunk.VAO);
        glDrawArrays(GL_TRIANGLES, 0, chunk.vertexCount);
    }
    glBindVertexArray(0);
}
