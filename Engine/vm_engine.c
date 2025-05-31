#include "vm_engine.h"
#include "renderer.h"
#include "checkinstance.h"
#include <stdlib.h>
#include <string.h>

vm_state* vm_create(ANativeWindow* window) {
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
