#include "flags.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Array of flags
static flag flags[MAX_FLAGS];
static int flag_count = 0;
static bool initialized = false;

// Initialize the flags system
void flags_init() {
    if (!initialized) {
        memset(flags, 0, sizeof(flags));
        flag_count = 0;
        initialized = true;
    }
}

// Clean up the flags system
void flags_cleanup() {
    if (initialized) {
        for (int i = 0; i < flag_count; i++) {
            if (flags[i].type == FLAG_TYPE_STRING && flags[i].value.string_value != NULL) {
                free(flags[i].value.string_value);
            }
            if (flags[i].type == FLAG_TYPE_STRING && flags[i].default_value.string_value != NULL) {
                free(flags[i].default_value.string_value);
            }
        }
        memset(flags, 0, sizeof(flags));
        flag_count = 0;
        initialized = false;
    }
}

// Find a flag by name
static int find_flag(const char* name) {
    for (int i = 0; i < flag_count; i++) {
        if (strcmp(flags[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Register a boolean flag
bool flag_register_bool(const char* name, bool default_value) {
    if (!initialized) {
        flags_init();
    }
    
    if (flag_count >= MAX_FLAGS) {
        return false;
    }
    
    if (find_flag(name) != -1) {
        return false;  // Flag already exists
    }
    
    strncpy(flags[flag_count].name, name, MAX_FLAG_NAME_LENGTH - 1);
    flags[flag_count].name[MAX_FLAG_NAME_LENGTH - 1] = '\0';
    flags[flag_count].type = FLAG_TYPE_BOOL;
    flags[flag_count].value.bool_value = default_value;
    flags[flag_count].default_value.bool_value = default_value;
    flags[flag_count].initialized = true;
    
    flag_count++;
    return true;
}

// Register an integer flag
bool flag_register_int(const char* name, int default_value) {
    if (!initialized) {
        flags_init();
    }
    
    if (flag_count >= MAX_FLAGS) {
        return false;
    }
    
    if (find_flag(name) != -1) {
        return false;  // Flag already exists
    }
    
    strncpy(flags[flag_count].name, name, MAX_FLAG_NAME_LENGTH - 1);
    flags[flag_count].name[MAX_FLAG_NAME_LENGTH - 1] = '\0';
    flags[flag_count].type = FLAG_TYPE_INT;
    flags[flag_count].value.int_value = default_value;
    flags[flag_count].default_value.int_value = default_value;
    flags[flag_count].initialized = true;
    
    flag_count++;
    return true;
}

// Register a float flag
bool flag_register_float(const char* name, float default_value) {
    if (!initialized) {
        flags_init();
    }
    
    if (flag_count >= MAX_FLAGS) {
        return false;
    }
    
    if (find_flag(name) != -1) {
        return false;  // Flag already exists
    }
    
    strncpy(flags[flag_count].name, name, MAX_FLAG_NAME_LENGTH - 1);
    flags[flag_count].name[MAX_FLAG_NAME_LENGTH - 1] = '\0';
    flags[flag_count].type = FLAG_TYPE_FLOAT;
    flags[flag_count].value.float_value = default_value;
    flags[flag_count].default_value.float_value = default_value;
    flags[flag_count].initialized = true;
    
    flag_count++;
    return true;
}

// Register a string flag
bool flag_register_string(const char* name, const char* default_value) {
    if (!initialized) {
        flags_init();
    }
    
    if (flag_count >= MAX_FLAGS) {
        return false;
    }
    
    if (find_flag(name) != -1) {
        return false;  // Flag already exists
    }
    
    strncpy(flags[flag_count].name, name, MAX_FLAG_NAME_LENGTH - 1);
    flags[flag_count].name[MAX_FLAG_NAME_LENGTH - 1] = '\0';
    flags[flag_count].type = FLAG_TYPE_STRING;
    
    if (default_value != NULL) {
        flags[flag_count].value.string_value = strdup(default_value);
        flags[flag_count].default_value.string_value = strdup(default_value);
    } else {
        flags[flag_count].value.string_value = NULL;
        flags[flag_count].default_value.string_value = NULL;
    }
    
    flags[flag_count].initialized = true;
    
    flag_count++;
    return true;
}

// Set a boolean flag value
bool flag_set_bool(const char* name, bool value) {
    int index = find_flag(name);
    if (index == -1) {
        return false;
    }
    
    if (flags[index].type != FLAG_TYPE_BOOL) {
        return false;
    }
    
    flags[index].value.bool_value = value;
    return true;
}

// Set an integer flag value
bool flag_set_int(const char* name, int value) {
    int index = find_flag(name);
    if (index == -1) {
        return false;
    }
    
    if (flags[index].type != FLAG_TYPE_INT) {
        return false;
    }
    
    flags[index].value.int_value = value;
    return true;
}

// Set a float flag value
bool flag_set_float(const char* name, float value) {
    int index = find_flag(name);
    if (index == -1) {
        return false;
    }
    
    if (flags[index].type != FLAG_TYPE_FLOAT) {
        return false;
    }
    
    flags[index].value.float_value = value;
    return true;
}

// Set a string flag value
bool flag_set_string(const char* name, const char* value) {
    int index = find_flag(name);
    if (index == -1) {
        return false;
    }
    
    if (flags[index].type != FLAG_TYPE_STRING) {
        return false;
    }
    
    if (flags[index].value.string_value != NULL) {
        free(flags[index].value.string_value);
    }
    
    if (value != NULL) {
        flags[index].value.string_value = strdup(value);
    } else {
        flags[index].value.string_value = NULL;
    }
    
    return true;
}

// Get a boolean flag value
bool flag_get_bool(const char* name) {
    int index = find_flag(name);
    if (index == -1 || flags[index].type != FLAG_TYPE_BOOL) {
        return false;  // Default to false if flag doesn't exist or is wrong type
    }
    
    return flags[index].value.bool_value;
}

// Get an integer flag value
int flag_get_int(const char* name) {
    int index = find_flag(name);
    if (index == -1 || flags[index].type != FLAG_TYPE_INT) {
        return 0;  // Default to 0 if flag doesn't exist or is wrong type
    }
    
    return flags[index].value.int_value;
}

// Get a float flag value
float flag_get_float(const char* name) {
    int index = find_flag(name);
    if (index == -1 || flags[index].type != FLAG_TYPE_FLOAT) {
        return 0.0f;  // Default to 0.0 if flag doesn't exist or is wrong type
    }
    
    return flags[index].value.float_value;
}

// Get a string flag value
const char* flag_get_string(const char* name) {
    int index = find_flag(name);
    if (index == -1 || flags[index].type != FLAG_TYPE_STRING) {
        return NULL;  // Default to NULL if flag doesn't exist or is wrong type
    }
    
    return flags[index].value.string_value;
}

// Check if a flag exists
bool flag_exists(const char* name) {
    return find_flag(name) != -1;
}

// Parse flags from command line arguments
void flags_parse_args(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == '-') {
            // Format: --flag=value or --flag
            char* flag_name = argv[i] + 2;  // Skip the "--"
            char* value = strchr(flag_name, '=');
            
            if (value != NULL) {
                // Split the flag name and value
                *value = '\0';  // Replace '=' with null terminator
                value++;  // Move to the value part
                
                int index = find_flag(flag_name);
                if (index != -1) {
                    switch (flags[index].type) {
                        case FLAG_TYPE_BOOL:
                            if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                                flags[index].value.bool_value = true;
                            } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                                flags[index].value.bool_value = false;
                            }
                            break;
                        case FLAG_TYPE_INT:
                            flags[index].value.int_value = atoi(value);
                            break;
                        case FLAG_TYPE_FLOAT:
                            flags[index].value.float_value = (float)atof(value);
                            break;
                        case FLAG_TYPE_STRING:
                            if (flags[index].value.string_value != NULL) {
                                free(flags[index].value.string_value);
                            }
                            flags[index].value.string_value = strdup(value);
                            break;
                    }
                }
            } else {
                // Boolean flag without value, assume true
                int index = find_flag(flag_name);
                if (index != -1 && flags[index].type == FLAG_TYPE_BOOL) {
                    flags[index].value.bool_value = true;
                }
            }
        }
    }
}

// Parse flags from a configuration file
bool flags_parse_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return false;
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove newline character
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }
        
        // Parse "flag_name = value" format
        char* equals = strchr(line, '=');
        if (equals != NULL) {
            *equals = '\0';  // Split the line at '='
            
            // Trim whitespace from flag name
            char* flag_name = line;
            while (*flag_name == ' ' || *flag_name == '\t') {
                flag_name++;
            }
            char* end = equals - 1;
            while (end > flag_name && (*end == ' ' || *end == '\t')) {
                *end = '\0';
                end--;
            }
            
            // Trim whitespace from value
            char* value = equals + 1;
            while (*value == ' ' || *value == '\t') {
                value++;
            }
            end = value + strlen(value) - 1;
            while (end > value && (*end == ' ' || *end == '\t')) {
                *end = '\0';
                end--;
            }
            
            // Set the flag value
            int index = find_flag(flag_name);
            if (index != -1) {
                switch (flags[index].type) {
                    case FLAG_TYPE_BOOL:
                        if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
                            flags[index].value.bool_value = true;
                        } else if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
                            flags[index].value.bool_value = false;
                        }
                        break;
                    case FLAG_TYPE_INT:
                        flags[index].value.int_value = atoi(value);
                        break;
                    case FLAG_TYPE_FLOAT:
                        flags[index].value.float_value = (float)atof(value);
                        break;
                    case FLAG_TYPE_STRING:
                        if (flags[index].value.string_value != NULL) {
                            free(flags[index].value.string_value);
                        }
                        flags[index].value.string_value = strdup(value);
                        break;
                }
            }
        }
    }
    
    fclose(file);
    return true;
}

