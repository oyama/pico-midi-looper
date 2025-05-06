/*
 * Copyright 2025, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

void ble_midi_init(void);

bool ble_midi_is_connected(void);

void ble_midi_send_note(uint8_t channel, uint8_t note, uint8_t velocity);
