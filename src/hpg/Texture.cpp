//
// Texture class definition
//

#include <hpg/Texture.h>

#include <common/utils.h> // utils namespace
#include <common/vkinit.h> // utils namespace

// image loading
#include <stb_image.h>


void Texture::createTexture(VulkanContext* pVkSetup, const VkCommandPool& commandPool, const ImageData& imageData) {
    vkSetup = pVkSetup;

    Buffer stagingBuffer; // staging buffer containing image in host memory

    Buffer::CreateInfo createInfo{};
    createInfo.size = imageData.pixels._size;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    createInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    createInfo.pBuffer = &stagingBuffer;

    Buffer::createBuffer(vkSetup, &createInfo);

    void* data;
    vkMapMemory(vkSetup->device, stagingBuffer._memory, 0, imageData.pixels._size, 0, &data);
    memcpy(data, imageData.pixels._data, imageData.pixels._size);
    vkUnmapMemory(vkSetup->device, stagingBuffer._memory);

    // create the image and its memory
    Image::ImageCreateInfo imgCreateInfo{};
    imgCreateInfo.extent = { imageData.width, imageData.height };
    imgCreateInfo.format = imageData.format;
    imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgCreateInfo.usage = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    imgCreateInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imgCreateInfo.pImage = &textureImage;

    Image::createImage(vkSetup, commandPool, imgCreateInfo);

    // copy host data to device
    Image::LayoutTransitionInfo transitionData{};
    transitionData.pImage = &textureImage;
    transitionData.renderCommandPool = commandPool;
    transitionData.format = imageData.format;
    transitionData.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transitionData.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    Image::transitionImageLayout(vkSetup, transitionData); // specify the initial layout VK_IMAGE_LAYOUT_UNDEFINED

    // need to specify which parts of the buffer we are going to copy to which part of the image
    std::vector<VkBufferImageCopy> regions = {
        { 0, 0, 0, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }, { 0, 0, 0 }, { imageData.width, imageData.height, 1 } }
    };

    Buffer::copyBufferToImage(vkSetup, commandPool, stagingBuffer._vkBuffer, textureImage._vkImage, regions);

    // need another transfer to give the shader access to the texture
    transitionData.pImage = &textureImage;
    transitionData.renderCommandPool = commandPool;
    transitionData.format = imageData.format;
    transitionData.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transitionData.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    Image::transitionImageLayout(vkSetup, transitionData);

    // cleanup the staging buffer and its memory
    stagingBuffer.cleanupBufferData(vkSetup->device);

    // then create the image view
    VkImageViewCreateInfo imageViewCreateInfo = vkinit::imageViewCreateInfo(textureImage._vkImage,
        VK_IMAGE_VIEW_TYPE_2D, imageData.format, {}, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
    textureImageView = Image::createImageView(vkSetup, imageViewCreateInfo);

    // create the sampler
    createTextureSampler();
}

void Texture::cleanupTexture() {
    // destroy the texture image view and sampler
    vkDestroySampler(vkSetup->device, textureSampler, nullptr);
    vkDestroyImageView(vkSetup->device, textureImageView, nullptr);

    // destroy the texture image and its memory
    textureImage.cleanupImage(vkSetup);
}

void Texture::createTextureSampler() {
    // configure the sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // how to interpolate texels that are magnified or minified
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    // addressing mode
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
    // VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
    // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : Take the color of the edge closest to the coordinate beyond the image dimensions.
    // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE : Like clamp to edge, but instead uses the edge opposite to the closest edge.
    // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : Return a solid color when sampling beyond the dimensions of the image

    // use unless performance is a concern
    samplerInfo.anisotropyEnable = VK_TRUE; 

    // limits texel samples that used to calculate final colours
    samplerInfo.maxAnisotropy = vkSetup->deviceProperties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    // which coordinate system we want to use to address texels!
    samplerInfo.unnormalizedCoordinates = VK_FALSE; 

    // if comparison enabled, texels will be compared to a value and result is used in filtering (useful for shadow maps)
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    // mipmapping fields
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    // now create the configured sampler
    if (vkCreateSampler(vkSetup->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}