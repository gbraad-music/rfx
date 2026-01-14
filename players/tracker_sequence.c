/*
 * Generic Tracker Sequence Implementation
 */

#include "tracker_sequence.h"
#include <stdlib.h>
#include <string.h>

void tracker_sequence_init(TrackerSequence* seq) {
    memset(seq, 0, sizeof(TrackerSequence));
}

void tracker_sequence_set_data(TrackerSequence* seq,
                               TrackerSequenceEntry* entries,
                               int32_t length,
                               int32_t speed) {
    if (seq->entries) {
        free(seq->entries);
    }

    seq->entries = entries;
    seq->length = length;
    seq->speed = speed;
    seq->current = 0;
    seq->wait = speed;
    seq->active = false;
}

void tracker_sequence_free(TrackerSequence* seq) {
    if (seq->entries) {
        free(seq->entries);
        seq->entries = NULL;
    }
    seq->length = 0;
    seq->current = 0;
}

void tracker_sequence_start(TrackerSequence* seq) {
    seq->active = true;
    seq->current = 0;
    seq->wait = seq->speed;
}

void tracker_sequence_stop(TrackerSequence* seq) {
    seq->active = false;
}

void tracker_sequence_jump(TrackerSequence* seq, int32_t step) {
    if (step >= 0 && step < seq->length) {
        seq->current = step;
        seq->wait = seq->speed;
    }
}

void tracker_sequence_set_speed(TrackerSequence* seq, int32_t speed) {
    seq->speed = speed;
    seq->wait = speed;
}

const TrackerSequenceEntry* tracker_sequence_update(TrackerSequence* seq) {
    if (!seq->active || seq->length == 0 || !seq->entries) {
        return NULL;
    }

    // Check if we've reached the end
    if (seq->current >= seq->length) {
        // For AHX-style: continue waiting but don't advance
        if (seq->wait > 0) {
            seq->wait--;
        }
        return NULL;
    }

    // Handle signed overflow case (AHX uses int8 for wait, can wrap to 128)
    bool signed_overflow = (seq->wait == 128);

    seq->wait--;
    if (signed_overflow || seq->wait <= 0) {
        // Return current entry before advancing
        const TrackerSequenceEntry* entry = &seq->entries[seq->current];

        // Advance to next step
        seq->current++;
        seq->wait = seq->speed;

        return entry;
    }

    return NULL;
}

const TrackerSequenceEntry* tracker_sequence_get_current(const TrackerSequence* seq) {
    if (!seq->active || seq->length == 0 || !seq->entries) {
        return NULL;
    }

    if (seq->current >= 0 && seq->current < seq->length) {
        return &seq->entries[seq->current];
    }

    return NULL;
}

bool tracker_sequence_is_active(const TrackerSequence* seq) {
    return seq->active;
}

bool tracker_sequence_is_finished(const TrackerSequence* seq) {
    return seq->active && seq->current >= seq->length;
}
