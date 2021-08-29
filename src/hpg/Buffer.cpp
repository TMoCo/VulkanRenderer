//
// Buffer class definition
//

#include <hpg/Buffer.h>

#include <common/utils.h>
#include <common/vkinit.h>
#include <common/commands.h>

void Buffer::cleanupBufferData(VkDevice device) {
    vkDestroyBuffer(device, _vkBuffer, nullptr);
    vkFreeMemory(device, _memory, nullptr);
}

void Buffer::copyBufferToImage(const VulkanContext* vkSetup, const VkCommandPool& renderCommandPool, VkBuffer buffer,
	VkImage image, const std::vector<VkBufferImageCopy>& regions) {
    // copying buffer to image
    VkCommandBuffer commandBuffer = cmd::beginSingleTimeCommands(vkSetup->device, renderCommandPool);
    // buffer to image copy operations are enqueued thus
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        static_cast<uint32_t>(regions.size()), regions.data());
    cmd::endSingleTimeCommands(vkSetup->device, vkSetup->graphicsQueue, commandBuffer, renderCommandPool);
}

Buffer Buffer::createBuffer(const VulkanContext& context, UI64 size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBufferCreateFlags flags) {
    // initialise an empty buffer
    Buffer newBuffer{};

    // fill in the corresponding struct
    VkBufferCreateInfo bufferCreateInfo = vkinit::bufferCreateInfo(size, usage);
    // attempt to create a buffer
    if (vkCreateBuffer(context.device, &bufferCreateInfo, nullptr, &newBuffer._vkBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    // created a buffer, but haven't assigned any memory yet, also get the right memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(context.device, newBuffer._vkBuffer, &memRequirements);

    // allocate the memory for the buffer
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = utils::findMemoryType(context.physicalDevice, memRequirements.memoryTypeBits, properties);

    // allocate memory for the buffer. In a real world application, not supposed to actually call vkAllocateMemory for every individual buffer. 
    // The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount physical device limit. The right way to 
    // allocate memory for large number of objects at the same time is to create a custom allocator that splits up a single allocation among many 
    // different objects by using the offset parameters seen in other functions
    if (vkAllocateMemory(context.device, &allocInfo, nullptr, &newBuffer._memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    // associate memory with buffer
    vkBindBufferMemory(context.device, newBuffer._vkBuffer, newBuffer._memory, 0);

    return newBuffer;
}

void Buffer::copyBuffer(const VulkanContext* vkSetup, const VkCommandPool& commandPool, Buffer::CopyInfo* bufferCopyInfo) {
    // memory transfer operations are executed using command buffers, like drawing commands. We need to allocate a temporary command buffer
    // could use a command pool for these short lived operations using the flag VK_COMMAND_POOL_CREATE_TRANSIENT_BIT 
    VkCommandBuffer commandBuffer = cmd::beginSingleTimeCommands(vkSetup->device, commandPool);
    vkCmdCopyBuffer(commandBuffer, *bufferCopyInfo->pSrc, *bufferCopyInfo->pDst, 1, &bufferCopyInfo->copyRegion);
    cmd::endSingleTimeCommands(vkSetup->device, vkSetup->graphicsQueue, commandBuffer, commandPool);
}

Buffer Buffer::createDeviceLocalBuffer(const VulkanContext* vkSetup, const VkCommandPool& commandPool, 
    const BufferData& bufferData, VkBufferUsageFlagBits usage) {

    Buffer stagingBuffer = createBuffer(*vkSetup, bufferData._size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* data;
    vkMapMemory(vkSetup->device, stagingBuffer._memory, 0, bufferData._size, 0, &data);
    memcpy(data, bufferData._data, bufferData._size);
    vkUnmapMemory(vkSetup->device, stagingBuffer._memory);

    Buffer deviceLocalBuffer = Buffer::createBuffer(*vkSetup, bufferData._size, 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferData._size;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;

    Buffer::CopyInfo copyInfo{};
    copyInfo.pSrc = &stagingBuffer._vkBuffer;
    copyInfo.pDst = &deviceLocalBuffer._vkBuffer;
    copyInfo.copyRegion = copyRegion;

    Buffer::copyBuffer(vkSetup, commandPool, &copyInfo);

    stagingBuffer.cleanupBufferData(vkSetup->device);

    return deviceLocalBuffer;
}