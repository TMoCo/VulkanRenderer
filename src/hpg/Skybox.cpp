///////////////////////////////////////////////////////
//
// Skybox class definition
//
///////////////////////////////////////////////////////

#include <app/AppConstants.h>

#include <common/Print.h>
#include <common/Assert.h>
#include <common/vkinit.h>

#include <hpg/Skybox.h>
#include <hpg/Shader.h>
#include <hpg/Buffer.h>

#include <stb_image.h>


bool Skybox::load(const std::string& path) {
    // load the skybox data from the 6 images
    const char* faces[6] = { "Right.png", "Left.png", "Bottom.png", "Top.png", "Front.png", "Back.png" };

    // query the dimensions of the file
    int width, height, channels;
    if (!stbi_info((SKYBOX_PATH + faces[0]).c_str(), &width, &height, &channels)) {
        print("Could not query dimensions of cubemap using image at: %s\n", path.c_str());
        return _onCpu;
    }

    // use these to assign the width and height of the whole image, and allocate an array of bytes accordingly
    _imageData.extent = { static_cast<UI32>(width), static_cast<UI32>(height), 1 };
    _imageData.format = Image::getImageFormat(channels);
    _imageData.pixels._size = height * width * channels * 6; // 6 images of dimensions w x h with pixels of n channels
    _imageData.pixels._data = (UC*)malloc(_imageData.pixels._size);

    if (!_imageData.pixels._data) {
        throw std::runtime_error("Error, could not allocate memory!");
    }

    stbi_set_flip_vertically_on_load(true);

    // for each face, load the pixels and copy them into the image data
    UI32 offset = 0;
    for (UI16 i = 0; i < 6; i++) {
        UC* data = stbi_load((SKYBOX_PATH + faces[i]).c_str(), &width, &height, &channels, 0);
        if (!data) {
            throw std::runtime_error("Could not load desired image file!");
        }
        if (width != _imageData.extent.width || height != _imageData.extent.height) {
            print("image at %s does not share same dimensions!", (SKYBOX_PATH + faces[i]).c_str());
            // stop loading image and free pixels already loaded
            if (_imageData.pixels._data) {
                free(_imageData.pixels._data);
                return _onCpu;
            }
        }
        // copy newly loaded pixels into image buffer for later use
        memcpy(_imageData.pixels._data + offset, data, height * width * channels);
        // release source pixels
        stbi_image_free(data);
        offset += height * width * channels;
    }

    _onCpu = true;
    return _onCpu;
}

