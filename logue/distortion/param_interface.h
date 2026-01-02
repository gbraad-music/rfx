/*
 * Parameter Interface Stub for NTS-3
 * The actual param_interface.h is only needed for LV2 plugins
 */

#ifndef PARAM_INTERFACE_H
#define PARAM_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char* name;
    const char* label;
    float default_value;
    float min_value;
    float max_value;
    int group;
    int is_integer;
} ParameterInfo;

#define DEFINE_PARAM_METADATA_ACCESSORS(prefix, params_array, param_count, groups_array, group_count)

#ifdef __cplusplus
}
#endif

#endif // PARAM_INTERFACE_H
