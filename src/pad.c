/*
 * PS2Claw - DualShock 2 Controller Input Layer
 * Implementation
 */

#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <stdio.h>
#include <string.h>
#include <libpad.h>
#include <debug.h>
#include "pad.h"

/* Global pad state */
static PadState pad_state[PAD_MAX_PORTS][PAD_MAX_SLOTS];
static PadPressure pad_pressure[PAD_MAX_PORTS][PAD_MAX_SLOTS];
static int pad_initialized = 0;

/* Debounce tracking */
static u64 last_poll_time = 0;
static u16 last_button_state[PAD_MAX_PORTS][PAD_MAX_SLOTS];

/* DMA buffer for pad data - must be 64-byte aligned */
static char pad_dma_buf[256] __attribute__((aligned(64)));

/* Load required IOP modules */
static int load_pad_modules(void)
{
    int ret;

    /* Load SIO2MAN - required for PADMAN */
    ret = SifLoadModule("rom0:SIO2MAN", 0, NULL);
    if (ret < 0) {
        scr_printf("[PAD] Error loading SIO2MAN: %d\n", ret);
        return -1;
    }

    /* Load PADMAN - controller driver */
    ret = SifLoadModule("rom0:PADMAN", 0, NULL);
    if (ret < 0) {
        scr_printf("[PAD] Error loading PADMAN: %d\n", ret);
        return -1;
    }

    return 0;
}

/* Wait for pad to be ready */
int pad_wait_ready(int port, int slot)
{
    int state;
    int lastState;
    char stateString[16];

    state = padGetState(port, slot);
    lastState = -1;

    while ((state != PAD_STATE_STABLE) && (state != PAD_STATE_FINDCTP1)) {
        if (state != lastState) {
            padStateInt2String(state, stateString);
            scr_printf("[PAD] Waiting for pad(%d,%d): %s\n",
                       port, slot, stateString);
        }
        lastState = state;
        state = padGetState(port, slot);
    }

    return 0;
}

/* Initialize a single pad port/slot */
static int init_pad_port(int port, int slot)
{
    int modes;
    int i;

    pad_wait_ready(port, slot);

    /* Get number of available modes */
    modes = padInfoMode(port, slot, PAD_MODETABLE, -1);

    if (modes > 0) {
        /* Check for DualShock mode */
        for (i = 0; i < modes; i++) {
            if (padInfoMode(port, slot, PAD_MODETABLE, i) == PAD_TYPE_DUALSHOCK) {
                break;
            }
        }

        if (i < modes) {
            /* Enable DualShock mode */
            padSetMainMode(port, slot, PAD_MMODE_DUALSHOCK, PAD_MMODE_LOCK);
            pad_wait_ready(port, slot);

            /* Enable pressure sensitivity if available */
            if (padInfoPressMode(port, slot)) {
                padEnterPressMode(port, slot);
            }

            scr_printf("[PAD] DualShock 2 controller detected on port %d\n", port);
        } else {
            scr_printf("[PAD] Digital controller on port %d\n", port);
        }
    }

    /* Open pad port */
    if (!padPortOpen(port, slot, pad_dma_buf)) {
        scr_printf("[PAD] Failed to open port %d slot %d\n", port, slot);
        return -1;
    }

    pad_wait_ready(port, slot);

    /* Initialize state */
    memset(&pad_state[port][slot], 0, sizeof(PadState));
    memset(&pad_pressure[port][slot], 0, sizeof(PadPressure));
    last_button_state[port][slot] = 0;

    return 0;
}

/* Initialize controller subsystem */
int pad_init(void)
{
    int port, slot;
    int ret;

    if (pad_initialized) {
        return 0;
    }

    scr_printf("[PAD] Initializing controller subsystem...\n");

    /* Load required IOP modules */
    ret = load_pad_modules();
    if (ret < 0) {
        scr_printf("[PAD] Failed to load modules\n");
        return -1;
    }

    /* Initialize libpad */
    padInit(0);

    /* Initialize each port/slot */
    for (port = 0; port < padGetPortMax(); port++) {
        for (slot = 0; slot < padGetSlotMax(port); slot++) {
            /* Check if controller is connected */
            int state = padGetState(port, slot);
            if (state == PAD_STATE_DISCONN) {
                scr_printf("[PAD] No controller on port %d slot %d\n", port, slot);
                continue;
            }

            ret = init_pad_port(port, slot);
            if (ret < 0) {
                continue;
            }
        }
    }

    /* Default to port 0, slot 0 if available */
    if (padGetState(0, 0) == PAD_STATE_STABLE) {
        scr_printf("[PAD] Controller ready on port 0\n");
        pad_initialized = 1;
        return 0;
    }

    /* No controller found */
    scr_printf("[PAD] WARNING: No controller detected!\n");
    return -1;
}

