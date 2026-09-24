#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include "FastNoise/Utility/SmartNode.h"
#include "GLDebug.cpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "World/WorldGenerator.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
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

#include <glad/glad.h>
#include <GLFW/glfw3.h>

static int WIDTH = 800;
static int HEIGHT = 600;
static float deltaTime;
static float lastFrame;
static glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
static int RENDER_DISTANCE = 2;


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

Camera camera {};

unsigned int loadTexture(char const* path);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);

void ensureChunksAround(const glm::vec3 &playerPos, World &world);

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

    World worldChunks;


    Shader shader(SHADER_DIR"shader.vs", SHADER_DIR"shader.fs");

    GLuint texture = loadTexture(TEXTURE_DIR"textureAtlas.png");
    shader.use();
    shader.setInt("texture1", 0);
    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    lastFrame = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window))
    {
        //Deltatime calculation
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //Input calculation
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ensureChunksAround(camera.cameraPos, worldChunks);

        std::vector<Vertex> vertices;
        for (auto& [key, chunk] : worldChunks)
        {
            if(!chunk.needsMeshRebuild) continue;
            vertices.clear();
            buildChunkMesh(worldChunks, chunk, vertices);
            uploadMesh(chunk, vertices);
            chunk.needsMeshRebuild = false;
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

void ensureChunksAround(const glm::vec3 &playerPos, World &world)
{
    int pcX = static_cast<int>(std::floor(playerPos.x / CHUNK_X));
    int pcZ = static_cast<int>(std::floor(playerPos.z / CHUNK_Z));

    for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; ++dz)
    {
        for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; ++dx)
        {
            ChunkKey key{pcX + dx, pcZ + dz};
            auto it = world.find(key);
            if (it == world.end())
            {
                Chunk& chunk = world[key];
                chunk.chunkX = key.x;
                chunk.chunkZ = key.z;
                generateChunk(chunk);
                chunk.needsMeshRebuild = true;
            }
        }
    }
}

