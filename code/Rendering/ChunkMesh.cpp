#include <glad/glad.h>
#include <vector>
#include "ChunkMesh.h"
#include "../World/WorldGenerator.h"


void buildChunkMesh(const World &world, const Chunk &chunk, std::vector<Vertex> &vertices)
{
    vertices.clear();

    static bool debugRenderAllFaces = false;

    for (int y = 0; y < CHUNK_Y; ++y)
    {
        for (int z = 0; z < CHUNK_Z; ++z)
        {
            for (int x = 0; x < CHUNK_X; ++x)
            {
                Block block = getBlock(const_cast<Chunk&>(chunk), x, y, z);

                if (block.blockType == AIR)
                    continue;

                int worldX = chunk.chunkX * CHUNK_X + x;
                int worldZ = chunk.chunkZ * CHUNK_Z + z;

                if (debugRenderAllFaces)
                {
                    appendFace(vertices, cubeVertices, Front,
                               worldX, y, worldZ,
                               getFaceTexture(block.blockType, Front));

                    appendFace(vertices, cubeVertices, Back,
                               worldX, y, worldZ,
                               getFaceTexture(block.blockType, Back));

                    appendFace(vertices, cubeVertices, Left,
                               worldX, y, worldZ,
                               getFaceTexture(block.blockType, Left));

                    appendFace(vertices, cubeVertices, Right,
                               worldX, y, worldZ,
                               getFaceTexture(block.blockType, Right));

                    appendFace(vertices, cubeVertices, Bottom,
                               worldX, y, worldZ,
                               getFaceTexture(block.blockType, Bottom));

                    appendFace(vertices, cubeVertices, Top,
                               worldX, y, worldZ,
                               getFaceTexture(block.blockType, Top));
                }
                else {

                 if (isTransparent(
                        getWorldBlock(world, worldX, y, worldZ - 1)))
                {
                    appendFace(
                        vertices, cubeVertices, Front,
                        worldX, y, worldZ, getFaceTexture(block.blockType, Front));
                }

                if (isTransparent(
                        getWorldBlock(world, worldX, y, worldZ + 1)))
                {
                    appendFace(
                        vertices, cubeVertices, Back,
                        worldX, y, worldZ, getFaceTexture(block.blockType, Back));
                }

                if (isTransparent(
                        getWorldBlock(world, worldX - 1, y, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Left,
                        worldX, y, worldZ, getFaceTexture(block.blockType, Left));
                }

                if (isTransparent(
                        getWorldBlock(world, worldX + 1, y, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Right,
                        worldX, y, worldZ, getFaceTexture(block.blockType, Right));
                }

                if (isTransparent(
                        getWorldBlock(world, worldX, y - 1, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Bottom,
                        worldX, y, worldZ, getFaceTexture(block.blockType, Bottom));;
                }

                if (isTransparent(
                        getWorldBlock(world, worldX, y + 1, worldZ)))
                {
                    appendFace(
                        vertices, cubeVertices, Top,
                        worldX, y, worldZ, getFaceTexture(block.blockType, Top));
                }
                }
            }
        }
    }
}


void appendFace(
        std::vector<Vertex> &meshVertices,
        const float cubeVertices[],
        int faceIndex,
        float blockX,
        float blockY,
        float blockZ,
        const TextureRegion &region
){
    constexpr int floatsPerVertex = 5;
    constexpr int verticesPerFace = 6;

    int faceOffset = faceIndex * verticesPerFace * floatsPerVertex;

    for (int i = 0; i < verticesPerFace; ++i)
    {
        int offset = faceOffset + i * floatsPerVertex;

        float localU = cubeVertices[offset + 3];
        float localV = cubeVertices[offset + 4];

        Vertex vertex{
            cubeVertices[offset + 0] + blockX,
            cubeVertices[offset + 1] + blockY,
            cubeVertices[offset + 2] + blockZ,
            region.u0 + localU * (region.u1 - region.u0),
            region.v0 + localV * (region.v1 - region.v0),
        };

        meshVertices.push_back(vertex);
    }
}

TextureRegion getFaceTexture(BlockType blockType, Face face)
{
    switch (blockType)
    {
        case GRASS:
            switch (face)
            {
                case Top: return renderRegion(30, 3);
                case Bottom: return renderRegion(30, 1);
                default: return renderRegion(30, 2);
            }
        case DIRT:
            return renderRegion(30, 2);
        case STONE:
            return renderRegion(32, 22);
        case SAND:
            return renderRegion(34, 2);
        default:
            return renderRegion(0, 0);
    }
}

bool isTransparent(BlockType blockType)
{
    return blockType == AIR;
}

void uploadMesh(Chunk &chunk, const std::vector<Vertex> &vertices)
{
    if (chunk.VAO == 0)
    {
        glGenVertexArrays(1, &chunk.VAO);
        glGenBuffers(1, &chunk.VBO);
    }

    glBindVertexArray(chunk.VAO);
    glBindBuffer(GL_ARRAY_BUFFER,chunk.VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, x)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, u)));

    chunk.vertexCount = static_cast<int>(vertices.size());

    glBindVertexArray(0);
}

void markNeighborsDirty(World &world, const ChunkKey& key)
{
    const ChunkKey neighbors[4] = {
        {key.x - 1, key.z}, {key.x + 1, key.z},
        {key.x, key.z - 1}, {key.x, key.z + 1},
    };
    for (const auto& n : neighbors)
    {
        auto it = world.find(n);
        if (it != world.end())
            it->second->needsMeshRebuild = true;
    }
}
