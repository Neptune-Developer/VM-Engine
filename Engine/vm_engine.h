#pragma once
#include "renderer.h"
#include <android/native_window.h>

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
    vm_stack stack;
    vulkan_context vk;
    ANativeWindow* window;
    int initialized;
    float clear_color[4];
} vm_state;

vm_state* vm_create(ANativeWindow* window);
void vm_destroy(vm_state* state);
void vm_push(vm_state* state, vm_command_type type, void* data, void (*callback)(void));
int vm_execute_next(vm_state* state);
void vm_execute_all(vm_state* state);
int vm_is_empty(vm_state* state);
int vm_stack_size(vm_state* state);
