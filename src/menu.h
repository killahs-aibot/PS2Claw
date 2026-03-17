/*
 * PS2Claw - Simple Menu System
 * D-pad navigation with X button selection
 */

#ifndef __PS2CLAW_MENU_H__
#define __PS2CLAW_MENU_H__

/* Menu modes */
typedef enum {
    MODE_MENU,     /* Main menu navigation */
    MODE_CHAT,     /* Chat/AI conversation mode */
    MODE_SETTINGS, /* Settings screen */
    MODE_ABOUT    /* About screen */
} AppMode;

/* Menu item */
typedef struct {
    const char *label;      /* Display label */
    AppMode target_mode;    /* Mode this item switches to */
} MenuItem;

/* Initialize menu system */
void menu_init(void);

/* Handle menu input - call each frame */
void menu_update(void);

/* Render menu */
void menu_render(void);

/* Get current app mode */
AppMode menu_get_mode(void);

/* Set app mode */
void menu_set_mode(AppMode mode);

#endif /* __PS2CLAW_MENU_H__ */