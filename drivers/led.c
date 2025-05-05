/*
 * led.c
 *
 * LED driver module: Controls the built-in LED on Raspberry Pi Pico and Pico W.
 * Provides initialization, state setting, and update functions for the status LED.
 *
 * Copyright 2025, Hiroyuki OYAMA
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "pico/stdlib.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

static bool status_led_on = false;

/**
 * Initialize the status LED hardware.
 *
 * Configures the appropriate GPIO pin (PICO_DEFAULT_LED_PIN) or
 * initializes the CYW43 architecture for Pico W's onboard LED.
 * Must be called once at startup before using led_set() or led_update().
 */
void led_init(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_init();
#endif
}

/*
 * Controls the built-in LED on the Pico.
 * Used for indicating the active track or recording.
 */
void led_set(bool on) { status_led_on = on; }

/*
 * Manages LED blinking
 */
void led_update(void) {
#if defined(PICO_DEFAULT_LED_PIN)
    gpio_put(PICO_DEFAULT_LED_PIN, status_led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, status_led_on);
#endif
}
