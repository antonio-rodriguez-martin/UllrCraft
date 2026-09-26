#pragma once
#include "World/WorldGenerator.h"
#include <atomic>
#include <glm/vec3.hpp>

struct Task
{
    enum Type
    {
        none = 0,
        generateChunk,
    };

    int type = 0;
    glm::ivec3 pos {};
    int priority = 0;
};

struct TaskCompare
{
    bool operator()(const Task& a, const Task& b) const
    {
        return a.priority > b.priority;
    }
};

void submitTask(Task& task);
void submitTask(std::vector<Task> &t);
std::vector<Task> waitForTasks();

void submitChunk(Chunk *c);
std::vector<Chunk*> getChunks(int maxCount = 0);

void setPlayerChunk(int cx, int cz);

void startServer(int numThreads = 0);
void stopServer();

extern std::atomic<bool> serverRunning;
