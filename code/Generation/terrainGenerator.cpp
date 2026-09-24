#include "TerrainGenerator.h"
#include "noiseGeneration.h"

float terrainHeight(int worldX, int worldZ)
{
    float nx = worldX * 0.005f;
    float nz = worldZ * 0.005f;

    float value = NoiseGenerator::terrainNoise->GenSingle2D(nx, nz, 2);
    value = (value + 1.f) * 0.5f;

    const float baseHeight = 64.0f;
    const float amplitude = 40.0f;
    return baseHeight + value * amplitude;
}

