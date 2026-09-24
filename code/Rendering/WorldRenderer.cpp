#include <glad/glad.h>
#include "WorldRenderer.h"

void renderWorld(const World &world)
{
    for (const auto& [key, chunk] : world)
    {
        if (chunk.vertexCount == 0)
            continue;

        glBindVertexArray(chunk.VAO);
        glDrawArrays(GL_TRIANGLES, 0, chunk.vertexCount);
    }
    glBindVertexArray(0);
}
