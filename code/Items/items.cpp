#include "items.h"

float getMiningSpeed(const Block& block, const ToolDefinition* tool)
{
    if (!tool)
        return 1.0f;

    if (tool->type != block.preferredToolType)
        return 1.0f;

    return tool->miningSpeed;
}
