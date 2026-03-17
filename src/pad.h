/*
 * PS2Claw - DualShock 2 Controller Input Layer
 * Header file for pad.c
 */

#ifndef __PS2CLAW_PAD_H__
#define __PS2CLAW_PAD_H__

#include <libpad.h>
#include <tamtypes.h>

/* Number of controllers to check */
#define PAD_MAX_PORTS 2
#define PAD_MAX_SLOTS 1

/* Debounce delay in ms */
#define PAD_DEBOUNCE_MS 150

/* Pad button structure */
typedef struct {
    u16 buttons;           /* Current button state */
    u16 pressed;           /* Just pressed this frame */
    u16 released;          /* Just released this frame */
    u8 rjoy_h;             /* Right stick horizontal */
    u8 rjoy_v;             /* Right stick vertical */
    u8 ljoy_h;             /* Left stick horizontal */
    u8 ljoy_v;             /* Left stick vertical */
} PadState;

/* Button constants for DualShock 2 */
#define PAD_BTN_UP         PAD_UP
#define PAD_BTN_DOWN       PAD_DOWN
#define PAD_BTN_LEFT       PAD_LEFT
#define PAD_BTN_RIGHT      PAD_RIGHT
#define PAD_BTN_START      PAD_START
#define PAD_BTN_SELECT     PAD_SELECT
#define PAD_BTN_CROSS      PAD_CROSS      /* X button */
#define PAD_BTN_CIRCLE     PAD_CIRCLE     /* O button */
#define PAD_BTN_TRIANGLE   PAD_TRIANGLE   /* △ button */
#define PAD_BTN_SQUARE     PAD_SQUARE     /* □ button */
#define PAD_BTN_L1         PAD_L1
#define PAD_BTN_R1         PAD_R1
#define PAD_BTN_L2         PAD_L2
#define PAD_BTN_R2         PAD_R2
#define PAD_BTN_L3         PAD_L3
#define PAD_BTN_R3         PAD_R3

/* Extended buttons (pressure sensitive) */
typedef struct {
    u8 up_p;
    u8 down_p;
    u8 left_p;
    u8 right_p;
    u8 triangle_p;
    u8 circle_p;
    u8 cross_p;
    u8 square_p;
    u8 l1_p;
    u8 r1_p;
    u8 l2_p;
    u8 r2_p;
} PadPressure;

/* Initialize controller subsystem */
int pad_init(void);

/* Poll current controller state - call each frame */
void pad_poll(void);

/* Check if a button is currently pressed */
int pad_button_pressed(u16 button);

/* Check if a button was just pressed this frame */
int pad_button_just_pressed(u16 button);

/* Check if a button was just released this frame */
int pad_button_just_released(u16 button);

/* Get analog stick position (0-255, 128 = center) */
u8 pad_get_rjoy_h(void);
u8 pad_get_rjoy_v(void);
u8 pad_get_ljoy_h(void);
u8 pad_get_ljoy_v(void);

/* Check if controller is connected */
int pad_is_connected(int port, int slot);

/* Shutdown controller subsystem */
void pad_shutdown(void);

/* Wait for pad to be ready */
int pad_wait_ready(int port, int slot);

#endif /* __PS2CLAW_PAD_H__ */