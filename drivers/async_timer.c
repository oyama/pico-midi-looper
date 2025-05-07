/**
 * async_dispatch.c
 *
 * Provides a shared async_context for scheduling periodic or delayed work.
 * - On Pico W (CYW43), forwards to cyw43_arch_async_context().
 * - On other Pico boards, initializes a threadsafe-background context.
 */
#if CYW43_ENABLE_BLUETOOTH
#include "pico/cyw43_arch.h"
#else
#include "pico/async_context.h"
#include "pico/async_context_threadsafe_background.h"

static async_context_threadsafe_background_t tick_async_context;
#endif

void async_timer_init(void) {
#if !defined(CYW43_ENABLE_BLUETOOTH)
    async_context_threadsafe_background_config_t config = async_context_threadsafe_background_default_config();
    if (async_context_threadsafe_background_init(&tick_async_context, &config)) {
        ;
    }
#endif
}

async_context_t *async_timer_async_context(void) {
#if CYW43_ENABLE_BLUETOOTH
    return cyw43_arch_async_context();
#else
    return &tick_async_context.core;
#endif
}
