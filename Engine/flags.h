#pragma once

#include <stdbool.h>
#include <string.h>

// Maximum number of flags that can be registered
#define MAX_FLAGS 64

// Maximum length of flag name
#define MAX_FLAG_NAME_LENGTH 32

// Flag types
typedef enum {
    FLAG_TYPE_BOOL,
    FLAG_TYPE_INT,
    FLAG_TYPE_FLOAT,
    FLAG_TYPE_STRING
} flag_type;

// Flag value union
typedef union {
    bool bool_value;
    int int_value;
    float float_value;
    char* string_value;
} flag_value;

// Flag structure
typedef struct {
    char name[MAX_FLAG_NAME_LENGTH];
    flag_type type;
    flag_value value;
    flag_value default_value;
    bool initialized;
} flag;

// Initialize the flags system
void flags_init();

// Clean up the flags system
void flags_cleanup();

// Register a boolean flag
bool flag_register_bool(const char* name, bool default_value);

// Register an integer flag
bool flag_register_int(const char* name, int default_value);

// Register a float flag
bool flag_register_float(const char* name, float default_value);

// Register a string flag
bool flag_register_string(const char* name, const char* default_value);

// Set a boolean flag value
bool flag_set_bool(const char* name, bool value);

// Set an integer flag value
bool flag_set_int(const char* name, int value);

// Set a float flag value
bool flag_set_float(const char* name, float value);

// Set a string flag value
bool flag_set_string(const char* name, const char* value);

// Get a boolean flag value
bool flag_get_bool(const char* name);

// Get an integer flag value
int flag_get_int(const char* name);

// Get a float flag value
float flag_get_float(const char* name);

// Get a string flag value
const char* flag_get_string(const char* name);

// Check if a flag exists
bool flag_exists(const char* name);

// Parse flags from command line arguments
void flags_parse_args(int argc, char** argv);

// Parse flags from a configuration file
bool flags_parse_file(const char* filename);

// Reset a flag to its default value
bool flag_reset(const char* name);

// Reset all flags to their default values
void flags_reset_all();
