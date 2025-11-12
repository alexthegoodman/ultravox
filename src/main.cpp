#define GLM_ENABLE_EXPERIMENTAL

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h" 

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vk_mem_alloc.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <imgui_impl_vulkan.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <mutex>
#include <map>
#include <set>
#include <glm/gtx/string_cast.hpp>

#include <unordered_map>
#include <utility>

#include "Logger.h"
#include "Vertex.h"
#include "Voxel.cpp"
#include "Chunk.h"
#include "ChunkManager.h"
#include "Editor.cpp"
#include "Camera3D.cpp"
#include "PhysicsSystem.h"
#include "helpers.h"
#include "PlayerCharacter.h"
#include "Sphere.h"

const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;
const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

const char* VkResultToString(VkResult result) {
    switch (result) {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_NOT_PERMITTED: return "VK_ERROR_NOT_PERMITTED";
        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_ERROR_NOT_ENOUGH_SPACE_KHR: return "VK_ERROR_NOT_ENOUGH_SPACE_KHR";
        default: return "UNKNOWN_VK_RESULT";
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// struct UniformBufferObject {
//     glm::mat4 model;
//     glm::mat4 view;
//     glm::mat4 proj;
// };

struct alignas(16) UniformBufferObject {
    // glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct MeshPushConstants {
	glm::mat4 model;
};

std::string matrixToString(const glm::mat4& matrix) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4);
    for (int i = 0; i < 4; i++) {
        ss << "[ ";
        for (int j = 0; j < 4; j++) {
            ss << std::setw(10) << matrix[j][i];
            if (j < 3) ss << ", ";
        }
        ss << " ]\n";
    }
    return ss.str();
}

// Usage:
// glm::mat4 modelMatrix = editor.playerCharacter->getModelMatrix();
// LOG("Model Matrix:\n" + matrixToString(modelMatrix));


