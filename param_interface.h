/*
 * Common Parameter Interface
 * Shared infrastructure for generic parameter access across synths and effects
 *
 * Copyright (C) 2024
 * SPDX-License-Identifier: ISC
 */

#ifndef PARAM_INTERFACE_H
#define PARAM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

// Common parameter metadata structure
typedef struct {
    const char* name;
    const char* label;
    float default_value;
    float min_value;
    float max_value;
    int group;
    int is_integer;
} ParameterInfo;

// Macro to define all parameter metadata accessor functions
// Usage: DEFINE_PARAM_METADATA_ACCESSORS(fx_filter, filter_params, FX_FILTER_PARAM_COUNT, group_names, FX_FILTER_GROUP_COUNT)
#define DEFINE_PARAM_METADATA_ACCESSORS(prefix, params_array, param_count, groups_array, group_count) \
    const char* prefix##_get_parameter_name(int index) { \
        if (index < 0 || index >= param_count) return ""; \
        return params_array[index].name; \
    } \
    \
    const char* prefix##_get_parameter_label(int index) { \
        if (index < 0 || index >= param_count) return ""; \
        return params_array[index].label; \
    } \
    \
    float prefix##_get_parameter_default(int index) { \
        if (index < 0 || index >= param_count) return 0.0f; \
        return params_array[index].default_value; \
    } \
    \
    float prefix##_get_parameter_min(int index) { \
        if (index < 0 || index >= param_count) return 0.0f; \
        return params_array[index].min_value; \
    } \
    \
    float prefix##_get_parameter_max(int index) { \
        if (index < 0 || index >= param_count) return 1.0f; \
        return params_array[index].max_value; \
    } \
    \
    int prefix##_get_parameter_group(int index) { \
        if (index < 0 || index >= param_count) return 0; \
        return params_array[index].group; \
    } \
    \
    const char* prefix##_get_group_name(int group) { \
        if (group < 0 || group >= group_count) return ""; \
        return groups_array[group]; \
    } \
    \
    int prefix##_parameter_is_integer(int index) { \
        if (index < 0 || index >= param_count) return 0; \
        return params_array[index].is_integer; \
    }

#ifdef __cplusplus
}
#endif

#endif // PARAM_INTERFACE_H
