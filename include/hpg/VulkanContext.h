///////////////////////////////////////////////////////
// VulkanContext class declaration
///////////////////////////////////////////////////////

// 
// A helper class that creates an contains the vulkan data used in an application
// it aims to contain the vulkan structs that do not change during the lifetime of the
// application, such as the instance, the device, and the surface (although this is the case
// for single windowed applications, I assume support for multiple windows would require
// multiple surfaces). An application will have to use this class's member variables to
// run, the latter are NOT initiated when the object is created but when the initSetup function
// is called. A GLFW window needs to be initialised first and passed as an argument to the
// function so that vulkan can work with it. A reference of the window is kept as a pointer 
// for convenience.
//

#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

// constants and structs
#include <common/utils.h>

// vulkan definitions
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

// glfw window library
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


class VulkanContext {
public:
    //-----------------------------------------------------------------------------------------------------------//
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

public:
    void init(GLFWwindow* window);
    void cleanup();

    //-Swap chain support----------------------------------------------------------------------------------------//
    inline SwapChainSupportDetails supportDetails() { return _swapChainSupportDetails; }
    void querySwapChainSupport();

private:
    //-Vulkan instance-------------------------------------------------------------------------------------------//
	void createInstance();	
	std::vector<const char*> getRequiredExtensions(); // extensions required for instance

    //-Validation layers-----------------------------------------------------------------------------------------//
    void setupDebugMessenger();
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
            const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks* pAllocator);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    bool checkValidationLayerSupport();
    
    //-Vulkan surface (window)-----------------------------------------------------------------------------------//
    void createSurface();
    
    //-Vulkan devices--------------------------------------------------------------------------------------------//
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    void createLogicalDevice();


public:
    //-Members---------------------------------------------------------------------------------------------------//
    GLFWwindow* _window;

	VkInstance instance;
    
    VkDebugUtilsMessengerEXT debugMessenger;

    VkSurfaceKHR surface;

    VkPhysicalDevice physicalDevice;
    VkDevice         device;
    VkQueue          graphicsQueue;
    VkQueue          presentQueue;

    SwapChainSupportDetails  _swapChainSupportDetails;

    VkPhysicalDeviceProperties deviceProperties;
};

#endif // !VULKAN_CONTEXT_H

