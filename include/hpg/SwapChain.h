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

#include <common/Model.h>

#include <hpg/VulkanContext.h> // for referencing the device
#include <hpg/DepthResource.h> // for referencing the depth resource

#include <vector> // vector container

#include <vulkan/vulkan_core.h>

class SwapChain {
public:
    //-----------------------------------------------------------------------------------------------------------//
    struct SupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };
public:
    //-Initialisation and cleanup--------------------------------------------------------------------------------//    
    void init(VulkanContext* pVkSetup);
    void cleanup();

    static SupportDetails querySwapChainSupport(VulkanContext* pVkContext);

private:
    //-Swap chain creation helpers-------------------------------------------------------------------------------//    
    void createSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    // TODO: move render pass and pipelines outside of swap chain
    //-Render passes---------------------------------------------------------------------------------------------//    
    void createRenderPass();
    void createImGuiRenderPass();
    
    //-Pipelines-------------------------------------------------------------------------------------------------//  
    void createForwardPipeline(VkDescriptorSetLayout* descriptorSetLayout);

public:
    //-Members---------------------------------------------------------------------------------------------------//    
    VulkanContext* _vkContext;

    VkSwapchainKHR _swapChain;
    
    VkExtent2D _extent;
    VkFormat   _format;
    
    F32 _aspectRatio;
    UI32 _imageCount;
    std::vector<VkImage>     _images;
    std::vector<VkImageView> _imageViews;
    
    SupportDetails  _supportDetails;

    VkRenderPass _renderPass;
    VkRenderPass _guiRenderPass;

    VkPipelineLayout _pipelineLayout;
    VkPipeline       _pipeline;
};

#endif // !VULKAN_SWAP_CHAIN_H
