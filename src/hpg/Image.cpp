//
// Image class definition
//

#include <hpg/Image.h>

#include <common/Print.h>
#include <common/commands.h>

// image loading
#include <stb_image.h>

VkImageView Image::createImageView(const VulkanContext* vkSetup, const VkImageViewCreateInfo& imageViewCreateInfo) {
    VkImageView imageView;
    if (vkCreateImageView(vkSetup->device, &imageViewCreateInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
    return imageView;
}

ImageData Image::loadImageFromFile(const std::string& path) {
    ImageData imageData{};

    int width, height, channels;
    UC* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

    if (!data) {
        throw std::runtime_error("could not load image!");
    }

    // allocate memory, make a copy, then delete original
    imageData.pixels._data = (UC*)malloc(width * height * channels * sizeof(UC));
    memcpy(imageData.pixels._data, data, width * height * channels);
    stbi_image_free(data);

    imageData.extent = { static_cast<UI32>(width), static_cast<UI32>(height), 1 };
    imageData.pixels._size = width * height * channels * sizeof(UC);
    imageData.format = getImageFormat(channels);
        
    // !!PIXELS NEED TO BE FREED!! do so when no longer in use at the end of application lifecycle
    return imageData;
}

VkFormat Image::getImageFormat(int numChannels) {
    switch (numChannels) {
    case 1:
        return VK_FORMAT_R8_SRGB;
    case 2:
        return VK_FORMAT_R8G8_SRGB;
    case 3:
        return VK_FORMAT_R8G8B8_SRGB;
    case 4:
        return VK_FORMAT_R8G8B8A8_SRGB;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

Image::ImageFormatSupportDetails Image::queryFormatSupport(VkPhysicalDevice device, VkFormat format, VkImageType type,
    VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags) {
    // given a set of desired image parameters, determine if a format is supported or not
    Image::ImageFormatSupportDetails details = { format, {} };
    if (vkGetPhysicalDeviceImageFormatProperties(device, format, type, tiling, usage, flags, &details.properties) != VK_SUCCESS) {
        print("!!! format %i not supported !!!\n", format);
    }
    return details;
}

VkBool32 Image::formatIsFilterable(VkPhysicalDevice physicalDevice, VkFormat format, VkImageTiling tiling) {
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);

    if (tiling == VK_IMAGE_TILING_OPTIMAL)
        return formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

    if (tiling == VK_IMAGE_TILING_LINEAR)
        return formatProps.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

    return false;
}