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

#define SizeofArray(arr)          ((size_t)(sizeof(arr) / sizeof(*(arr))))


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

    //-Memory type-----------------------------------------------------------------------------------------------//
    UI32 findMemoryType(const VkPhysicalDevice* physicalDevice, UI32 typeFilter, VkMemoryPropertyFlags properties);

    //-Begin & end single use cmds-------------------------------------------------------------------------------//
    VkCommandBuffer beginSingleTimeCommands(const VkDevice* device, const VkCommandPool& commandPool);
    void endSingleTimeCommands(const VkDevice* device, const VkQueue* queue, const VkCommandBuffer* commandBuffer, const VkCommandPool* commandPool);

    //-Texture operation info structs----------------------------------------------------------------------------//
    bool hasStencilComponent(VkFormat format);

    //-Vulkan struct initialisation------------------------------------------------------------------------------//
    // heavily inspired from: https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanInitializers.hpp

    // Pipeline structs
    VkPipelineVertexInputStateCreateInfo initPipelineVertexInputStateCreateInfo(
        UI32 bindingCount,
        VkVertexInputBindingDescription* pVertexBindingDescriptions,
        UI32 attributesCount, 
        VkVertexInputAttributeDescription* pVertexAttributesDescriptions,
        VkPipelineVertexInputStateCreateFlags flags = 0);

    VkPipelineShaderStageCreateInfo initPipelineShaderStageCreateInfo(
        VkShaderStageFlagBits stage,
        VkShaderModule& shader,
        const char* name);

    VkPipelineInputAssemblyStateCreateInfo initPipelineInputAssemblyStateCreateInfo(
        VkPrimitiveTopology topology,
        VkBool32 restartEnabled,
        VkPipelineInputAssemblyStateCreateFlags flags = 0);

    VkPipelineRasterizationStateCreateInfo initPipelineRasterStateCreateInfo(
        VkPolygonMode polyMode,
        VkCullModeFlags cullMode,
        VkFrontFace frontFace,
        VkPipelineRasterizationStateCreateFlags flags = 0,
        F32 lineWidth = 1.0f);

    VkPipelineColorBlendStateCreateInfo initPipelineColorBlendStateCreateInfo(
        UI32 attachmentCount,
        const VkPipelineColorBlendAttachmentState* pAttachments);

    VkPipelineDepthStencilStateCreateInfo initPipelineDepthStencilStateCreateInfo(
        VkBool32 depthTestEnable,
        VkBool32 depthWriteEnable,
        VkCompareOp depthCompareOp);

    VkPipelineViewportStateCreateInfo initPipelineViewportStateCreateInfo(
        UI32 viewportCount,
        VkViewport* pViewports,
        UI32 scissorCount,
        VkRect2D* pScissors,
        VkPipelineViewportStateCreateFlags flags = 0);

    VkPipelineMultisampleStateCreateInfo initPipelineMultisampleStateCreateInfo(
        VkSampleCountFlagBits rasterizationSamples,
        VkPipelineMultisampleStateCreateFlags flags = 0);

    VkPipelineLayoutCreateInfo initPipelineLayoutCreateInfo(
        UI32 layoutCount,
        VkDescriptorSetLayout* layouts,
        VkPipelineLayoutCreateFlags flags = 0);

    VkPipelineLayoutCreateInfo initPipelineLayoutCreateInfo(
        VkDescriptorSetLayout* pSetLayouts,
        UI32 count);

    VkGraphicsPipelineCreateInfo initGraphicsPipelineCreateInfo(
        VkPipelineLayout layout,
        VkRenderPass renderPass,
        VkPipelineCreateFlags flags = 0);

    VkPipelineColorBlendAttachmentState initPipelineColorBlendAttachmentState(
        VkColorComponentFlags mask,
        VkBool32 blendEnable);

    VkPipelineDynamicStateCreateInfo initPipelineDynamicStateCreateInfo(
        VkDynamicState* pDynamicStates,
        UI32 dynamicStateCount,
        VkPipelineDynamicStateCreateFlags flags = 0);

    // Descriptor set structs
    VkDescriptorSetLayoutBinding initDescriptorSetLayoutBinding(
        UI32 binding,
        VkDescriptorType type,
        VkPipelineStageFlags flags = 0);

    VkDescriptorSetAllocateInfo initDescriptorSetAllocInfo(
        VkDescriptorPool pool,
        UI32 count,
        VkDescriptorSetLayout* pDescSetLayouts);

    VkWriteDescriptorSet initWriteDescriptorSet(
        VkDescriptorSet dst,
        UI32 binding,
        VkDescriptorType type,
        VkDescriptorBufferInfo* pBufferInfo);

    VkWriteDescriptorSet initWriteDescriptorSet(
        VkDescriptorSet dst,
        UI32 binding,
        VkDescriptorType type,
        VkDescriptorImageInfo* pImageInfo);

    // Command buffer structs
    VkCommandBufferBeginInfo initCommandBufferBeginInfo(
        VkCommandBufferResetFlags flags = 0);

    // Image structs
    VkImageViewCreateInfo initImageViewCreateInfo(VkImage image, VkImageViewType type, VkFormat format,
        VkComponentMapping componentMapping, VkImageSubresourceRange subresourceRange);

    VkSamplerCreateInfo initSamplerCreateInfo(F32 maxAnisotropy = 1.0f);
}

#endif // !UTILS_H
