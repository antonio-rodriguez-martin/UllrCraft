#pragma once
enum BlockType
{
    AIR = 0,
    GRASS,
    DIRT,
    STONE,
    SAND
};

struct Block
{
    BlockType blockType = AIR;
};

inline float blockHardness(BlockType bt)
{
    switch (bt)
    {
        case BlockType::AIR: return 0.0f;
        case BlockType::DIRT: return 0.5f;
        case BlockType::GRASS: return 0.5f;
        case BlockType::STONE: return 0.5f;
        case BlockType::SAND: return 0.5f;
        default: return 1.0f;
    }
}
