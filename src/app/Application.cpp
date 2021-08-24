///////////////////////////////////////////////////////
//
// Application definition
//
///////////////////////////////////////////////////////

// include class definition
#include <app/Application.h>
#include <app/AppConstants.h>

// include constants and utility
#include <common/utils.h>
#include <common/vkinit.h>
#include <common/Print.h>
#include <common/Assert.h>

#include <hpg/Shader.h>

// transformations
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // because OpenGL uses depth range -1.0-1.0 and Vulkan uses 0.0-1.0
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <glm/gtx/string_cast.hpp>

#include <algorithm> // min, max
#include <fstream> // file (shader) loading
#include <cstdint> // UINT32_MAX
#include <set> // set for queues

// ImGui includes for a nice gui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

void Application::run() {
    init();
    mainLoop();
    cleanup();
}

void Application::init() {
    initWindow();

    _renderer.init(_window);

    buildScene();

    initVulkan();

    initImGui();
}

void Application::buildScene() {
    camera = Camera({ 0.0f, 0.0f, 0.0f }, 2.0f, 10.0f);
    
    model.loadModel(MODEL_PATH);

    lights[0] = { {20.0f, 20.0f, 0.0f, 0.0f}, {150.0f, 150.0f, 150.0f}, 40.0f }; // pos, colour, radius

    spotLight = SpotLight({ 20.0f, 20.0f, 0.0f }, 0.1f, 40.0f);
    
    floor = Plane(20.0f, 20.0f);

    skybox.createSkybox(&_renderer._context, _renderer._commandPools[kCmdPools::RENDER]);

    // textures
    const std::vector<ImageData>* textureImages = model.getMaterialTextureData(0);
    textures.resize(textureImages->size());

    for (size_t i = 0; i < textureImages->size(); i++) {
        textures[i].createTexture(&_renderer._context, _renderer._commandPools[kCmdPools::RENDER], textureImages->data()[i]);
    }

    // vertex buffer 
    std::vector<Model::Vertex>* modelVertexBuffer = model.getVertexBuffer(0);
    // generate the vertices for a 2D plane
    //std::vector<Model::Vertex> planeVertexBuffer = floor.getVertices();

    for (auto& v : floor.getVertices()) {
        modelVertexBuffer->push_back(v);
    }

    Buffer::createDeviceLocalBuffer(& _renderer._context, _renderer._commandPools[kCmdPools::RENDER],
        BufferData{ (UC*)modelVertexBuffer->data(), modelVertexBuffer->size() * sizeof(Model::Vertex) }, // vertex data as buffer
        &vertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    // index buffer
    std::vector<UI32>* iBuffer = model.getIndexBuffer(0);
    // generate indices for a quad
    UI32 offset = static_cast<UI32>(iBuffer->size());
    for (auto& i : floor.getIndices()) {
        iBuffer->push_back(i + offset);
    }

    Buffer::createDeviceLocalBuffer(&_renderer._context, _renderer._commandPools[kCmdPools::RENDER],
        BufferData{ (UC*)iBuffer->data(), iBuffer->size() * sizeof(UI32) }, // index data as buffer
        &indexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void Application::initVulkan() {
    // swap chain independent
    createDescriptorSetLayout();
    createDeferredPipelines(&descriptorSetLayout);

    shadowMap.createShadowMap(&_renderer._context, &descriptorSetLayout, _renderer._commandPools[kCmdPools::RENDER]);

    Buffer::createUniformBuffer<OffScreenUbo>(&_renderer._context, 1,
        &_offScreenUniform, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    Buffer::createUniformBuffer<CompositionUBO>(&_renderer._context, _renderer._swapChain.imageCount(),
        &_compositionUniforms, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    createDescriptorPool();
    createDescriptorSets(_renderer._swapChain.imageCount());

    // record commands
    for (UI32 i = 0; i < _renderer._swapChain.imageCount(); i++) {
        buildShadowMapCommandBuffer(_renderer._renderCommandBuffers[i]);
        recordCommandBuffer(_renderer._renderCommandBuffers[i], i);
    }
}

void Application::recreateVulkanData() {
    static I32 width = 0, height = 0;
    glfwGetFramebufferSize(_window, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_renderer._context.device); // wait if in use by device


    shadowMap.cleanupShadowMap();

    _renderer.recreateSwapchain();

    // create new swap chain etc...
    shadowMap.createShadowMap(&_renderer._context, &descriptorSetLayout, _renderer._commandPools[kCmdPools::RENDER]);

    createDescriptorSets(_renderer._swapChain.imageCount());

    for (UI32 i = 0; i < _renderer._swapChain.imageCount(); i++) {
        buildShadowMapCommandBuffer(_renderer._renderCommandBuffers[i]);
        recordCommandBuffer(_renderer._renderCommandBuffers[i], i);
    }

    // update ImGui aswell
    ImGui_ImplVulkan_SetMinImageCount(_renderer._swapChain.imageCount());
}

void Application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(_window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance       = _renderer._context.instance;
    init_info.PhysicalDevice = _renderer._context.physicalDevice;
    init_info.Device         = _renderer._context.device;
    init_info.QueueFamily    = utils::QueueFamilyIndices::findQueueFamilies(_renderer._context.physicalDevice, _renderer._context.surface).graphicsFamily.value();
    init_info.Queue          = _renderer._context.graphicsQueue;
    init_info.PipelineCache  = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Allocator      = nullptr;
    init_info.MinImageCount  = _renderer._context._swapChainSupportDetails.capabilities.minImageCount + 1;
    init_info.ImageCount     = _renderer._swapChain.imageCount();

    // the imgui render pass
    ImGui_ImplVulkan_Init(&init_info, _renderer._guiRenderPass);

    uploadFonts();
}

void Application::uploadFonts() {
    VkCommandBuffer commandbuffer = utils::beginSingleTimeCommands(&_renderer._context.device, _renderer._commandPools[kCmdPools::GUI]);
    ImGui_ImplVulkan_CreateFontsTexture(commandbuffer);
    utils::endSingleTimeCommands(&_renderer._context.device, &_renderer._context.graphicsQueue, &commandbuffer, &_renderer._commandPools[kCmdPools::GUI]);
}

void Application::initWindow() {
    glfwInit();

    // set parameters
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // initially for opengl, so tell it not to create opengl context

    _window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Deferred Rendering Demo", nullptr, nullptr);

    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

// Descriptors

void Application::createDescriptorPool() {
    uint32_t swapChainImageCount = _renderer._swapChain.imageCount();
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER,                IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       IMGUI_POOL_NUM }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets       = IMGUI_POOL_NUM * _renderer._swapChain.imageCount();
    poolInfo.poolSizeCount = static_cast<uint32_t>(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
    poolInfo.pPoolSizes    = poolSizes; // the descriptors

    if (vkCreateDescriptorPool(_renderer._context.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void Application::createDescriptorSetLayout() {

    std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
        // binding 0: vertex shader uniform buffer 
        vkinit::descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
        // binding 1: model albedo texture / shadow map sampler
        vkinit::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 2: model metallic roughness 
        vkinit::descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 3: composition fragment shader uniform buffer 
        vkinit::descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 4: position input attachment
        vkinit::descriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 5: normal input attachment
        vkinit::descriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 6: albedo input attachment
        vkinit::descriptorSetLayoutBinding(6, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 6: albedo input attachment
        vkinit::descriptorSetLayoutBinding(7, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInf{};
    layoutCreateInf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInf.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
    layoutCreateInf.pBindings = setLayoutBindings.data();

    if (vkCreateDescriptorSetLayout(_renderer._context.device, &layoutCreateInf, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void Application::createDescriptorSets(UI32 swapChainImages) {
    std::vector<VkDescriptorSetLayout> layouts(swapChainImages, descriptorSetLayout);

    VkDescriptorSetAllocateInfo allocInfo = vkinit::descriptorSetAllocInfo(descriptorPool, 1, layouts.data());

    // offscreen descriptor set
    if (vkAllocateDescriptorSets(_renderer._context.device, &allocInfo, &offScreenDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkWriteDescriptorSet> writeDescriptorSets;

    // offscreen uniform
    VkDescriptorBufferInfo offScreenUboInf{};
    offScreenUboInf.buffer = _offScreenUniform._vkBuffer;
    offScreenUboInf.offset = 0;
    offScreenUboInf.range  = sizeof(OffScreenUbo);
    
    // offscreen textures in scene
    std::vector<VkDescriptorImageInfo> offScreenTexDescriptors(textures.size());
    for (size_t i = 0; i < textures.size(); i++) {
        offScreenTexDescriptors[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        offScreenTexDescriptors[i].imageView   = textures[i].textureImageView;
        offScreenTexDescriptors[i].sampler     = textures[i].textureSampler;
    }

    writeDescriptorSets = {
        // binding 0: vertex shader uniform buffer 
        vkinit::writeDescriptorSet(offScreenDescriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &offScreenUboInf),
        // binding 1: model albedo texture 
        vkinit::writeDescriptorSet(offScreenDescriptorSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offScreenTexDescriptors[0]),
        // binding 2: model metallic roughness texture
        vkinit::writeDescriptorSet(offScreenDescriptorSet, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &offScreenTexDescriptors[1])
    };

    vkUpdateDescriptorSets(_renderer._context.device, static_cast<UI32>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

    // skybox descriptor set
    if (vkAllocateDescriptorSets(_renderer._context.device, &allocInfo, &skyboxDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // skybox uniform
    VkDescriptorBufferInfo skyboxUboInf{};
    skyboxUboInf.buffer = skybox.uniformBuffer._vkBuffer;
    skyboxUboInf.offset = 0;
    skyboxUboInf.range = sizeof(Skybox::UBO);

    // skybox texture
    VkDescriptorImageInfo skyboxTexDescriptor{};
    skyboxTexDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    skyboxTexDescriptor.imageView = skybox.skyboxImageView;
    skyboxTexDescriptor.sampler = skybox.skyboxSampler;

    writeDescriptorSets = {
        // binding 0: vertex shader uniform buffer 
        vkinit::writeDescriptorSet(skyboxDescriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &skyboxUboInf),
        // binding 1: skybox texture 
        vkinit::writeDescriptorSet(skyboxDescriptorSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &skyboxTexDescriptor)
    };

    vkUpdateDescriptorSets(_renderer._context.device, static_cast<UI32>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

    // shadowMap descriptor set
    if (vkAllocateDescriptorSets(_renderer._context.device, &allocInfo, &shadowMapDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // shadowMap uniform
    VkDescriptorBufferInfo shadowMapUboInf{};
    shadowMapUboInf.buffer = shadowMap.shadowMapUniformBuffer._vkBuffer;
    shadowMapUboInf.offset = 0;
    shadowMapUboInf.range = sizeof(ShadowMap::UBO);

    writeDescriptorSets = {
        // binding 0: vertex shader uniform buffer 
        vkinit::writeDescriptorSet(shadowMapDescriptorSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &shadowMapUboInf),
    };

    vkUpdateDescriptorSets(_renderer._context.device, static_cast<UI32>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

    // composition descriptor sets
    allocInfo.descriptorSetCount = static_cast<UI32>(layouts.size());
    compositionDescriptorSets.resize(layouts.size());

    if (vkAllocateDescriptorSets(_renderer._context.device, &allocInfo, compositionDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // image descriptors for gBuffer color attachments and shadow map
    VkDescriptorImageInfo texDescriptorPosition{};
    texDescriptorPosition.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorPosition.imageView = _renderer._gbuffer[POSITION]._view;
    texDescriptorPosition.sampler = _renderer._colorSampler;

    VkDescriptorImageInfo texDescriptorNormal{};
    texDescriptorNormal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorNormal.imageView = _renderer._gbuffer[NORMAL]._view;
    texDescriptorNormal.sampler = _renderer._colorSampler;

    VkDescriptorImageInfo texDescriptorAlbedo{};
    texDescriptorAlbedo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorAlbedo.imageView = _renderer._gbuffer[ALBEDO]._view;
    texDescriptorAlbedo.sampler = _renderer._colorSampler;

    VkDescriptorImageInfo texDescriptorMetallicRoughness{};
    texDescriptorMetallicRoughness.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorMetallicRoughness.imageView = _renderer._gbuffer[METALLIC_ROUGHNESS]._view;
    texDescriptorMetallicRoughness.sampler = _renderer._colorSampler;

    VkDescriptorImageInfo texDescriptorShadowMap{};
    texDescriptorShadowMap.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    texDescriptorShadowMap.imageView = shadowMap.imageView;
    texDescriptorShadowMap.sampler = shadowMap.depthSampler;

    for (size_t i = 0; i < compositionDescriptorSets.size(); i++) {
        // forward rendering uniform buffer
        VkDescriptorBufferInfo compositionUboInf{};
        compositionUboInf.buffer = _compositionUniforms._vkBuffer;
        compositionUboInf.offset = sizeof(CompositionUBO) * i;
        compositionUboInf.range  = sizeof(CompositionUBO);

        // composition descriptor writes
        writeDescriptorSets = {
            // binding 1: shadow map
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &texDescriptorShadowMap),
            // binding 3: composition fragment shader uniform
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &compositionUboInf),
            // binding 4: position input attachment 
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &texDescriptorPosition),
            // binding 5: normal input attachment
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 5, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &texDescriptorNormal),
            // binding 6: albedo input attachment
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 6, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &texDescriptorAlbedo),
            // binding 7: metallic roughness input attachment
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 7, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, &texDescriptorMetallicRoughness)
        };

        // update according to the configuration
        vkUpdateDescriptorSets(_renderer._context.device, static_cast<UI32>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
}

// Command buffers

void Application::buildGuiCommandBuffer(UI32 cmdBufferIndex) {
    VkCommandBufferBeginInfo commandbufferInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    if (vkBeginCommandBuffer(_renderer._guiCommandBuffers[cmdBufferIndex], &commandbufferInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkClearValue clearValue{}; 
    clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f }; // completely opaque clear value

    // begin the render pass
    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(_renderer._guiRenderPass,
        _renderer._guiFramebuffers[cmdBufferIndex], _renderer._swapChain.extent(), 1, &clearValue);

    vkCmdBeginRenderPass(_renderer._guiCommandBuffers[cmdBufferIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), _renderer._guiCommandBuffers[cmdBufferIndex]); // ends imgui render
    vkCmdEndRenderPass(_renderer._guiCommandBuffers[cmdBufferIndex]);

    if (vkEndCommandBuffer(_renderer._guiCommandBuffers[cmdBufferIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record ImGui command buffer!");
    }
}

void Application::buildShadowMapCommandBuffer(VkCommandBuffer cmdBuffer) {
    VkCommandBufferBeginInfo commandBufferBeginInfo = vkinit::commandBufferBeginInfo();

    // implicitly resets cmd buffer
    if (vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    // Clear values for all attachments written in the fragment shader
    VkClearValue clearValue{};
    clearValue.depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(shadowMap.shadowMapRenderPass, 
        shadowMap.shadowMapFrameBuffer, { shadowMap.extent, shadowMap.extent }, 1, &clearValue);

    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkExtent2D extent{ shadowMap.extent, shadowMap.extent };
    VkViewport viewport{ 0.0f, 0.0f, (F32)extent.width, (F32)extent.height, 0.0f, 1.0f };
    VkRect2D scissor{ { 0, 0 }, extent };

    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    vkCmdSetDepthBias(cmdBuffer, shadowMap.depthBiasConstant, 0.0f, shadowMap.depthBiasSlope);

    // scene pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMap.shadowMapPipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowMap.layout, 0, 1,
        &shadowMapDescriptorSet, 0, nullptr);

    VkDeviceSize offset = 0; // offset into vertex buffer
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer._vkBuffer, &offset);
    vkCmdBindIndexBuffer(cmdBuffer, indexBuffer._vkBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdDrawIndexed(cmdBuffer, model.getNumIndices(0) + 6, 1, 0, 0, 0);

    vkCmdEndRenderPass(cmdBuffer);
}

// USES THE NEW RENDER PASS
void Application::recordCommandBuffer(VkCommandBuffer cmdBuffer, UI32 index) {
    // Clear values for all attachments written in the fragment shader
    VkClearValue clearValues[kAttachments::NUM_ATTACHMENTS] {};
    clearValues[kAttachments::COLOR].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[kAttachments::GBUFFER_POSITION].color = clearValues[kAttachments::COLOR].color;
    clearValues[kAttachments::GBUFFER_NORMAL].color = clearValues[kAttachments::GBUFFER_POSITION].color;
    clearValues[kAttachments::GBUFFER_ALBEDO].color = clearValues[kAttachments::GBUFFER_NORMAL].color;
    clearValues[kAttachments::GBUFFER_DEPTH].depthStencil = { 1.0f, 0 };

    VkViewport viewport{ 0.0f, 0.0f, (F32)_renderer._swapChain.extent().width, 
        (F32)_renderer._swapChain.extent().height, 0.0f, 1.0f };

    VkRect2D scissor{ { 0, 0 }, _renderer._swapChain.extent() };


    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(_renderer._renderPass,
        _renderer._framebuffers[index], _renderer._swapChain.extent(), kAttachments::NUM_ATTACHMENTS, clearValues);

    // 1: offscreen scene render into gbuffer
    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    // scene pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _offScreenPipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _deferredPipelineLayout, 0, 1,
        &offScreenDescriptorSet, 0, nullptr);
    VkDeviceSize offset = 0; // offset into vertex buffer
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer._vkBuffer, &offset);
    vkCmdBindIndexBuffer(cmdBuffer, indexBuffer._vkBuffer, 0, VK_INDEX_TYPE_UINT32);
    // vkCmdDrawIndexed(cmdBuffer, model.getNumIndices(0) + 6, 1, 0, 0, 0);
    vkCmdDrawIndexed(cmdBuffer, model.getNumIndices(0), 1, 0, 0, 0);

    // skybox pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _skyboxPipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _deferredPipelineLayout, 0, 1,
        &skyboxDescriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &skybox.vertexBuffer._vkBuffer, &offset);
    vkCmdDraw(cmdBuffer, 36, 1, 0, 0);

    // 2: composition to screen
    vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

    //vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _compositionPipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        _deferredPipelineLayout, 0, 1, &compositionDescriptorSets[index], 0, nullptr);
    // draw a single triangle
    vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuffer);

    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

// pipelines

void Application::createDeferredPipelines(VkDescriptorSetLayout* descriptorSetLayout) {

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vkinit::pipelineLayoutCreateInfo(1, descriptorSetLayout);

    if (vkCreatePipelineLayout(_renderer._context.device, &pipelineLayoutCreateInfo, nullptr, &_deferredPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Could not create deferred pipeline layout!");
    }

    VkColorComponentFlags colBlendAttachFlag =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendAttachmentState colorBlendAttachment =
        vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE);

    VkShaderModule vertShaderModule, fragShaderModule;
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo =
        vkinit::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

    VkPipelineRasterizationStateCreateInfo rasterizerStateInfo =
        vkinit::pipelineRasterStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);

    VkPipelineColorBlendStateCreateInfo    colorBlendingStateInfo =
        vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);

    VkPipelineDepthStencilStateCreateInfo  depthStencilStateInfo =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

    VkPipelineViewportStateCreateInfo      viewportStateInfo =
        vkinit::pipelineViewportStateCreateInfo(1, nullptr, 1, nullptr);

    VkPipelineMultisampleStateCreateInfo   multisamplingStateInfo =
        vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

    VkPipelineLayoutCreateInfo             pipelineLayoutInfo =
        vkinit::pipelineLayoutCreateInfo(1, descriptorSetLayout);

    // dynamic view port for resizing
    VkDynamicState dynamicStateEnables[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = vkinit::pipelineDynamicStateCreateInfo(dynamicStateEnables, 2);

    // shared between the offscreen and composition pipelines
    VkGraphicsPipelineCreateInfo pipelineCreateInfo =
        vkinit::graphicsPipelineCreateInfo(_deferredPipelineLayout, _renderer._renderPass, 1); // composition pipeline uses swapchain render pass

    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();
    pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateInfo;
    pipelineCreateInfo.pViewportState = &viewportStateInfo;
    pipelineCreateInfo.pRasterizationState = &rasterizerStateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingStateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilStateInfo;
    pipelineCreateInfo.pColorBlendState = &colorBlendingStateInfo;
    pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

#ifndef NDEBUG
    vertShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile("composition_debug.vert.spv"));
    fragShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile("composition_debug.frag.spv"));
#else
    vertShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile("composition.vert.spv"));
    fragShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile("composition.frag.spv"));
#endif

    shaderStages[0] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main");
    shaderStages[1] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main");

    rasterizerStateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;

    VkPipelineVertexInputStateCreateInfo emptyInputStateInfo =
        vkinit::pipelineVertexInputStateCreateInfo(0, nullptr, 0, nullptr); // no vertex data input
    pipelineCreateInfo.pVertexInputState = &emptyInputStateInfo;

    if (vkCreateGraphicsPipelines(_renderer._context.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_compositionPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Could not create deferred graphics pipeline!");
    }

    vkDestroyShaderModule(_renderer._context.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_renderer._context.device, fragShaderModule, nullptr);

    // offscreen pipeline
    pipelineCreateInfo.subpass = 0;
    // pipelineCreateInfo.renderPass = _renderer._offscreenRenderPass;

    vertShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile("offscreen.vert.spv"));
    fragShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile("offscreen.frag.spv"));
    shaderStages[0] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main");
    shaderStages[1] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main");

    // rasterizerStateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
    rasterizerStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;

    auto bindingDescription = Model::getBindingDescriptions(0);
    auto attributeDescriptions = Model::getAttributeDescriptions(0);

    VkPipelineVertexInputStateCreateInfo   vertexInputStateInfo =
        vkinit::pipelineVertexInputStateCreateInfo(1, &bindingDescription,
            static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());
    pipelineCreateInfo.pVertexInputState = &vertexInputStateInfo; // vertex input bindings / attributes from gltf model

    std::array<VkPipelineColorBlendAttachmentState, 4> colorBlendAttachmentStates = {
        vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE),
        vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE),
        vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE),
        vkinit::pipelineColorBlendAttachmentState(colBlendAttachFlag, VK_FALSE)
    };

    colorBlendingStateInfo.attachmentCount = static_cast<uint32_t>(colorBlendAttachmentStates.size());
    colorBlendingStateInfo.pAttachments = colorBlendAttachmentStates.data();

    if (vkCreateGraphicsPipelines(_renderer._context.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_offScreenPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Could not create deferred graphics pipeline!");
    }

    vkDestroyShaderModule(_renderer._context.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_renderer._context.device, fragShaderModule, nullptr);

    // skybox pipeline
    vertShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile("skybox.vert.spv"));
    fragShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile("skybox.frag.spv"));
    shaderStages[0] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main");
    shaderStages[1] = vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main");

    bindingDescription = { 0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX };
    VkVertexInputAttributeDescription attributeDescription = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };

    vertexInputStateInfo = vkinit::pipelineVertexInputStateCreateInfo(1, &bindingDescription, 1, &attributeDescription);
    pipelineCreateInfo.pVertexInputState = &vertexInputStateInfo; // vertex input bindings / attributes from gltf model

    if (vkCreateGraphicsPipelines(_renderer._context.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_skyboxPipeline) != VK_SUCCESS) {
        throw std::runtime_error("Could not create deferred graphics pipeline!");
    }

    vkDestroyShaderModule(_renderer._context.device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_renderer._context.device, fragShaderModule, nullptr);
}

// Handling window resize events

void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    // pointer to this application class obtained from glfw, it doesnt know that it is a Application but we do so we can cast to it
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    // and set the resize flag to true
    app->framebufferResized = true;
}

//
// Main loop 
//

void Application::mainLoop() {
    prevTime = std::chrono::high_resolution_clock::now();
    
    // loop keeps window open
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();
        // get the time before the drawing frame
        currTime = std::chrono::high_resolution_clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currTime - prevTime).count();

        if (processKeyInput() == 0)
            break;

        // sets the current GUI
        setGUI();

        _renderer.render();

        // draw the frame
        drawFrame();

        prevTime = currTime;
    }
    vkDeviceWaitIdle(_renderer._context.device);
}

// Frame drawing, GUI setting and UI

void Application::drawFrame() {
    // will acquire an image from swap chain, exec commands in command buffer with images as attachments in the 
    // frameBuffer return the image to the swap buffer. These tasks are started simultaneously but executed 
    // asynchronously. However we want these to occur in sequence because each relies on the previous task success
    // For syncing we can use semaphores or fences and coordinate operations by having one operation signal another 
    // and another wait for a fence or semaphore to go from unsignaled to signaled.
    // We can access fence state with vkWaitForFences but not semaphores.
    // Fences are mainly for syncing app with rendering operations, used here to synchronise the frame rate.
    // Semaphores are for syncing operations within or across cmd queues. 
    // We want to sync queue operations to draw cmds and presentation, and we want to make sure the offscreen cmds
    // have finished before the final image composition using semaphores. 
    /*************************************************************************************************************/

    // previous frame finished will fence
    vkWaitForFences(_renderer._context.device, 1, &_renderer._inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(_renderer._context.device, *_renderer._swapChain.get(), UINT64_MAX,
        _renderer._imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateVulkanData();
        return; // return to acquire the image again
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { 
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    if (_renderer._imagesInFlight[imageIndex] != VK_NULL_HANDLE) { // Check if a previous frame is using this image
        vkWaitForFences(_renderer._context.device, 1, &_renderer._imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }

    _renderer._imagesInFlight[imageIndex] = _renderer._inFlightFences[currentFrame]; // set image as in use by current frame

    updateUniformBuffers(imageIndex);

    buildGuiCommandBuffer(imageIndex);

    VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = &waitStages;

    // offscreen rendering (scene data for gbuffer and shadow map)
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &_renderer._imageAvailableSemaphores[currentFrame];

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &_renderer._renderFinishedSemaphores[currentFrame];

    std::array<VkCommandBuffer, 2> submitCommandBuffers = { _renderer._renderCommandBuffers[imageIndex], _renderer._guiCommandBuffers[imageIndex] };
    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers    = submitCommandBuffers.data();

    // reset the fence so fence blocks when submitting 
    vkResetFences(_renderer._context.device, 1, &_renderer._inFlightFences[currentFrame]);

    // submit the command buffer to the graphics queue, takes an array of submitinfo when work load is much larger
    // last param is a fence, which is signaled when the cmd buffer finishes executing and is used to inform that the frame has finished
    // being rendered (the commands were all executed). The next frame can start rendering!
    if (vkQueueSubmit(_renderer._context.graphicsQueue, 1, &submitInfo, _renderer._inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // submitting the result back to the swap chain to have it shown onto the screen
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &_renderer._renderFinishedSemaphores[currentFrame];

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains    = _renderer._swapChain.get();
    presentInfo.pImageIndices  = &imageIndex;

    // submit request to put image from the swap chain to the presentation queue
    result = vkQueuePresentKHR(_renderer._context.presentQueue, &presentInfo);

    // check presentation queue can accept the image and any resize
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateVulkanData();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // increment current frame 
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::setGUI() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame(); // empty
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoMove);
    ImGui::Text("Application %.1f FPS", ImGui::GetIO().Framerate);
    ImGui::BulletText("Transforms:");
    ImGui::PushItemWidth(210);
    ImGui::SliderFloat3("translate", &translate[0], -2.0f, 2.0f);
    ImGui::SliderFloat3("rotate", &rotate[0], -180.0f, 180.0f);
    ImGui::SliderFloat("scale", &scale, 0.0f, 1.0f);
#ifndef NDEBUG
    ImGui::BulletText("Visualize:");
    const char* attachments[13] = { "composition", "position", "normal", "albedo", "depth", "shadow map", 
        "shadow NDC", "camera NDC", "shadow depth", "roughness", "metallic", "occlusion", "uv" };
    ImGui::Combo("", &attachmentNum, attachments, StaticArraySize(attachments));
#endif // !NDEBUG
    ImGui::PopItemWidth();

    ImGui::End();
}

// Uniforms

void Application::updateUniformBuffers(UI32 currentImage) {

    // offscreen ubo
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), _renderer.aspectRatio(), 0.1f, 40.0f);
    proj[1][1] *= -1.0f; // y coordinates inverted, Vulkan origin top left vs OpenGL bottom left

    glm::mat4 model = glm::translate(glm::mat4(1.0f), translate);
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    glm::mat4 rotateZ(1.0f);
    rotateZ[0][0] *= -1.0f;
    rotateZ[1][1] *= -1.0f;
    model *= rotateZ * glm::toMat4(glm::quat(glm::radians(rotate)));

    OffScreenUbo offscreenUbo{};
    offscreenUbo.model = model;
    offscreenUbo.view = camera.getViewMatrix();
    offscreenUbo.projection = proj;
    offscreenUbo.normal = glm::transpose(glm::inverse(glm::mat3(model)));

    void* data;
    vkMapMemory(_renderer._context.device, _offScreenUniform._memory, 0, sizeof(offscreenUbo), 0, &data);
    memcpy(data, &offscreenUbo, sizeof(offscreenUbo));
    vkUnmapMemory(_renderer._context.device, _offScreenUniform._memory);

    // shadow map ubo
    ShadowMap::UBO shadowMapUbo = { spotLight.getMVP(model) };
    shadowMap.updateShadowMapUniformBuffer(shadowMapUbo); 

    // skybox ubo
    Skybox::UBO skyboxUbo{};
    skyboxUbo.view = glm::mat4(glm::mat3(camera.getViewMatrix()));
    skyboxUbo.projection = proj;

    skybox.updateSkyboxUniformBuffer(skyboxUbo);

    // composition ubo
    CompositionUBO compositionUbo = {};
    compositionUbo.guiData = { camera.position, attachmentNum };
    compositionUbo.depthMVP = spotLight.getMVP();
    compositionUbo.cameraMVP = offscreenUbo.projection * offscreenUbo.view;
    compositionUbo.lights[0] = lights[0]; // pos, colour, radius 
    /*
    compositionUbo.lights[1] = lights[1];
    compositionUbo.lights[2] = lights[2];
    compositionUbo.lights[3] = lights[3];
    */
    vkMapMemory(_renderer._context.device, _compositionUniforms._memory, sizeof(compositionUbo) * currentImage, 
        sizeof(compositionUbo), 0, &data);
    memcpy(data, &compositionUbo, sizeof(compositionUbo));
    vkUnmapMemory(_renderer._context.device, _compositionUniforms._memory);
}

int Application::processKeyInput() {
    // special case return 0 to exit the program
    if (glfwGetKey(_window, GLFW_KEY_ESCAPE))
        return 0;

    if (glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT)) {
        // up/down 
        if (glfwGetKey(_window, GLFW_KEY_UP))
            camera.processInput(CameraMovement::Upward, deltaTime);
        if (glfwGetKey(_window, GLFW_KEY_DOWN))
            camera.processInput(CameraMovement::Downward, deltaTime);
    }
    else {
        // front/back
        if (glfwGetKey(_window, GLFW_KEY_UP))
            camera.processInput(CameraMovement::Forward, deltaTime);
        if (glfwGetKey(_window, GLFW_KEY_DOWN))
            camera.processInput(CameraMovement::Backward, deltaTime);
    }

    // left/right
    if (glfwGetKey(_window, GLFW_KEY_LEFT))
        camera.processInput(CameraMovement::Left, deltaTime);
    if (glfwGetKey(_window, GLFW_KEY_RIGHT))
        camera.processInput(CameraMovement::Right, deltaTime);

    // pitch up/down
    if (glfwGetKey(_window, GLFW_KEY_W))
        camera.processInput(CameraMovement::PitchUp, deltaTime);
    if (glfwGetKey(_window, GLFW_KEY_S))
        camera.processInput(CameraMovement::PitchDown, deltaTime);

    // yaw left/right
    if (glfwGetKey(_window, GLFW_KEY_A))
        camera.processInput(CameraMovement::YawLeft, deltaTime);
    if (glfwGetKey(_window, GLFW_KEY_D))
        camera.processInput(CameraMovement::YawRight, deltaTime);

    if (glfwGetKey(_window, GLFW_KEY_Q))
        camera.processInput(CameraMovement::RollLeft, deltaTime);
    if (glfwGetKey(_window, GLFW_KEY_E))
        camera.processInput(CameraMovement::RollRight, deltaTime);

    if (glfwGetKey(_window, GLFW_KEY_SPACE)) {
        camera.orientation.orientation = { 0.0f, 0.0f, 0.0f, 1.0f };
        camera.position = { 0.0f, 0.0f, 3.0f };
    }

    // other wise just return true
    return 1;
}

void Application::processMouseInput(glm::dvec2& curr) {
    // https://learnopengl.com/Getting-started/Camera

    if (firstMouse) {
        prevMouse = curr;
        firstMouse = false;
    }
    glm::dvec2 deltaMouse = curr - prevMouse;

    double sensitivity = 15;
    deltaMouse *= sensitivity;

    // offset dictates the amount we rotate by 
    //camera.yaw   = static_cast<float>(deltaMouse.x);
    //camera.pitch = static_cast<float>(deltaMouse.y);

    // pass the angles to the camera
    //camera.orientation.applyRotation(WORLD_UP, glm::radians(camera.yaw));
    //camera.orientation.applyRotation(WORLD_RIGHT, glm::radians(camera.pitch));
}

//
// Cleanup
//

void Application::cleanup() {
    // destroy the imgui context when the program ends
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    for (auto& texture : textures) {
        texture.cleanupTexture();
    }

    skybox.cleanupSkybox();

    shadowMap.cleanupShadowMap();

    _offScreenUniform.cleanupBufferData(_renderer._context.device);
    _compositionUniforms.cleanupBufferData(_renderer._context.device);


    // cleanup the descriptor pools and descriptor set layouts
    vkDestroyDescriptorPool(_renderer._context.device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(_renderer._context.device, descriptorSetLayout, nullptr);

    // destroy the index and vertex buffers
    indexBuffer.cleanupBufferData(_renderer._context.device);
    vertexBuffer.cleanupBufferData(_renderer._context.device);

    // destroy pipelines
    vkDestroyPipeline(_renderer._context.device, _fwdPipeline, nullptr);
    vkDestroyPipelineLayout(_renderer._context.device, _fwdPipelineLayout, nullptr);

    vkDestroyPipelineLayout(_renderer._context.device, _deferredPipelineLayout, nullptr);
    vkDestroyPipeline(_renderer._context.device, _compositionPipeline, nullptr);
    vkDestroyPipeline(_renderer._context.device, _offScreenPipeline, nullptr);
    vkDestroyPipeline(_renderer._context.device, _skyboxPipeline, nullptr);

    _renderer.cleanup();

    // destory the window
    glfwDestroyWindow(_window);

    // terminate glfw
    glfwTerminate();
}

