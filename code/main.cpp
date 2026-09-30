#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include "FastNoise/Utility/SmartNode.h"
#include "GLDebug.cpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Rendering/ChunkMesh.h"
#include "World/Block.h"
#include "World/WorldGenerator.h"
#include "glm/common.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

#include "Rendering/ShaderReader.h"
#include "Rendering/stb_image.h"

#include "World/Chunk.h"
#include "World/ChunKey.h"

// Implementation files - unity build
#include "World/WorldGenerator.cpp"
#include "Generation/noiseGeneration.cpp"
#include "Generation/terrainGenerator.cpp"
#include "Rendering/ChunkMesh.cpp"
#include "Rendering/WorldRenderer.cpp"
#include "Rendering/textureMap.cpp"
#include "Rendering/stb_image.cpp"

#include "threading.cpp"
#include "threading.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <unordered_set>
#include <vector>

static int WIDTH = 800;
static int HEIGHT = 600;
static float deltaTime;
static float lastFrame;
static glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
static int LOAD_DISTANCE = 6;
static int UNLOAD_DISTANCE = 10;
constexpr float INTERACTION_MAX_REACH  = 5.0f;

std::vector<Task> tasksVect;
World worldChunks;


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
    float zoom = 55.0f;
    bool firstMouse = true;
};

Camera camera {};

unsigned int loadTexture(char const* path);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

void ensureChunksAround(const glm::vec3 &playerPos, World &world);
void unloadDistantChunk(int playerChunkX, int playerChunkZ, World &world);
RayHit raycastBlock(const glm::vec3& origin, const glm::vec3& dir, float maxDist);
bool setBlockAt(World &world, glm::ivec3 worldPos, Block newBlock);
bool destroyBlock(World& world, glm::ivec3 worldPos);
void tryDestroyBlock(const glm::vec3& playerPos, World &world, RayHit hit);
void tryPlaceBlock(const glm::vec3& playerPos, World &world, RayHit hit, Block newBlock);

struct Node{
    ChunkKey key;
    std::unique_ptr<Chunk> value;
    Node *next;
    Node *prev;

    Node(ChunkKey k, std::unique_ptr<Chunk> c)
        :key{k},
        value(std::move(c)),
        next{nullptr},
        prev{nullptr}
    {
    }
};

class LRUCache
{
    public:
        int capacity;
        std::unordered_map<ChunkKey, Node*, ChunkKeyHash> cacheMap;
        Node *head;
        Node *tail;
        LRUCache(int capacity)
            :capacity{capacity},
            head{nullptr},
            tail{nullptr}
        {
            ChunkKey dummyKey{0, 0};
            head = new Node(dummyKey, nullptr);
            tail = new Node( dummyKey,nullptr);
            head->next = tail;
            tail->prev = head;
        }

        ~LRUCache()
        {
            Node* node = head;
            while (node != nullptr)
            {
                Node* next = node->next;
                delete node;
                node = next;
            }
            cacheMap.clear();
        }

        Chunk* get(const ChunkKey& key)
        {
            auto it = cacheMap.find(key);
            if (it == cacheMap.end())
                return nullptr;

            Node *node = it->second;
            remove(node);
            add(node);
            return node->value.get();
        }

        // Tranfers the ownership of the chunk from the URL to the world map
        std::unique_ptr<Chunk> take(const ChunkKey& key)
        {
            auto it = cacheMap.find(key);
            if (it == cacheMap.end())
               return nullptr;

            Node *node = it->second;
            remove(node);
            cacheMap.erase(it);
            std::unique_ptr<Chunk> chunk = std::move(node->value);
            delete node;
            return chunk;
        }

