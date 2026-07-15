// PebGoBrrrr - vibration engine
// Encodes the current time as a tactile buzz sequence.
// Two patterns: digit-group and Morse code.

#include <pebble.h>
#include "vibe_engine.h"

// ---------------------------------------------------------------------------
// Timing defaults (milliseconds) - overridden at runtime via Clay settings
// ---------------------------------------------------------------------------

#define DEFAULT_BUZZ_SHORT_MS  120u
#define DEFAULT_BUZZ_LONG_MS   360u
#define DEFAULT_GAP_DIGIT_MS   120u
#define DEFAULT_GAP_CHAR_MS    350u
#define DEFAULT_GAP_GROUP_MS   800u
#define DEFAULT_DELAY_MS         0u
#define MIN_BUZZ_SHORT_MS       50u
#define MAX_BUZZ_SHORT_MS      200u
#define MIN_BUZZ_LONG_MS       100u
#define MAX_BUZZ_LONG_MS       600u
#define MIN_GAP_DIGIT_MS        50u
#define MAX_GAP_DIGIT_MS       200u
#define MIN_GAP_CHAR_MS        150u
#define MAX_GAP_CHAR_MS        600u
#define MIN_GAP_GROUP_MS       400u
#define MAX_GAP_GROUP_MS      1500u
#define MIN_DELAY_MS             0u
#define MAX_DELAY_MS          5000u

// ---------------------------------------------------------------------------
// Compile-time guard - VibePattern max is 256 segments
// ---------------------------------------------------------------------------

#define MAX_VIBE_SEGMENTS  256u
#define MORSE_MAX_SEGMENTS  60u

_Static_assert(MORSE_MAX_SEGMENTS <= MAX_VIBE_SEGMENTS,
    "MORSE_MAX_SEGMENTS exceeds Pebble VibePattern limit of 256");

// ---------------------------------------------------------------------------
// Persist keys
// ---------------------------------------------------------------------------

#define PERSIST_KEY_MODE             1
#define PERSIST_KEY_OUTPUT_MODE      2
#define PERSIST_KEY_BUZZ_SHORT       3
#define PERSIST_KEY_BUZZ_LONG        4
#define PERSIST_KEY_GAP_DIGIT        5
#define PERSIST_KEY_GAP_CHAR         6
#define PERSIST_KEY_GAP_GROUP        7
#define PERSIST_KEY_LONG_BUZZ_HOURS  8
#define PERSIST_KEY_DELAY            9

// ---------------------------------------------------------------------------
// Runtime config state
// ---------------------------------------------------------------------------

static VibePatternMode s_mode             = PATTERN_DIGIT_GROUP;
static VibeOutputMode  s_output_mode      = OUTPUT_FULL_TIME;
static bool            s_long_buzz_hours  = false;
static uint32_t s_buzz_short = DEFAULT_BUZZ_SHORT_MS;
static uint32_t s_buzz_long  = DEFAULT_BUZZ_LONG_MS;
static uint32_t s_gap_digit  = DEFAULT_GAP_DIGIT_MS;
static uint32_t s_gap_char   = DEFAULT_GAP_CHAR_MS;
static uint32_t s_gap_group  = DEFAULT_GAP_GROUP_MS;
static uint32_t s_delay      = DEFAULT_DELAY_MS;

// Delayed playback state
static AppTimer *s_delay_timer = NULL;
static uint32_t s_pending_segments[MAX_VIBE_SEGMENTS];
static uint16_t s_pending_count = 0;

