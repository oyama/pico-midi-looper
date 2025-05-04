# Pico MIDI Looper: Architecture Overview

This document describes the internal architecture of the Pico MIDI Looper firmware. It is designed to explain how the system operates from a structural point of view, with an emphasis on clarity, simplicity, and extensibility.

## Overview

The Pico MIDI Looper is a USB‑MIDI‑based loop recorder for Raspberry Pi Pico.
It captures and replays a two‑bar (32‑step) pattern of four drum tracks with a single button.
Besides recording, a tap‑tempo mode lets you adjust the global BPM on‑the‑fly with that same button.

Internally, the firmware is powered by two core mechanisms:

1. **Main looper FSM** – governs the global state (*waiting*, *playing*, *recording*, *track‑switch*, *tap‑tempo prompt*, *clear*).
2. **Timer‑driven sequencer** – a Pico SDK repeating‑timer fires once per step, advancing the pattern and sending the corresponding MIDI notes with sample‑accurate timing.

A lightweight **tap‑tempo FSM** accompanies them, translating a series of taps into a new BPM value and handing it off to the looper without disturbing the timing loop.

## Looper State Machine

The firmware centers on a main FSM with six states: `Waiting / Playing / Recording / TrackSwitch / TapTempo / ClearTracks`. The updated diagram reflects this.

![Looper FSM](looper_fsm.svg)

- **Waiting**: Initial idle state before USB connection is established.
- **Playing**: Default playback state, running the sequencer.
- **Recording**: Temporarily active while recording note input for 2 bars.
- **TrackSwitch**: Transition state when switching between drum tracks.
- **TapTempo**: Temporary mode for tap-tempo BPM entry.
- **ClearTracks**: Transition state when clearing all track patterns.

State transitions are triggered by USB connection events and button presses. The state machine is evaluated on every step clock tick within the main loop logic.

## Sequencer Timing

The sequencer timer period is re-computed whenever the global BPM changes (e.g. after tap-tempo):

```c
/* updated every time looper_update_bpm() is called */
looper_status.step_duration_ms = 60000 / (bpm * LOOPER_STEPS_PER_BEAT);
btstack_run_loop_set_timer(&step_timer, looper_status.step_duration_ms);
```

- Each loop consists of 32 steps (4 beats x 4 subdivisions x 2 bars).
- A timer (btstack run loop timer) triggers every LOOPER\_STEP\_DURATION\_MS.
- On each tick, the looper updates the current step, outputs any matching notes, and transitions state if necessary.

## Button Handling

The BOOTSEL button is monitored by reading its state using a method specific to the Pico's onboard configuration.
An internal FSM (in `drivers/button.c`) detects five types of user actions:

- `BUTTON_EVENT_DOWN`
- `BUTTON_EVENT_CLICK_RELEASE`
- `BUTTON_EVENT_HOLD_RELEASE`
- `BUTTON_EVENT_LONG_HOLD_RELEASE`
- `BUTTON_EVENT_VERY_LONG_HOLD_RELEASE`

These events are interpreted by the looper (in `src/main.c`) depending on its current state:

- **Click**: Starts recording, and toggles a note at the quantized step.
- **Hold-release(≥ 0.5 s)**: Switches to the next track.
- **Long-hold-release (≥2 s)**: Enters Tap-tempo mode from Playing. Inside Tap-tempo a ≥0.5 s hold-release confirms the BPM and returns to Playing.
- **Very-long-hold-release (≥5 s)**: Enters Clear-track mode from Playing. After deleting the tracks, return to Playing.

## Track Structure

Each track is represented by a `track_t` structure containing:

- A note number (MIDI note)
- A MIDI channel
- A `pattern[]` bit array (one bool per step)
- A `hold_pattern[]` to revert recording on press

Four tracks are predefined: `Bass`, `Snare`, `Closed hi-hat` and `Open hi-hat` both on MIDI channel 10.

## USB MIDI Integration

USB MIDI communication is handled via TinyUSB. The system registers a USB device descriptor, and sends MIDI note-on messages via `tud_midi_stream_write` when a note is triggered.

The USB connection status is monitored and used to gate playback and visual LED feedback.

## Code Structure Summary

| File             | Responsibility                                              |
| ---------------- | ----------------------------------------------------------- |
| `src/main.c`     | Initialization & run‑loop glue |
| `src/looper.c`   | Looper state machine, step sequencer, button event handling |
| `src/tap_tempo.c`| Tap-tempo detection & BPM estimation sub-FSM                |
| `drivers/button.c`   | Button press detection, debouncing, and press-type FSM      |
| `drivers/usb_midi.c` | Define USB descriptor and MIDI note delivery                |
| `drivers/display.c`  | Display looper and track status on UART or USB CDC          |

## Design Goals

- **Minimalism**: Core logic in under 300 lines of C
- **Readability**: Clear structure and FSM-based design
- **Extensibility**: Easy to add new tracks, note types, or input methods
- **Educational Value**: Suitable for workshops and experimentation
