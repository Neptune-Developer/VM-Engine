#pragma once
#include <android/native_window.h>

void check_instance_init(void);
int check_instance_register(ANativeWindow* window);
void check_instance_unregister(ANativeWindow* window);
void check_instance_validate(void);
void check_instance_cleanup(void);
