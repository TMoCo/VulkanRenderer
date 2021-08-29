
#ifndef VERTEX_H
#define VERTEX_H

#include <glm/glm.hpp> 

#include <vulkan/vulkan_core.h>

struct Vertex {
    glm::vec4 positionU; // (posX, posY, posZ, texU)
    glm::vec4 normalV;   // (norX, norY, norZ, texV)
    glm::vec4 tangent;   // (tanx, tanY, tanZ, tanW)

    inline static VkVertexInputBindingDescription getBindingDescriptions(uint32_t primitiveNum) {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = primitiveNum;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    inline static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions(uint32_t primitiveNum) {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, positionU) };
        attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, normalV) };
        attributeDescriptions[2] = { 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent) };
        return attributeDescriptions;
    }
};

#endif // !VERTEX_H