/*
 * Generic Tracker Modulator Implementation
 */

#include "tracker_modulator.h"
#include <string.h>

void tracker_modulator_init(TrackerModulator* mod) {
    memset(mod, 0, sizeof(TrackerModulator));
    mod->sign = 1;
}

void tracker_modulator_set_limits(TrackerModulator* mod, int32_t lower, int32_t upper) {
    if (lower > upper) {
        int32_t temp = lower;
        lower = upper;
        upper = temp;
    }
    mod->lower_limit = lower;
    mod->upper_limit = upper;
}

void tracker_modulator_set_speed(TrackerModulator* mod, int32_t speed) {
    mod->speed = speed;
}

void tracker_modulator_set_position(TrackerModulator* mod, int32_t position) {
    mod->position = position;
}

void tracker_modulator_set_active(TrackerModulator* mod, bool active) {
    mod->active = active;
    if (active) {
        mod->init_pending = true;
    }
}

void tracker_modulator_set_direction(TrackerModulator* mod, int32_t sign) {
    mod->sign = (sign >= 0) ? 1 : -1;
}

bool tracker_modulator_update(TrackerModulator* mod) {
    if (!mod->active) {
        return false;
    }

    // Initialize if needed
    if (mod->init_pending) {
        mod->init_pending = false;

        if (mod->position <= mod->lower_limit) {
            mod->sliding_in = true;
            mod->sign = 1;
        } else if (mod->position >= mod->upper_limit) {
            mod->sliding_in = true;
            mod->sign = -1;
        }
    }

    // Check if at boundary
    if (mod->position == mod->lower_limit || mod->position == mod->upper_limit) {
        if (mod->sliding_in) {
            mod->sliding_in = false;
        } else {
            // Reverse direction
            mod->sign = -mod->sign;
        }
    }

    // Update position
    int32_t old_position = mod->position;
    mod->position += mod->sign;

    // Clamp to limits (optional - depends on use case)
    // For AHX-style, we don't clamp here, we reverse at limits
    // But for other uses you might want:
    // if (mod->position < mod->lower_limit) mod->position = mod->lower_limit;
    // if (mod->position > mod->upper_limit) mod->position = mod->upper_limit;

    return mod->position != old_position;
}

int32_t tracker_modulator_get_position(const TrackerModulator* mod) {
    return mod->position;
}

bool tracker_modulator_is_active(const TrackerModulator* mod) {
    return mod->active;
}