static uint32_t clamp_u32(uint32_t value, uint32_t min_value, uint32_t max_value) {
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

static VibePatternMode sanitize_pattern_mode(int value) {
    switch (value) {
        case PATTERN_MORSE:
            return PATTERN_MORSE;
        case PATTERN_SHORTHAND:
            return PATTERN_SHORTHAND;
        case PATTERN_DIGIT_GROUP:
        default:
            return PATTERN_DIGIT_GROUP;
    }
}

static VibeOutputMode sanitize_output_mode(int value) {
    switch (value) {
        case OUTPUT_HOUR_ONLY:
        case OUTPUT_ROUND10:
            return (VibeOutputMode)value;
        case OUTPUT_FULL_TIME:
        default:
            return OUTPUT_FULL_TIME;
    }
}

// ---------------------------------------------------------------------------
// Config getters / setters
// ---------------------------------------------------------------------------

VibePatternMode vibe_config_get_mode(void) { return s_mode; }
void vibe_config_set_mode(VibePatternMode mode) {
    s_mode = sanitize_pattern_mode((int)mode);
    persist_write_int(PERSIST_KEY_MODE, (int)s_mode);
}

VibeOutputMode vibe_config_get_output_mode(void) { return s_output_mode; }
void vibe_config_set_output_mode(VibeOutputMode mode) {
    s_output_mode = sanitize_output_mode((int)mode);
    persist_write_int(PERSIST_KEY_OUTPUT_MODE, (int)s_output_mode);
}

bool vibe_config_get_long_buzz_hours(void) { return s_long_buzz_hours; }
void vibe_config_set_long_buzz_hours(bool enabled) {
    s_long_buzz_hours = enabled;
    persist_write_bool(PERSIST_KEY_LONG_BUZZ_HOURS, enabled);
}

void vibe_config_set_buzz_short(uint32_t ms) {
    s_buzz_short = clamp_u32(ms, MIN_BUZZ_SHORT_MS, MAX_BUZZ_SHORT_MS);
    persist_write_int(PERSIST_KEY_BUZZ_SHORT, (int)s_buzz_short);
}
void vibe_config_set_buzz_long(uint32_t ms) {
    s_buzz_long = clamp_u32(ms, MIN_BUZZ_LONG_MS, MAX_BUZZ_LONG_MS);
    persist_write_int(PERSIST_KEY_BUZZ_LONG, (int)s_buzz_long);
}
void vibe_config_set_gap_digit(uint32_t ms) {
    s_gap_digit = clamp_u32(ms, MIN_GAP_DIGIT_MS, MAX_GAP_DIGIT_MS);
    persist_write_int(PERSIST_KEY_GAP_DIGIT, (int)s_gap_digit);
}
void vibe_config_set_gap_char(uint32_t ms) {
    s_gap_char = clamp_u32(ms, MIN_GAP_CHAR_MS, MAX_GAP_CHAR_MS);
    persist_write_int(PERSIST_KEY_GAP_CHAR, (int)s_gap_char);
}
void vibe_config_set_gap_group(uint32_t ms) {
    s_gap_group = clamp_u32(ms, MIN_GAP_GROUP_MS, MAX_GAP_GROUP_MS);
    persist_write_int(PERSIST_KEY_GAP_GROUP, (int)s_gap_group);
}
void vibe_config_set_delay(uint32_t ms) {
    s_delay = clamp_u32(ms, MIN_DELAY_MS, MAX_DELAY_MS);
    persist_write_int(PERSIST_KEY_DELAY, (int)s_delay);
}

void vibe_config_load(void) {
    if (persist_exists(PERSIST_KEY_MODE)) {
        s_mode = sanitize_pattern_mode(persist_read_int(PERSIST_KEY_MODE));
    }
    if (persist_exists(PERSIST_KEY_OUTPUT_MODE)) {
        s_output_mode = sanitize_output_mode(persist_read_int(PERSIST_KEY_OUTPUT_MODE));
    }
    if (persist_exists(PERSIST_KEY_LONG_BUZZ_HOURS)) s_long_buzz_hours = persist_read_bool(PERSIST_KEY_LONG_BUZZ_HOURS);
    if (persist_exists(PERSIST_KEY_BUZZ_SHORT)) {
        s_buzz_short = clamp_u32((uint32_t)persist_read_int(PERSIST_KEY_BUZZ_SHORT), MIN_BUZZ_SHORT_MS, MAX_BUZZ_SHORT_MS);
    }
    if (persist_exists(PERSIST_KEY_BUZZ_LONG)) {
        s_buzz_long = clamp_u32((uint32_t)persist_read_int(PERSIST_KEY_BUZZ_LONG), MIN_BUZZ_LONG_MS, MAX_BUZZ_LONG_MS);
    }
    if (persist_exists(PERSIST_KEY_GAP_DIGIT)) {
        s_gap_digit = clamp_u32((uint32_t)persist_read_int(PERSIST_KEY_GAP_DIGIT), MIN_GAP_DIGIT_MS, MAX_GAP_DIGIT_MS);
    }
    if (persist_exists(PERSIST_KEY_GAP_CHAR)) {
        s_gap_char = clamp_u32((uint32_t)persist_read_int(PERSIST_KEY_GAP_CHAR), MIN_GAP_CHAR_MS, MAX_GAP_CHAR_MS);
    }
    if (persist_exists(PERSIST_KEY_GAP_GROUP)) {
        s_gap_group = clamp_u32((uint32_t)persist_read_int(PERSIST_KEY_GAP_GROUP), MIN_GAP_GROUP_MS, MAX_GAP_GROUP_MS);
    }
    if (persist_exists(PERSIST_KEY_DELAY)) {
        s_delay = clamp_u32((uint32_t)persist_read_int(PERSIST_KEY_DELAY), MIN_DELAY_MS, MAX_DELAY_MS);
    }
}

// ---------------------------------------------------------------------------
// Internal sequence builder
// ---------------------------------------------------------------------------

typedef struct {
    uint32_t segments[MAX_VIBE_SEGMENTS];
    uint16_t count;
} VibeSequence;

static bool seq_push(VibeSequence *seq, uint32_t ms) {
    if (seq->count >= MAX_VIBE_SEGMENTS) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "VibeSequence overflow -- sequence truncated");
        return false;
    }
    seq->segments[seq->count++] = ms;
    return true;
}