        void put(const ChunkKey& key, std::unique_ptr<Chunk> value)
        {
            auto it = cacheMap.find(key);
            if (it != cacheMap.end())
            {
                Node *oldNode = it->second;
                remove(oldNode);
                delete oldNode;
                cacheMap.erase(it);
            }

            Node *node = new Node(key, std::move(value));
            cacheMap[key] = node;
            add(node);

            if (cacheMap.size() > static_cast<std::size_t>(capacity))
            {
                Node *nodeToDelete = tail->prev;

                if(nodeToDelete != head)
                {
                    remove(nodeToDelete);
                    cacheMap.erase(nodeToDelete->key);
                    delete nodeToDelete;
                }
            }
        }

        void add(Node *node)
        {
            Node *nextNode = head->next;
            head->next = node;
            node->prev = head;
            node->next = nextNode;
            nextNode->prev = node;
        }

        void remove(Node *node)
        {
            Node *prevNode = node->prev;
            Node *nextNode = node->next;
            prevNode->next = nextNode;
            nextNode->prev = prevNode;
        }

        bool contains(const ChunkKey& key)
        {
            return cacheMap.find(key) != cacheMap.end();
        }

        void erase(const ChunkKey& key)
        {
            auto it = cacheMap.find(key);
            if (it == cacheMap.end())
                return;

            Node* node = it->second;
            remove(node);
            cacheMap.erase(it);
            delete node;
        }
};
static LRUCache cache(25);

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

    NoiseGenerator::initNoise();



    Shader shader(SHADER_DIR"shader.vs", SHADER_DIR"shader.fs");

    GLuint texture = loadTexture(TEXTURE_DIR"textureAtlas.png");
    shader.use();
    shader.setInt("texture1", 0);
    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_UNLIMITED_MOUSE_BUTTONS, GLFW_TRUE);

    lastFrame = static_cast<float>(glfwGetTime());

    startServer(8);

    MAX_RELEVANT = UNLOAD_DISTANCE;
    while (!glfwWindowShouldClose(window))
    {
        //Deltatime calculation
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //Input calculation
        processInput(window);

        glClearColor(0.47f, 0.65f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto generatedChunks = getChunks(2);
        for (Chunk* c : generatedChunks)
        {
            ChunkKey key(c->chunkX, c->chunkZ);
            unmarkGenerating(key);
            worldChunks.emplace(key, std::unique_ptr<Chunk>(c));
            markNeighborsDirty(worldChunks, key);
        }

        ensureChunksAround(camera.cameraPos, worldChunks);

        //Build mesh
        std::vector<Vertex> vertices;
        int meshBudget = 2;
        for (auto& [key, chunk] : worldChunks)
        {
            if(meshBudget ==0) break;
            if(!chunk->needsMeshRebuild) continue;
            --meshBudget;
            vertices.clear();
            buildChunkMesh(worldChunks, *chunk, vertices);
            uploadMesh(*chunk, vertices);
            chunk->needsMeshRebuild = false;
        }

        shader.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glm::mat4 projection = glm::perspective(
                glm::radians(camera.zoom),
                (float)WIDTH/(float)HEIGHT,
                 0.1f,
                 100.0f);
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::lookAt(camera.cameraPos, camera.cameraPos + camera.DirectionFront, camera.DirectionUp);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);

        renderWorld(worldChunks);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    stopServer();

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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        RayHit hit = raycastBlock(camera.cameraPos, camera.DirectionFront, INTERACTION_MAX_REACH);
        tryDestroyBlock(camera.cameraPos, worldChunks, hit);
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        RayHit hit = raycastBlock(camera.cameraPos, camera.DirectionFront, INTERACTION_MAX_REACH);
        Block block{};
        block.blockType = GRASS;
        tryPlaceBlock(camera.cameraPos, worldChunks, hit, block);
    }

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

