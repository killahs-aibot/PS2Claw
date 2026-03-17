/*
 * PS2Claw - Virtual Keyboard for Terminal Input
 * D-pad navigation, X to select, O for backspace
 * L1/R1 to switch pages (letters/numbers/symbols)
 */

#ifndef __PS2CLAW_KEYBOARD_H__
#define __PS2CLAW_KEYBOARD_H__

/* Keyboard page types */
typedef enum {
    KB_PAGE_LETTERS,   /* A-Z */
    KB_PAGE_NUMBERS,   /* 0-9 */
    KB_PAGE_SYMBOLS,   /* Common symbols */
    KB_PAGE_MAX
} KeyboardPage;

/* Initialize virtual keyboard */
void kb_init(void);

/* Update keyboard state - call each frame */
void kb_update(void);

/* Check if keyboard is visible */
int kb_is_visible(void);

/* Show/hide keyboard */
void kb_show(void);
void kb_hide(void);

/* Get current input text */
const char* kb_get_text(void);

/* Clear input */
void kb_clear(void);

/* Handle keyboard input - returns 1 if key processed */
int kb_handle_input(void);

/* Render keyboard to screen */
void kb_render(void);

/* Check if enter was pressed (message ready) */
int kb_enter_pressed(void);

/* Check if escape/back was pressed */
int kb_escape_pressed(void);

/* Get suggested completions for autocomplete */
const char** kb_get_suggestions(int *count);

/* Add to command history */
void kb_add_history(const char *cmd);

#endif /* __PS2CLAW_KEYBOARD_H__ */