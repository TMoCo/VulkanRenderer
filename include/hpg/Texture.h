///////////////////////////////////////////////////////
// Texture class declaration
///////////////////////////////////////////////////////

//
// A texture class for handling texture related operations and data.
//

#ifndef TEXTURE_H
#define TEXTURE_H

#include <hpg/Buffer.h>
#include <hpg/Image.h>
#include <hpg/VulkanContext.h>

#include <string> // string class

#include <vulkan/vulkan_core.h>

class Texture {
public:
    //-Initialisation and cleanup----------------------------------------//    
    void createTexture(VulkanContext* pVkSetup, const VkCommandPool& commandPool, const ImageData& imageData);
    void cleanupTexture();

private:
    //-Texture sampler creation------------------------------------------//    
    void createTextureSampler();

public:
    //-Members-----------------------------------------------------------//    
    VulkanContext* vkSetup;

    Image    textureImage;
    VkImageView    textureImageView = nullptr;

    VkSampler textureSampler;
};

#endif // !TEXTURE_H