class VulkanEngine {
public:
    void run() {
        LOG("Starting VulkanEngine");
        initWindow();
        initVulkan();
        initImGui();
        mainLoop();
        cleanup();
        LOG("VulkanEngine finished");
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    bool framebufferResized = false;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    
    VmaAllocator allocator;
    VkDescriptorPool imguiDescriptorPool;

    Editor editor;
    Camera3D camera;
    PhysicsSystem physicsSystem;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VmaAllocation> uniformBuffersAllocations;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    // Chunk rendering data
    std::unordered_map<Chunk::ChunkCoord, std::pair<VkBuffer, VmaAllocation>> chunkVertexBuffers;
    std::unordered_map<Chunk::ChunkCoord, std::pair<VkBuffer, VmaAllocation>> chunkIndexBuffers;

    // Player rendering data
    VkBuffer playerVertexBuffer;
    VmaAllocation playerVertexBufferAllocation;
    VkBuffer playerIndexBuffer;
    VmaAllocation playerIndexBufferAllocation;
    uint32_t playerIndexCount;

    // Map to store active Jolt physics bodies, keyed by voxel world position
    std::map<glm::vec3, JPH::BodyID> activePhysicsBodies;
    float physicsActivationRadius = 10.0f; // Define the radius around the camera for active physics bodies

    // Camera control variables for ImGui
    float currentPitch = 0.0f;
    float currentYaw = 0.0f;
    float prevPanX = 0.0f;
    float prevPanY = 0.0f;
    float panX = prevPanX;
    float panY = prevPanY;

    // Painting state
    bool isLeftMouseButtonPressed = false;
    bool wasLeftMouseButtonPressed = false;
    float paintYLevel = 0.0f;
    std::set<glm::vec3> paintedVoxelsInStroke;

    void initWindow() {
        LOG("Initializing window");
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Engine", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        LOG("Window initialized");
    }

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {
        LOG("Initializing Vulkan");
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
        createAllocator();
        physicsSystem.init();

        LOG("Continuing Vulkan Initialization...");

        // LOG("MeshPushConstants size " + std::to_string(sizeof(MeshPushConstants)));

        createDescriptorSetLayout();
        createGraphicsPipeline();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createPlayerBuffers();


        LOG("Vulkan initialized!");
    }

    void createPlayerBuffers() {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        createSphereMesh(vertices, indices, 20, 20, 0.5f);
        playerIndexCount = static_cast<uint32_t>(indices.size());

        // Create vertex buffer
        VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, playerVertexBuffer, playerVertexBufferAllocation);
        void* vertexData;
        vmaMapMemory(allocator, playerVertexBufferAllocation, &vertexData);
        memcpy(vertexData, vertices.data(), (size_t)vertexBufferSize);
        vmaUnmapMemory(allocator, playerVertexBufferAllocation);

        // Create index buffer
        VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, playerIndexBuffer, playerIndexBufferAllocation);
        void* indexData;
        vmaMapMemory(allocator, playerIndexBufferAllocation, &indexData);
        memcpy(indexData, indices.data(), (size_t)indexBufferSize);
        vmaUnmapMemory(allocator, playerIndexBufferAllocation);
    }

    void destroyPlayerBuffers() {
        vmaDestroyBuffer(allocator, playerVertexBuffer, playerVertexBufferAllocation);
        vmaDestroyBuffer(allocator, playerIndexBuffer, playerIndexBufferAllocation);
    }

    void createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        // if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        //     throw std::runtime_error("Failed to create instance!");
        // }

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error(VkResultToString(result));
        }
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);
        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers!");
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create synchronization objects!");
            }
        }
    }

    void createAllocator() {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.instance = instance;

        if (vmaCreateAllocator(&allocatorInfo, &allocator) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VMA allocator!");
        }
    }

    void initImGui() {
        LOG("Initializing ImGui");
        // Create descriptor pool for ImGui
        VkDescriptorPoolSize pool_sizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        LOG("Creating pool...");

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000;
        pool_info.poolSizeCount = std::size(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;

        if (vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiDescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create ImGui descriptor pool!");
        }

        LOG("Creating context...");

        // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Setup ImGui style
        ImGui::StyleColorsDark();

        LOG("Init ImGUI with GLFW and Vulkan...");

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(window, true);
        
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.ApiVersion = VK_API_VERSION_1_3;
        init_info.Instance = instance;
        init_info.PhysicalDevice = physicalDevice;
        init_info.Device = device;
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        init_info.QueueFamily = indices.graphicsFamily.value();
        init_info.Queue = graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = imguiDescriptorPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = 2;
        init_info.ImageCount = static_cast<uint32_t>(swapChainImages.size());
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        init_info.RenderPass = renderPass;

        LOG("ImGUI Vulkan Init...");

        ImGui_ImplVulkan_Init(&init_info);

        // Upload fonts // NOTE: now handled automatically
        // VkCommandBuffer command_buffer = beginSingleTimeCommands();
        // ImGui_ImplVulkan_CreateFontsTexture();
        // endSingleTimeCommands(command_buffer);
        // ImGui_ImplVulkan_DestroyFontUploadObjects();
        // ImGui_ImplVulkan_DestroyFontsTexture(); // needed?
        LOG("ImGui initialized");
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // // Update view/proj once per frame
        // updateUniformBuffer(currentFrame);
        // vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

        // Update view/proj once per frame
        updateUniformBuffer(currentFrame);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);


        // Render chunks
        for (const auto& pair : editor.chunkManager.getLoadedChunks()) {
            const Chunk::ChunkCoord& coord = pair.first;
            const std::unique_ptr<Chunk>& chunk = pair.second;

            if (!chunk->empty()) {
                if (chunk->isDirty() || chunkVertexBuffers.find(coord) == chunkVertexBuffers.end()) {
                    LOG("chunk->isDirty() recordCommandBuffer: Drawing chunk at " + std::to_string(coord.x) + "," + std::to_string(coord.y) + "," + std::to_string(coord.z) + " with index count: " + std::to_string(chunk->getIndices().size()));
                    updateChunkBuffers(coord, chunk->getVertices(), chunk->getIndices());
                }

                // Push model matrix for this chunk
                glm::mat4 chunkModel = glm::mat4(1.0f);

                MeshPushConstants constants{};
                constants.model = chunkModel;

                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);


                VkBuffer vertexBuffers[] = {chunkVertexBuffers[coord].first};
                VkDeviceSize offsets[] = {0};
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, chunkIndexBuffers[coord].first, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(chunk->getIndices().size()), 1, 0, 0, 0);
            }
        }

        // Render player
        if (editor.playerCharacter) {
            // glm::mat4 playerModel = editor.playerCharacter->getModelMatrix();

            MeshPushConstants constants{};
            constants.model = editor.playerCharacter->getModelMatrix();

            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

            VkBuffer vertexBuffers[] = {playerVertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, playerIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, playerIndexCount, 1, 0, 0, 0);
        }

        // Render ImGui
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer!");
        }
    }

    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("Failed to acquire swap chain image!");
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createGraphicsPipeline();
        createFramebuffers();
    }

    void cleanupSwapChain() {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    // }

        // vkDeviceWaitIdle(device);
    }

    void initializeUniformBuffers() {
        updateUniformBuffer(currentFrame);
    }

    void updateUniformBuffer(uint32_t currentImage) {
        UniformBufferObject ubo{};
        // ubo.model = modelMatrix;
        ubo.view = camera.getView();
        ubo.proj = camera.getProjection(swapChainExtent.width / (float) swapChainExtent.height);
        ubo.proj[1][1] *= -1;

        void* data;
        vmaMapMemory(allocator, uniformBuffersAllocations[currentImage], &data);
        memcpy(data, &ubo, sizeof(ubo));
        vmaUnmapMemory(allocator, uniformBuffersAllocations[currentImage]);
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayoutBinding;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("../../shaders/shader.vert.spv");
        auto fragShaderCode = readFile("../../shaders/shader.frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        // rasterizer.cullMode = VK_CULL_MODE_NONE;
        // rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT; // works with a single voxel, still bad for many
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // ADD THIS:
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        // pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        // pipelineLayoutInfo.setLayoutCount = 1;
        // pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

        // 2. Add push constant range to pipeline layout creation
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(MeshPushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersAllocations.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, uniformBuffers[i], uniformBuffersAllocations[i]);
        }
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = memoryUsage;

        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    void createChunkBuffers(const Chunk::ChunkCoord& coord, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        // Create vertex buffer
        VkDeviceSize vertexBufferSize = sizeof(vertices[0]) * vertices.size();
        VkBuffer vertexStagingBuffer;
        VmaAllocation vertexStagingBufferAllocation;
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, vertexStagingBuffer, vertexStagingBufferAllocation);
        void* vertexData;
        vmaMapMemory(allocator, vertexStagingBufferAllocation, &vertexData);
        memcpy(vertexData, vertices.data(), (size_t) vertexBufferSize);
        vmaUnmapMemory(allocator, vertexStagingBufferAllocation);

        VkBuffer vertexBuffer;
        VmaAllocation vertexBufferAllocation;
        createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, vertexBuffer, vertexBufferAllocation);
        copyBuffer(vertexStagingBuffer, vertexBuffer, vertexBufferSize);
        vmaDestroyBuffer(allocator, vertexStagingBuffer, vertexStagingBufferAllocation);
        chunkVertexBuffers[coord] = {vertexBuffer, vertexBufferAllocation};

        // Create index buffer
        VkDeviceSize indexBufferSize = sizeof(indices[0]) * indices.size();
        LOG("createChunkBuffers: Index vector size: " + std::to_string(indices.size()) + ", calculated buffer size: " + std::to_string(indexBufferSize));
        VkBuffer indexStagingBuffer;
        VmaAllocation indexStagingBufferAllocation;
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY, indexStagingBuffer, indexStagingBufferAllocation);
        void* indexData;
        vmaMapMemory(allocator, indexStagingBufferAllocation, &indexData);
        memcpy(indexData, indices.data(), (size_t) indexBufferSize);
        vmaUnmapMemory(allocator, indexStagingBufferAllocation);

        VkBuffer indexBuffer;
        VmaAllocation indexBufferAllocation;
        createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_GPU_ONLY, indexBuffer, indexBufferAllocation);
        copyBuffer(indexStagingBuffer, indexBuffer, indexBufferSize);
        vmaDestroyBuffer(allocator, indexStagingBuffer, indexStagingBufferAllocation);
        chunkIndexBuffers[coord] = {indexBuffer, indexBufferAllocation};
        LOG("createChunkBuffers: Index buffer created for chunk " + std::to_string(coord.x) + "," + std::to_string(coord.y) + "," + std::to_string(coord.z) + ". Handle: " + std::to_string((uint64_t)indexBuffer) + ", Allocation: " + std::to_string((uint64_t)indexBufferAllocation));
        
        // Mark chunk as not dirty after buffers are updated
        // We need to get the chunk object from the chunk manager to set its dirty flag
        Chunk* chunkPtr = editor.chunkManager.getChunk(coord);
        if (chunkPtr) {
            chunkPtr->meshDirty = false;
        }
    }

    void updateChunkBuffers(const Chunk::ChunkCoord& coord, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
        vkDeviceWaitIdle(device);
        // Destroy existing buffers
        destroyChunkBuffers(coord);
        // Recreate buffers with new data
        createChunkBuffers(coord, vertices, indices);
    }

    void destroyChunkBuffers(const Chunk::ChunkCoord& coord) {
        auto vertexIt = chunkVertexBuffers.find(coord);
        if (vertexIt != chunkVertexBuffers.end()) {
            vmaDestroyBuffer(allocator, vertexIt->second.first, vertexIt->second.second);
            chunkVertexBuffers.erase(vertexIt);
        }

        auto indexIt = chunkIndexBuffers.find(coord);
        if (indexIt != chunkIndexBuffers.end()) {
            vmaDestroyBuffer(allocator, indexIt->second.first, indexIt->second.second);
            chunkIndexBuffers.erase(indexIt);
        }
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    void mainLoop() {
        LOG("Entering main loop");

        initializeUniformBuffers(); // running update in drawFrame()

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            float deltaTime = 1.0f / ImGui::GetIO().Framerate;

            // if (editor.playerCharacter) {
            //     glm::vec3 playerPos = editor.playerCharacter->getPosition();
            //     editor.playerCharacter->update(playerPos);
            // }

            if (editor.isPlayingPreview) {
                physicsSystem.update(deltaTime, 1);
            }

            if (editor.isPlayingPreview && editor.playerCharacter) {
                glm::vec3 movement(0.0f);
                if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                    movement.z -= 1.0f;
                }
                if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                    movement.z += 1.0f;
                }
                if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                    movement.x -= 1.0f;
                }
                if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                    movement.x += 1.0f;
                }

                if (glm::length(movement) > 0.0f) {
                    movement = glm::normalize(movement) * 1.0f;

                    editor.playerCharacter->setLinearVelocity(movement);

                    // JPH::Vec3 currentVel = editor.playerCharacter->character->GetLinearVelocity();
                    // glm::vec3 currentVelGLM = glm::vec3(currentVel.GetX(), currentVel.GetY(), currentVel.GetZ());
                    // glm::vec3 targetVel = movement; // your input
                    // glm::vec3 deltaVel = targetVel - currentVelGLM;

                    // // optionally clamp deltaVel to prevent sudden large jumps
                    // float maxDelta = 0.2f; 
                    // if (glm::length(deltaVel) > maxDelta)
                    //     deltaVel = glm::normalize(deltaVel) * maxDelta;

                    // editor.playerCharacter->character->AddLinearVelocity(JPH::Vec3(deltaVel.x, deltaVel.y, deltaVel.z));

                    glm::vec3 playerPos = editor.playerCharacter->getPosition();
                    // editor.playerCharacter->update(playerPos);
                    editor.playerCharacter->sphere.transform.position = playerPos;
                }
            }

            if (editor.playerCharacter) {                
                // Update camera to follow player
                if (editor.isPlayingPreview) {
                    glm::vec3 playerPos = editor.playerCharacter->getPosition();
                    // glm::vec3 playerPos = editor.playerCharacter->sphere.getPosition();
                    glm::vec3 newPos = playerPos + glm::vec3(0.0f, 2.0f, 5.0f);
                    camera.setPosition(newPos.x, newPos.y, newPos.z);
                    camera.lookAt(playerPos);

                    // editor.playerCharacter->sphere.transform.position = playerPos;
                }
            }

            

            // Update mouse button state
            wasLeftMouseButtonPressed = isLeftMouseButtonPressed;
            isLeftMouseButtonPressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

            // Painting logic
            if (editor.isPainting && isLeftMouseButtonPressed) {
                double mouseX, mouseY;
                glfwGetCursorPos(window, &mouseX, &mouseY);

                // Convert mouse coordinates to normalized device coordinates
                float ndcX = (float)(mouseX / swapChainExtent.width) * 2.0f - 1.0f;
                float ndcY = (float)(mouseY / swapChainExtent.height) * 2.0f - 1.0f; // Y is inverted in Vulkan

                glm::vec4 clipCoords = glm::vec4(ndcX, -ndcY, -1.0f, 1.0f); // -1.0 for Z to get near plane

                glm::mat4 inverseProjection = glm::inverse(camera.getProjection(swapChainExtent.width / (float)swapChainExtent.height));
                glm::vec4 eyeCoords = inverseProjection * clipCoords;
                eyeCoords.z = -1.0f;
                eyeCoords.w = 0.0f;

                glm::mat4 inverseView = glm::inverse(camera.getView());
                glm::vec4 worldCoords = inverseView * eyeCoords;
                glm::vec3 rayDirection = glm::normalize(glm::vec3(worldCoords));

                glm::vec3 rayOrigin = camera.position3D;

                // Perform raycast
                PhysicsSystem::RayCastResult rayCastResult = physicsSystem.castRay(rayOrigin, rayDirection);

                if (rayCastResult.hasHit) {
                    // On initial press, store the Y-level
                    if (!wasLeftMouseButtonPressed) {
                        // paintYLevel = rayCastResult.hitPosition.y;
                        paintYLevel = rayCastResult.hitPosition.y + Chunk::VOXEL_SIZE;
                        paintedVoxelsInStroke.clear(); // Clear previous stroke
                    }

                    // Calculate the position for the new voxel
                    glm::vec3 newVoxelPos = rayCastResult.hitPosition + rayCastResult.hitNormal * (Chunk::VOXEL_SIZE / 2.0f);
                    newVoxelPos.y = paintYLevel; // Lock Y-coordinate

                    // Quantize to voxel grid
                    newVoxelPos.x = floor(newVoxelPos.x / Chunk::VOXEL_SIZE) * Chunk::VOXEL_SIZE;
                    newVoxelPos.y = floor(newVoxelPos.y / Chunk::VOXEL_SIZE) * Chunk::VOXEL_SIZE;
                    newVoxelPos.z = floor(newVoxelPos.z / Chunk::VOXEL_SIZE) * Chunk::VOXEL_SIZE;

                    // LOG("NEW VOXEL COORDS: " + std::to_string(newVoxelPos.x) + " " + std::to_string(newVoxelPos.y) + " " + std::to_string(newVoxelPos.z) + " ");

                    // Check if a voxel already exists at this position or if it was painted in this stroke
                    if (paintedVoxelsInStroke.find(newVoxelPos) == paintedVoxelsInStroke.end() &&
                        editor.chunkManager.getVoxelWorld(newVoxelPos).type == 0) { // Check if it's air
                        
                        LOG("Add Voxel!");

                        // Add the voxel
                        editor.chunkManager.setVoxelWorld(newVoxelPos, Chunk::VoxelData(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), 1)); // Red voxel
                        paintedVoxelsInStroke.insert(newVoxelPos);
                    }
                }
            } else if (!isLeftMouseButtonPressed && wasLeftMouseButtonPressed) {
                // Mouse button released, save modified chunks
                // TODO: enable this after testing
                // editor.chunkManager.saveModifiedChunks();
                // paintedVoxelsInStroke.clear();
            }

            // Start ImGui frame
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            ImGuizmo::BeginFrame();

            // Set ImGuizmo display size
            ImGuizmo::SetRect(0, 0, (float)swapChainExtent.width, (float)swapChainExtent.height);

            // Get camera matrices
            glm::mat4 view = camera.getView();
            glm::mat4 projection = camera.getProjection(swapChainExtent.width / (float)swapChainExtent.height);

            // ViewManipulate needs the INVERSE of the view matrix (camera's world transform)
            glm::mat4 cameraMatrix = glm::inverse(view);
            
            ImGuizmo::ViewManipulate(
                glm::value_ptr(cameraMatrix),
                // glm::value_ptr(projection),
                20.0f, // length
                ImVec2(swapChainExtent.width - 128, 0),
                ImVec2(128, 128),
                0x10101010
            );

            glm::vec3 newPosition, skew;
            glm::quat newRotation;
            glm::vec3 newScale;
            glm::vec4 perspective;
            glm::decompose(cameraMatrix, newScale, newRotation, newPosition, skew, perspective);

            camera.position3D = newPosition;
            camera.rotation = newRotation;

            // Camera Controls ImGui Window
            ImGui::Begin("Camera Controls");

            currentPitch = camera.getPitch();
            currentYaw = camera.getYaw();

            ImGui::SliderFloat("Pitch", &currentPitch, -89.0f, 89.0f);
            ImGui::SliderFloat("Yaw", &currentYaw, -180.0f, 180.0f);
            ImGui::SliderFloat("Zoom", &camera.zoom, 0.1f, 25.0f);

            if (currentPitch != camera.getPitch()) {
                camera.setPitch(currentPitch);
            }
            if (currentYaw != camera.getYaw()) {
                camera.setYaw(currentYaw);
            }

            ImGui::SliderFloat("Pan X", &panX, -20.0f, 20.0f);
            ImGui::SliderFloat("Pan Y", &panY, -20.0f, 20.0f);

            float deltaX = panX - prevPanX;
            float deltaY = panY - prevPanY;

            if (deltaX != 0.0f || deltaY != 0.0f) {
                camera.pan(deltaX, deltaY);
            }

            prevPanX = panX;
            prevPanY = panY;

            // Optional: Add a reset button
            if (ImGui::Button("Reset Pan")) {
                prevPanX = 0.0f;
                prevPanY = 0.0f;
            }

            if (ImGui::Button("Reset Camera")) {
                camera.resetCamera();
            }
            if (ImGui::Button("Top View")) {
                camera.setPosition(0.0f, 10.0f, 0.0f);
                camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));
            }
            ImGui::SameLine();
            if (ImGui::Button("Front View")) {
                camera.setPosition(0.0f, 5.0f, 10.0f);
                camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));
            }
            ImGui::SameLine();
            if (ImGui::Button("Side View")) {
                camera.setPosition(10.0f, 5.0f, 0.0f);
                camera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));
            }

            ImGui::End();

            // Example ImGui window
            ImGui::Begin("Vulkan Engine");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
                        1000.0f / ImGui::GetIO().Framerate, 
                        ImGui::GetIO().Framerate);

            if (ImGui::Button("Add Landscape")) {
                editor.chunkManager.generateFlatLandscape();
            }

            if (ImGui::Button("Inspect Landscape Data")) {
                editor.chunkManager.exportChunkDataToTextFile("world_data/chunk_0_0_0.dat", "chunk_0_0_0.txt");
            }

            if (ImGui::Button("Paint Voxels")) {
                editor.isPainting = !editor.isPainting;
            }
            ImGui::Text(editor.isPainting ? "Painting enabled" : "Painting disabled");

            ImGui::Text("Active Physics Bodies %i", 
                        activePhysicsBodies.size());

            if (ImGui::Button("Add Character")) {
                if (!editor.playerCharacter) {
                    editor.playerCharacter = std::make_unique<PlayerCharacter>(physicsSystem, glm::vec3(25.0f, 4.0f, 25.0f));
                    LOG("PC X: " + std::to_string(editor.playerCharacter->sphere.transform.position.x));
                }
            }

            if (editor.playerCharacter) {
                if (ImGui::Button(editor.isPlayingPreview ? "Stop Preview" : "Play Preview")) {
                    if (editor.isPlayingPreview) {
                        editor.stopPlayingPreview();
                    } else {
                        editor.startPlayingPreview();
                    }
                }
            }
            
            ImGui::End();

            // if (editor.isPlayingPreview && editor.playerCharacter) {
            //     glm::vec3 movement(0.0f);
            //     if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            //         // LOG("W (FORWARD) PRESS");
            //         movement.z -= 1.0f;
            //     }
            //     if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            //         movement.z += 1.0f;
            //     }
            //     if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            //         movement.x -= 1.0f;
            //     }
            //     if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            //         movement.x += 1.0f;
            //     }

            //     if (glm::length(movement) > 0.0f) {
            //         movement = glm::normalize(movement) * 5.0f;
            //     }
                
            //     editor.playerCharacter->setLinearVelocity(movement);
            //     editor.playerCharacter->update();

            //     // Update camera to follow player
            //     glm::vec3 playerPos = editor.playerCharacter->getPosition();
            //     glm::vec3 newPos = playerPos + glm::vec3(0.0f, 2.0f, 5.0f);
            //     camera.setPosition(newPos.x, newPos.y, newPos.z);
            //     camera.lookAt(playerPos);
            // }

            // Render ImGui
            ImGui::Render();

            // Update chunk manager
            editor.chunkManager.updateLoadedChunks(camera.position3D);
            editor.chunkManager.rebuildDirtyChunks();

            // --- Physics Body Management ---
            
            std::vector<OctreeData<Chunk::PhysicsVoxelData>> voxelsInRadius = 
                editor.chunkManager.physicsOctree.queryRadius(toCustomVector3(camera.position3D), physicsActivationRadius);

            std::set<glm::vec3> shouldBeActivePositions;
            for (const auto& octreeData : voxelsInRadius) {
                shouldBeActivePositions.insert(PhysicsSystem::toGLMVec3(octreeData.position));

                if (activePhysicsBodies.find(PhysicsSystem::toGLMVec3(octreeData.position)) == activePhysicsBodies.end()) {
                    // Create new Jolt body
                    JPH::BodyID newBodyID = physicsSystem.createBoxBody(
                        PhysicsSystem::toGLMVec3(octreeData.position),
                        glm::vec3(octreeData.data.size / 2.0f), // Half extent
                        JPH::EMotionType::Static, // Voxels are static
                        ObjectLayer::NON_MOVING
                    );
                    if (!newBodyID.IsInvalid()) {
                        activePhysicsBodies[PhysicsSystem::toGLMVec3(octreeData.position)] = newBodyID;
                    }
                }
            }

            // Clean up inactive bodies
            std::vector<glm::vec3> toRemove;
            for (const auto& pair : activePhysicsBodies) {
                if (shouldBeActivePositions.find(pair.first) == shouldBeActivePositions.end()) {
                    toRemove.push_back(pair.first);
                }
            }

            for (const auto& pos : toRemove) {
                physicsSystem.destroyBody(activePhysicsBodies[pos]);
                activePhysicsBodies.erase(pos);
            }
            // --- End Physics Body Management ---

            drawFrame();
        }

        LOG("Exiting main loop");

        vkDeviceWaitIdle(device);
    }

    void cleanup() {
        LOG("Starting cleanup");

        // editor.chunkManager.saveAllChunks(); ? or perhaps autosave is more appropriate, besides, dont want to overwrite all files maybe? or better to?

        // Destroy all chunk buffers
        for (auto const& [coord, bufferPair] : chunkVertexBuffers) {
            vmaDestroyBuffer(allocator, bufferPair.first, bufferPair.second);
        }
        for (auto const& [coord, bufferPair] : chunkIndexBuffers) {
            vmaDestroyBuffer(allocator, bufferPair.first, bufferPair.second);
        }

        destroyPlayerBuffers();

        cleanupSwapChain();

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vmaDestroyBuffer(allocator, uniformBuffers[i], uniformBuffersAllocations[i]);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        // Cleanup ImGui
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        vkDestroyDescriptorPool(device, imguiDescriptorPool, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        vmaDestroyAllocator(allocator);

        vkDestroyRenderPass(device, renderPass, nullptr);

        physicsSystem.shutdown();

        // causes exception, although its problematic not to destroy it?
        // if (editor.playerCharacter) {
        //     physicsSystem.destroyCharacter(editor.playerCharacter->character);
        // }

        vkDestroyDevice(device, nullptr);

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
        LOG("Cleanup finished");
    }
};

int main() {
    LOG("Application starting");
    VulkanEngine app;

    try {
        app.run();
    } catch (const std::exception& e) {
        LOG("Exception caught: " + std::string(e.what()));
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    LOG("Application finished successfully");
    return EXIT_SUCCESS;
}