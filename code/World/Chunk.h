#pragma once
#include <array>
#include <GLFW/glfw3.h>
#include "Block.h"

const int CHUNK_X = 16;
const int CHUNK_Y = 256;
const int CHUNK_Z = 16;

const int BLOCK_COUNT = CHUNK_X * CHUNK_Y * CHUNK_Z;
struct Chunk
{
    int chunkX;
    int chunkZ;
    //Block blocks [CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    std::array<Block, BLOCK_COUNT> blocks;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint vertexCount = 0;

    //rendering flags
    bool needsMeshRebuild = false;
    bool generated = false; //for ram generation False -> in disk true -> ram
};


