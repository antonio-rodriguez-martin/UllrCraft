#pragma once
struct TextureRegion
{
    float u0;
    float v0;
    float u1;
    float v1;
};

TextureRegion makeRegion(int x, int y, int width, int height);
TextureRegion renderRegion(int renderX, int renderY);

/*
namespace textureNames
{
    const TextureRegion grassBottom = makeRegion(30*16, 1*16, 16, 16);
    const TextureRegion grassSide = makeRegion(30 * 16, 2 * 16, 16, 16);
    const TextureRegion grassTop = makeRegion(30*16, 3*16, 16, 16);
};
*/
