#pragma once
#include <vulkan/vulkan.h>
#include <android/native_window.h>

typedef struct {
    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physical_device;
    VkQueue graphics_queue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swap_chain;
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    VkRenderPass render_pass;
    VkFramebuffer* framebuffers;
    VkImage* swap_chain_images;
    VkImageView* swap_chain_image_views;
    VkFormat swap_chain_format;
    VkExtent2D swap_chain_extent;
    uint32_t graphics_family;
    uint32_t image_count;
} vulkan_context;

void renderer_init(vulkan_context* ctx, ANativeWindow* window);
void renderer_draw(vulkan_context* ctx, float* clear_color);
void renderer_cleanup(vulkan_context* ctx);

void create_swapchain(vulkan_context* ctx);
void create_render_pass(vulkan_context* ctx);
void create_framebuffers(vulkan_context* ctx);
void create_command_pool(vulkan_context* ctx);
void create_command_buffer(vulkan_context* ctx);
