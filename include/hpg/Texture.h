///////////////////////////////////////////////////////
// Texture class declaration
///////////////////////////////////////////////////////

//
// A texture class for handling texture related operations and data.
//

#ifndef TEXTURE_H
#define TEXTURE_H

#include <hpg/Buffer.h>
#include <hpg/Renderer.h>
#include <hpg/Image.h>

#include <vulkan/vulkan_core.h>

class Texture {
public:
    // must be overriden by subsequent texture types
    virtual bool uploadToGpu(const Renderer& renderer, const ImageData& imageData) = 0;

    inline void cleanupTexture(VkDevice device) {
        if (_onGpu) {
            vkDestroySampler(device, _sampler, nullptr);
            vkDestroyImageView(device, _imageView, nullptr);
            vkDestroyImage(device, _image, nullptr);
            vkFreeMemory(device, _memory, nullptr);
        }
    }

    bool _onGpu;

    VkImage _image;
    VkDeviceMemory _memory;
    VkImageView _imageView;
    VkSampler _sampler;
};

#endif // !TEXTURE_H
