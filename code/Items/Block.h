#pragma once
#include "tools.h"

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
    float hardness {};
    ToolType preferredToolType {};
    ToolTier minimumTier {};
};

inline float setBlockHardness(BlockType bt)
{
    switch (bt)
    {
        case BlockType::AIR: return 0.0f;
        case BlockType::DIRT: return 1.0f;
        case BlockType::GRASS: return 1.0f;
        case BlockType::STONE: return 2.0f;
        case BlockType::SAND: return 1.0f;
        default: return 1.0f;
    }
}

inline ToolTier setMinimumTier(BlockType bt)
{
    switch (bt)
    {
        case BlockType::AIR: return ToolTier::None;
        case BlockType::DIRT: return ToolTier::None;
        case BlockType::GRASS: return ToolTier::None;
        case BlockType::STONE: return ToolTier::Wood;
        case BlockType::SAND: return ToolTier::None;
        default: return ToolTier::None;
    }
}

inline ToolType setPreferedTool(BlockType bt)
{
    switch (bt)
    {
        case BlockType::AIR: return ToolType::None;
        case BlockType::DIRT: return ToolType::Shovel;
        case BlockType::GRASS: return ToolType::Shovel;
        case BlockType::STONE: return ToolType::Pickaxe;
        case BlockType::SAND: return ToolType::Shovel;
        default: return ToolType::None;
    }
}
