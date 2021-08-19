///////////////////////////////////////////////////////
// FrameBuffer class declaration
///////////////////////////////////////////////////////

//
// A class that contains the data related to a frame buffer.
//

#ifndef FRAME_BUFFER_H
#define FRAME_BUFFER_H

#include <hpg/VulkanContext.h> // for referencing the device
#include <hpg/SwapChain.h> // for referencing the swap chain

#include <vulkan/vulkan_core.h>

class FrameBuffer {
public:
    //-Initialisation and cleanup-----------------------------------------//    
    void createFrameBuffer(VulkanContext* pVkSetup, const SwapChain* swapChain, const VkCommandPool& commandPool);
    void cleanupFrameBuffers();

private:
    //-Framebuffer creation-----------------------------------------------//    
    void createFrameBuffers(const SwapChain* swapChain);
    void createImGuiFramebuffers(const SwapChain* swapChain);

public:
    //-Members------------------------------------------------------------//    
    VulkanContext* vkSetup;

    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkFramebuffer> imGuiFramebuffers;
};


#endif // !FRAMEBUFFER_DATA_H

