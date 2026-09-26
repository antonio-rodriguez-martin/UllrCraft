#include "threading.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include "World/Chunk.h"
#include "World/WorldGenerator.h"

std::mutex taskMutex;
std::condition_variable taskCondition;
std::priority_queue<Task, std::vector<Task>, TaskCompare> tasks;

std::mutex genMutex;

std::mutex chunkMutex;
std::queue<Chunk*> chunks;

std::vector<std::thread> workers;

std::atomic<bool> stopFlag(false);
std::atomic<bool> serverRunning(true);

void submitTask(Task& t)
{
    {
        std::lock_guard<std::mutex> lock(taskMutex);
        tasks.push(t);
        std::cout << "[submit] task (" << t.pos.x << "," << t.pos.z
                  << ") prio=" << t.priority
                  << " queue=" << tasks.size() << "\n";
    }
    taskCondition.notify_all();
}

void submitTask(std::vector<Task>& t)
{
    std::lock_guard<std::mutex> lock(taskMutex);
    for (const auto& i : t)
    {
        tasks.push(i);
    }
    taskCondition.notify_all();
}


void submitChunk(Chunk* c)
{
    std::lock_guard<std::mutex> lock(chunkMutex);
    chunks.push(c);
}

std::vector<Chunk*> getChunks(int maxCount)
{
    std::vector<Chunk*> retVector;
    std::lock_guard<std::mutex> lock(chunkMutex);
    std::cout << "[getChunks] disponibles=" << chunks.size()
              << " maxCount=" << maxCount << "\n";

    auto total = chunks.size();
    const std::size_t limit =
        (maxCount <= 0) ? total : std::min<std::size_t>(total, static_cast<std::size_t>(maxCount));

    retVector.reserve(limit);
    for (int i = 0; i < limit; i++)
    {
        retVector.push_back(chunks.front());
        chunks.pop();
    }
    return retVector;
}
//Worker function
static void workerFunction()
{
    while(true)
    {
        Task task;
        {
            std::unique_lock<std::mutex> lock(taskMutex);
            taskCondition.wait_for(lock,std::chrono::milliseconds(100),[]{return !tasks.empty() || stopFlag.load();});

            if(stopFlag.load() && tasks.empty())
                return;

            if (tasks.empty())
                return;

            task = tasks.top();
            tasks.pop();
            std::cout << "[worker] sacada tarea (" << task.pos.x << "," << task.pos.z
                      << ") queue=" << tasks.size() << "\n";
        }
        if (task.type == Task::generateChunk)
        {
            std::cout << "[worker] generando chunk " <<task.pos.x << ", " << task.pos.z << "\n";
            Chunk* c = new Chunk;
            c->chunkX = task.pos.x;
            c->chunkZ = task.pos.z;

            {
                std::lock_guard<std::mutex> lock(genMutex);
                generateChunk(*c);
            }
            std::cout << "[worker] chunk listo " << c->chunkX << "," << c->chunkZ << "\n";
            submitChunk(c);
        }
    }
}

//pool
void startServer(int numThreads)
{
    if (!workers.empty())
        return;

    if (numThreads <= 0)
    {
        unsigned hw = std::thread::hardware_concurrency();
        if (hw == 0) hw = 4;
        numThreads = static_cast<int>(hw) -1;
        if (numThreads < 1) numThreads = 1;
    }
    stopFlag = false;
    serverRunning = true;

    workers.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
        workers.emplace_back(workerFunction);
}

void stopServer()
{
    serverRunning = false;
    stopFlag = true;
    taskCondition.notify_all();

    for (auto& w : workers)
        if (w.joinable()) w.join();

    workers.clear();
}
