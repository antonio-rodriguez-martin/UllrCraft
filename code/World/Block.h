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


