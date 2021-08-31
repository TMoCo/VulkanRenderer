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
#include <common/commands.h>

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

void Application::run(const char* arg) {
    init(arg);
    mainLoop();
    cleanup();
}

void Application::init(const char* arg) {
    initWindow();

    _renderer.init(_window);

    buildScene(arg);

    initVulkan();

    initImGui();
}

void Application::buildScene(const char* arg) {
    camera = Camera({ 0.0f, 0.0f, 0.0f }, 2.0f, 1.5f);
    
    _gltfModel.load(arg);
    _gltfModel.uploadToGpu(_renderer);

    lights[0] = { {0.0f, 10.0f, 5.0f, 0.0f}, { 200.0f, 200.0f, 200.0f , 40.0f } }; // pos, colour + radius

    spotLight = SpotLight({ 20.0f, 20.0f, 0.0f }, 0.1f, 40.0f);
    
    floor = Plane(20.0f, 20.0f);

    _skybox.load(SKYBOX_PATH);
    _skybox.uploadToGpu(_renderer);

}

void Application::initVulkan() {
    // swap chain independent
    //shadowMap.createShadowMap(_renderer);

    // record commands
    for (UI32 i = 0; i < _renderer._swapChain.imageCount(); i++) {
        // buildShadowMapCommandBuffer(_renderer._renderCommandBuffers[i]);
        recordCommandBuffer(_renderer._renderCommandBuffers[i], i);
    }
}

