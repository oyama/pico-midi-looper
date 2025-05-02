/* main.c
 * BLE MIDI looper for Raspberry Pi Pico W.
 * A minimal 2-bars loop recorder using a single button to record and switch tracks.
 *
 * Copyright 2025, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "ble_midi.h"
#include "usb_midi.h"
#include "button.h"
#include "display.h"
#include "looper.h"
#include "tap_tempo.h"

enum {
    MIDI_CHANNEL_1 = 0,
    MIDI_CHANNEL_10 = 9,
};

enum {
    BASS_DRUM = 36,
    RIM_SHOT = 37,
    SNARE_DRUM = 38,
    HAND_CLAP = 39,
    CLOSED_HIHAT = 42,
    OPEN_HIHAT = 46,
    CYMBAL = 49,
};

static looper_status_t looper_status = {.bpm = LOOPER_DEFAULT_BPM, .state = LOOPER_STATE_WAITING};

static track_t tracks[] = {
    {"Bass", BASS_DRUM, MIDI_CHANNEL_10, {0}, {0}},
    {"Snare", SNARE_DRUM, MIDI_CHANNEL_10, {0}, {0}},
    {"Hi-hat", CLOSED_HIHAT, MIDI_CHANNEL_10, {0}, {0}},
    {"Open Hi-hat", OPEN_HIHAT, MIDI_CHANNEL_10, {0}, {0}},
};
static const size_t NUM_TRACKS = sizeof(tracks) / sizeof(track_t);

static bool status_led_on = false;

/*
 * Controls the built-in LED on the Pico W.
 * Used for indicating the active track or recording.
 */
static inline void looper_set_status_led(bool on) { status_led_on = on; }

/*
 * Manages LED blinking when in WAITING state (BLE disconnected).
 * Otherwise mirrors the `status_led_on` flag during playback/recording.
 */
static void looper_update_status_led(void) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, status_led_on);
}

static void looper_update_bpm(uint32_t bpm) {
    looper_status.bpm = bpm;
    looper_status.step_duration_ms = 60000 / (bpm * LOOPER_STEPS_PER_BEAT);
}

// Check if the note output destination is ready.
static bool looper_perform_ready(void) {
    return ble_midi_is_connected()
           || usb_midi_is_connected();
}

// Send a note event to the output destination.
static void looper_perform_note(uint8_t channel, uint8_t note, uint8_t velocity) {
    ble_midi_send_note(channel, note, velocity);
    usb_midi_send_note(channel, note, velocity);
}

// Sends a MIDI click at specific steps to indicate rhythm.
static void send_click_if_needed(void) {
    if ((looper_status.current_step % LOOPER_CLICK_DIV) == 0)
        looper_perform_note(MIDI_CHANNEL_1, RIM_SHOT, 0x20);
}

// Perform all note events for the current step across all tracks.
// If the current track is active, also update the status LED.
static void looper_perform_step(void) {
    for (uint8_t i = 0; i < NUM_TRACKS; i++) {
        bool note_on = tracks[i].pattern[looper_status.current_step];
        if (note_on) {
            looper_perform_note(tracks[i].channel, tracks[i].note, 0x7f);
            if (i == looper_status.current_track)
                looper_set_status_led(1);
        } else if (i == looper_status.current_track) {
            looper_set_status_led(0);
        }
    }
}

// Perform note events for the current step while recording.
// In recording mode, the status LED is always turned on.
static void looper_perform_step_recording(void) {
    looper_set_status_led(1);

    for (uint8_t i = 0; i < NUM_TRACKS; i++) {
        bool note_on = tracks[i].pattern[looper_status.current_step];
        if (note_on)
            looper_perform_note(tracks[i].channel, tracks[i].note, 0x7f);
    }
}

// Updates the current step index and timestamp based on current loop progress.
static void looper_next_step(uint64_t now_us) {
    looper_status.timing.last_step_time_us = now_us;
    looper_status.current_step = (looper_status.current_step + 1) % LOOPER_TOTAL_STEPS;
}

// Re-arms the timer to fire again after LOOPER_STEP_DURATION_MS, adjusting for processing time.
static void looper_reset_step_timer(btstack_timer_source_t *ts, uint64_t start_us) {
    uint64_t handler_delay_ms = (time_us_64() - start_us) / 1000;
    uint32_t delay = (handler_delay_ms >= looper_status.step_duration_ms)
                         ? 1
                         : looper_status.step_duration_ms - handler_delay_ms;
    btstack_run_loop_set_timer(ts, delay);
    btstack_run_loop_add_timer(ts);
}

/*
 * Returns the step index nearest to the stored `button_press_start_us` timestamp.
 * The result is quantized to the nearest step relative to the last tick.
 */
static uint8_t looper_quantize_step() {
    int64_t delta_us =
        looper_status.timing.button_press_start_us - looper_status.timing.last_step_time_us;
    // Convert to step offset using rounding (nearest step)
    int32_t relative_steps =
        (int32_t)round((double)delta_us / 1000.0 / looper_status.step_duration_ms);
    uint8_t previous_step =
        (looper_status.current_step + LOOPER_TOTAL_STEPS - 1) % LOOPER_TOTAL_STEPS;
    uint8_t estimated_step =
        (previous_step + relative_steps + LOOPER_TOTAL_STEPS) % LOOPER_TOTAL_STEPS;
    return estimated_step;
}

static void looper_clear_all_tracks() {
    for (uint8_t i = 0; i < NUM_TRACKS; i++)
        memset(tracks[i].pattern, 0, sizeof(tracks[i].pattern));
}