static bool seq_buzz(VibeSequence *seq, uint32_t buzz_ms, uint32_t gap_ms) {
    if (!seq_push(seq, buzz_ms)) return false;
    if (gap_ms > 0 && !seq_push(seq, gap_ms)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Digit-group pattern
//
// Hours are encoded as a single count from 1-12.
// Minutes are encoded as digit groups (e.g. 27 => 2, then 7).
//
// after_gap: silence appended after the last buzz of a digit/group.
//   GAP_CHAR  - another digit follows in the same minute group
//   GAP_GROUP - the minute group follows the hour group
//   0         - end of sequence
// ---------------------------------------------------------------------------

static bool encode_digit(VibeSequence *seq, uint8_t digit,
                         uint32_t buzz_ms, uint32_t after_gap) {
    if (digit == 0) {
        // Zero = one long buzz regardless of buzz_ms, so it stays distinct
        return seq_buzz(seq, s_buzz_long, after_gap);
    }
    for (uint8_t i = 0; i < digit; i++) {
        bool last_buzz = (i == digit - 1);
        uint32_t gap = last_buzz ? after_gap : s_gap_digit;
        if (!seq_buzz(seq, buzz_ms, gap)) return false;
    }
    return true;
}

static bool build_digit_group(VibeSequence *seq, uint8_t hour, uint8_t minute,
                              bool include_minutes) {
    // Hour is a single count/group in digit-group mode.
    // LongBuzzHours changes the pulse length for the whole hour group.
    uint32_t hour_buzz = s_long_buzz_hours ? s_buzz_long : s_buzz_short;

    // Encode the hour as N pulses, not split digits.
    for (uint8_t i = 0; i < hour; i++) {
        bool last_buzz = (i == hour - 1);
        uint32_t gap = last_buzz ? (include_minutes ? s_gap_group : 0) : s_gap_digit;
        if (!seq_buzz(seq, hour_buzz, gap)) return false;
    }

    if (include_minutes) {
        uint8_t m1 = minute / 10;
        uint8_t m2 = minute % 10;

        // In round-to-10 mode, suppress a trailing zero minute digit:
        // 7:40 => 7, gap, 4
        // not 7, gap, 4, gap, 0
        bool suppress_trailing_zero = (s_output_mode == OUTPUT_ROUND10 && m2 == 0);

        if (suppress_trailing_zero) {
            if (!encode_digit(seq, m1, s_buzz_short, 0)) return false;
        } else {
            if (!encode_digit(seq, m1, s_buzz_short, s_gap_char)) return false;
            if (!encode_digit(seq, m2, s_buzz_short, 0))          return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Morse pattern
// ---------------------------------------------------------------------------

static const char *const MORSE_DIGITS[10] = {
    "-----", ".----", "..---", "...--", "....-",
    ".....", "-....", "--...", "---..", "----.",
};

static bool encode_morse_digit(VibeSequence *seq, uint8_t digit, uint32_t after_gap) {
    const char *code = MORSE_DIGITS[digit];
    for (uint8_t i = 0; code[i] != '\0'; i++) {
        bool last_sym = (code[i + 1] == '\0');
        uint32_t buzz = (code[i] == '.') ? s_buzz_short : s_buzz_long;
        uint32_t gap  = last_sym ? after_gap : s_gap_digit;
        if (!seq_buzz(seq, buzz, gap)) return false;
    }
    return true;
}

static bool build_morse(VibeSequence *seq, uint8_t hour, uint8_t minute, bool include_minutes) {
    uint8_t h1 = hour / 10;
    uint8_t h2 = hour % 10;

    if (!encode_morse_digit(seq, h1, s_gap_char)) return false;
    if (!encode_morse_digit(seq, h2, include_minutes ? s_gap_group : 0)) return false;

    if (include_minutes) {
        uint8_t m1 = minute / 10;
        uint8_t m2 = minute % 10;
        if (!encode_morse_digit(seq, m1, s_gap_char)) return false;
        if (!encode_morse_digit(seq, m2, 0))          return false;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Shorthand pattern
//
// Hours: current hour mod 6 (22:00 becomes 4)
// Minutes: encoded as per digit groups, except if the second minute group is
//          greater than 5 it long buzzes the second group and then short 
//          buzzes the remainder (e.g. 27 => 2, then 1 (long buzz), then 2).
// ---------------------------------------------------------------------------

static bool build_shorthand(VibeSequence *seq, uint8_t hour, uint8_t minute, bool include_minutes) {
    // Convert hour to 0-5 range (mod 6)
    uint8_t hour_mod = hour % 6;
    
    // Buzz hours. If hour_mod is 0, we long buzz to indicate 6/12/18/00 hours
    if (!encode_digit(seq, hour_mod, s_buzz_short, s_gap_char)) return false;

    if (include_minutes) {
        uint8_t m1 = minute / 10;
        uint8_t m2 = minute % 10;

        // In round-to-10 mode, suppress a trailing zero minute digit:
        // 7:40 => 7, gap, 4
        // not 7, gap, 4, gap, 0
        bool suppress_trailing_zero = (s_output_mode == OUTPUT_ROUND10 && m2 == 0);

        bool m2_split = m2 >= 5;
        bool m2_split_remainder = (m2 % 5) > 0;

        if (suppress_trailing_zero) {
            if (!encode_digit(seq, m1, s_buzz_short, 0)) return false;
        } else {
            if (!encode_digit(seq, m1, s_buzz_short, s_gap_char)) return false;

            if (m2_split) {
                // m2 greater than or equal to 5
                if (!encode_digit(seq, 1, s_buzz_long, s_gap_char))          return false;
                if (m2_split_remainder) {
                    // m2 remainder is greater than 0
                    if (!encode_digit(seq, m2%5, s_buzz_short, 0))          return false;
                }
            } else if (m2 != 0) {
                // m2 is less than 5 but more than 0
                if (!encode_digit(seq, m2, s_buzz_short, 0))          return false;
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Delayed playback
// ---------------------------------------------------------------------------

static void delay_timer_callback(void *context) {
    s_delay_timer = NULL;

    if (s_pending_count == 0) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "delay_timer_callback: no pending pattern");
        return;
    }

    VibePattern pat = {
        .durations    = s_pending_segments,
        .num_segments = s_pending_count,
    };

    vibes_enqueue_custom_pattern(pat);
}

// ---------------------------------------------------------------------------
// Calculate the vibe pattern for the current time and fire it.
// Return total lifetime in ms including initial delay.
// ---------------------------------------------------------------------------

uint32_t vibe_speak_time(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    uint8_t hour   = (uint8_t)t->tm_hour;
    uint8_t minute = (uint8_t)t->tm_min;

    uint32_t pattern_duration_ms = 0;

    // 12-hour conversion
    if (hour == 0)      hour = 12;
    else if (hour > 12) hour -= 12;

    // Apply output mode
    bool include_minutes = true;
    switch (s_output_mode) {
        case OUTPUT_HOUR_ONLY:
            include_minutes = false;
            break;

        case OUTPUT_ROUND10:
            // Round minutes to nearest 10: 5:27 -> 5:3 (30), 5:58 -> 6:0
            minute = ((minute + 5) / 10) * 10;
            if (minute >= 60) {
                // Rolled over - increment hour, zero minutes
                minute = 0;
                hour = (hour % 12) + 1;
            }
            // If minutes rounded to exactly 0, suppress the minute group -
            // a long buzz for zero would be indistinguishable from the hour
            // zero treatment and is confusing. Silence is cleaner.
            if (minute == 0) {
                include_minutes = false;
            }
            break;

        case OUTPUT_FULL_TIME:
        default:
            break;
    }

    VibeSequence seq = { .count = 0 };
    bool ok = false;

    switch (s_mode) {
        case PATTERN_DIGIT_GROUP:
            ok = build_digit_group(&seq, hour, minute, include_minutes);
            break;
        case PATTERN_MORSE:
            ok = build_morse(&seq, hour, minute, include_minutes);
            break;
        case PATTERN_SHORTHAND:
            ok = build_shorthand(&seq, hour, minute, include_minutes);
            break;
    }

    if (!ok || seq.count == 0) {
        APP_LOG(APP_LOG_LEVEL_ERROR, "vibe_speak_time: sequence build failed");
        return 0;
    }

    for (uint16_t i = 0; i < seq.count; ++i) {
        pattern_duration_ms += seq.segments[i];
    }

    // Cancel any prior delayed playback and replace it
    if (s_delay_timer) {
        app_timer_cancel(s_delay_timer);
        s_delay_timer = NULL;
    }
    s_pending_count = 0;

    if (s_delay > 0) {
        s_pending_count = seq.count;
        for (uint16_t i = 0; i < seq.count; ++i) {
            s_pending_segments[i] = seq.segments[i];
        }

        s_delay_timer = app_timer_register(s_delay, delay_timer_callback, NULL);
        if (!s_delay_timer) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "vibe_speak_time: failed to register delay timer");
            return 0;
        }

        return s_delay + pattern_duration_ms;
    }

    VibePattern pat = {
        .durations    = seq.segments,
        .num_segments = seq.count,
    };

    vibes_enqueue_custom_pattern(pat);
    return pattern_duration_ms;
}