// Reset a flag to its default value
bool flag_reset(const char* name) {
    int index = find_flag(name);
    if (index == -1) {
        return false;
    }
    
    switch (flags[index].type) {
        case FLAG_TYPE_BOOL:
            flags[index].value.bool_value = flags[index].default_value.bool_value;
            break;
        case FLAG_TYPE_INT:
            flags[index].value.int_value = flags[index].default_value.int_value;
            break;
        case FLAG_TYPE_FLOAT:
            flags[index].value.float_value = flags[index].default_value.float_value;
            break;
        case FLAG_TYPE_STRING:
            if (flags[index].value.string_value != NULL) {
                free(flags[index].value.string_value);
            }
            if (flags[index].default_value.string_value != NULL) {
                flags[index].value.string_value = strdup(flags[index].default_value.string_value);
            } else {
                flags[index].value.string_value = NULL;
            }
            break;
    }
    
    return true;
}

// Reset all flags to their default values
void flags_reset_all() {
    for (int i = 0; i < flag_count; i++) {
        switch (flags[i].type) {
            case FLAG_TYPE_BOOL:
                flags[i].value.bool_value = flags[i].default_value.bool_value;
                break;
            case FLAG_TYPE_INT:
                flags[i].value.int_value = flags[i].default_value.int_value;
                break;
            case FLAG_TYPE_FLOAT:
                flags[i].value.float_value = flags[i].default_value.float_value;
                break;
            case FLAG_TYPE_STRING:
                if (flags[i].value.string_value != NULL) {
                    free(flags[i].value.string_value);
                }
                if (flags[i].default_value.string_value != NULL) {
                    flags[i].value.string_value = strdup(flags[i].default_value.string_value);
                } else {
                    flags[i].value.string_value = NULL;
                }
                break;
        }
    }
}
