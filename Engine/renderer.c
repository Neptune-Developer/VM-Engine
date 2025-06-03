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

    VkResult result = vkCreateInstance(&create_info, NULL, &ctx->instance);
    if (result != VK_SUCCESS) {
        return;
    }

    VkAndroidSurfaceCreateInfoKHR surface_info = {0};
    surface_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surface_info.window = window;
    
    PFN_vkCreateAndroidSurfaceKHR create_surface = 
        (PFN_vkCreateAndroidSurfaceKHR)vkGetInstanceProcAddr(ctx->instance, "vkCreateAndroidSurfaceKHR");
    if (!create_surface || create_surface(ctx->instance, &surface_info, NULL, &ctx->surface) != VK_SUCCESS) {
        vkDestroyInstance(ctx->instance, NULL);
        return;
    }

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, NULL);
    if (device_count == 0) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        vkDestroyInstance(ctx->instance, NULL);
        return;
    }
    
    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    if (!devices) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        vkDestroyInstance(ctx->instance, NULL);
        return;
    }
    
    vkEnumeratePhysicalDevices(ctx->instance, &device_count, devices);
    
    int device_found = 0;
    for (uint32_t i = 0; i < device_count; i++) {
        VkPhysicalDeviceProperties device_props;
        vkGetPhysicalDeviceProperties(devices[i], &device_props);
        
        if (device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
            device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            ctx->physical_device = devices[i];
            device_found = 1;
            break;
        }
    }
    
    if (!device_found && device_count > 0) {
        ctx->physical_device = devices[0];
        device_found = 1;
    }
    
    free(devices);
    
    if (!device_found) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        vkDestroyInstance(ctx->instance, NULL);
        return;
    }

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &queue_family_count, NULL);
    if (queue_family_count == 0) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        vkDestroyInstance(ctx->instance, NULL);
        return;
    }
    
    VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    if (!queue_families) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        vkDestroyInstance(ctx->instance, NULL);
        return;
    }
    
    vkGetPhysicalDeviceQueueFamilyProperties(ctx->physical_device, &queue_family_count, queue_families);

    int found_queue = 0;
    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(ctx->physical_device, i, ctx->surface, &present_support);
            if (present_support) {
                ctx->graphics_family = i;
                found_queue = 1;
                break;
            }
        }
    }
    free(queue_families);
    
    if (!found_queue) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        vkDestroyInstance(ctx->instance, NULL);
        return;
    }

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

    if (vkCreateDevice(ctx->physical_device, &device_info, NULL, &ctx->device) != VK_SUCCESS) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
        vkDestroyInstance(ctx->instance, NULL);
        return;
    }
    
    vkGetDeviceQueue(ctx->device, ctx->graphics_family, 0, &ctx->graphics_queue);

    ctx->image_available_semaphore = VK_NULL_HANDLE;
    ctx->render_finished_semaphore = VK_NULL_HANDLE;
    ctx->in_flight_fence = VK_NULL_HANDLE;
    
    VkSemaphoreCreateInfo semaphore_info = {0};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fence_info = {0};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    if (vkCreateSemaphore(ctx->device, &semaphore_info, NULL, &ctx->image_available_semaphore) != VK_SUCCESS ||
        vkCreateSemaphore(ctx->device, &semaphore_info, NULL, &ctx->render_finished_semaphore) != VK_SUCCESS ||
        vkCreateFence(ctx->device, &fence_info, NULL, &ctx->in_flight_fence) != VK_SUCCESS) {
        renderer_cleanup(ctx);
        return;
    }

    if (!create_swapchain(ctx) ||
        !create_render_pass(ctx) ||
        !create_framebuffers(ctx) ||
        !create_command_pool(ctx) ||
        !create_command_buffer(ctx)) {
        renderer_cleanup(ctx);
        return;
    }
}

int create_swapchain(vulkan_context* ctx) {
    VkSurfaceCapabilitiesKHR capabilities;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->physical_device, ctx->surface, &capabilities) != VK_SUCCESS) {
        return 0;
    }

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->surface, &format_count, NULL);
    if (format_count == 0) {
        return 0;
    }
    
    VkSurfaceFormatKHR* formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    if (!formats) {
        return 0;
    }
    
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->physical_device, ctx->surface, &format_count, formats);
    
    VkSurfaceFormatKHR surface_format = formats[0];
    for (uint32_t i = 0; i < format_count; i++) {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
            surface_format = formats[i];
            break;
        }
    }
    free(formats);
    
    ctx->swap_chain_extent = capabilities.currentExtent;
    ctx->swap_chain_format = surface_format.format;

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount) {
        image_count = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swap_info = {0};
    swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_info.surface = ctx->surface;
    swap_info.minImageCount = image_count;
    swap_info.imageFormat = ctx->swap_chain_format;
    swap_info.imageColorSpace = surface_format.colorSpace;
    swap_info.imageExtent = ctx->swap_chain_extent;
    swap_info.imageArrayLayers = 1;
    swap_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_info.preTransform = capabilities.currentTransform;
    swap_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swap_info.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(ctx->device, &swap_info, NULL, &ctx->swap_chain) != VK_SUCCESS) {
        return 0;
    }

    vkGetSwapchainImagesKHR(ctx->device, ctx->swap_chain, &ctx->image_count, NULL);
    ctx->swap_chain_images = malloc(sizeof(VkImage) * ctx->image_count);
    if (!ctx->swap_chain_images) {
        vkDestroySwapchainKHR(ctx->device, ctx->swap_chain, NULL);
        return 0;
    }
    
    vkGetSwapchainImagesKHR(ctx->device, ctx->swap_chain, &ctx->image_count, ctx->swap_chain_images);

    ctx->swap_chain_image_views = malloc(sizeof(VkImageView) * ctx->image_count);
    if (!ctx->swap_chain_image_views) {
        free(ctx->swap_chain_images);
        vkDestroySwapchainKHR(ctx->device, ctx->swap_chain, NULL);
        return 0;
    }
    
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
        
        if (vkCreateImageView(ctx->device, &view_info, NULL, &ctx->swap_chain_image_views[i]) != VK_SUCCESS) {
            for (uint32_t j = 0; j < i; j++) {
                vkDestroyImageView(ctx->device, ctx->swap_chain_image_views[j], NULL);
            }
            free(ctx->swap_chain_image_views);
            free(ctx->swap_chain_images);
            vkDestroySwapchainKHR(ctx->device, ctx->swap_chain, NULL);
            return 0;
        }
    }
    
    return 1;
}

