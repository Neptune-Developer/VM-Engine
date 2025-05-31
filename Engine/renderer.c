#include "renderer.h"
#include <stdlib.h>
#include <string.h>

void renderer_init(vulkan_context* ctx, ANativeWindow* window) {
    memset(ctx, 0, sizeof(vulkan_context));
    
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "vm engine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "vm engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    const char* extensions[] = {"VK_KHR_surface", "VK_KHR_android_surface"};
    
    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = 2;
    create_info.ppEnabledExtensionNames = extensions;

    vkCreateInstance(&create_info, NULL, &ctx->instance);

    VkAndroidSurfaceCreateInfoKHR surface_info = {0};
    surface_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surface_info.window = window;
    
    PFN_vkCreateAndroidSurfaceKHR create_surface = 
        (PFN_vkCreateAndroidSurfaceKHR)vkGetInstanceProcAddr(ctx->instance, "vkCreateAndroidSurfaceKHR");
    create_surface(ctx->instance, &surface_info, NULL, &ctx->surface);

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, NULL);
    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, devices);
    ctx->physical_device = devices[0];
    free(devices);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &queue_family_count, queue_families);

    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physical_device, i, ctx->surface, &present_support);
            if (present_support) {
                ctx->graphics_family = i;
                break;
            }
        }
    }
    free(queue_families);

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {0};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = ctx->graphics_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    const char* device_extensions[] = {"VK_KHR_swapchain"};
    
    VkDeviceCreateInfo device_info = {0};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = device_extensions;

    vkCreateDevice(ctx->physical_device, &device_info, NULL, &ctx->device);
    vkGetDeviceQueue(ctx->device, ctx->graphics_family, 0, &ctx->graphics_queue);

    create_swapchain(ctx);
    create_render_pass(ctx);
    create_framebuffers(ctx);
    create_command_pool(ctx);
    create_command_buffer(ctx);
}

void create_swapchain(vulkan_context* ctx) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_device, ctx->surface, &capabilities);

    ctx->swap_chain_extent = capabilities.currentExtent;
    ctx->swap_chain_format = VK_FORMAT_B8G8R8A8_UNORM;

    VkSwapchainCreateInfoKHR swap_info = {0};
    swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_info.surface = ctx->surface;
    swap_info.minImageCount = capabilities.minImageCount + 1;
    swap_info.imageFormat = ctx->swap_chain_format;
    swap_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swap_info.imageExtent = ctx->swap_chain_extent;
    swap_info.imageArrayLayers = 1;
    swap_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_info.preTransform = capabilities.currentTransform;
    swap_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swap_info.clipped = VK_TRUE;

    vkCreateSwapchainKHR(ctx->device, &swap_info, NULL, &ctx->swap_chain);

    vkGetSwapchainImagesKHR(ctx->device, ctx->swap_chain, &ctx->image_count, NULL);
    ctx->swap_chain_images = malloc(sizeof(VkImage) * ctx->image_count);
    vkGetSwapchainImagesKHR(ctx->device, ctx->swap_chain, &ctx->image_count, ctx->swap_chain_images);

    ctx->swap_chain_image_views = malloc(sizeof(VkImageView) * ctx->image_count);
    for (uint32_t i = 0; i < ctx->image_count; i++) {
        VkImageViewCreateInfo view_info = {0};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = ctx->swap_chain_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = ctx->swap_chain_format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        
        vkCreateImageView(ctx->device, &view_info, NULL, &ctx->swap_chain_image_views[i]);
    }
}

void create_render_pass(vulkan_context* ctx) {
    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = ctx->swap_chain_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {0};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;

    vkCreateRenderPass(ctx->device, &render_pass_info, NULL, &ctx->render_pass);
}

void create_framebuffers(vulkan_context* ctx) {
    ctx->framebuffers = malloc(sizeof(VkFramebuffer) * ctx->image_count);
    for (uint32_t i = 0; i < ctx->image_count; i++) {
        VkImageView attachments[] = {ctx->swap_chain_image_views[i]};

        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = ctx->render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = ctx->swap_chain_extent.width;
        framebuffer_info.height = ctx->swap_chain_extent.height;
        framebuffer_info.layers = 1;

        vkCreateFramebuffer(ctx->device, &framebuffer_info, NULL, &ctx->framebuffers[i]);
    }
}

void create_command_pool(vulkan_context* ctx) {
    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = ctx->graphics_family;

    vkCreateCommandPool(ctx->device, &pool_info, NULL, &ctx->command_pool);
}

void create_command_buffer(vulkan_context* ctx) {
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = ctx->command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(ctx->device, &alloc_info, &ctx->command_buffer);
}

void renderer_draw(vulkan_context* ctx, float* clear_color) {
    uint32_t image_index;
    vkAcquireNextImageKHR(ctx->device, ctx->swap_chain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &image_index);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(ctx->command_buffer, &begin_info);

    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = ctx->render_pass;
    render_pass_info.framebuffer = ctx->framebuffers[image_index];
    render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
    render_pass_info.renderArea.extent = ctx->swap_chain_extent;

    VkClearValue clear_val = {{{clear_color[0], clear_color[1], clear_color[2], clear_color[3]}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_val;

    vkCmdBeginRenderPass(ctx->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(ctx->command_buffer);
    vkEndCommandBuffer(ctx->command_buffer);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &ctx->command_buffer;

    vkQueueSubmit(ctx->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(ctx->graphics_queue);

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &ctx->swap_chain;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(ctx->graphics_queue, &present_info);
}

void renderer_cleanup(vulkan_context* ctx) {
    vkDeviceWaitIdle(ctx->device);
    
    for (uint32_t i = 0; i < ctx->image_count; i++) {
        vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], NULL);
        vkDestroyImageView(ctx->device, ctx->swap_chain_image_views[i], NULL);
    }
    
    free(ctx->framebuffers);
    free(ctx->swap_chain_images);
    free(ctx->swap_chain_image_views);
    
    vkDestroyCommandPool(ctx->device, ctx->command_pool, NULL);
    vkDestroyRenderPass(ctx->device, ctx->render_pass, NULL);
    vkDestroySwapchainKHR(ctx->device, ctx->swap_chain, NULL);
    vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    vkDestroyDevice(ctx->device, NULL);
    vkDestroyInstance(ctx->instance, NULL);
}
