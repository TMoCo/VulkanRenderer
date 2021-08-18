///////////////////////////////////////////////////////
// Application class declaration
///////////////////////////////////////////////////////

//
// The main application class. Member variables include the vulkan data needed
// for rendering some basis scenes, scene data and GUI. Contains the main rendering
// loop and is instantiated in program main.
//

#ifndef APPLICATION_H
#define APPLICATION_H

#include <common/Model.h> // the model class
#include <common/Camera.h> // the camera struct
#include <common/SpotLight.h>
#include <common/types.h>

#include <math/primitives/Plane.h>
#include <math/primitives/Cube.h>

// classes for vulkan
#include <hpg/VulkanSetup.h>
#include <hpg/SwapChain.h>
#include <hpg/FrameBuffer.h>
#include <hpg/GBuffer.h>
#include <hpg/Buffers.h>
#include <hpg/Skybox.h>
#include <hpg/ShadowMap.h>

// glfw window library
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// vectors, matrices ...
#include <glm/glm.hpp>

// reporting and propagating exceptions
#include <iostream> 
#include <stdexcept>
#include <vector> // very handy containers of objects
#include <array>
#include <string> // string for file name
#include <chrono> // time 

class Application {

public:
    void run();

private:
    //-Initialise the app----------------------------------------------------------------------------------------//
    void init();

    //-Build the scene by setting its data-----------------------------------------------------------------------//
    void buildScene();

    //-Initialise all our data for rendering---------------------------------------------------------------------//
    void initVulkan();
    void recreateVulkanData();

    //-Initialise Imgui data-------------------------------------------------------------------------------------//
    void initImGui();
    void uploadFonts();

    //-Initialise GLFW window------------------------------------------------------------------------------------//
    void initWindow();

    //-Descriptor initialisation functions-----------------------------------------------------------------------//
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets(UI32 swapChainImages);

    //-Update uniform buffer-------------------------------------------------------------------------------------//
    void updateUniformBuffers(uint32_t currentImage);

    //-Command buffer initialisation functions-------------------------------------------------------------------//
    void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags);
    void createCommandBuffers(uint32_t count, VkCommandBuffer* commandBuffers, VkCommandPool& commandPool);

    //-Record command buffers for rendering (geom and gui)-------------------------------------------------------//
    void buildCompositionCommandBuffer(UI32 cmdBufferIndex);
    void buildGuiCommandBuffer(UI32 cmdBufferIndex);
    void buildOffscreenCommandBuffer(UI32 cmdBufferIndex);
    void buildShadowMapCommandBuffer(VkCommandBuffer cmdBuffer);

    //-Window/Input Callbacks------------------------------------------------------------------------------------//
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    //-Sync structures-------------------------------------------------------------------------------------------//
    void createSyncObjects();

    //-The main loop---------------------------------------------------------------------------------------------//
    void mainLoop();

    //-Per frame functions---------------------------------------------------------------------------------------//
    void drawFrame();
    void setGUI();
    int processKeyInput();
    void processMouseInput(glm::dvec2& offset);

    //-End of application cleanup--------------------------------------------------------------------------------//
    void cleanup();

public:
    //-Members---------------------------------------------------------------------------------------------------//
    GLFWwindow* window;

    VulkanSetup vkSetup; // instance, device (logical, physical), ...
    SwapChain swapChain; // sc images, pipelines, ...
    FrameBuffer frameBuffer;
    GBuffer gBuffer;

    Model model;

    Skybox skybox;

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;
    std::vector<Texture> textures;

    Light lights[1];

    SpotLight spotLight;

    ShadowMap shadowMap;

    Camera camera;

    Plane floor;
    Cube cube;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;
    // descriptor set handles
    std::vector<VkDescriptorSet> compositionDescriptorSets; 
    VkDescriptorSet offScreenDescriptorSet;
    VkDescriptorSet skyboxDescriptorSet;
    VkDescriptorSet shadowMapDescriptorSet;

    VkCommandPool renderCommandPool;
    std::vector<VkCommandBuffer> offScreenCommandBuffers;
    std::vector<VkCommandBuffer> renderCommandBuffers;
    std::vector<VkCommandBuffer> shadowMapCommandBuffers;

    VkCommandPool imGuiCommandPool;
    std::vector<VkCommandBuffer> imGuiCommandBuffers;

    std::vector<VkSemaphore> offScreenSemaphores;
    std::vector<VkSemaphore> imageAvailableSemaphores; // 1 semaphore per frame, GPU-GPU sync
    std::vector<VkSemaphore> renderFinishedSemaphores;

    std::vector<VkFence> inFlightFences; // 1 fence per frame, CPU-GPU sync
    std::vector<VkFence> imagesInFlight;

    glm::vec3 translate = glm::vec3(0.0f);
    glm::vec3 rotate = glm::vec3(0.0f);;
    float scale = 1.0f;
    
    bool shouldExit         = false;
    bool framebufferResized = false;
    bool firstMouse         = true;

    int attachmentNum = 0;

    glm::dvec2 prevMouse;
    glm::dvec2 currMouse;

    std::chrono::steady_clock::time_point prevTime;
    std::chrono::steady_clock::time_point currTime;
    float deltaTime;

    size_t currentFrame = 0;
    uint32_t imageIndex = 0; // idx of curr sc image
};

//
// Template function definitions
//

#endif // !APPLICATION_H