void Application::recreateVulkanData() {
    static I32 width = 0, height = 0;
    glfwGetFramebufferSize(_window, &width, &height);

    // hang the application while 
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(_window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(_renderer._context.device); // wait if in use by device


    //shadowMap.cleanupShadowMap();
    _renderer.resize();

    // create new swap chain etc...
    //shadowMap.createShadowMap(&_renderer._context, &descriptorSetLayout, _renderer._commandPools[RENDER_CMD_POOL]);

    for (UI32 i = 0; i < _renderer._swapChain.imageCount(); i++) {
        // buildShadowMapCommandBuffer(_renderer._renderCommandBuffers[i]);
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
    init_info.DescriptorPool = _renderer._descriptorPool;
    init_info.Allocator      = nullptr;
    init_info.MinImageCount  = _renderer._context._swapChainSupportDetails.capabilities.minImageCount + 1;
    init_info.ImageCount     = _renderer._swapChain.imageCount();

    // the imgui render pass
    ImGui_ImplVulkan_Init(&init_info, _renderer._guiRenderPass);

    uploadFonts();
}

void Application::uploadFonts() {
    VkCommandBuffer commandbuffer = cmd::beginSingleTimeCommands(_renderer._context.device, _renderer._commandPools[GUI_CMD_POOL]);
    ImGui_ImplVulkan_CreateFontsTexture(commandbuffer);
    cmd::endSingleTimeCommands(_renderer._context.device, _renderer._context.graphicsQueue, commandbuffer, 
        _renderer._commandPools[GUI_CMD_POOL]);
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

    _gltfModel.draw(cmdBuffer);

    vkCmdEndRenderPass(cmdBuffer);
}

// USES THE NEW RENDER PASS
void Application::recordCommandBuffer(VkCommandBuffer cmdBuffer, UI32 index) {
    VkCommandBufferBeginInfo commandBufferBeginInfo = vkinit::commandBufferBeginInfo();

    // implicitly resets cmd buffer
    if (vkBeginCommandBuffer(cmdBuffer, &commandBufferBeginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    
    VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
    
    // Clear values for all attachments (color and depth) written in the fragment shader
    VkClearValue clearValues[ATTACHMENTS_MAX_ENUM] {};
    clearValues[COLOR_ATTACHMENT].color = clearColor;
    clearValues[GBUFFER_POSITION_ATTACHMENT].color = clearColor;
    clearValues[GBUFFER_NORMAL_ATTACHMENT].color = clearColor;
    clearValues[GBUFFER_ALBEDO_ATTACHMENT].color = clearColor;

    clearValues[GBUFFER_DEPTH_ATTACHMENT].depthStencil = { 1.0f, 0 };

    VkViewport viewport{ 0.0f, 0.0f, (F32)_renderer._swapChain.extent().width, 
        (F32)_renderer._swapChain.extent().height, 0.0f, 1.0f };

    VkRect2D scissor{ { 0, 0 }, _renderer._swapChain.extent() };


    VkRenderPassBeginInfo renderPassBeginInfo = vkinit::renderPassBeginInfo(_renderer._renderPass,
        _renderer._framebuffers[index], _renderer._swapChain.extent(), ATTACHMENTS_MAX_ENUM, clearValues);

    // 1: offscreen scene render into gbuffer
    vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    // TODO: MOVE PIPELINE BINDING INTO model.draw
    // scene pipeline
    /*
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _offScreenPipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _deferredPipelineLayout, 0, 1,
        &offScreenDescriptorSet, 0, nullptr);
    */
    
    _gltfModel.draw(cmdBuffer);

    _skybox.draw(cmdBuffer);

    // 2: composition to screen
    vkCmdNextSubpass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);

    //vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _renderer._compositionPipeline);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        _renderer._compositionPipelineLayout, 0, 1, &_renderer._compositionDescriptorSets[index], 0, nullptr);

    // draw a single triangle
    vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(cmdBuffer);

    if (vkEndCommandBuffer(cmdBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
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
    ImGui::SliderFloat("scale", &scale, 1.0f, 50.0f);
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
    // TODO: MAKE UNIFORM UPDATES MORE EFFICIENT (mapping/unmapping is costly operation every frame)

    // offscreen ubo
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), _renderer.aspectRatio(), 0.1f, 40.0f);
    proj[1][1] *= -1.0f; // y coordinates inverted, Vulkan origin top left vs OpenGL bottom left

    glm::mat4 model = glm::translate(glm::mat4(1.0f), translate);
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    glm::mat4 rotateZ(1.0f);
    rotateZ[0][0] *= -1.0f;
    rotateZ[1][1] *= -1.0f;
    model *= rotateZ * glm::toMat4(glm::quat(glm::radians(rotate)));

    OffscreenUBO offscreenUbo{};
    offscreenUbo.model = model;
    offscreenUbo.projectionView = proj * camera.getViewMatrix();

    void* data;
    vkMapMemory(_renderer._context.device, _gltfModel._uniformBuffer._memory, 0, sizeof(offscreenUbo), 0, &data);
    memcpy(data, &offscreenUbo, sizeof(offscreenUbo));
    vkUnmapMemory(_renderer._context.device, _gltfModel._uniformBuffer._memory);

    // shadow map ubo
    /*
    ShadowMap::UBO shadowMapUbo = { spotLight.getMVP(model) };
    shadowMap.updateShadowMapUniformBuffer(shadowMapUbo); 
    */

    // skybox ubo
    SkyboxUBO skyboxUbo{};
    skyboxUbo.projectionView = proj * glm::mat4(glm::mat3(camera.getViewMatrix()));

    vkMapMemory(_renderer._context.device, _skybox._uniformBuffer._memory, 0, sizeof(skyboxUbo), 0, &data);
    memcpy(data, &skyboxUbo, sizeof(skyboxUbo));
    vkUnmapMemory(_renderer._context.device, _skybox._uniformBuffer._memory);

    // composition ubo
    CompositionUBO compositionUbo = {};
    compositionUbo.guiData = { camera.position, attachmentNum };
    compositionUbo.depthMVP = spotLight.getMVP();
    compositionUbo.cameraMVP = offscreenUbo.projectionView;
    compositionUbo.lights[0] = lights[0]; // pos, colour, radius 
    /*
    compositionUbo.lights[1] = lights[1];
    compositionUbo.lights[2] = lights[2];
    compositionUbo.lights[3] = lights[3];
    */
    vkMapMemory(_renderer._context.device, _renderer._compositionUniforms._memory, sizeof(compositionUbo) * currentImage, 
        sizeof(compositionUbo), 0, &data);
    memcpy(data, &compositionUbo, sizeof(compositionUbo));
    vkUnmapMemory(_renderer._context.device, _renderer._compositionUniforms._memory);
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

    _skybox.cleanup(_renderer._context.device);

    _gltfModel.cleanup(_renderer);

    _offScreenUniform.cleanupBufferData(_renderer._context.device);
    _compositionUniforms.cleanupBufferData(_renderer._context.device);

    // cleanup the descriptor pools and descriptor set layouts
    vkDestroyDescriptorPool(_renderer._context.device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(_renderer._context.device, descriptorSetLayout, nullptr);

    _renderer.cleanup();

    // destory the window
    glfwDestroyWindow(_window);

    // terminate glfw
    glfwTerminate();
}

