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

#include <hpg/VulkanSetup.h> // for referencing the device
#include <hpg/DepthResource.h> // for referencing the depth resource

#include <vector> // vector container

#include <vulkan/vulkan_core.h>

class SwapChain {
public:
    //-----------------------------------------------------------------------------------------------------------//
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };
public:
    //-Initialisation and cleanup--------------------------------------------------------------------------------//    
    void createSwapChain(VulkanSetup* pVkSetup, Model* model, VkDescriptorSetLayout* descriptorSetLayout);
    void cleanupSwapChain();

    static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

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
    void createForwardPipeline(VkDescriptorSetLayout* descriptorSetLayout, Model* model);



public:
    //-Members---------------------------------------------------------------------------------------------------//    
    VulkanSetup* vkSetup;

    VkSwapchainKHR           swapChain;
    
    VkExtent2D               extent;
    VkFormat                 imageFormat;
    std::vector<VkImage>     images;
    std::vector<VkImageView> imageViews;
    
    SwapChainSupportDetails  supportDetails;

    VkRenderPass     renderPass;
    VkRenderPass     imGuiRenderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline       pipeline;

    bool enableDepthTest = true; // default
};

#endif // !VULKAN_SWAP_CHAIN_H
