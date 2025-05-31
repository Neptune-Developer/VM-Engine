#pragma once
#include <vulkan/vulkan.h>
#include <android/native_window.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    vm_cmd_render,
    vm_cmd_update,
    vm_cmd_init,
    vm_cmd_cleanup,
    vm_cmd_clear_color,
    vm_cmd_custom
} vm_command_type;

typedef struct {
    vm_command_type type;
    void* data;
    void (*callback)(void);
} vm_stack_item;

typedef struct {
    vm_stack_item* items;
    int top;
    int capacity;
} vm_stack;

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

typedef struct {
    vm_stack stack;
    vulkan_context vk;
    ANativeWindow* window;
    int initialized;
    float clear_color[4];
} vm_state;

vm_state* vm_create(ANativeWindow* window) {
    vm_state* state = malloc(sizeof(vm_state));
    memset(state, 0, sizeof(vm_state));
    
    state->window = window;
    state->stack.capacity = 256;
    state->stack.items = malloc(sizeof(vm_stack_item) * state->stack.capacity);
    state->stack.top = -1;
    state->clear_color[3] = 1.0f;
    
    return state;
}

void vm_destroy(vm_state* state) {
    if (!state) return;
    
    if (state->initialized) {
        vkDeviceWaitIdle(state->vk.device);
        
        for (uint32_t i = 0; i < state->vk.image_count; i++) {
            vkDestroyFramebuffer(state->vk.device, state->vk.framebuffers[i], NULL);
            vkDestroyImageView(state->vk.device, state->vk.swap_chain_image_views[i], NULL);
        }
        
        free(state->vk.framebuffers);
        free(state->vk.swap_chain_images);
        free(state->vk.swap_chain_image_views);
        
        vkDestroyCommandPool(state->vk.device, state->vk.command_pool, NULL);
        vkDestroyRenderPass(state->vk.device, state->vk.render_pass, NULL);
        vkDestroySwapchainKHR(state->vk.device, state->vk.swap_chain, NULL);
        vkDestroySurfaceKHR(state->vk.instance, state->vk.surface, NULL);
        vkDestroyDevice(state->vk.device, NULL);
        vkDestroyInstance(state->vk.instance, NULL);
    }
    
    free(state->stack.items);
    free(state);
}

void vm_push(vm_state* state, vm_command_type type, void* data, void (*callback)(void)) {
    if (state->stack.top >= state->stack.capacity - 1) return;
    
    state->stack.top++;
    state->stack.items[state->stack.top].type = type;
    state->stack.items[state->stack.top].data = data;
    state->stack.items[state->stack.top].callback = callback;
}

