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
    Texture() : _onGpu(false), _image(nullptr), _memory(nullptr), _imageView(nullptr), _sampler(nullptr) {}

    virtual bool uploadToGpu(const Renderer& renderer, const ImageData& imageData) = 0;

    inline void cleanup(VkDevice device) {
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
