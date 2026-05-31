// PebGoBrrrr - main.c
// Fire-and-forget tactile time app.
// Assign to a long-press via Settings > Notifications > Quickly Launch App.
// Normal launches buzz the current time then auto-close.
// Settings launches stay open until Clay sends updated values.

#include <pebble.h>
#include <stdlib.h>
#include "vibe_engine.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------

static Window    *s_window;
static TextLayer *s_title_layer;
static TextLayer *s_pattern_label_layer;
static TextLayer *s_mode_layer;
static TextLayer *s_hint_layer;
static AppTimer  *s_exit_timer;

// ---------------------------------------------------------------------------
// Splash window
// ---------------------------------------------------------------------------

static void exit_timer_callback(void *context) {
    s_exit_timer = NULL;
    window_stack_pop_all(true);
}

static void update_mode_layers(void) {
    text_layer_set_text(s_mode_layer, vibe_config_get_mode() == PATTERN_MORSE ? "Morse" : "Digit Groups");
}

static void window_load(Window *window) {
    Layer *root = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root);

    int16_t w = bounds.size.w;
    int16_t h = bounds.size.h;

    // Compact layout for shorter displays
    bool compact = h < 160;

    int16_t side_margin = compact ? 2 : 4;
    int16_t vertical_margin = compact ? 4 : 8;

    int16_t time_h  = compact ? 42 : 48;
    int16_t label_h = 20;
    int16_t mode_h  = compact ? 22 : 26;
    int16_t hint_h  = compact ? 28 : 40;

    int16_t gap1 = compact ? 4 : 6;
    int16_t gap2 = compact ? 0 : 2;
    int16_t gap3 = compact ? 4 : 8;

    int16_t content_h =
        time_h +
        gap1 + label_h +
        gap2 + mode_h +
        gap3 + hint_h;

    int16_t available_h = h - (vertical_margin * 2);
    int16_t top = vertical_margin + ((available_h - content_h) / 2);

    if (top < vertical_margin) {
        top = vertical_margin;
    }

    int16_t y = top;

    s_title_layer = text_layer_create(GRect(0, y, w, time_h));
    text_layer_set_text_alignment(s_title_layer, GTextAlignmentCenter);
    text_layer_set_font(s_title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_background_color(s_title_layer, GColorClear);
    text_layer_set_text_color(s_title_layer, GColorBlack);
    text_layer_set_text(s_title_layer, "PebGoBrrrr");
    layer_add_child(root, text_layer_get_layer(s_title_layer));

    y += time_h + gap1;

    s_pattern_label_layer = text_layer_create(GRect(0, y, w, label_h));
    text_layer_set_text_alignment(s_pattern_label_layer, GTextAlignmentCenter);
    text_layer_set_font(s_pattern_label_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_background_color(s_pattern_label_layer, GColorClear);
    text_layer_set_text_color(s_pattern_label_layer, GColorDarkGray);
    text_layer_set_text(s_pattern_label_layer, "Vibration Pattern:");
    layer_add_child(root, text_layer_get_layer(s_pattern_label_layer));

    y += label_h + gap2;

    s_mode_layer = text_layer_create(GRect(0, y, w, mode_h));
    text_layer_set_text_alignment(s_mode_layer, GTextAlignmentCenter);
    text_layer_set_font(s_mode_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_background_color(s_mode_layer, GColorClear);
    text_layer_set_text_color(s_mode_layer, GColorBlack);
    layer_add_child(root, text_layer_get_layer(s_mode_layer));

    y += mode_h + gap3;

    s_hint_layer = text_layer_create(GRect(side_margin, y, w - (side_margin * 2), hint_h));
    text_layer_set_text_alignment(s_hint_layer, GTextAlignmentCenter);
    text_layer_set_font(s_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_background_color(s_hint_layer, GColorClear);
    text_layer_set_text_color(s_hint_layer, GColorDarkGray);
    text_layer_set_overflow_mode(s_hint_layer, GTextOverflowModeWordWrap);
    text_layer_set_text(s_hint_layer, "(Long Press to Change)");
    layer_add_child(root, text_layer_get_layer(s_hint_layer));

    update_mode_layers();
}
static void window_unload(Window *window) {
    if (s_exit_timer) {
        app_timer_cancel(s_exit_timer);
        s_exit_timer = NULL;
    }
    text_layer_destroy(s_title_layer);
    text_layer_destroy(s_pattern_label_layer);
    text_layer_destroy(s_mode_layer);
    text_layer_destroy(s_hint_layer);
}

// ---------------------------------------------------------------------------
// Button handlers
// ---------------------------------------------------------------------------

static void select_click(ClickRecognizerRef recognizer, void *context) {
    window_stack_pop_all(true);
}

static void select_long_press(ClickRecognizerRef recognizer, void *context) {
    VibePatternMode next = (vibe_config_get_mode() == PATTERN_DIGIT_GROUP) ? PATTERN_MORSE : PATTERN_DIGIT_GROUP;
    vibe_config_set_mode(next);
    update_mode_layers();
    vibe_speak_time();
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click);
    window_single_click_subscribe(BUTTON_ID_UP,     select_click);
    window_single_click_subscribe(BUTTON_ID_DOWN,   select_click);
    window_long_click_subscribe(BUTTON_ID_SELECT, 500, select_long_press, NULL);
}

// ---------------------------------------------------------------------------
// AppMessage - receive Clay settings from the phone
//
// Clay's select/radio components send their values as STRINGS even when the
// value looks like a number ("0", "1"). We must use atoi() on those.
// Slider components send actual integers - int32 is safe there.
// ---------------------------------------------------------------------------

static void inbox_received(DictionaryIterator *iter, void *context) {
    Tuple *t;

    // PatternMode and OutputMode come from Clay <select> - values are strings
    if ((t = dict_find(iter, MESSAGE_KEY_PatternMode)))
        vibe_config_set_mode((VibePatternMode)atoi(t->value->cstring));

    if ((t = dict_find(iter, MESSAGE_KEY_OutputMode)))
        vibe_config_set_output_mode((VibeOutputMode)atoi(t->value->cstring));

    // Slider values are integers
    if ((t = dict_find(iter, MESSAGE_KEY_BuzzShortMs)))
        vibe_config_set_buzz_short((uint32_t)t->value->int32);

    if ((t = dict_find(iter, MESSAGE_KEY_BuzzLongMs)))
        vibe_config_set_buzz_long((uint32_t)t->value->int32);

    if ((t = dict_find(iter, MESSAGE_KEY_GapDigitMs)))
        vibe_config_set_gap_digit((uint32_t)t->value->int32);

    if ((t = dict_find(iter, MESSAGE_KEY_GapCharMs)))
        vibe_config_set_gap_char((uint32_t)t->value->int32);

    if ((t = dict_find(iter, MESSAGE_KEY_GapGroupMs)))
        vibe_config_set_gap_group((uint32_t)t->value->int32);

    // Toggle - Clay sends 0/1 as integer
    if ((t = dict_find(iter, MESSAGE_KEY_LongBuzzHours)))
        vibe_config_set_long_buzz_hours((bool)t->value->int32);

    // Update the on-screen label if the window is visible
    if (s_mode_layer) {
        update_mode_layers();
    }

    // Settings saved - close so Clay can dismiss cleanly
    window_stack_pop_all(true);
}

static void inbox_dropped(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage dropped: %d", (int)reason);
}

// ---------------------------------------------------------------------------
// App lifecycle
// ---------------------------------------------------------------------------

static void init(void) {
    vibe_config_load();

    app_message_register_inbox_received(inbox_received);
    app_message_register_inbox_dropped(inbox_dropped);
    app_message_open(256, 64);

    bool config_launch = (launch_reason() == APP_LAUNCH_PHONE);

    s_window = window_create();
    window_set_background_color(s_window, GColorWhite);
    window_set_window_handlers(s_window, (WindowHandlers){
        .load   = window_load,
        .unload = window_unload,
    });
    window_set_click_config_provider(s_window, click_config_provider);
    window_stack_push(s_window, true);

    // Normal launch: auto-exit after vibration sequence.
    // Config launch: stay open until inbox_received closes us.
    if (!config_launch) {
        uint32_t s_exit_duration = vibe_speak_time();
        s_exit_timer = app_timer_register(s_exit_duration + 2000, exit_timer_callback, NULL);
    }
}

static void deinit(void) {
    if (s_window) {
        window_destroy(s_window);
    }
}

int main(void) {
    init();
    app_event_loop();
    deinit();
    return 0;
}