// Processes the looper's main state machine, called by the step timer.
static void looper_process_state(btstack_timer_source_t *ts) {
    uint64_t start_us = time_us_64();
    bool ready  = looper_perform_ready();
    display_update_looper_status(ready, &looper_status, tracks, NUM_TRACKS);

    if (!ready)
        looper_status.state = LOOPER_STATE_WAITING;
    switch (looper_status.state) {
        case LOOPER_STATE_WAITING:
            if (ready) {
                looper_status.state = LOOPER_STATE_PLAYING;
                looper_status.current_step = 0;
            }
            looper_set_status_led((looper_status.current_step % (LOOPER_CLICK_DIV * 4)) == 0);
            looper_next_step(start_us);
            break;
        case LOOPER_STATE_PLAYING:
            send_click_if_needed();
            looper_perform_step();
            looper_next_step(start_us);
            break;
        case LOOPER_STATE_RECORDING:
            send_click_if_needed();
            looper_perform_step_recording();
            looper_next_step(start_us);

            looper_status.recording_step_count++;
            if (looper_status.recording_step_count >= LOOPER_TOTAL_STEPS) {
                looper_set_status_led(0);
                looper_status.state = LOOPER_STATE_PLAYING;
            }
            break;
        case LOOPER_STATE_TRACK_SWITCH:
            looper_status.current_track = (looper_status.current_track + 1) % NUM_TRACKS;
            looper_perform_note(MIDI_CHANNEL_10, HAND_CLAP, 0x7f);
            looper_next_step(start_us);
            looper_status.state = LOOPER_STATE_PLAYING;
            break;
        case LOOPER_STATE_TAP_TEMPO:
            send_click_if_needed();
            looper_set_status_led((looper_status.current_step % LOOPER_CLICK_DIV) == 0);
            looper_next_step(start_us);
            break;
        case LOOPER_STATE_CLEAR_TRACKS:
            looper_clear_all_tracks();
            looper_status.current_track = 0;
            looper_next_step(start_us);
            looper_status.state = LOOPER_STATE_PLAYING;
        default:
            break;
    }

    looper_reset_step_timer(ts, start_us);
}

// Routes button events related to tap-tempo mode.
static tap_result_t taptempo_handle_button_event(button_event_t event) {
    tap_result_t result = taptempo_handle_event(event);
    switch (result) {
        case TAP_PRELIM:
        case TAP_FINAL:
            looper_update_bpm(taptempo_get_bpm());
            break;
        case TAP_EXIT: /* leave mode */
            break;
        default:
            break;
    }
    return result;
}

/*
 * Handles button events and updates the looper state accordingly.
 *
 * This includes triggering MIDI notes, starting/stopping recording,
 * and switching tracks based on short or long presses.
 *
 * Event constants are defined in button.c (see button_event_t).
 */
static void looper_handle_button_event(button_event_t event) {
    track_t *track = &tracks[looper_status.current_track];

    switch (event) {
        case BUTTON_EVENT_DOWN:
            // Button pressed: start timing and preview sound
            looper_status.timing.button_press_start_us = time_us_64();
            looper_perform_note(track->channel, track->note, 0x7f);
            // Backup track pattern in case this press becomes a long-press (undo)
            memcpy(track->hold_pattern, track->pattern, LOOPER_TOTAL_STEPS);
            break;
        case BUTTON_EVENT_CLICK_RELEASE:
            // Short press release: quantize and record step
            if (looper_status.state != LOOPER_STATE_RECORDING) {
                looper_status.recording_step_count = 0;
                looper_status.state = LOOPER_STATE_RECORDING;
                memset(track->pattern, 0, LOOPER_TOTAL_STEPS);
            }
            uint8_t quantized_step = looper_quantize_step();
            track->pattern[quantized_step] = true;
            break;
        case BUTTON_EVENT_HOLD_RELEASE:
            // Long press release: revert track and switch
            memcpy(track->pattern, track->hold_pattern, LOOPER_TOTAL_STEPS);
            looper_status.state = LOOPER_STATE_TRACK_SWITCH;
            break;
        case BUTTON_EVENT_LONG_HOLD_RELEASE:
            // ≥2 s hold: enter Tap-tempo (no track switch)
            looper_status.state = LOOPER_STATE_TAP_TEMPO;
            looper_perform_note(MIDI_CHANNEL_10, HAND_CLAP, 0x7f);
            break;
        case BUTTON_EVENT_VERY_LONG_HOLD_RELEASE:
            // ≥5 s hold: clear track data
            looper_status.state = LOOPER_STATE_CLEAR_TRACKS;
            looper_perform_note(MIDI_CHANNEL_10, HAND_CLAP, 0x7f);
            break;
        default:
            break;
    }
}

int main(void) {
    stdio_init_all();
    cyw43_arch_init();
    looper_update_bpm(LOOPER_DEFAULT_BPM);

    ble_midi_init(looper_process_state, looper_status.step_duration_ms);
    usb_midi_init();

    printf("[MAIN] BLE MIDI Looper start\n");
    while (true) {
        button_event_t event = button_poll_event();
        if (looper_status.state == LOOPER_STATE_TAP_TEMPO) {
            if (taptempo_handle_button_event(event) == TAP_EXIT) {
                looper_status.state = LOOPER_STATE_PLAYING;
            }
        } else {
            looper_handle_button_event(event);
        }
        looper_update_status_led();

        usb_midi_task();
    }
    return 0;
}
