#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
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
    STONE
};

const int CHUNK_X = 16;
const int CHUNK_Y = 256;
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
    glm::vec3 cameraPos = glm::vec3(16.0f, 80.0f, 50.0f);
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
void appendFace( std::vector<Vertex> &meshVertices, const float cubeVertices[], int faceIndex, float blockX, float blockY, float blockZ);
void renderWorld(const World &world);
void uploadMesh(Chunk &chunk, const std::vector<Vertex> &vertices);
void buildChunkMesh(const World &world, const Chunk &chunk, std::vector<Vertex> &vertices);
bool isTransparent(Block block);
Block getWorldBlock(const World &world, int worldX, int worldY, int worldZ);
void generateChunk(Chunk &chunk);
float terrainHeight(int worldX, int worldZ);


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

    const int NUM_CHUNKS = 6;

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

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(myVertices), myVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    */

    GLuint texture = loadTexture(TEXTURE_DIR"grass.jpg");
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
    float nx = worldX * 0.01f;
    float nz = worldZ * 0.01f;

    float value =
        std::sin(nx) * 8.0f +
        std::cos(nz * 0.8f) * 6.0f +
        std::sin((nx + nz) * 0.4f) + 4.0f;

    return 64.0f + value;
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

//Meshing functions
bool isTransparent(Block block)
{
    return block == AIR;
}


//NOTE(ullr): Can create a Struct of vertex to store position, normal, texture and blocktype instead of flat array
void buildChunkMesh(const World &world, const Chunk &chunk, std::vector<Vertex> &vertices)
{
    vertices.clear();

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

                 if (isTransparent(
                        getWorldBlock(world, worldX, y, worldZ - 1)))
                {
                    appendFace(
                        vertices, cubeVertices, Front,
                        worldX, y, worldZ);
                }

                if (isTransparent(
                        getWorldBlock(world, worldX, y, worldZ + 1)))
                {
                    appendFace(
                        vertices, cubeVertices, Back,
                        worldX, y, worldZ);
                }

                if (isTransparent(
                        getWorldBlock(world, worldX - 1, y, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Left,
                        worldX, y, worldZ);
                }

                if (isTransparent(
                        getWorldBlock(world, worldX + 1, y, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Right,
                        worldX, y, worldZ);
                }

                if (isTransparent(
                        getWorldBlock(world, worldX, y - 1, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Bottom,
                        worldX, y, worldZ);
                }

                if (isTransparent(
                        getWorldBlock(world, worldX, y + 1, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Top,
                        worldX, y, worldZ);
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

void appendFace(
        std::vector<Vertex> &meshVertices,
        const float cubeVertices[],
        int faceIndex,
        float blockX,
        float blockY,
        float blockZ
){
    constexpr int floatsPerVertex = 5;
    constexpr int verticesPerFace = 6;

    int faceOffset = faceIndex * verticesPerFace * floatsPerVertex;

    for (int i = 0; i < verticesPerFace; ++i)
    {
        int offset = faceOffset + i * floatsPerVertex;
        Vertex vertex{
            cubeVertices[offset + 0] + blockX,
            cubeVertices[offset + 1] + blockY,
            cubeVertices[offset + 2] + blockZ,
            cubeVertices[offset + 3] + blockX,
            cubeVertices[offset + 4] + blockX,
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
