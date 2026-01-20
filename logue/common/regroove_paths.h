#pragma once
/**
 * Regroove Resource Paths
 *
 * Defines platform-specific paths for loading Regroove data files
 * on DrumLogue and MicroKorg2 devices.
 *
 * Based on official Korg SDK path specifications.
 */

// Platform-specific base paths
#if defined(UNIT_TARGET_PLATFORM_DRUMLOGUE)

#ifndef REGROOVE_BASE_PATH
#define REGROOVE_BASE_PATH "/var/lib/drumlogued/userfs"
#endif

#elif defined(UNIT_TARGET_PLATFORM_MICROKORG2)

#ifndef REGROOVE_BASE_PATH
#define REGROOVE_BASE_PATH "/var/lib/microkorgd/userfs"
#endif

#else
// Fallback for unknown platforms
#warning "Unknown platform - using fallback path"
#define REGROOVE_BASE_PATH "/tmp"
#endif

// Default subdirectory for Regroove data
// Can be overridden by defining REGROOVE_SUBDIR before including this header
#ifndef REGROOVE_SUBDIR
#define REGROOVE_SUBDIR "Regroove"
#endif

// Full resource path: base + subdirectory
#define REGROOVE_RESOURCE_PATH REGROOVE_BASE_PATH "/" REGROOVE_SUBDIR

/**
 * Helper macro to construct full file paths
 * Usage: REGROOVE_PATH("preset_0.sfz") -> "/var/lib/drumlogued/userfs/Regroove/preset_0.sfz"
 */
#define REGROOVE_PATH(filename) REGROOVE_RESOURCE_PATH "/" filename

/**
 * Helper function to build paths at runtime
 *
 * @param buffer Output buffer for path
 * @param buffer_size Size of output buffer
 * @param filename Filename to append to resource path
 * @return Number of characters written (excluding null terminator)
 */
static inline int regroove_build_path(char* buffer, size_t buffer_size, const char* filename) {
    return snprintf(buffer, buffer_size, "%s/%s", REGROOVE_RESOURCE_PATH, filename);
}

/**
 * Helper function to build paths with custom subdirectory
 *
 * @param buffer Output buffer for path
 * @param buffer_size Size of output buffer
 * @param subdir Custom subdirectory (NULL to use default)
 * @param filename Filename to append
 * @return Number of characters written (excluding null terminator)
 */
static inline int regroove_build_custom_path(char* buffer, size_t buffer_size,
                                              const char* subdir, const char* filename) {
    if (subdir) {
        return snprintf(buffer, buffer_size, "%s/%s/%s", REGROOVE_BASE_PATH, subdir, filename);
    } else {
        return snprintf(buffer, buffer_size, "%s/%s", REGROOVE_RESOURCE_PATH, filename);
    }
}
