//
// the utils namespace definition
//

#include <common/utils.h>

#include <glm/gtc/type_ptr.hpp> // construct vec from ptr

namespace utils {
    QueueFamilyIndices QueueFamilyIndices::findQueueFamilies(const VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        QueueFamilyIndices indices;

        // similar to physical device and extensions and layers....
        UI32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        // create a vector to store queue families
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            // if the queue supports the desired queue operation, then the bitwise & operator returns true
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                // gaphics family was assigned a value! optional wrapper has_value now returns true.
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            // checks device queuefamily can present on the surface
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            // return the first valid queue family
            if (indices.isComplete()) {
                break;
            }
            i++;
        }
        return indices;
    }

    glm::vec2* toVec2(F32* pVec) {
        return reinterpret_cast<glm::vec2*>(pVec);
    }

    glm::vec3* toVec3(F32* pVec) {
        return reinterpret_cast<glm::vec3*>(pVec);
    }

    glm::vec4* toVec4(F32* pVec) {
        return reinterpret_cast<glm::vec4*>(pVec);
    }

    UI32 findMemoryType(VkPhysicalDevice physicalDevice, UI32 typeFilter, VkMemoryPropertyFlags properties) {
        // find best type of memory
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
        // two arrays in the struct, memoryTypes and memoryHeaps. Heaps are distinct ressources like VRAM and swap space in RAM
        // types exist within these heaps

        for (UI32 i = 0; i < memProperties.memoryTypeCount; i++) {
            // we want a memory type that is suitable for the vertex buffer, but also able to write our vertex data to memory
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        // otherwise we can't find the right type!
        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkFormat findSupportedFormat(const VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates, 
        VkImageTiling tiling, VkFormatFeatureFlags features) {
        // instead of a fixed format, get a list of formats ranked from most to least desirable and iterate through it
        for (VkFormat format : candidates) {
            // query the support of the format by the device
            VkFormatProperties props; // contains three fields
            // linearTilingFeatures: Use cases that are supported with linear tiling
            // optimalTilingFeatures: Use cases that are supported with optimal tiling
            // bufferFeatures : Use cases that are supported for buffer
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

            // test if the format is supported
            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        // could either return a special value or throw an exception
        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat(const VkPhysicalDevice physicalDevice) {
        // return a certain depth format if available
        return findSupportedFormat(
            physicalDevice,
            // list of candidate formats
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            // the desired tiling
            VK_IMAGE_TILING_OPTIMAL,
            // the device format properties flag that we want 
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    bool hasStencilComponent(VkFormat format) {
        // depth formats with stencil components
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; 
    }
}