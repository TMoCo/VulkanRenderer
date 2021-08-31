///////////////////////////////////////////////////////
// SwapChainData class declaration
///////////////////////////////////////////////////////

//
// A class that contains the swap chain setup and data. It has a create function
// to facilitate the recreation when a window is resized. It contains all the variables
// that depend on the VkSwapChainKHR object.
//

#ifndef VULKAN_SWAP_CHAIN_H
#define VULKAN_SWAP_CHAIN_H

#include <hpg/VulkanContext.h> // for referencing the device

#include <vulkan/vulkan_core.h>

#include <vector> // vector container

class SwapChain {
    friend class Renderer;
public:
    SwapChain() : 
        _imageCount(0), 
        _aspectRatio(0), 
        _swapChain(nullptr), 
        _extent({0, 0}), 
        _surfaceFormat({ VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR }) {}

    //-Initialisation and cleanup--------------------------------------------------------------------------------//    
    bool create(VulkanContext& context);
    void cleanup(VkDevice device);

    inline VkSwapchainKHR* get() { return &_swapChain; }
    inline VkExtent2D extent() { return _extent; }
    inline VkFormat format() { return _surfaceFormat.format; }
    inline UI32 imageCount() { return _imageCount; }

private:
    //-Swap chain creation helpers-------------------------------------------------------------------------------//
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);

public:
    //-Members---------------------------------------------------------------------------------------------------//
    UI32 _imageCount;
    F32 _aspectRatio;

    VkSwapchainKHR _swapChain;
    
    VkExtent2D _extent;
    VkSurfaceFormatKHR _surfaceFormat;
    
    std::vector<VkImage> _images;
    std::vector<VkImageView> _imageViews;
};

#endif // !VULKAN_SWAP_CHAIN_H
