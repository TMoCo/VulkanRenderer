#include <common/commands.h>
#include <common/utils.h>
#include <common/vkinit.h>


namespace cmd {
    //-Begin & end single use cmds-------------------------------------------------------------------------------//
    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool) {
        VkCommandBuffer commandBuffer;

        // allocate the command buffer
        {
            VkCommandBufferAllocateInfo allocInfo = vkinit::commandBufferAllocateInfo(commandPool, 
                VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
            vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
        }

        // begin the command buffer
        {
            VkCommandBufferBeginInfo beginInfo = vkinit::commandBufferBeginInfo(
                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
            vkBeginCommandBuffer(commandBuffer, &beginInfo);
        }

        return commandBuffer;
    }

    void endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandBuffer commandBuffer, VkCommandPool commandPool) {
        vkEndCommandBuffer(commandBuffer);

        // submit the queue for execution
        {
            VkSubmitInfo submitInfo = vkinit::submitInfo(nullptr, 0, nullptr, 0, nullptr, 1, &commandBuffer);
            vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        }

        // free the command buffer when the queue is idle
        {
            vkQueueWaitIdle(queue);
            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }
    }

    //-Image layout transitions----------------------------------------------------------------------------------//
    void transitionLayoutUndefinedToTransferDest(VkCommandBuffer commandBuffer, VkImage image, 
        UI32 baseMip, UI32 levelCount, UI32 baseArr, UI32 layerCount) {

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // mip mapping
        barrier.subresourceRange.baseMipLevel = baseMip;
        barrier.subresourceRange.levelCount = levelCount;
        // image array
        barrier.subresourceRange.baseArrayLayer = baseArr;
        barrier.subresourceRange.layerCount = layerCount;
        // access masks
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // declare stages 
        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        
        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 
            0, nullptr, // memory barriers
            0, nullptr, // buffer memory barriers
            1, &barrier // image memory barriers
        );
    }

    void transitionLayoutTransferDestToFragShaderRead(VkCommandBuffer commandBuffer, VkImage image,
        UI32 baseMip, UI32 levelCount, UI32 baseArr, UI32 layerCount) {

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // mip mapping
        barrier.subresourceRange.baseMipLevel = baseMip;
        barrier.subresourceRange.levelCount = levelCount;
        // image array
        barrier.subresourceRange.baseArrayLayer = baseArr;
        barrier.subresourceRange.layerCount = layerCount;
        // access masks
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // declare stages 
        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0,
            0, nullptr, // memory barriers
            0, nullptr, // buffer memory barriers
            1, &barrier // image memory barriers
        );
    }

    void transitionLayoutUndefinedToDepthAttachment(VkCommandBuffer commandBuffer, VkImage image,
        UI32 baseMip, UI32 levelCount, UI32 baseArr, UI32 layerCount, VkFormat format) {

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        barrier.image = image;
        // if format supports depth stencil, include in mask
        barrier.subresourceRange.aspectMask = utils::hasStencilComponent(format) ?
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
        // mip mapping
        barrier.subresourceRange.baseMipLevel = baseMip;
        barrier.subresourceRange.levelCount = levelCount;
        // image array
        barrier.subresourceRange.baseArrayLayer = baseArr;
        barrier.subresourceRange.layerCount = layerCount;
        // access masks
        barrier.srcAccessMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | 
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        // declare stages 
        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0,
            0, nullptr, // memory barriers
            0, nullptr, // buffer memory barriers
            1, &barrier // image memory barriers
        );
    }
}