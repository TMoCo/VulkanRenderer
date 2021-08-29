#ifndef COMMANDS_H
#define COMMANDS_H

#include <common/types.h>

#include <vulkan/vulkan_core.h>

namespace cmd {
    //-Begin & end single use cmds-------------------------------------------------------------------------------//
    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);

    void endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer, 
        VkCommandPool commandPool);

    //-Image layout transitions----------------------------------------------------------------------------------//
    void transitionLayoutUndefinedToTransferDest(VkCommandBuffer commandBuffer, VkImage image,
        UI32 baseMip, UI32 levelCount, UI32 baseArr, UI32 layerCount);

    void transitionLayoutTransferDestToFragShaderRead(VkCommandBuffer commandBuffer, VkImage image,
        UI32 baseMip, UI32 levelCount, UI32 baseArr, UI32 layerCount);

    void transitionLayoutUndefinedToDepthAttachment(VkCommandBuffer commandBuffer, VkImage image,
        UI32 baseMip, UI32 levelCount, UI32 baseArr, UI32 layerCount, VkFormat format);

};

#endif // !COMMANDS_H
