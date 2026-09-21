#include "textureMap.h"

TextureRegion makeRegion(int x, int y, int width, int height)
{
    constexpr float atlasWidth = 1024.0f;
    constexpr float atlasHeight = 1024.0f;

    constexpr float inset = 0.5f / 1024.0f;

    float u0 = x / atlasWidth;
    float u1 = (x + width) / atlasWidth;

    float v0 = 1.0f - (y + height) / atlasHeight;
    float v1 = 1.0f - y / atlasHeight;

    return {
        u0 + inset, v0 + inset,
        u1 - inset, v1 - inset
    };
}

TextureRegion renderRegion(int renderX, int renderY)
{
    return makeRegion(renderX*16, renderY*16, 16, 16);
}
