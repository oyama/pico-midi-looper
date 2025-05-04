/*
 * display.c
 *
 * UART-based text UI for the Pico MIDI Looper.
 * This module is responsible for rendering the USB connection status,
 * current looper state, and per-track step patterns over a serial console.
 *
 * Copyright 2025, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdbool.h>
#include <stdio.h>

#include "looper.h"

#define ANSI_RESET "\x1b[0m"
#define ANSI_BRIGHT_RED "\x1b[91m"
#define ANSI_BRIGHT_GREEN "\x1b[92m"
#define ANSI_BRIGHT_BLUE "\x1b[94m"
#define ANSI_BRIGHT_MAGENTA "\x1b[95m"
#define ANSI_BRIGHT_CYAN "\x1b[96m"

#define ANSI_BOLD "\x1b[1m"
#define ANSI_FG_WHITE "\x1b[97m"
#define ANSI_BG_STEP_HL "\x1b[105m"

// Prints a single track row with step highlighting and note indicators.
static void print_track(const char *label, const bool *steps, uint8_t current_step,
                        bool is_selected) {
    printf("%s%-11s %s[", is_selected ? ANSI_BOLD ">" : " ", label, ANSI_RESET);
    for (int i = 0; i < LOOPER_TOTAL_STEPS; ++i) {
        bool note_on = steps[i];
        if (current_step == i)
            printf("%s%s%s", ANSI_BRIGHT_CYAN ANSI_BG_STEP_HL, note_on ? "*" : " ", ANSI_RESET);
        else if (note_on)
            printf("*");
        else
            printf(" ");
    }
    printf("]\n");
}

// Displays the looper's playback state, connection status, and track patterns.
void display_update_looper_status(bool output_connected, const looper_status_t *looper,
                                  const track_t *tracks, size_t num_tracks) {
    printf(ANSI_BOLD "#Pico_MIDI_Looper" ANSI_RESET "\n");

    const char *state_label = ANSI_BRIGHT_BLUE "WAITING" ANSI_RESET;
    if (output_connected) {
        switch (looper->state) {
            case LOOPER_STATE_PLAYING:
            case LOOPER_STATE_TRACK_SWITCH:
                state_label = ANSI_BRIGHT_GREEN "PLAYING" ANSI_RESET;
                break;
            case LOOPER_STATE_RECORDING:
                state_label = ANSI_BRIGHT_RED "RECORDING" ANSI_RESET;
                break;
            case LOOPER_STATE_TAP_TEMPO:
                state_label = ANSI_BRIGHT_MAGENTA "TAP TEMPO" ANSI_RESET;
                break;
            default:
                break;
        }
    }
    printf("[%s] %s%u bpm\n" ANSI_RESET, state_label,
           ((looper->current_step % LOOPER_CLICK_DIV) == 0 ? ANSI_BOLD : ""), looper->bpm);

    for (uint8_t i = 0; i < num_tracks; i++)
        print_track(tracks[i].name, tracks[i].pattern, looper->current_step,
                    i == looper->current_track);
    fflush(stdout);
}
