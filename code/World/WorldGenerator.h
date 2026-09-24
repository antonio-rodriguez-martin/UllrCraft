#pragma once
#include <algorithm>
#include <unordered_map>
#include "Chunk.h"
#include "ChunKey.h"

using World = std::unordered_map<ChunkKey, Chunk, ChunkKeyHash>;

Block& getBlock(Chunk& chunk, int x, int y, int z);
const Block& getBlock(const Chunk& chunk, int x, int y, int z);
constexpr int blockIndex(int x, int y, int z);
BlockType getWorldBlock(const World &world, int worldX, int worldY, int worldZ);
void generateChunk(Chunk &chunk);
