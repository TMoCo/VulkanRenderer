#include <hpg/TextureCube.h>

#include <common/vkinit.h>
#include <common/commands.h>

bool TextureCube::uploadToGpu(const Renderer& renderer, const ImageData& imageData) {
    if (_onGpu) {
        return _onGpu;
    }

    // create image and allocate image memory on Gpu
    {
        VkImageCreateInfo imageCreateInfo = vkinit::imageCreateInfo(imageData.format, imageData.extent, 1, 6,
            VK_IMAGE_TILING_OPTIMAL, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT); // cube texture flag
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        if (vkCreateImage(renderer._context.device, &imageCreateInfo, nullptr, &_image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        // get memory requirements for this particular image
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(renderer._context.device, _image, &memRequirements);

        VkMemoryAllocateInfo allocInfo = vkinit::memoryAllocateInfo(memRequirements.size,
            utils::findMemoryType(renderer._context.physicalDevice, memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        // allocate memory on device
        if (vkAllocateMemory(renderer._context.device, &allocInfo, nullptr, &_memory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }
        
        // bind image to memory
        vkBindImageMemory(renderer._context.device, _image, _memory, 0);
    }

    Buffer stagingBuffer;
    
    // copy pixels on cpu into a staging buffer
    {
        stagingBuffer = Buffer::createBuffer(renderer._context, imageData.pixels._size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(renderer._context.device, stagingBuffer._memory, 0, imageData.pixels._size, 0, &data);
        memcpy(data, imageData.pixels._data, imageData.pixels._size);
        vkUnmapMemory(renderer._context.device, stagingBuffer._memory);
    }

    // copy host data to device
    {
        VkCommandBuffer commandBuffer;

        // transition layout from undefined to transfer destination
        commandBuffer = cmd::beginSingleTimeCommands(renderer._context.device, 
            renderer._commandPools[RENDER_CMD_POOL]);
        cmd::transitionLayoutUndefinedToTransferDest(commandBuffer, _image, 0, 1, 0, 6);
        cmd::endSingleTimeCommands(renderer._context.device, renderer._context.graphicsQueue, commandBuffer,
            renderer._commandPools[RENDER_CMD_POOL]);

        // need to specify which parts of the buffer we are going to copy to which part of the image
        std::vector<VkBufferImageCopy> regions;
        
        int offset = 0;
        // loop over the 6 faces and create the buffer copy regions
        for (int i = 0; i < 6; i++) {
            VkBufferImageCopy region = {};
            region.bufferOffset = offset;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.baseArrayLayer = i;
            region.imageSubresource.layerCount = 1;
            region.imageSubresource.mipLevel = 0;
            region.imageExtent = imageData.extent;
            regions.push_back(region);
            // increment offset into staging buffer
            offset += imageData.extent.width * imageData.extent.height * 4; // TODO remove hardcoded channel depth
        }

        Buffer::copyBufferToImage(&renderer._context, renderer._commandPools[RENDER_CMD_POOL], stagingBuffer._vkBuffer, _image, regions);

        // transfer layout from transfer dest to shader read
        commandBuffer = cmd::beginSingleTimeCommands(renderer._context.device,
            renderer._commandPools[RENDER_CMD_POOL]);
        cmd::transitionLayoutTransferDestToFragShaderRead(commandBuffer, _image, 0, 1, 0, 6);
        cmd::endSingleTimeCommands(renderer._context.device, renderer._context.graphicsQueue, commandBuffer,
            renderer._commandPools[RENDER_CMD_POOL]);
    }

    // cleanup the staging buffer and its memory
    stagingBuffer.cleanupBufferData(renderer._context.device);

    // create image view
    {
        VkImageViewCreateInfo imageViewCreateInfo = vkinit::imageViewCreateInfo(_image,
            VK_IMAGE_VIEW_TYPE_CUBE, imageData.format, {}, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 });
        _imageView = Image::createImageView(&renderer._context, imageViewCreateInfo);

    }

    // create the sampler
    {
        // TODO: get sampler config from gltf model
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
        samplerInfo.maxAnisotropy = renderer._context.deviceProperties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        // which coordinate system we want to use to address texels!
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        // if comparison enabled, texels will be compared to a value and result is used in filtering (useful for shadow maps)
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

        // mipmapping fields
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        // now create the configured sampler
        if (vkCreateSampler(renderer._context.device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    _onGpu = true;
    return _onGpu;
}
