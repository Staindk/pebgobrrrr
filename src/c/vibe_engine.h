// PebGoBrrrr - vibe_engine.h
// Public interface for the vibration engine.

#pragma once

#include <pebble.h>

// Pattern modes
typedef enum {
    PATTERN_DIGIT_GROUP = 0,
    PATTERN_MORSE       = 1,
} VibePatternMode;

// Output (time detail) modes
typedef enum {
    OUTPUT_FULL_TIME  = 0,  // HH:MM exact
    OUTPUT_HOUR_ONLY  = 1,  // hour group only, no minutes
    OUTPUT_ROUND10    = 2,  // round minutes to nearest 10 (e.g. 5:27 -> 5:3)
                            // if minutes round to 0, suppress minute group
} VibeOutputMode;

// Config - call vibe_config_load() once in init()
void            vibe_config_load(void);
VibePatternMode vibe_config_get_mode(void);
void            vibe_config_set_mode(VibePatternMode mode);
VibeOutputMode  vibe_config_get_output_mode(void);
void            vibe_config_set_output_mode(VibeOutputMode mode);
bool            vibe_config_get_long_buzz_hours(void);
void            vibe_config_set_long_buzz_hours(bool enabled);

// Individual timing overrides
void vibe_config_set_buzz_short(uint32_t ms);
void vibe_config_set_buzz_long(uint32_t ms);
void vibe_config_set_gap_digit(uint32_t ms);
void vibe_config_set_gap_char(uint32_t ms);
void vibe_config_set_gap_group(uint32_t ms);
void vibe_config_set_delay(uint32_t ms);

// Fire the vibration sequence for the current time. Returns total duration in ms.
uint32_t vibe_speak_time(void);
