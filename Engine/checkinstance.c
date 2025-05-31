#include "checkinstance.h"
#include <android/log.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_INSTANCES 1
#define LOG_TAG "vm_engine"

static ANativeWindow* active_windows[MAX_INSTANCES];
static int window_count = 0;
static pthread_mutex_t instance_mutex = PTHREAD_MUTEX_INITIALIZER;
static int initialized = 0;

void check_instance_init(void) {
    pthread_mutex_lock(&instance_mutex);
    if (!initialized) {
        for (int i = 0; i < MAX_INSTANCES; i++) {
            active_windows[i] = NULL;
        }
        window_count = 0;
        initialized = 1;
    }
    pthread_mutex_unlock(&instance_mutex);
}

int check_instance_register(ANativeWindow* window) {
    if (!window) return 0;
    
    pthread_mutex_lock(&instance_mutex);
    
    if (!initialized) {
        check_instance_init();
    }
    
    if (window_count >= MAX_INSTANCES) {
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, 
            "more than one instances = someone trying to Clone the app instance or a hacker. Crashing the game engine and app");
        
        pthread_mutex_unlock(&instance_mutex);
        abort();
    }
    
    for (int i = 0; i < window_count; i++) {
        if (active_windows[i] == window) {
            pthread_mutex_unlock(&instance_mutex);
            return 1;
        }
    }
    
    active_windows[window_count] = window;
    window_count++;
    
    pthread_mutex_unlock(&instance_mutex);
    return 1;
}

void check_instance_unregister(ANativeWindow* window) {
    if (!window) return;
    
    pthread_mutex_lock(&instance_mutex);
    
    for (int i = 0; i < window_count; i++) {
        if (active_windows[i] == window) {
            for (int j = i; j < window_count - 1; j++) {
                active_windows[j] = active_windows[j + 1];
            }
            window_count--;
            active_windows[window_count] = NULL;
            break;
        }
    }
    
    pthread_mutex_unlock(&instance_mutex);
}

void check_instance_validate(void) {
    pthread_mutex_lock(&instance_mutex);
    
    if (window_count > MAX_INSTANCES) {
        __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, 
            "more than one instances = someone trying to Clone the app instance or a hacker. Crashing the game engine and app");
        
        pthread_mutex_unlock(&instance_mutex);
        abort();
    }
    
    pthread_mutex_unlock(&instance_mutex);
}

void check_instance_cleanup(void) {
    pthread_mutex_lock(&instance_mutex);
    
    for (int i = 0; i < MAX_INSTANCES; i++) {
        active_windows[i] = NULL;
    }
    window_count = 0;
    
    pthread_mutex_unlock(&instance_mutex);
}