//TODO(ullr): Configure the take and the URL cache to use ownnership from world to Cache and from cache to World if coming back
void ensureChunksAround(const glm::vec3 &playerPos, World &world)
{
    int pcX = static_cast<int>(std::floor(playerPos.x / CHUNK_X));
    int pcZ = static_cast<int>(std::floor(playerPos.z / CHUNK_Z));

    setPlayerChunk(pcX, pcZ);
    for (int dz = -LOAD_DISTANCE; dz <= LOAD_DISTANCE; ++dz)
    {
        for (int dx = -LOAD_DISTANCE; dx <= LOAD_DISTANCE; ++dx)
        {
            ChunkKey key{pcX + dx, pcZ + dz};

            if (world.find(key) != world.end())
                continue;
            if (isGenerating(key))
                continue;

            auto it = world.find(key);
            if (it == world.end())
            {
                auto chunk = cache.take(key);
                if (chunk)
                {
                    unmarkGenerating(key);
                    world.emplace(key, std::move(chunk));
                    markNeighborsDirty(world, key);
                    continue;
                }
                Task task;
                task.type = Task::generateChunk;
                task.pos = glm::ivec3(key.x, 0, key.z);
                task.priority = dx * dx + dz * dz;
                submitTask(task);
                markGenerating(key);
            }
        }
    }
    unloadDistantChunk(pcX, pcZ, world);
}

void unloadDistantChunk(int playerChunkX, int playerChunkZ, World &world)
{
    for (auto it = world.begin(); it != world.end();)
    {
        const int dx = it->first.x - playerChunkX;
        const int dz = it->first.z - playerChunkZ;

        const int distanceSquared = dx * dx + dz * dz;


        if (distanceSquared > UNLOAD_DISTANCE * UNLOAD_DISTANCE)
        {
            ChunkKey key = it->first;

            std::unique_ptr<Chunk> chunk = std::move(it->second);

            it = world.erase(it);

            cache.put(key, std::move(chunk));
        }
        else
         ++it;
    }
}

RayHit raycastBlock(const glm::vec3& origin, const glm::vec3& dir, float maxDist)
{
    RayHit result;

    glm::ivec3 voxel = glm::floor(origin);
    glm::ivec3 step(
        dir.x > 0 ? 1 : -1,
        dir.y > 0 ? 1 : -1,
        dir.z > 0 ? 1 : -1
    );

    glm::vec3 tMax, tDelta;
    for (int i = 0; i < 3; ++i)
    {
        if (dir[i] == 0.0f)
        {
            tMax[i] = 1e30f;
            tDelta[i] = 1e30f;
        }
        else
        {
            //if we move positively then left, if we move negatively then right
            float nextBoundary = (step[i] > 0) ? (voxel[i] + 1) : voxel[i];
            // origin + dir * t = boundary -> we rearrange to get the t (tMax)
            tMax[i] = (nextBoundary - origin[i]) / dir[i];
            tDelta[i] = 1.0f / std::abs(dir[i]); //distance in ray parameter t between voxel boundaries
        }
    }

    float t = 0.0f;
    glm::ivec3 normal(0);

    while (t <= maxDist)
    {
        if (isBlockSolid(worldChunks, voxel.x, voxel.y, voxel.z))
        {
            result.hit = true;
            result.blockPos = voxel;
            result.normal = normal;
    std::cout << "Result hit|position|normal: " << result.hit  << "|" << result.blockPos.x << "," << result.blockPos.y << "," << result.blockPos.z << "|" << result.normal.x << "," << result.normal.y << "," << result.normal.z<< "\n";
            return result;
        }

        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            voxel.x += step.x; t = tMax.x; tMax.x += tDelta.x;
            normal = glm::ivec3(-step.x, 0, 0);
        }
        else if (tMax.y < tMax.z)
        {
            voxel.y += step.y; t = tMax.y; tMax.y += tDelta.y;
            normal = glm::ivec3(0, -step.y, 0);
        }
        else
        {
            voxel.z += step.z; t = tMax.z; tMax.z += tDelta.z;
            normal = glm::ivec3(0, 0, -step.z);
        }
    }
    std::cout << "Result hit|position|normal: " << result.hit  << "|" << result.blockPos.x << "," << result.blockPos.y << "," << result.blockPos.z << "|" << result.normal.x << "," << result.normal.y << "," << result.normal.z<< "\n";
    return result; //hit = false
}

