#include "jobs.h"
#include <stdlib.h>
#include <string.h>

static job_queue queue;

void jobs_init() {
    queue.head = NULL;
    queue.tail = NULL;
    queue.count = 0;
    queue.running = false;
}

void jobs_shutdown() {
    job* current = queue.head;
    while (current != NULL) {
        job* next = current->next;
        free(current);
        current = next;
    }
    
    queue.head = NULL;
    queue.tail = NULL;
    queue.count = 0;
    queue.running = false;
}

job* job_create_render(vm_state* state) {
    job* new_job = (job*)malloc(sizeof(job));
    if (new_job == NULL) {
        return NULL;
    }
    
    new_job->type = JOB_TYPE_RENDER;
    new_job->data.render.state = state;
    new_job->completed = false;
    new_job->next = NULL;
    
    return new_job;
}

job* job_create_data(vm_state* state, void* output_data) {
    job* new_job = (job*)malloc(sizeof(job));
    if (new_job == NULL) {
        return NULL;
    }
    
    new_job->type = JOB_TYPE_DATA;
    new_job->data.data.state = state;
    new_job->data.data.output_data = output_data;
    new_job->completed = false;
    new_job->next = NULL;
    
    return new_job;
}

job* job_create_custom(void* custom_data, custom_job_func callback) {
    job* new_job = (job*)malloc(sizeof(job));
    if (new_job == NULL) {
        return NULL;
    }
    
    new_job->type = JOB_TYPE_CUSTOM;
    new_job->data.custom.custom_data = custom_data;
    new_job->data.custom.callback = callback;
    new_job->completed = false;
    new_job->next = NULL;
    
    return new_job;
}

void job_queue_add(job* job) {
    if (job == NULL) {
        return;
    }
    
    if (queue.head == NULL) {
        queue.head = job;
        queue.tail = job;
    } else {
        queue.tail->next = job;
        queue.tail = job;
    }
    
    queue.count++;
}

static void process_render_job(job* job) {
    if (job->data.render.state != NULL) {
        vm_push(job->data.render.state, vm_cmd_render, NULL, NULL);
        vm_execute_next(job->data.render.state);
    }
    job->completed = true;
}

static void process_data_job(job* job) {
    if (job->data.data.state != NULL && job->data.data.output_data != NULL) {
        // Copy relevant state data to the output
        memcpy(job->data.data.output_data, job->data.data.state, sizeof(vm_state));
    }
    job->completed = true;
}

static void process_custom_job(job* job) {
    if (job->data.custom.callback != NULL) {
        job->data.custom.callback(job->data.custom.custom_data);
    }
    job->completed = true;
}

void job_queue_process() {
    if (queue.running || queue.head == NULL) {
        return;
    }
    
    queue.running = true;
    
    job* current = queue.head;
    job* prev = NULL;
    
    while (current != NULL) {
        switch (current->type) {
            case JOB_TYPE_RENDER:
                process_render_job(current);
                break;
                
            case JOB_TYPE_DATA:
                process_data_job(current);
                break;
                
            case JOB_TYPE_CUSTOM:
                process_custom_job(current);
                break;
        }
        
        // If job is completed and marked for auto-release, remove it from the queue
        if (current->completed) {
            job* completed_job = current;
            
            if (prev == NULL) {
                queue.head = current->next;
            } else {
                prev->next = current->next;
            }
            
            if (queue.tail == current) {
                queue.tail = prev;
            }
            
            current = current->next;
            free(completed_job);
            queue.count--;
        } else {
            prev = current;
            current = current->next;
        }
    }
    
    queue.running = false;
}

void job_queue_wait_completion() {
    while (!job_queue_is_empty()) {
        job_queue_process();
    }
}

bool job_is_completed(job* job) {
    return job != NULL && job->completed;
}

void job_release(job* job) {
    if (job == NULL) {
        return;
    }
    
    // Find the job in the queue
    job* current = queue.head;
    job* prev = NULL;
    
    while (current != NULL) {
        if (current == job) {
            if (prev == NULL) {
                queue.head = current->next;
            } else {
                prev->next = current->next;
            }
            
            if (queue.tail == current) {
                queue.tail = prev;
            }
            
            free(job);
            queue.count--;
            return;
        }
        
        prev = current;
        current = current->next;
    }
}

int job_queue_get_count() {
    return queue.count;
}

bool job_queue_is_empty() {
    return queue.count == 0;
}

bool job_queue_is_running() {
    return queue.running;
}
