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

#include <common/types.h>

#include <scene/Model.h> // the model class
#include <scene/Camera.h> // the camera struct
#include <scene/SpotLight.h>
#include <scene/GLTFModel.h>

#include <math/primitives/Plane.h>
#include <math/primitives/Cube.h>

// classes for rendering
#include <hpg/Renderer.h>
#include <hpg/VulkanContext.h>
#include <hpg/SwapChain.h>
#include <hpg/Buffer.h>
#include <hpg/Skybox.h>
#include <hpg/Texture.h>
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
    void run(const char* arg);

private:
    //-Initialise the app----------------------------------------------------------------------------------------//
    void init(const char* arg);

    //-Build the scene by setting its data-----------------------------------------------------------------------//
    void buildScene(const char* arg);

    //-Initialise all our data for rendering---------------------------------------------------------------------//
    void initVulkan();
    void recreateVulkanData();

    //-Initialise Imgui data-------------------------------------------------------------------------------------//
    void initImGui();
    void uploadFonts();

    //-Initialise GLFW window------------------------------------------------------------------------------------//
    void initWindow();

    //-Update uniform buffer-------------------------------------------------------------------------------------//
    void updateUniformBuffers(UI32 currentImage);

    //-Record command buffers for rendering (geom and gui)-------------------------------------------------------//
    void buildGuiCommandBuffer(UI32 cmdBufferIndex);
    void buildShadowMapCommandBuffer(VkCommandBuffer cmdBuffer);
    void recordCommandBuffer(VkCommandBuffer cmdBuffer, UI32 index);

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

    // TODO: MAKE SCENE MORE COHERENT
    Model model;

    GLTFModel _gltfModel;

    Skybox _skybox;

    std::vector<Texture> textures;

    Light lights[1];

    SpotLight spotLight;

    ShadowMap shadowMap;

    Camera camera;

    Plane floor;
    Cube cube;

    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorSetLayout;

    std::vector<VkDescriptorSet> compositionDescriptorSets; 
    VkDescriptorSet offScreenDescriptorSet;
    VkDescriptorSet shadowMapDescriptorSet;

    // TODO: UPDATE BUFFER MANAGEMENT
    Buffer _offScreenUniform;
    Buffer _compositionUniforms;

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