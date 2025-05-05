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

#define ANSI_BLACK "\x1b[30m"
#define ANSI_BRIGHT_BLACK "\x1b[90m"
#define ANSI_BRIGHT_BLUE "\x1b[94m"
#define ANSI_BRIGHT_MAGENTA "\x1b[95m"
#define ANSI_CALM_GREEN "\x1b[38;5;64m"
#define ANSI_SOFT_RED "\x1b[38;5;160m"
#define ANSI_BG_WHITE "\x1b[47m"
#define ANSI_BOLD "\x1b[1m"
#define ANSI_RESET "\x1b[0m"

#define ANSI_CLEAR_SCREEN "\x1b[2J"
#define ANSI_CURSOR_HOME "\x1b[H"
#define ANSI_CLEAR_HOME ANSI_CURSOR_HOME ANSI_CLEAR_SCREEN
#define ANSI_HIDE_CURSOR "\x1b[?25l"
#define ANSI_CURSOR_FMT "\x1b[%u;%uH"
#define ANSI_CLEAR_EOL "\x1b[K"

#define ANSI_ENABLE_ALTSCREEN  "\x1b[?1049h"
#define ANSI_DISABLE_ALTSCREEN "\x1b[?1049l"

// Prints a single track row with step highlighting and note indicators.
static void print_track(const char *label, const bool *steps, uint8_t current_step,
                        bool is_selected) {
    if (is_selected)
        printf("  " ANSI_BOLD ">%-11s [", label);
    else
        printf("  " ANSI_BRIGHT_BLACK " %-11s " ANSI_RESET ANSI_BOLD "[", label);

    for (int i = 0; i < LOOPER_TOTAL_STEPS; ++i) {
        bool note_on = steps[i];
        if (current_step == i)
            printf(ANSI_BG_WHITE ANSI_BLACK "%s" ANSI_RESET ANSI_BOLD, (note_on ? "*" : "_"));
        else if (note_on)
            printf("*");
        else
            printf("_");
    }
    printf("]\n" ANSI_RESET);
}

// Displays the looper's playback state, connection status, and track patterns.
void display_update_looper_status(bool output_connected, const looper_status_t *looper,
                                  const track_t *tracks, size_t num_tracks) {
    printf(ANSI_ENABLE_ALTSCREEN);

    if (looper->current_step % 32 == 0)
        printf(ANSI_CLEAR_HOME);
    printf(ANSI_HIDE_CURSOR);
    printf(ANSI_CURSOR_FMT ANSI_CLEAR_EOL, 1, 1);

    printf(ANSI_BOLD "\n                    #Pico_MIDI_Looper\n\n" ANSI_RESET);

    const char *state_label = "[WAITING]";
    const char *label_color = ANSI_BRIGHT_BLUE;
    if (output_connected) {
        switch (looper->state) {
            case LOOPER_STATE_PLAYING:
            case LOOPER_STATE_TRACK_SWITCH:
                state_label = "[PLAYING]";
                label_color = ANSI_CALM_GREEN;
                break;
            case LOOPER_STATE_RECORDING:
                state_label = "[RECORDING]";
                label_color = ANSI_SOFT_RED;
                break;
            case LOOPER_STATE_TAP_TEMPO:
                state_label = "[TAP TEMPO]";
                label_color = ANSI_BRIGHT_MAGENTA;
                break;
            default:
                break;
        }
    }
    printf("   %s%-12s%s", label_color, state_label, ANSI_RESET);

    printf(" %s♩=%u\n" ANSI_RESET,
           ((looper->current_step % LOOPER_CLICK_DIV) == 0 ? ANSI_BOLD : ANSI_BRIGHT_BLACK),
           looper->bpm);

    printf(ANSI_BOLD "                1   2   3   4   5   6   7   8\n" ANSI_RESET);
    for (uint8_t i = 0; i < num_tracks; i++)
        print_track(tracks[i].name, tracks[i].pattern, looper->current_step,
                    i == looper->current_track);
    fflush(stdout);
}