/* Poll current controller state */
void pad_poll(void)
{
    int port = 0;
    int slot = 0;
    struct padButtonStatus buttons;
    u16 current_buttons;

    if (!pad_initialized) {
        return;
    }

    /* Read pad data */
    if (!padRead(port, slot, &buttons)) {
        return;
    }

    /* Check if pad is still connected */
    int state = padGetState(port, slot);
    if (state != PAD_STATE_STABLE) {
        return;
    }

    /* Get current button state */
    current_buttons = buttons.btns;

    /* Calculate pressed/released */
    pad_state[port][slot].buttons = current_buttons;
    pad_state[port][slot].pressed = current_buttons & ~last_button_state[port][slot];
    pad_state[port][slot].released = last_button_state[port][slot] & ~current_buttons;

    /* Store for next frame */
    last_button_state[port][slot] = current_buttons;

    /* Analog sticks */
    pad_state[port][slot].rjoy_h = buttons.rjoy_h;
    pad_state[port][slot].rjoy_v = buttons.rjoy_v;
    pad_state[port][slot].ljoy_h = buttons.ljoy_h;
    pad_state[port][slot].ljoy_v = buttons.ljoy_v;

    /* Pressure sensitive buttons */
    pad_pressure[port][slot].up_p = buttons.up_p;
    pad_pressure[port][slot].down_p = buttons.down_p;
    pad_pressure[port][slot].left_p = buttons.left_p;
    pad_pressure[port][slot].right_p = buttons.right_p;
    pad_pressure[port][slot].triangle_p = buttons.triangle_p;
    pad_pressure[port][slot].circle_p = buttons.circle_p;
    pad_pressure[port][slot].cross_p = buttons.cross_p;
    pad_pressure[port][slot].square_p = buttons.square_p;
    pad_pressure[port][slot].l1_p = buttons.l1_p;
    pad_pressure[port][slot].r1_p = buttons.r1_p;
    pad_pressure[port][slot].l2_p = buttons.l2_p;
    pad_pressure[port][slot].r2_p = buttons.r2_p;
}

/* Check if a button is currently pressed */
int pad_button_pressed(u16 button)
{
    return (pad_state[0][0].buttons & button) != 0;
}

/* Check if a button was just pressed this frame */
int pad_button_just_pressed(u16 button)
{
    return (pad_state[0][0].pressed & button) != 0;
}

/* Check if a button was just released this frame */
int pad_button_just_released(u16 button)
{
    return (pad_state[0][0].released & button) != 0;
}

/* Get analog stick position */
u8 pad_get_rjoy_h(void)
{
    return pad_state[0][0].rjoy_h;
}

u8 pad_get_rjoy_v(void)
{
    return pad_state[0][0].rjoy_v;
}

u8 pad_get_ljoy_h(void)
{
    return pad_state[0][0].ljoy_h;
}

u8 pad_get_ljoy_v(void)
{
    return pad_state[0][0].ljoy_v;
}

/* Check if controller is connected */
int pad_is_connected(int port, int slot)
{
    return padGetState(port, slot) == PAD_STATE_STABLE;
}

/* Shutdown controller subsystem */
void pad_shutdown(void)
{
    if (!pad_initialized) {
        return;
    }

    /* Close all ports */
    for (int port = 0; port < PAD_MAX_PORTS; port++) {
        for (int slot = 0; slot < PAD_MAX_SLOTS; slot++) {
            padPortClose(port, slot);
        }
    }

    padEnd();
    pad_initialized = 0;

    scr_printf("[PAD] Controller subsystem shutdown\n");
}