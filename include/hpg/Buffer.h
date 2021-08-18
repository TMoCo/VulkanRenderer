///////////////////////////////////////////////////////
// Buffer class declaration and Buffer struct
///////////////////////////////////////////////////////

//
// The Buffer class is a wrapper class for a vkBuffer and its vkMemory for ease of
// access. It also contains some static helper functions for creating buffers and some
// buffer operations like copying buffers. 
// The Buffer struct is a minimal struct that contains information on a buffer, essentially
// a pointer to some contiguous memory and the size of the buffer, useful for creating a vulkan
// buffer.
//

#ifndef BUFFERS_H
#define BUFFERS_H

#include <hpg/VulkanContext.h>

#include <vulkan/vulkan_core.h> // vulkan core structs &c


// POD Buffer struct
struct BufferData {
    unsigned char* _data;
    size_t _size;
};

class Buffer {
public:
    //-Buffer operation info structs--------------------------------------//
    // TODO: Need refactoring...
    struct CreateInfo {
        VkDeviceSize          size       = 0;
        VkBufferUsageFlags    usage      = 0;
        VkMemoryPropertyFlags properties = 0;
        Buffer*               pBuffer    = nullptr; // ptr to the buffer object we want to create
    };

    struct CopyInfo {
        VkBuffer*    pSrc = nullptr;
        VkBuffer*    pDst = nullptr;
        VkBufferCopy copyRegion{};
    };

public:

    //-Buffer creation and cleanup-------------------------------------------------------------------------------//
    static void createBuffer(const VulkanContext* vkSetup, CreateInfo* bufferCreateInfo);
    void cleanupBufferData(const VkDevice& device);

    //-Buffer copying--------------------------------------------------------------------------------------------//
    static void copyBuffer(const VulkanContext* vkSetup, const VkCommandPool& commandPool, CopyInfo* bufferCopyInfo);
    static void copyBufferToImage(const VulkanContext* vkSetup, const VkCommandPool& renderCommandPool, 
        VkBuffer buffer, VkImage image, const std::vector<VkBufferImageCopy>& regions);

    //-Buffer creation on GPU------------------------------------------------------------------------------------//
    static void createDeviceLocalBuffer(const VulkanContext* vkSetup, const VkCommandPool& commandPool, 
        const BufferData& buffer, Buffer* vkBuffer, VkBufferUsageFlagBits usage);

    //-Utility uniform buffer creation---------------------------------------------------------------------------//
    template<typename T>
    static void createUniformBuffer(const VulkanContext* vkSetup, VkDeviceSize imagesSize, Buffer* buffer, VkMemoryPropertyFlags properties);

public:
    VkBuffer       _vkBuffer = nullptr;
    VkDeviceMemory _memory = nullptr;
};

//
// Template definitions
//

template <typename T>
void Buffer::createUniformBuffer(const VulkanContext* vkSetup, VkDeviceSize imagesSize, Buffer* pBuffer, VkMemoryPropertyFlags properties) {
    Buffer::CreateInfo createInfo { imagesSize * sizeof(T), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, properties, pBuffer };
    Buffer::createBuffer(vkSetup, &createInfo);
}

#endif // !BUFFERS_H