static void vm_init_vulkan(vm_state* state) {
    if (state->initialized) return;
    
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

    vkCreateInstance(&create_info, NULL, &state->vk.instance);

    VkAndroidSurfaceCreateInfoKHR surface_info = {0};
    surface_info.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surface_info.window = state->window;
    
    PFN_vkCreateAndroidSurfaceKHR create_surface = 
        (PFN_vkCreateAndroidSurfaceKHR)vkGetInstanceProcAddr(state->vk.instance, "vkCreateAndroidSurfaceKHR");
    create_surface(state->vk.instance, &surface_info, NULL, &state->vk.surface);

    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(state->vk.instance, &device_count, NULL);
    VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(state->vk.instance, &device_count, devices);
    state->vk.physical_device = devices[0];
    free(devices);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(state->vk.physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(state->vk.physical_device, &queue_family_count, queue_families);

    for (uint32_t i = 0; i < queue_family_count; i++) {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(state->vk.physical_device, i, state->vk.surface, &present_support);
            if (present_support) {
                state->vk.graphics_family = i;
                break;
            }
        }
    }
    free(queue_families);

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {0};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = state->vk.graphics_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    const char* device_extensions[] = {"VK_KHR_swapchain"};
    
    VkDeviceCreateInfo device_info = {0};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = device_extensions;

    vkCreateDevice(state->vk.physical_device, &device_info, NULL, &state->vk.device);
    vkGetDeviceQueue(state->vk.device, state->vk.graphics_family, 0, &state->vk.graphics_queue);

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->vk.physical_device, state->vk.surface, &capabilities);

    state->vk.swap_chain_extent = capabilities.currentExtent;
    state->vk.swap_chain_format = VK_FORMAT_B8G8R8A8_UNORM;

    VkSwapchainCreateInfoKHR swap_info = {0};
    swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swap_info.surface = state->vk.surface;
    swap_info.minImageCount = capabilities.minImageCount + 1;
    swap_info.imageFormat = state->vk.swap_chain_format;
    swap_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swap_info.imageExtent = state->vk.swap_chain_extent;
    swap_info.imageArrayLayers = 1;
    swap_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swap_info.preTransform = capabilities.currentTransform;
    swap_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swap_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swap_info.clipped = VK_TRUE;

    vkCreateSwapchainKHR(state->vk.device, &swap_info, NULL, &state->vk.swap_chain);

    vkGetSwapchainImagesKHR(state->vk.device, state->vk.swap_chain, &state->vk.image_count, NULL);
    state->vk.swap_chain_images = malloc(sizeof(VkImage) * state->vk.image_count);
    vkGetSwapchainImagesKHR(state->vk.device, state->vk.swap_chain, &state->vk.image_count, state->vk.swap_chain_images);

    state->vk.swap_chain_image_views = malloc(sizeof(VkImageView) * state->vk.image_count);
    for (uint32_t i = 0; i < state->vk.image_count; i++) {
        VkImageViewCreateInfo view_info = {0};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = state->vk.swap_chain_images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = state->vk.swap_chain_format;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;
        
        vkCreateImageView(state->vk.device, &view_info, NULL, &state->vk.swap_chain_image_views[i]);
    }

    VkAttachmentDescription color_attachment = {0};
    color_attachment.format = state->vk.swap_chain_format;
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

    vkCreateRenderPass(state->vk.device, &render_pass_info, NULL, &state->vk.render_pass);

    state->vk.framebuffers = malloc(sizeof(VkFramebuffer) * state->vk.image_count);
    for (uint32_t i = 0; i < state->vk.image_count; i++) {
        VkImageView attachments[] = {state->vk.swap_chain_image_views[i]};

        VkFramebufferCreateInfo framebuffer_info = {0};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = state->vk.render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = state->vk.swap_chain_extent.width;
        framebuffer_info.height = state->vk.swap_chain_extent.height;
        framebuffer_info.layers = 1;

        vkCreateFramebuffer(state->vk.device, &framebuffer_info, NULL, &state->vk.framebuffers[i]);
    }

    VkCommandPoolCreateInfo pool_info = {0};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = state->vk.graphics_family;

    vkCreateCommandPool(state->vk.device, &pool_info, NULL, &state->vk.command_pool);

    VkCommandBufferAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = state->vk.command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(state->vk.device, &alloc_info, &state->vk.command_buffer);

    state->initialized = 1;
}

static void vm_render_frame(vm_state* state) {
    if (!state->initialized) return;

    uint32_t image_index;
    vkAcquireNextImageKHR(state->vk.device, state->vk.swap_chain, UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &image_index);

    VkCommandBufferBeginInfo begin_info = {0};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(state->vk.command_buffer, &begin_info);

    VkRenderPassBeginInfo render_pass_info = {0};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = state->vk.render_pass;
    render_pass_info.framebuffer = state->vk.framebuffers[image_index];
    render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
    render_pass_info.renderArea.extent = state->vk.swap_chain_extent;

    VkClearValue clear_color = {{{state->clear_color[0], state->clear_color[1], state->clear_color[2], state->clear_color[3]}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(state->vk.command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(state->vk.command_buffer);
    vkEndCommandBuffer(state->vk.command_buffer);

    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &state->vk.command_buffer;

    vkQueueSubmit(state->vk.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(state->vk.graphics_queue);

    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &state->vk.swap_chain;
    present_info.pImageIndices = &image_index;

    vkQueuePresentKHR(state->vk.graphics_queue, &present_info);
}

int vm_execute_next(vm_state* state) {
    if (state->stack.top < 0) return 0;
    
    vm_stack_item item = state->stack.items[state->stack.top];
    state->stack.top--;
    
    switch (item.type) {
        case vm_cmd_init:
            vm_init_vulkan(state);
            break;
        case vm_cmd_render:
            vm_render_frame(state);
            break;
        case vm_cmd_update:
            break;
        case vm_cmd_clear_color:
            if (item.data) {
                memcpy(state->clear_color, item.data, sizeof(float) * 4);
            }
            break;
        case vm_cmd_cleanup:
            vm_destroy(state);
            break;
        case vm_cmd_custom:
            if (item.callback) {
                item.callback();
            }
            break;
    }
    
    return 1;
}

void vm_execute_all(vm_state* state) {
    while (vm_execute_next(state));
}

int vm_is_empty(vm_state* state) {
    return state->stack.top < 0;
}

int vm_stack_size(vm_state* state) {
    return state->stack.top + 1;
}
