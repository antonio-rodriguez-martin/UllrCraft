#pragma once

enum class ToolType
{
    None,
    Pickaxe,
    Axe,
    Shovel,
    Hoe,
    Sword
};

enum class ToolTier
{
    None,
    Wood,
    Stone,
    Iron,
    Diamond
};

struct ToolDefinition
{
    ToolType type;
    ToolTier tier;

    float miningSpeed;
    int maxDurability;
};


