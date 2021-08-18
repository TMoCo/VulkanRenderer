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
    void createSwapChain(VulkanContext* pVkSetup, VkDescriptorSetLayout* descriptorSetLayout);
    void cleanupSwapChain();

    static SupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

private:
    //-Swap chain creation helpers-------------------------------------------------------------------------------//    
    void createSwapChain();
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    //-Render passes---------------------------------------------------------------------------------------------//    
    void createRenderPass();
    void createImGuiRenderPass();
    
    //-Pipelines-------------------------------------------------------------------------------------------------//  
    void createForwardPipeline(VkDescriptorSetLayout* descriptorSetLayout);



public:
    //-Members---------------------------------------------------------------------------------------------------//    
    VulkanContext* vkSetup;

    VkSwapchainKHR           swapChain;
    
    VkExtent2D               extent;
    VkFormat                 imageFormat;
    std::vector<VkImage>     images;
    std::vector<VkImageView> imageViews;
    
    SupportDetails  supportDetails;

    VkRenderPass     renderPass;
    VkRenderPass     imGuiRenderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;

    bool enableDepthTest = true; // default
};

#endif // !VULKAN_SWAP_CHAIN_H