bool Skybox::uploadToGpu(Renderer& renderer) {
    m_assert(_onCpu, "skybox texture not loaded on CPU! Cannot upload to GPU.");

    if (_onGpu) {
        return _onGpu;
    }

    // pipeline layout 
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
            vkinit::pipelineLayoutCreateInfo(1, &renderer._descriptorSetLayouts[OFFSCREEN_SKYBOX_DESCRIPTOR_LAYOUT]);

        if (vkCreatePipelineLayout(renderer._context.device, &pipelineLayoutCreateInfo, nullptr, &_pipelineLayout)
            != VK_SUCCESS) {
            throw std::runtime_error("Could not create skybox pipeline layout!");
        }
    }

    // pipeline
    {
        VkShaderModule vertShaderModule, fragShaderModule;
        vertShaderModule = Shader::createShaderModule(&renderer._context,
            Shader::readFile(kShaders[OFFSCREEN_SKYBOX_DESCRIPTOR_LAYOUT].first));
        fragShaderModule = Shader::createShaderModule(&renderer._context,
            Shader::readFile(kShaders[OFFSCREEN_SKYBOX_DESCRIPTOR_LAYOUT].second));

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
        shaderStages[0] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main");
        shaderStages[1] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main");

        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo =
            vkinit::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

        VkPipelineRasterizationStateCreateInfo rasterizerStateInfo =
            vkinit::pipelineRasterStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

        // writes to gbuffer
        std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachmentStates = {
            vkinit::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            vkinit::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            vkinit::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
            vkinit::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
        };

        VkPipelineColorBlendStateCreateInfo    colorBlendingStateInfo = vkinit::pipelineColorBlendStateCreateInfo(
            static_cast<UI32>(colorBlendAttachmentStates.size()), colorBlendAttachmentStates.data());

        // enable writing to gbuffer depth attachment
        VkPipelineDepthStencilStateCreateInfo  depthStencilStateInfo =
            vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

        VkPipelineViewportStateCreateInfo      viewportStateInfo =
            vkinit::pipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);

        VkPipelineMultisampleStateCreateInfo   multisamplingStateInfo =
            vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

        // input consists of positions in 3D
        VkVertexInputBindingDescription bindingDescription = { 0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX };
        VkVertexInputAttributeDescription attributeDescription = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo =
            vkinit::pipelineVertexInputStateCreateInfo(1, &bindingDescription, 1, &attributeDescription);

        // dynamic view port for resizing
        VkDynamicState dynamicStateEnables[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = vkinit::pipelineDynamicStateCreateInfo(dynamicStateEnables, 2);

        // skybox in offscreen subpass
        VkGraphicsPipelineCreateInfo pipelineCreateInfo =
            vkinit::graphicsPipelineCreateInfo(_pipelineLayout, renderer._renderPass, OFFSCREEN_SUBPASS);

        pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineCreateInfo.pStages = shaderStages.data();
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateInfo;
        pipelineCreateInfo.pViewportState = &viewportStateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizerStateInfo;
        pipelineCreateInfo.pMultisampleState = &multisamplingStateInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencilStateInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendingStateInfo;
        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
        pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo; // vertex input bindings / attributes from gltf model
        colorBlendingStateInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachmentStates.size());
        colorBlendingStateInfo.pAttachments = colorBlendAttachmentStates.data();

        // skybox pipeline
        if (vkCreateGraphicsPipelines(renderer._context.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
            &_pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Could not create skybox graphics pipeline!");
        }

        vkDestroyShaderModule(renderer._context.device, vertShaderModule, nullptr);
        vkDestroyShaderModule(renderer._context.device, fragShaderModule, nullptr);
    }

    // vertex buffer
    _vertexBuffer = Buffer::createDeviceLocalBuffer(&renderer._context, renderer._commandPools[RENDER_CMD_POOL],
        BufferData{ (UC*)Skybox::cubeVerts, 36 * sizeof(glm::vec3) }, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    // create the desriptors
    {
        // cube map
        _cubeMap.uploadToGpu(renderer, _imageData);
         
        // uniform buffer
        _uniformBuffer = Buffer::createBuffer(renderer._context, sizeof(SkyboxUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    // create the descriptor sets
    {
        VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocInfo(renderer._descriptorPool,
            1, &renderer._descriptorSetLayouts[OFFSCREEN_SKYBOX_DESCRIPTOR_LAYOUT]);

        // skybox descriptor set
        if (vkAllocateDescriptorSets(renderer._context.device, &allocInfo, &_descriptorSet) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        // skybox uniform
        VkDescriptorBufferInfo skyboxUboInf{};
        skyboxUboInf.buffer = _uniformBuffer._vkBuffer;
        skyboxUboInf.offset = 0;
        skyboxUboInf.range = sizeof(SkyboxUBO);

        // skybox texture
        VkDescriptorImageInfo skyboxTexDescriptor{};
        skyboxTexDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        skyboxTexDescriptor.imageView = _cubeMap._imageView;
        skyboxTexDescriptor.sampler = renderer._colorSampler;

        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
            // binding 0: vertex shader uniform buffer 
            vkinit::writeDescriptorSet(_descriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &skyboxUboInf),
            // binding 1: skybox texture 
            vkinit::writeDescriptorSet(_descriptorSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &skyboxTexDescriptor)
        };

        vkUpdateDescriptorSets(renderer._context.device, static_cast<UI32>(writeDescriptorSets.size()),
            writeDescriptorSets.data(), 0, nullptr);
    }
    
    _onGpu = true;
    return _onGpu;
}

void Skybox::draw(VkCommandBuffer cmdBuffer) {
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline);

    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1,
        &_descriptorSet, 0, nullptr);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &_vertexBuffer._vkBuffer, &offset);
    
    vkCmdDraw(cmdBuffer, 36, 1, 0, 0);
}

void Skybox::cleanupSkybox(VkDevice device) {
    _uniformBuffer.cleanupBufferData(device);
    _vertexBuffer.cleanupBufferData(device);
    _cubeMap.cleanupTexture(device);

    if (_imageData.pixels._data) {
        free(_imageData.pixels._data);
    }

}

