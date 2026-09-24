#include "WorldGenerator.h"
#include "../Generation/noiseGeneration.h"
#include "../Generation/terrainGenerator.h"

BlockType getWorldBlock(const World &world, int worldX, int worldY, int worldZ)
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

    return getBlock(it->second, localX, worldY, localZ).blockType;
}

Block& getBlock(Chunk& chunk, int x, int y, int z)
{
    return chunk.blocks[blockIndex(x, y, z)];
}

const Block& getBlock(const Chunk& chunk, int x, int y, int z)
{
    return chunk.blocks[blockIndex(x, y, z)];
}

constexpr int blockIndex(int x, int y, int z)
{
    return x + CHUNK_X * (z + CHUNK_Z * y);
}

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
                Block block;
                block.blockType = AIR;
                if (y < height - 4) {block.blockType = STONE;}
                else if (y < height - 1) {block.blockType = DIRT;}
                else if (y == height - 1) {block.blockType = GRASS;}

                //carving caves only into solid blocks below surface
                if (block.blockType != AIR &&
                    y >= 5 &&
                    y < height - 5 &&
                    NoiseGenerator::isCave(worldX, y, worldZ))
                {
                    block.blockType = AIR;
                }

                getBlock(chunk, x, y, z) = block;
            }
        }
    }
    chunk.generated = true;
    chunk.needsMeshRebuild = true;
}
