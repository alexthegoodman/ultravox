#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

class TextureManager {
public:
    TextureManager(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    ~TextureManager();

    bool loadTexture(const std::string& path);
    const std::vector<std::string>& getTextureNames() const { return textureNames; }
    VkImageView getTextureImageView(int textureId) const;
    VkSampler getTextureSampler() const { return textureSampler; }

private:
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);

    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;

    std::vector<VkImage> textureImages;
    std::vector<VkDeviceMemory> textureImageMemories;
    std::vector<VkImageView> textureImageViews;
    VkSampler textureSampler;

    std::vector<std::string> textureNames;
};
