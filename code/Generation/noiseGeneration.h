#pragma once
#include <FastNoise/FastNoise.h>
#include "FastNoise/Utility/SmartNode.h"

namespace NoiseGenerator
{
    static FastNoise::SmartNode<> terrainNoise;
    static FastNoise::SmartNode<> caveNoise;

    void initNoise();
    bool isCave(int worldX, int worldY, int worldZ);
}
