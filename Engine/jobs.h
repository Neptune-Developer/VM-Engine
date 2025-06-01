#pragma once

#include "vm_engine.h"
#include <stdbool.h>

typedef enum {
    JOB_TYPE_RENDER,
    JOB_TYPE_DATA,
    JOB_TYPE_CUSTOM
} job_type;

typedef struct job job;

typedef void (*render_job_func)(vm_state* state);
typedef void (*data_job_func)(vm_state* state, void* output_data);
typedef void (*custom_job_func)(void* custom_data);

typedef struct {
    vm_state* state;
} render_job_data;

typedef struct {
    vm_state* state;
    void* output_data;
} data_job_data;

typedef struct {
    void* custom_data;
    custom_job_func callback;
} custom_job_data;

typedef union {
    render_job_data render;
    data_job_data data;
    custom_job_data custom;
} job_data;

struct job {
    job_type type;
    job_data data;
    bool completed;
    job* next;
};

typedef struct {
    job* head;
    job* tail;
    int count;
    bool running;
} job_queue;

void jobs_init();
void jobs_shutdown();

job* job_create_render(vm_state* state);
job* job_create_data(vm_state* state, void* output_data);
job* job_create_custom(void* custom_data, custom_job_func callback);

void job_queue_add(job* job);
void job_queue_process();
void job_queue_wait_completion();

bool job_is_completed(job* job);
void job_release(job* job);

int job_queue_get_count();
bool job_queue_is_empty();
bool job_queue_is_running();
