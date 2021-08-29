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
    //-Initialisation and cleanup--------------------------------------------------------------------------------//    
    void init(VulkanContext* pContext);
    void cleanup();

    inline VkSwapchainKHR* get() { return &_swapChain; }
    inline VkExtent2D extent() { return _extent; }
    inline VkFormat format() { return _format; }
    inline UI32 imageCount() { return _imageCount; }

private:
    //-Swap chain creation helpers-------------------------------------------------------------------------------//    
    void createSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

public:
    //-Members---------------------------------------------------------------------------------------------------//    
    VulkanContext* _context;

    VkSwapchainKHR _swapChain;
    
    VkExtent2D _extent;
    VkFormat   _format;
    
    F32 _aspectRatio;
    UI32 _imageCount;
    std::vector<VkImage>     _images;
    std::vector<VkImageView> _imageViews;
};

#endif // !VULKAN_SWAP_CHAIN_H
