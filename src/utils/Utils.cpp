//
// the utils namespace definition
//

#include <utils/utils.h>

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

    UI32 findMemoryType(const VkPhysicalDevice* physicalDevice, UI32 typeFilter, VkMemoryPropertyFlags properties) {
        // find best type of memory
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(*physicalDevice, &memProperties);
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

    VkCommandBuffer beginSingleTimeCommands(const VkDevice* device, const VkCommandPool& commandPool) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool        = commandPool;
        allocInfo.commandBufferCount = 1;

        // allocate the command buffer
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(*device, &allocInfo, &commandBuffer);

        // set the struct for the command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // command buffer usage 

        // start recording the command buffer
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void endSingleTimeCommands(const VkDevice* device, const VkQueue* queue, const VkCommandBuffer* commandBuffer, const VkCommandPool* commandPool) {
        // end recording
        vkEndCommandBuffer(*commandBuffer);

        // execute the command buffer by completing the submitinfo struct
        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = commandBuffer;

        // submit the queue for execution
        vkQueueSubmit(*queue, 1, &submitInfo, VK_NULL_HANDLE);
        // here we could use a fence to schedule multiple transfers simultaneously and wait for them to complete instead
        // of executing all at the same time, alternatively use wait for the queue to execute
        vkQueueWaitIdle(*queue);

        // free the command buffer once the queue is no longer in use
        vkFreeCommandBuffers(*device, *commandPool, 1, commandBuffer);
    }

    bool hasStencilComponent(VkFormat format) {
        // depth formats with stencil components
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; 
    }
}