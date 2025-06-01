#include "vm_engine.h"
#include "renderer.h"
#include "checkinstance.h"
#include "flags.h"
#include <stdlib.h>
#include <string.h>

vm_state* vm_create(ANativeWindow* window) {
    flags_init();
    flag_register_bool("limitfps30", false);
    flag_register_bool("vsync", true);
    flag_register_bool("fullscreen", false);
    flag_register_int("msaa", 4);
    flag_register_int("resolution_width", 1280);
    flag_register_int("resolution_height", 720);
    flag_register_float("gamma", 1.0f);
    flag_register_string("renderer", "vulkan");
    
    check_instance_init();
    check_instance_register(window);
    
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
    
    check_instance_unregister(state->window);
    
    if (state->initialized) {
        renderer_cleanup(&state->vk);
    }
    
    free(state->stack.items);
    free(state);
    
    flags_cleanup();
}

void vm_push(vm_state* state, vm_command_type type, void* data, void (*callback)(void)) {
    if (state->stack.top >= state->stack.capacity - 1) return;
    
    state->stack.top++;
    state->stack.items[state->stack.top].type = type;
    state->stack.items[state->stack.top].data = data;
    state->stack.items[state->stack.top].callback = callback;
}

int vm_execute_next(vm_state* state) {
    if (state->stack.top < 0) return 0;
    
    check_instance_validate();
    
    vm_stack_item item = state->stack.items[state->stack.top];
    state->stack.top--;
    
    switch (item.type) {
        case vm_cmd_init:
            if (!state->initialized) {
                renderer_init(&state->vk, state->window);
                state->initialized = 1;
            }
            break;
            
        case vm_cmd_render:
            if (state->initialized) {
                if (flag_get_bool("limitfps30")) {
                    state->frame_time = 33333333;
                } else {
                    state->frame_time = 0;
                }
                
                if (flag_get_bool("vsync")) {
                    state->vsync_enabled = 1;
                } else {
                    state->vsync_enabled = 0;
                }
                
                renderer_draw(&state->vk, state->clear_color);
            }
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

void vm_set_flag_bool(const char* name, bool value) {
    flag_set_bool(name, value);
}

bool vm_get_flag_bool(const char* name) {
    return flag_get_bool(name);
}

void vm_set_flag_int(const char* name, int value) {
    flag_set_int(name, value);
}

int vm_get_flag_int(const char* name) {
    return flag_get_int(name);
}

void vm_set_flag_float(const char* name, float value) {
    flag_set_float(name, value);
}

float vm_get_flag_float(const char* name) {
    return flag_get_float(name);
}

void vm_set_flag_string(const char* name, const char* value) {
    flag_set_string(name, value);
}

const char* vm_get_flag_string(const char* name) {
    return flag_get_string(name);
}

void vm_parse_flags_from_args(int argc, char** argv) {
    flags_parse_args(argc, argv);
}

void vm_parse_flags_from_file(const char* filename) {
    flags_parse_file(filename);
}
