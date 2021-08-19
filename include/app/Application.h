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

// classes for rendering
#include <hpg/Renderer.h>
#include <hpg/VulkanContext.h>
#include <hpg/SwapChain.h>
#include <hpg/Buffer.h>
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

struct Light {
    glm::vec4 pos;
    glm::vec3 color;
    float radius;
};

// TODO: organise uniform buffer objects better
struct OffScreenUbo {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
    glm::mat4 normal;
};

struct CompositionUBO {
    glm::vec4 guiData;
    glm::mat4 depthMVP;
    glm::mat4 cameraMVP;
    Light lights[1];
};


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
    void updateUniformBuffers(UI32 currentImage);

    //-Command buffer initialisation functions-------------------------------------------------------------------//
    void createCommandBuffers(UI32 count, VkCommandBuffer* commandBuffers, VkCommandPool& commandPool);

    //-Record command buffers for rendering (geom and gui)-------------------------------------------------------//
    void buildCompositionCommandBuffer(UI32 cmdBufferIndex);
    void buildGuiCommandBuffer(UI32 cmdBufferIndex);
    void buildOffscreenCommandBuffer(UI32 cmdBufferIndex);
    void buildShadowMapCommandBuffer(VkCommandBuffer cmdBuffer);

    //-Pipelines-------------------------------------------------------------------------------------------------//  
    void createForwardPipeline(VkDescriptorSetLayout* descriptorSetLayout);
    void createDeferredPipelines(VkDescriptorSetLayout* descriptorSetLayout);

    //-Window/Input Callbacks------------------------------------------------------------------------------------//
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

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
    GLFWwindow* _window;

    Renderer _renderer;

    Model model;

    Skybox skybox;

    Buffer vertexBuffer;
    Buffer indexBuffer;
    std::vector<Texture> textures;

    Light lights[1];

    SpotLight spotLight;

    ShadowMap shadowMap;

    Camera camera;

    Plane floor;
    Cube cube;


    VkPipelineLayout _fwdPipelineLayout;
    VkPipeline       _fwdPipeline;

    VkPipelineLayout _deferredPipelineLayout;
    VkPipeline _compositionPipeline;
    VkPipeline _offScreenPipeline;
    VkPipeline _skyboxPipeline;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;

    std::vector<VkDescriptorSet> compositionDescriptorSets; 
    VkDescriptorSet offScreenDescriptorSet;
    VkDescriptorSet skyboxDescriptorSet;
    VkDescriptorSet shadowMapDescriptorSet;

    Buffer _offScreenUniform;
    Buffer _compositionUniforms;

    std::vector<VkCommandBuffer> offScreenCommandBuffers;
    std::vector<VkCommandBuffer> renderCommandBuffers;

    std::vector<VkCommandBuffer> imGuiCommandBuffers;

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