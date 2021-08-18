//
// VulkanImage class definition
//

//
// A wrapper for a VkImage and its associated view and memory, along with some helper 
// image utility static functions
//

#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include <common/types.h>

#include <hpg/Buffer.h>

#include <vulkan/vulkan_core.h>

// POD struct for an image from file (.png, .jpeg, &c)
struct ImageData {
    UI32 width;
    UI32 height;
    VkFormat format;
    BufferData pixels;
};

class Image {
public:
    //-Texture operation info structs-------------------------------------//
    struct ImageCreateInfo {
        uint32_t              width  = 0;
        uint32_t              height = 0;
        VkFormat              format = VK_FORMAT_UNDEFINED;
        VkImageTiling         tiling = VK_IMAGE_TILING_OPTIMAL;
        VkImageUsageFlags     usage = VK_NULL_HANDLE;
        uint32_t              arrayLayers = 1; // default to 1 for convenience
        VkMemoryPropertyFlags properties = VK_NULL_HANDLE;
        VkImageCreateFlags    flags = 0;
        Image*                pImage = nullptr;
    };

    struct LayoutTransitionInfo {
        Image*        pImage      = nullptr;
        VkCommandPool renderCommandPool = VK_NULL_HANDLE;
        VkFormat      format            = VK_FORMAT_UNDEFINED;
        VkImageLayout oldLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageLayout newLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t      arrayLayers       = 1;
    };

    //-Querying a format's support----------------------------------------//
    struct ImageFormatSupportDetails {
        VkFormat format;
        VkImageFormatProperties properties;
    };

public:
    //-Initialisation and cleanup-----------------------------------------//
    static void createImage(const VulkanContext* vkSetup, const VkCommandPool& commandPool, const ImageCreateInfo& info);
    void cleanupImage(const VulkanContext* vkSetup);

    //-Image view creation------------------------------------------------//
    static VkImageView createImageView(const VulkanContext* vkSetup, const VkImageViewCreateInfo& imageViewCreateInfo);

    //-Image layout transition--------------------------------------------//
    static void transitionImageLayout(const VulkanContext* vkSetup, const LayoutTransitionInfo& transitionInfo);

    //-Image Loading from file--------------------------------------------//
    static ImageData loadImageFromFile(const std::string& path);

    //-Helpers for image formats------------------------------------------//
    static VkFormat getImageFormat(int numChannels);
    static ImageFormatSupportDetails queryFormatSupport(VkPhysicalDevice device, VkFormat format, VkImageType type, 
        VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags);
    static VkBool32 formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling);

public:
    VkExtent2D     _extent        = { 0, 0 };
    VkFormat       _format        = VK_FORMAT_UNDEFINED;
    VkImage        _vkImage       = nullptr;
    VkDeviceMemory _memory        = nullptr;
};

#endif // !VULKAN_IMAGE_H