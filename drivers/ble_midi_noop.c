#include "pico/async_context.h"

void ble_midi_init() { }

void ble_midi_send_note(uint8_t channel, uint8_t note, uint8_t velocity) { }

bool ble_midi_is_connected(void) { return false; }

async_context_t *ble_midi_async_context(void) { return NULL; }
