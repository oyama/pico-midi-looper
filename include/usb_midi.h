/*
 * Copyright 2025, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

void usb_midi_init(void);

bool usb_midi_is_connected(void);

void usb_midi_send_note(uint8_t channel, uint8_t note, uint8_t velocity);

void usb_midi_task(void);