void markBlockChanged(World& world, int worldX, int worldY, int worldZ)
{
    int chunkX = static_cast<int>(std::floor(static_cast<float>(worldX) / CHUNK_X));
    int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldZ) / CHUNK_Z));

    auto markDirty = [&](int x, int z){
        auto it = world.find(ChunkKey{x, z});
        if (it != world.end())
            it->second->needsMeshRebuild = true;
    };

    markDirty(chunkX, chunkZ);

    if ((worldX & 15) == 0) markDirty(chunkX - 1, chunkZ);
    if ((worldX & 15) == 15) markDirty(chunkX + 1, chunkZ);
    if ((worldZ & 15) == 0) markDirty(chunkX, chunkZ - 1);
    if ((worldZ & 15) == 15) markDirty(chunkX, chunkZ + 1);

    if ((worldX & 15) == 0 && (worldZ & 15) == 0) markDirty(chunkX - 1, chunkZ - 1);
    if ((worldX & 15) == 15 && (worldZ & 15) == 0) markDirty(chunkX + 1, chunkZ - 1);
    if ((worldX & 15) == 0 && (worldZ & 15) == 15) markDirty(chunkX - 1, chunkZ + 1);
    if ((worldX & 15) == 15 && (worldZ & 15) == 15) markDirty(chunkX + 1, chunkZ + 1);
}

//TODO(ullr): I have the selection function, Create the destruct and set blocks functions
void tryPlaceBlock(const glm::vec3& playerPos, World &world, RayHit hit, Block newBlock)
{
    if (!hit.hit ) return;

    glm::ivec3 target = hit.blockPos + hit.normal;

    //TODO(ullr): change this for the AABB intersection collision
    glm::ivec3 playerVoxel = glm::floor(playerPos);
    if (target == playerVoxel) return;
    if (target == playerVoxel + glm::ivec3(0, 1, 0)) return;

    setBlockAt(world, target, newBlock);
    markBlockChanged(world, target.x, target.y, target.z);
}

void tryDestroyBlock(const glm::vec3& playerPos, World &world, RayHit hit)
{
    if (!hit.hit ) return;

    glm::ivec3 target = hit.blockPos;

    //TODO(ullr): change this for the AABB intersection collision
    glm::ivec3 playerVoxel = glm::floor(playerPos);
    if (target == playerVoxel) return;
    if (target == playerVoxel + glm::ivec3(0, 1, 0)) return;

    destroyBlock(world, target);
    markBlockChanged(world, target.x, target.y, target.z);
}

bool setBlockAt(World &world, glm::ivec3 worldPos, Block newBlock)
{
    int chunkX = static_cast<int>(std::floor(static_cast<float>(worldPos.x) / CHUNK_X));
    int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldPos.z) / CHUNK_Z));

    auto it = world.find(ChunkKey{chunkX, chunkZ});

    if (it == world.end()) return false; //not loaded chunk

    int lx = worldPos.x & 15;
    int lz = worldPos.z & 15;
    int ly = worldPos.y;
    if (ly < 0 || ly >= CHUNK_Y) return false;

    it->second->blocks[blockIndex(lx, ly, lz)].blockType = newBlock.blockType;
    return true;
}

bool destroyBlock(World& world, glm::ivec3 worldPos)
{

    int chunkX = static_cast<int>(std::floor(static_cast<float>(worldPos.x) / CHUNK_X));
    int chunkZ = static_cast<int>(std::floor(static_cast<float>(worldPos.z) / CHUNK_Z));

    auto it = world.find(ChunkKey{chunkX, chunkZ});

    if (it == world.end()) return false; //not loaded chunk

    int lx = worldPos.x & 15;
    int lz = worldPos.z & 15;
    int ly = worldPos.y;
    if (ly < 0 || ly >= CHUNK_Y) return false;

    it->second->blocks[blockIndex(lx, ly, lz)].blockType = AIR;
    return true;

}
