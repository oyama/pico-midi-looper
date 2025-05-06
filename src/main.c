/* main.c
 * Pico MIDI looper for Raspberry Pi Pico.
 * A minimal 2-bars loop recorder using a single button to record and switch tracks.
 *
 * Copyright 2025, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stdio.h>
#include "pico/stdlib.h"

#include "looper.h"
#include "drivers/led.h"
#include "drivers/ble_midi.h"
#include "drivers/usb_midi.h"
#include "drivers/async_timer.h"

/*
 * Entry point for the Pico MIDI Looper application.
 *
 * Looper is driven by two input sources:
 *  - Timer ticks (looper_handle_tick) for sequencer state progression
 *  - Button events (looper_handle_input) for user-driven updates
 */
int main(void) {
    stdio_init_all();
    led_init();

    usb_midi_init();
    ble_midi_init();

    // Async timer + sequencer tick setup
    async_timer_init();
    looper_schedule_step_timer();

    printf("[MAIN] Pico MIDI Looper start\n");
    while (true) {
        looper_handle_input();
        usb_midi_task();

        sleep_us(500);
    }
    return 0;
}