int create_render_pass(vulkan_context* ctx) {
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

    VkSubpassDependency dependency = {0};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(ctx->device, &render_pass_info, NULL, &ctx->render_pass) != VK_SUCCESS) {
        return 0;
    }
    
    return 1;
}

int create_framebuffers(vulkan_context* ctx) {
    ctx->framebuffers = malloc(sizeof(VkFramebuffer) * ctx->image_count);
    if (!ctx->framebuffers) {
        return 0;
    }
    
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

        if (vkCreateFramebuffer(ctx->device, &framebuffer_info, NULL, &ctx->framebuffers[i]) != VK_SUCCESS) {
            for (uint32_t j = 0; j < i; j++) {
                vkDestroyFramebuffer(ctx->device, ctx->framebuffers[j], NULL);
            }
            free(ctx->framebuffers);
            return 0;
        }
    }
    
    return 1;
}

int create_command_pool(vulkan_context* ctx) {
    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = ctx->graphics_family;

    if (vkCreateCommandPool(ctx->device, &pool_info, NULL, &ctx->command_pool) != VK_SUCCESS) {
        return 0;
    }
    
    return 1;
}

int create_command_buffer(vulkan_context* ctx) {
    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = ctx->command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(ctx->device, &alloc_info, &ctx->command_buffer) != VK_SUCCESS) {
        return 0;
    }
    
    return 1;
}

void renderer_draw(vulkan_context* ctx, float* clear_color) {
    vkWaitForFences(ctx->device, 1, &ctx->in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(ctx->device, 1, &ctx->in_flight_fence);
    
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(ctx->device, ctx->swap_chain, UINT64_MAX, 
                                           ctx->image_available_semaphore, VK_NULL_HANDLE, &image_index);
    
    if (result != VK_SUCCESS) {
        return;
    }

    vkResetCommandBuffer(ctx->command_buffer, 0);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(ctx->command_buffer, &begin_info) != VK_SUCCESS) {
        return;
    }

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
    
    if (vkEndCommandBuffer(ctx->command_buffer) != VK_SUCCESS) {
        return;
    }

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &ctx->image_available_semaphore;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &ctx->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &ctx->render_finished_semaphore;

    if (vkQueueSubmit(ctx->graphics_queue, 1, &submit_info, ctx->in_flight_fence) != VK_SUCCESS) {
        return;
    }

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &ctx->render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &ctx->swap_chain;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(ctx->graphics_queue, &present_info);
}

void renderer_cleanup(vulkan_context* ctx) {
    if (ctx->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ctx->device);
    }
    
    if (ctx->framebuffers) {
        for (uint32_t i = 0; i < ctx->image_count; i++) {
            if (ctx->framebuffers[i] != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(ctx->device, ctx->framebuffers[i], NULL);
            }
        }
        free(ctx->framebuffers);
    }
    
    if (ctx->swap_chain_image_views) {
        for (uint32_t i = 0; i < ctx->image_count; i++) {
            if (ctx->swap_chain_image_views[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(ctx->device, ctx->swap_chain_image_views[i], NULL);
            }
        }
        free(ctx->swap_chain_image_views);
    }
    
    if (ctx->swap_chain_images) {
        free(ctx->swap_chain_images);
    }
    
    if (ctx->command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(ctx->device, ctx->command_pool, NULL);
    }
    
    if (ctx->render_pass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(ctx->device, ctx->render_pass, NULL);
    }
    
    if (ctx->swap_chain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ctx->device, ctx->swap_chain, NULL);
    }
    
    if (ctx->image_available_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(ctx->device, ctx->image_available_semaphore, NULL);
    }
    
    if (ctx->render_finished_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(ctx->device, ctx->render_finished_semaphore, NULL);
    }
    
    if (ctx->in_flight_fence != VK_NULL_HANDLE) {
        vkDestroyFence(ctx->device, ctx->in_flight_fence, NULL);
    }
    
    if (ctx->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
    }
    
    if (ctx->device != VK_NULL_HANDLE) {
        vkDestroyDevice(ctx->device, NULL);
    }
    
    if (ctx->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(ctx->instance, NULL);
    }
}
