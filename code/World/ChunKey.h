#pragma once
#include <cstddef>
#include <functional>

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


