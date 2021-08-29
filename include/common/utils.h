//
// A convenience header file for constants etc. used in the application
//

#ifndef UTILS_H
#define UTILS_H

#include <common/types.h>

#include <stdint.h> // UI32

#include <vector> // vector container
#include <string> // string class
#include <optional> // optional wrapper
#include <ios> // streamsize

// reporting and propagating exceptions
#include <iostream> 
#include <stdexcept>

#include <vulkan/vulkan_core.h> // vulkan core structs &c

// vectors, matrices ...
#include <glm/glm.hpp>


/* ********************************************** Debug Copy Paste ************************************************
#ifndef NDEBUG
    std::cout << "DEBUG\t" << __FUNCTION__ << '\n';
    std::cout << v << '\n';
    std::cout << std::endl;
    for (auto& v : arr) {

    }
#endif
* ****************************************************************************************************************/

#ifndef NDEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

//#define VERBOSE
#ifdef VERBOSE
const bool enableVerboseValidation = true;
#else
const bool enableVerboseValidation = false;
#endif

// only works for statically defined arrays
#define StaticArraySize(arr)          ((size_t)(sizeof(arr) / sizeof(*(arr))))

const size_t N_DESCRIPTOR_LAYOUTS = 2;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

const size_t MAX_FRAMES_IN_FLIGHT = 2;

const UI32 IMGUI_POOL_NUM = 1000;

namespace utils {
    //-Command queue family info --------------------------------------------------------------------------------//
    struct QueueFamilyIndices {
        std::optional<UI32> graphicsFamily; // queue supporting drawing commands
        std::optional<UI32> presentFamily; // queue for presenting image to vk surface 

        inline bool isComplete() {
            // if device supports drawing cmds AND image can be presented to surface
            return graphicsFamily.has_value() && presentFamily.has_value();
        }

        static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
    };

    //-Type conversion-------------------------------------------------------------------------------------------//
    glm::vec2* toVec2(F32* pVec);
    glm::vec3* toVec3(F32* pVec);
    glm::vec4* toVec4(F32* pVec);

    //-Image formats---------------------------------------------------------------------------------------------//
    VkFormat findSupportedFormat(const VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates,
        VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat findDepthFormat(const VkPhysicalDevice physicalDevice);

    //-Memory type-----------------------------------------------------------------------------------------------//
    UI32 findMemoryType(VkPhysicalDevice physicalDevice, UI32 typeFilter, VkMemoryPropertyFlags properties);

    //-Texture operation info structs----------------------------------------------------------------------------//
    bool hasStencilComponent(VkFormat format);
}

#endif // !UTILS_H
