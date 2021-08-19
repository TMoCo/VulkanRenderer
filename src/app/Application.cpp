///////////////////////////////////////////////////////
//
// Application definition
//
///////////////////////////////////////////////////////

// include class definition
#include <app/Application.h>
#include <app/AppConstants.h>

// include constants and utility
#include <utils/utils.h>
#include <utils/vkinit.h>
#include <utils/Print.h>
#include <utils/Assert.h>

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

    lights[0] = { {5.0f, -5.0f, 0.0f, 0.0f}, {0.5f, 0.5f, 0.5f}, 40.0f }; // pos, colour, radius

    spotLight = SpotLight({ 5.0f, -5.0f, 0.0f }, 0.1f, 40.0f);
    
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

    // swap chain dependent
    gBuffer.createGBuffer(&_renderer._context, &_renderer._swapChain, &descriptorSetLayout, 
        _renderer._commandPools[kCmdPools::RENDER], _renderer._renderPass);
    shadowMap.createShadowMap(&_renderer._context, &descriptorSetLayout, _renderer._commandPools[kCmdPools::RENDER]);

    createDescriptorPool();
    createDescriptorSets(_renderer._swapChain.imageCount());

    renderCommandBuffers.resize(_renderer._swapChain.imageCount());
    offScreenCommandBuffers.resize(_renderer._swapChain.imageCount());
    imGuiCommandBuffers.resize(_renderer._swapChain.imageCount());

    createCommandBuffers(_renderer._swapChain.imageCount(), renderCommandBuffers.data(), _renderer._commandPools[kCmdPools::RENDER]);
    createCommandBuffers(_renderer._swapChain.imageCount(), offScreenCommandBuffers.data(), _renderer._commandPools[kCmdPools::RENDER]);

    createCommandBuffers(_renderer._swapChain.imageCount(), imGuiCommandBuffers.data(), _renderer._commandPools[kCmdPools::GUI]);

    // record commands
    for (UI32 i = 0; i < _renderer._swapChain.imageCount(); i++) {
        buildOffscreenCommandBuffer(i); // offscreen gbuffer commands
        buildShadowMapCommandBuffer(offScreenCommandBuffers[i]);
        buildCompositionCommandBuffer(i); // final image composition
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

    // destroy old swap chain dependencies
    vkFreeCommandBuffers(_renderer._context.device, _renderer._commandPools[kCmdPools::GUI], 
        static_cast<uint32_t>(imGuiCommandBuffers.size()), imGuiCommandBuffers.data());

    vkFreeCommandBuffers(_renderer._context.device, _renderer._commandPools[kCmdPools::RENDER], 
        static_cast<uint32_t>(renderCommandBuffers.size()), renderCommandBuffers.data());
    vkFreeCommandBuffers(_renderer._context.device, _renderer._commandPools[kCmdPools::RENDER], 
        static_cast<uint32_t>(offScreenCommandBuffers.size()), offScreenCommandBuffers.data());

    shadowMap.cleanupShadowMap();
    gBuffer.cleanupGBuffer();

    _renderer.recreateSwapchain();

    // create new swap chain etc...
    gBuffer.createGBuffer(&_renderer._context, &_renderer._swapChain, &descriptorSetLayout, 
        _renderer._commandPools[kCmdPools::RENDER], _renderer._renderPass);
    shadowMap.createShadowMap(&_renderer._context, &descriptorSetLayout, _renderer._commandPools[kCmdPools::RENDER]);

    createDescriptorSets(_renderer._swapChain.imageCount());

    createCommandBuffers(_renderer._swapChain.imageCount(), renderCommandBuffers.data(), _renderer._commandPools[kCmdPools::RENDER]);
    createCommandBuffers(_renderer._swapChain.imageCount(), offScreenCommandBuffers.data(), _renderer._commandPools[kCmdPools::RENDER]);

    createCommandBuffers(_renderer._swapChain.imageCount(), imGuiCommandBuffers.data(), _renderer._commandPools[kCmdPools::GUI]);

    for (UI32 i = 0; i < _renderer._swapChain.imageCount(); i++) {
        buildOffscreenCommandBuffer(i);
        buildCompositionCommandBuffer(i);
        buildShadowMapCommandBuffer(offScreenCommandBuffers[i]);
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
    init_info.MinImageCount  = _renderer._swapChain.supportDetails().capabilities.minImageCount + 1;
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
        // binding 1: model albedo texture / position texture
        vkinit::descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 2: model metallic roughness / normal texture
        vkinit::descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 3: albedo texture
        vkinit::descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 4: fragment shader uniform buffer 
        vkinit::descriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT),
        // binding 5: fragment shader shadow map sampler
        vkinit::descriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
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
    offScreenUboInf.buffer = gBuffer.offScreenUniform._vkBuffer;
    offScreenUboInf.offset = 0;
    offScreenUboInf.range  = sizeof(GBuffer::OffScreenUbo);
    
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

    vkUpdateDescriptorSets(_renderer._context.device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

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

    vkUpdateDescriptorSets(_renderer._context.device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

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

    vkUpdateDescriptorSets(_renderer._context.device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

    // composition descriptor sets
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    compositionDescriptorSets.resize(layouts.size());

    if (vkAllocateDescriptorSets(_renderer._context.device, &allocInfo, compositionDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // image descriptors for gBuffer color attachments and shadow map
    VkDescriptorImageInfo texDescriptorPosition{};
    texDescriptorPosition.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorPosition.imageView = gBuffer.attachments["position"].imageView;
    texDescriptorPosition.sampler = gBuffer.colourSampler;

    VkDescriptorImageInfo texDescriptorNormal{};
    texDescriptorNormal.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorNormal.imageView = gBuffer.attachments["normal"].imageView;
    texDescriptorNormal.sampler = gBuffer.colourSampler;

    VkDescriptorImageInfo texDescriptorAlbedo{};
    texDescriptorAlbedo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texDescriptorAlbedo.imageView = gBuffer.attachments["albedo"].imageView;
    texDescriptorAlbedo.sampler = gBuffer.colourSampler;

    VkDescriptorImageInfo texDescriptorShadowMap{};
    texDescriptorShadowMap.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    texDescriptorShadowMap.imageView = shadowMap.imageView;
    texDescriptorShadowMap.sampler = shadowMap.depthSampler;

    for (size_t i = 0; i < compositionDescriptorSets.size(); i++) {
        // forward rendering uniform buffer
        VkDescriptorBufferInfo compositionUboInf{};
        compositionUboInf.buffer = gBuffer.compositionUniforms._vkBuffer;
        compositionUboInf.offset = sizeof(GBuffer::CompositionUBO) * i;
        compositionUboInf.range  = sizeof(GBuffer::CompositionUBO);

        // offscreen descriptor writes
        writeDescriptorSets = {
            // binding 1: position texture target 
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &texDescriptorPosition),
            // binding 2: normal texture target
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &texDescriptorNormal),
            // binding 3: albedo texture target
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &texDescriptorAlbedo),
            // binding 4: fragment shader uniform
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &compositionUboInf),
            // binding 5: shadow map
            vkinit::writeDescriptorSet(compositionDescriptorSets[i], 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &texDescriptorShadowMap),
        };

        // update according to the configuration
        vkUpdateDescriptorSets(_renderer._context.device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
}

// Command buffers

void Application::createCommandBuffers(UI32 count, VkCommandBuffer* commandBuffers, VkCommandPool& commandPool) {
    VkCommandBufferAllocateInfo allocInfo = vkinit::commaneBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, count);
    if (vkAllocateCommandBuffers(_renderer._context.device, &allocInfo, commandBuffers) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void Application::buildCompositionCommandBuffer(UI32 cmdBufferIndex) {
    VkCommandBufferBeginInfo commandBufferBeginInfo = vkinit::commandBufferBeginInfo();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color           = { 0.0f, 0.0f, 0.0f, 1.0f };
    clearValues[1].depthStencil    = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(_renderer._renderPass,
        _renderer._framebuffers[cmdBufferIndex], _renderer._swapChain.extent(), static_cast<UI32>(clearValues.size()),
        clearValues.data());

    // implicitly resets cmd buffer
    if (vkBeginCommandBuffer(renderCommandBuffers[cmdBufferIndex], &commandBufferBeginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    
    vkCmdBeginRenderPass(renderCommandBuffers[cmdBufferIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(renderCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, gBuffer.deferredPipeline);
    vkCmdBindDescriptorSets(renderCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, 
        gBuffer.layout, 0, 1, &compositionDescriptorSets[cmdBufferIndex], 0, nullptr);
    // draw a single triangle
    vkCmdDraw(renderCommandBuffers[cmdBufferIndex], 3, 1, 0, 0);
    vkCmdEndRenderPass(renderCommandBuffers[cmdBufferIndex]);

    if (vkEndCommandBuffer(renderCommandBuffers[cmdBufferIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void Application::buildGuiCommandBuffer(UI32 cmdBufferIndex) {
    VkCommandBufferBeginInfo commandbufferInfo = vkinit::commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    if (vkBeginCommandBuffer(imGuiCommandBuffers[cmdBufferIndex], &commandbufferInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkClearValue clearValue{}; 
    clearValue.color = { 0.0f, 0.0f, 0.0f, 0.0f }; // completely opaque clear value

    // begin the render pass
    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(_renderer._guiRenderPass,
        _renderer._guiFramebuffers[cmdBufferIndex], _renderer._swapChain.extent(), 1, &clearValue);

    vkCmdBeginRenderPass(imGuiCommandBuffers[cmdBufferIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imGuiCommandBuffers[cmdBufferIndex]); // ends imgui render
    vkCmdEndRenderPass(imGuiCommandBuffers[cmdBufferIndex]);

    if (vkEndCommandBuffer(imGuiCommandBuffers[cmdBufferIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record ImGui command buffer!");
    }
}

void Application::buildOffscreenCommandBuffer(UI32 cmdBufferIndex) {
    VkCommandBufferBeginInfo commandBufferBeginInfo = vkinit::commandBufferBeginInfo();

    // Clear values for all attachments written in the fragment shader
    std::array<VkClearValue, 4> clearValues{};
    clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[2].color = { 0.0f, 0.0f, 0.0f, 0.0f };
    clearValues[3].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(gBuffer.deferredRenderPass,
        gBuffer.deferredFrameBuffer, gBuffer.extent, static_cast<uint32_t>(clearValues.size()), clearValues.data());

    // implicitly resets cmd buffer
    if (vkBeginCommandBuffer(offScreenCommandBuffers[cmdBufferIndex], &commandBufferBeginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    vkCmdBeginRenderPass(offScreenCommandBuffers[cmdBufferIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // scene pipeline
    vkCmdBindPipeline(offScreenCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, gBuffer.offScreenPipeline);
    vkCmdBindDescriptorSets(offScreenCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, gBuffer.layout, 0, 1,
        &offScreenDescriptorSet, 0, nullptr);
    VkDeviceSize offset = 0; // offset into vertex buffer
    vkCmdBindVertexBuffers(offScreenCommandBuffers[cmdBufferIndex], 0, 1, &vertexBuffer._vkBuffer, &offset);
    vkCmdBindIndexBuffer(offScreenCommandBuffers[cmdBufferIndex], indexBuffer._vkBuffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(offScreenCommandBuffers[cmdBufferIndex], model.getNumIndices(0) + 6, 1, 0, 0, 0);

    // skybox pipeline
    vkCmdBindPipeline(offScreenCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, gBuffer.skyboxPipeline);
    vkCmdBindDescriptorSets(offScreenCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, gBuffer.layout, 0, 1,
        &skyboxDescriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(offScreenCommandBuffers[cmdBufferIndex], 0, 1, &skybox.vertexBuffer._vkBuffer, &offset);
    vkCmdDraw(offScreenCommandBuffers[cmdBufferIndex], 36, 1, 0, 0);

    vkCmdEndRenderPass(offScreenCommandBuffers[cmdBufferIndex]);

    //if (vkEndCommandBuffer(offScreenCommandBuffers[cmdBufferIndex]) != VK_SUCCESS) {
    //    throw std::runtime_error("failed to record command buffer!");
    //}
}

void Application::buildShadowMapCommandBuffer(VkCommandBuffer cmdBuffer) {
    VkCommandBufferBeginInfo commandBufferBeginInfo = vkinit::commandBufferBeginInfo();

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

    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}


void Application::createForwardPipeline(VkDescriptorSetLayout* descriptorSetLayout) {
    VkShaderModule vertShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile(FWD_VERT_SHADER));
    VkShaderModule fragShaderModule = Shader::createShaderModule(&_renderer._context, Shader::readFile(FWD_FRAG_SHADER));

    auto bindingDescription = Model::getBindingDescriptions(0);
    auto attributeDescriptions = Model::getAttributeDescriptions(0);

    VkViewport viewport{ 0.0f, 0.0f, (F32)_renderer._swapChain.extent().width, (F32)_renderer._swapChain.extent().height, 0.0f, 1.0f };
    VkRect2D scissor{ { 0, 0 }, _renderer._swapChain.extent() };

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule, "main"),
        vkinit::pipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule, "main")
    };
    VkPipelineVertexInputStateCreateInfo vertexInputInfo =
        vkinit::pipelineVertexInputStateCreateInfo(1, &bindingDescription,
            static_cast<uint32_t>(attributeDescriptions.size()), attributeDescriptions.data());
    VkPipelineInputAssemblyStateCreateInfo inputAssembly =
        vkinit::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);
    VkPipelineViewportStateCreateInfo viewportState =
        vkinit::pipelineViewportStateCreateInfo(1, &viewport, 1, &scissor);
    VkPipelineRasterizationStateCreateInfo rasterizer =
        vkinit::pipelineRasterStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    VkPipelineMultisampleStateCreateInfo multisampling =
        vkinit::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);
    VkPipelineColorBlendStateCreateInfo colorBlending =
        vkinit::pipelineColorBlendStateCreateInfo(1, &colorBlendAttachment);
    VkPipelineDepthStencilStateCreateInfo depthStencil =
        vkinit::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo =
        vkinit::pipelineLayoutCreateInfo(1, descriptorSetLayout);

    if (vkCreatePipelineLayout(_renderer._context.device, &pipelineLayoutInfo, nullptr, &_fwdPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = 
        vkinit::graphicsPipelineCreateInfo(_fwdPipelineLayout, _renderer._renderPass, 0);

    // fixed function pipeline
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.stageCount = static_cast<UI32>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();

    if (vkCreateGraphicsPipelines(_renderer._context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_fwdPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(_renderer._context.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(_renderer._context.device, vertShaderModule, nullptr);
}


// Handling window resize events

void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    // pointer to this application class obtained from glfw, it doesnt know that it is a Application but we do so we can cast to it
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    // and set the resize flag to true
    app->framebufferResized = true;
}

// Synchronisation


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
    submitInfo.pSignalSemaphores    = &_renderer._offScreenSemaphores[currentFrame];
    
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &offScreenCommandBuffers[currentFrame];

    if (vkQueueSubmit(_renderer._context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // scene and Gui rendering
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &_renderer._offScreenSemaphores[currentFrame];

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &_renderer._renderFinishedSemaphores[currentFrame];

    std::array<VkCommandBuffer, 2> submitCommandBuffers = { renderCommandBuffers[imageIndex], imGuiCommandBuffers[imageIndex] };
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
    const char* attachments[] = { "composition", "position", "normal", "albedo", "depth", "shadow map", "shadow NDC", "camera NDC", "shadow depth" };

    ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoMove);
    ImGui::Text("Application %.1f FPS", ImGui::GetIO().Framerate);
    ImGui::BulletText("Transforms:");
    ImGui::PushItemWidth(210);
    ImGui::SliderFloat3("translate", &translate[0], -2.0f, 2.0f);
    ImGui::SliderFloat3("rotate", &rotate[0], -180.0f, 180.0f);
    ImGui::SliderFloat("scale", &scale, 0.0f, 1.0f);
    ImGui::BulletText("Visualize:");
    ImGui::Combo("", &attachmentNum, attachments, SizeofArray(attachments));
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

    GBuffer::OffScreenUbo offscreenUbo{};
    offscreenUbo.model = model;
    offscreenUbo.view = camera.getViewMatrix();
    offscreenUbo.projection = proj;
    offscreenUbo.normal = glm::transpose(glm::inverse(glm::mat3(model)));

    gBuffer.updateOffScreenUniformBuffer(offscreenUbo);

    // shadow map ubo
    ShadowMap::UBO shadowMapUbo = { spotLight.getMVP(model) };
    shadowMap.updateShadowMapUniformBuffer(shadowMapUbo); 

    // skybox ubo
    Skybox::UBO skyboxUbo{};
    skyboxUbo.view = glm::mat4(glm::mat3(camera.getViewMatrix()));
    skyboxUbo.projection = proj;

    skybox.updateSkyboxUniformBuffer(skyboxUbo);

    // composition ubo
    GBuffer::CompositionUBO compositionUbo = {};
    compositionUbo.guiData = { camera.position, attachmentNum };
    compositionUbo.depthMVP = spotLight.getMVP();
    compositionUbo.cameraMVP = offscreenUbo.projection * offscreenUbo.view;
    compositionUbo.lights[0] = lights[0]; // pos, colour, radius 
    /*
    compositionUbo.lights[1] = lights[1];
    compositionUbo.lights[2] = lights[2];
    compositionUbo.lights[3] = lights[3];
    */
    gBuffer.updateCompositionUniformBuffer(currentImage, compositionUbo);
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

    // destroy whatever is dependent on the old swap chain
    vkFreeCommandBuffers(_renderer._context.device, _renderer._commandPools[kCmdPools::RENDER], static_cast<uint32_t>(renderCommandBuffers.size()), renderCommandBuffers.data());
    vkFreeCommandBuffers(_renderer._context.device, _renderer._commandPools[kCmdPools::RENDER], static_cast<uint32_t>(offScreenCommandBuffers.size()), offScreenCommandBuffers.data());
    vkFreeCommandBuffers(_renderer._context.device, _renderer._commandPools[kCmdPools::GUI], static_cast<uint32_t>(imGuiCommandBuffers.size()), imGuiCommandBuffers.data());

    // call the function we created for destroying the swap chain and frame buffers
    // in the reverse order of their creation
    gBuffer.cleanupGBuffer();

    // cleanup the descriptor pools and descriptor set layouts
    vkDestroyDescriptorPool(_renderer._context.device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(_renderer._context.device, descriptorSetLayout, nullptr);

    // destroy the index and vertex buffers
    indexBuffer.cleanupBufferData(_renderer._context.device);
    vertexBuffer.cleanupBufferData(_renderer._context.device);

    _renderer.cleanup();

    // destory the window
    glfwDestroyWindow(_window);

    // terminate glfw
    glfwTerminate();
}

