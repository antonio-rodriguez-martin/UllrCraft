#pragma once
#include <memory>
#include <unordered_map>
#include "textureMap.h"
#include "../World/ChunKey.h"
#include "../World/Chunk.h"

struct ChunkMesh
{
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint vertexCount = 0;
};

struct Vertex
{
    float x, y, z; //position
    //float nx, ny, nz; // normals
    float u, v; //textures
};

enum Face {
    Front = 0,  // -Z
    Back   = 1, // +Z
    Left   = 2, // -X
    Right  = 3, // +X
    Bottom = 4, // -Y
    Top    = 5  // +Y
};

using ChunkMeshes = std::unordered_map<ChunkKey, ChunkMesh, ChunkKeyHash>;
using World = std::unordered_map<ChunkKey, std::unique_ptr<Chunk>, ChunkKeyHash>;


void buildChunkMesh(const World &world, const Chunk &chunk, std::vector<Vertex> &vertices);
void appendFace( std::vector<Vertex> &meshVertices, const float cubeVertices[], int faceIndex, float blockX, float blockY, float blockZ, const TextureRegion &region);
TextureRegion getFaceTexture(BlockType blockType, Face face);
bool isTransparent(BlockType blockType);
void uploadMesh(Chunk &chunk, const std::vector<Vertex> &vertices);
void markNeighborsDirty(World &world, const ChunkKey& key);

inline constexpr float cubeVertices[] = {
// Front face (-Z)
// position             // UV
0.0f, 0.0f, 0.0f,       0.0f, 0.0f,
1.0f, 0.0f, 0.0f,       1.0f, 0.0f,
1.0f, 1.0f, 0.0f,       1.0f, 1.0f,

1.0f, 1.0f, 0.0f,       1.0f, 1.0f,
0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
0.0f, 0.0f, 0.0f,       0.0f, 0.0f,

// Back face (+Z)
0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
1.0f, 0.0f, 1.0f,       1.0f, 0.0f,
1.0f, 1.0f, 1.0f,       1.0f, 1.0f,

1.0f, 1.0f, 1.0f,       1.0f, 1.0f,
0.0f, 1.0f, 1.0f,       0.0f, 1.0f,
0.0f, 0.0f, 1.0f,       0.0f, 0.0f,

// Left face (-X)
0.0f, 1.0f, 1.0f,       1.0f, 1.0f,
0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
0.0f, 0.0f, 0.0f,       0.0f, 0.0f,

0.0f, 0.0f, 0.0f,       0.0f, 0.0f,
0.0f, 0.0f, 1.0f,       1.0f, 0.0f,
0.0f, 1.0f, 1.0f,       1.0f, 1.0f,

// Right face (+X)
1.0f, 1.0f, 1.0f,       1.0f, 1.0f,
1.0f, 1.0f, 0.0f,       0.0f, 1.0f,
1.0f, 0.0f, 0.0f,       0.0f, 0.0f,

1.0f, 0.0f, 0.0f,       0.0f, 0.0f,
1.0f, 0.0f, 1.0f,       1.0f, 0.0f,
1.0f, 1.0f, 1.0f,       1.0f, 1.0f,

// Bottom face (-Y)
0.0f, 0.0f, 0.0f,       0.0f, 1.0f,
1.0f, 0.0f, 0.0f,       1.0f, 1.0f,
1.0f, 0.0f, 1.0f,       1.0f, 0.0f,

1.0f, 0.0f, 1.0f,       1.0f, 0.0f,
0.0f, 0.0f, 1.0f,       0.0f, 0.0f,
0.0f, 0.0f, 0.0f,       0.0f, 1.0f,

// Top face (+Y)
0.0f, 1.0f, 0.0f,       0.0f, 1.0f,
1.0f, 1.0f, 0.0f,       1.0f, 1.0f,
1.0f, 1.0f, 1.0f,       1.0f, 0.0f,

1.0f, 1.0f, 1.0f,       1.0f, 0.0f,
0.0f, 1.0f, 1.0f,       0.0f, 0.0f,
0.0f, 1.0f, 0.0f,       0.0f, 1.0f
};

