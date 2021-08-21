//
// Buffer class definition
//

#include <hpg/Buffer.h>

#include <common/utils.h>
#include <common/vkinit.h>

void Buffer::cleanupBufferData(const VkDevice& device) {
    vkDestroyBuffer(device, _vkBuffer, nullptr);
    vkFreeMemory(device, _memory, nullptr);
}

void Buffer::copyBufferToImage(const VulkanContext* vkSetup, const VkCommandPool& renderCommandPool, VkBuffer buffer,
	VkImage image, const std::vector<VkBufferImageCopy>& regions) {
    // copying buffer to image
    VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(&vkSetup->device, renderCommandPool);
    // buffer to image copy operations are enqueued thus
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        static_cast<uint32_t>(regions.size()), regions.data());
    utils::endSingleTimeCommands(&vkSetup->device, &vkSetup->graphicsQueue, &commandBuffer, &renderCommandPool);
}

void Buffer::createBuffer(const VulkanContext* vkSetup, Buffer::CreateInfo* createInfo) {
    // fill in the corresponding struct
    VkBufferCreateInfo bufferCreateInfo = vkinit::bufferCreateInfo(createInfo->size, createInfo->usage);
    // attempt to create a buffer
    if (vkCreateBuffer(vkSetup->device, &bufferCreateInfo, nullptr, &createInfo->pBuffer->_vkBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    // created a buffer, but haven't assigned any memory yet, also get the right memory requirements
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkSetup->device, createInfo->pBuffer->_vkBuffer, &memRequirements);

    // allocate the memory for the buffer
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = utils::findMemoryType(&vkSetup->physicalDevice, memRequirements.memoryTypeBits, createInfo->properties);

    // allocate memory for the buffer. In a real world application, not supposed to actually call vkAllocateMemory for every individual buffer. 
    // The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount physical device limit. The right way to 
    // allocate memory for large number of objects at the same time is to create a custom allocator that splits up a single allocation among many 
    // different objects by using the offset parameters seen in other functions
    if (vkAllocateMemory(vkSetup->device, &allocInfo, nullptr, &createInfo->pBuffer->_memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    // associate memory with buffer
    vkBindBufferMemory(vkSetup->device, createInfo->pBuffer->_vkBuffer, createInfo->pBuffer->_memory, 0);
}

void Buffer::copyBuffer(const VulkanContext* vkSetup, const VkCommandPool& commandPool, Buffer::CopyInfo* bufferCopyInfo) {
    // memory transfer operations are executed using command buffers, like drawing commands. We need to allocate a temporary command buffer
    // could use a command pool for these short lived operations using the flag VK_COMMAND_POOL_CREATE_TRANSIENT_BIT 
    VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(&vkSetup->device, commandPool);
    vkCmdCopyBuffer(commandBuffer, *bufferCopyInfo->pSrc, *bufferCopyInfo->pDst, 1, &bufferCopyInfo->copyRegion);
    utils::endSingleTimeCommands(&vkSetup->device, &vkSetup->graphicsQueue, &commandBuffer, &commandPool);
}

void Buffer::createDeviceLocalBuffer(const VulkanContext* vkSetup, const VkCommandPool& commandPool, 
    const BufferData& bufferData, Buffer* vkBuffer, VkBufferUsageFlagBits usage) {
    Buffer stagingBuffer;

    Buffer::CreateInfo createInfo{};
    createInfo.size = bufferData._size;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    createInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    createInfo.pBuffer = &stagingBuffer;

    Buffer::createBuffer(vkSetup, &createInfo);

    void* data;
    vkMapMemory(vkSetup->device, stagingBuffer._memory, 0, bufferData._size, 0, &data);
    memcpy(data, bufferData._data, bufferData._size);
    vkUnmapMemory(vkSetup->device, stagingBuffer._memory);

    // different usage bit flag VK_BUFFER_USAGE_INDEX_BUFFER_BIT instead of VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
    createInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    createInfo.pBuffer = vkBuffer;

    Buffer::createBuffer(vkSetup, &createInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferData._size;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;

    Buffer::CopyInfo copyInfo{};
    copyInfo.pSrc = &stagingBuffer._vkBuffer;
    copyInfo.pDst = &vkBuffer->_vkBuffer;
    copyInfo.copyRegion = copyRegion;

    Buffer::copyBuffer(vkSetup, commandPool, &copyInfo);

    stagingBuffer.cleanupBufferData(vkSetup->device);
}