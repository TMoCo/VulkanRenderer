//
// VulkanImage class definition
//

//
// A wrapper for a VkImage and its associated view and memory, along with some helper 
// image utility static functions
//

#ifndef VULKAN_IMAGE_H
#define VULKAN_IMAGE_H

#include <hpg/Buffer.h>

#include <common/types.h>

#include <vulkan/vulkan_core.h>

// POD struct for an image from file (.png, .jpeg, &c)
struct ImageData {
    VkExtent3D extent;
    VkFormat format;
    BufferData pixels;
};

class Image {
public:
    //-Texture operation info structs-------------------------------------//
    struct ImageCreateInfo {
        VkExtent2D            extent = { 0, 0 };
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

    //-Image view creation------------------------------------------------//
    static VkImageView createImageView(const VulkanContext* vkSetup, const VkImageViewCreateInfo& imageViewCreateInfo);

    //-Image Loading from file--------------------------------------------//
    static ImageData loadImageFromFile(const std::string& path);

    //-Helpers for image formats------------------------------------------//
    static VkFormat getImageFormat(int numChannels);
    static ImageFormatSupportDetails queryFormatSupport(VkPhysicalDevice device, VkFormat format, VkImageType type, 
        VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags);
    static VkBool32 formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling);

};

#endif // !VULKAN_IMAGE_H