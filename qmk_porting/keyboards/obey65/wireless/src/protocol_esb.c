#include "protocol_esb.h"
#include "CH58xBLE_LIB.H"

static void platform_initialize() {
    // TODO: Initialize ESB task
}

static void protocol_setup() {
    // TODO: Setup ESB protocol
}

static void protocol_init() {
    // TODO: Init ESB hardware
}

static void protocol_pre_task() {
    // TODO: ESB pre-task
}

static void protocol_post_task() {
    // TODO: ESB post-task
}

static void platform_run() {
    // TODO: ESB main loop step
    TMOS_SystemProcess();
}

static void platform_reboot() {
    // TODO: Handle reboot
    SYS_ResetExecute();
}

static void send_keyboard(report_keyboard_t *report) {
    // TODO: Send keyboard report over ESB
}

static void send_nkro(report_nkro_t *report) {
    // TODO: Send NKRO report over ESB
}

static void send_mouse(report_mouse_t *report) {
    // TODO: Send mouse report over ESB
}

static void send_extra(report_extra_t *report) {
    // TODO: Send extra report over ESB
}

static uint8_t esb_keyboard_leds(void) {
    // TODO: Return LED state from ESB
    return 0;
}

const ch582_interface_t ch582_protocol_esb = {
    .ch582_common_driver.keyboard_leds = esb_keyboard_leds,
    .ch582_common_driver.send_keyboard = send_keyboard,
    .ch582_common_driver.send_nkro = send_nkro,
    .ch582_common_driver.send_mouse = send_mouse,
    .ch582_common_driver.send_extra = send_extra,
    .ch582_platform_initialize = platform_initialize,
    .ch582_protocol_setup = protocol_setup,
    .ch582_protocol_init = protocol_init,
    .ch582_protocol_pre_task = protocol_pre_task,
    .ch582_protocol_post_task = protocol_post_task,
    .ch582_platform_run = platform_run,
    .ch582_platform_reboot = platform_reboot,
};

